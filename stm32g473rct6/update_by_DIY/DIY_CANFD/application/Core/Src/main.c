/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fdcan.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dbger.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// |-------------------------------------------------------------------------------------------------------|
// |				                                 Flash: Total 256 KB                                           |
// |-----------------------------------------------|-------|-----------------------------------------------|
// |              0x08000000~0x0801FFFF            |       |             0x08040000~0x0805FFFF	           |
// |                   BANK1: 128KB	               | NoDef |                  BANK2: 128KB                 |
// |                    page 0~63		          	   |       |                   page 0~63                   |
// |-----------------------|-----------------------|-------|-----------------------|-----------------------|
// | 0x08000000~0x08007FFF | 0x08008000~0x0801FFFF |       | 0x08040000~0x0805F7FF | 0x0805F800~0x0805FFFF |
// |    bootloader(32KB)   |   application(96KB)	 | NoDef |    NotUsed(126KB)     |       Flag(2KB)       |
// |       page 0~15       |     page 16~63        |       |      page 0~62        |        page 63        |
// |-----------------------|-----------------------|-------|-----------------------|-----------------------|
// refer: RM0440
#define APP_ADDRESS							(0x08008000)		// 32KB for bootloader
#define UPDATE_FLAG_ADDR        (0x0805F800)    // bank2 last flash page(2KB)
#define APP_MAX_SIZE            (96 * 1024)    	// 96KB

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int8_t set_update_info(uint64_t data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	SCB->VTOR = APP_ADDRESS;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */
	LOG_INIT();
	LOG_DBG("\n\nstm32g473rc firmware update demo: application start...\n");
	
	FDCAN_Config();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint32_t now, then = 0, cnt = 0;
	uint8_t rxLen;
  while (1)
  {
		now = HAL_GetTick();
		if(now - then > 10) {		// 10ms
			then = now;
			cnt++;
			
			if(cnt % 50 == 0) {		// 500ms
				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			}
			
			if(cnt % 1000 == 0) {		// 10s
				if(0 == set_update_info(FLAG_APP_NEED_UPDATE)) {
					LOG_DBG("set update flag[0x%08x] OK\n", FLAG_APP_NEED_UPDATE);
					LOG_DBG("Jump to bootloader...\n");
					HAL_Delay(5);
					__disable_irq();
					HAL_NVIC_SystemReset();
				}
			}
		}
		
		while(HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) != 0) {
			if(HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &canRxHeader, canRxBuf) == HAL_OK) {
				rxLen = can_getLen(canRxHeader.DataLength);
				
				LOG_INF("\nrecv(ID=0x%08x, len=%02d):", canRxHeader.Identifier, rxLen);
				for(uint8_t i = 0; i < rxLen; i++) {
					LOG_INF(" %02x", canRxBuf[i]);
				}
				LOG_INF("\n");
				
				if((canRxHeader.Identifier & 0xFFFFFF00) == CMD_SET_UPDATE_INFO) {
					if(*(uint32_t*)&canRxBuf[0] == FLAG_APP_NEED_UPDATE && *(uint32_t*)&canRxBuf[4] < APP_MAX_SIZE) {
						if(0 != set_update_info(*(uint64_t*)&canRxBuf[0])) {
							LOG_ERR("APP set update info(flag[0x%08x], fileSize[%d]) err\n", *(uint32_t*)canRxBuf, *((uint32_t*)canRxBuf + 1));
							canTxBuf[0] = STA_UPDATE_SET_FLAG_ERR;
						} else {
							LOG_ERR("APP set update info(flag[0x%08x], fileSize[%d]) OK\n", *(uint32_t*)canRxBuf, *((uint32_t*)canRxBuf + 1));
							canTxBuf[0] = STA_OK;
						}
						canSend(canRxHeader.FDFormat == FDCAN_FD_CAN, !REPORT, canRxHeader.Identifier, canTxBuf, 1);
						HAL_Delay(5);
						__disable_irq();
						HAL_NVIC_SystemReset();
					} else {
						LOG_ERR("set update info params err, flag[0x%08x] fileSize[%d]\n", *(uint32_t*)canRxBuf, *(uint32_t*)&canRxBuf[4]);
						canTxBuf[0] = (*(uint32_t*)canRxBuf == FLAG_APP_NEED_UPDATE) ? STA_UPDATE_FILE_SIZE_ERR : STA_UPDATE_FLAG_ERR;
						canSend(canRxHeader.FDFormat == FDCAN_FD_CAN, !REPORT, canRxHeader.Identifier, canTxBuf, 1);
					}
				} else {
					LOG_ERR("canfd recv NO define ID[0x%08x]\n", canRxHeader.Identifier);
					canTxBuf[0] = STA_FID_ERR;
					canSend(canRxHeader.Identifier == FDCAN_FD_CAN, !REPORT, canRxHeader.Identifier, canTxBuf, 1);
				}
			}
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int8_t set_update_info(uint64_t data)
{
	HAL_StatusTypeDef sta = HAL_OK;
	FLASH_EraseInitTypeDef eraseInit;
	uint32_t pageErr = 0;
	
	eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	eraseInit.Banks = FLASH_BANK_2;
	eraseInit.NbPages = 1;
	eraseInit.Page = 63;		// bank2 last page
	
	if(data != *(uint64_t*)UPDATE_FLAG_ADDR) {
		for(uint8_t i = 0; i < 3; i++) {
			__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
			sta = HAL_FLASH_Unlock();
			sta |= HAL_FLASHEx_Erase(&eraseInit, &pageErr);
			sta |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, UPDATE_FLAG_ADDR, data);
			sta |= HAL_FLASH_Lock();
			if(sta == HAL_OK) {
				return 0;
			}
		}
		return -1;
	}
	return 0;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
