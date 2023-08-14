/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fdcan.c
  * @brief   This file provides code for the configuration
  *          of the FDCAN instances.
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
#include "fdcan.h"

/* USER CODE BEGIN 0 */
#include "dbger.h"

FDCAN_TxHeaderTypeDef   canTxHeader;
FDCAN_RxHeaderTypeDef   canRxHeader;
uint8_t                 canTxBuf[FDCAN_BUF_LEN] = {0};
uint8_t                 canRxBuf[FDCAN_BUF_LEN] = {0};
/* USER CODE END 0 */

FDCAN_HandleTypeDef hfdcan1;

/* FDCAN1 init function */
void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 17;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 15;
  hfdcan1.Init.NominalTimeSeg2 = 4;
  hfdcan1.Init.DataPrescaler = 17;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 15;
  hfdcan1.Init.DataTimeSeg2 = 4;
  hfdcan1.Init.StdFiltersNbr = 0;
  hfdcan1.Init.ExtFiltersNbr = 1;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* fdcanHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(fdcanHandle->Instance==FDCAN1)
  {
  /* USER CODE BEGIN FDCAN1_MspInit 0 */

  /* USER CODE END FDCAN1_MspInit 0 */

  /** Initializes the peripherals clocks
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* FDCAN1 clock enable */
    __HAL_RCC_FDCAN_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**FDCAN1 GPIO Configuration
    PA11     ------> FDCAN1_RX
    PA12     ------> FDCAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN FDCAN1_MspInit 1 */

  /* USER CODE END FDCAN1_MspInit 1 */
  }
}

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* fdcanHandle)
{

  if(fdcanHandle->Instance==FDCAN1)
  {
  /* USER CODE BEGIN FDCAN1_MspDeInit 0 */

  /* USER CODE END FDCAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_FDCAN_CLK_DISABLE();

    /**FDCAN1 GPIO Configuration
    PA11     ------> FDCAN1_RX
    PA12     ------> FDCAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

  /* USER CODE BEGIN FDCAN1_MspDeInit 1 */

  /* USER CODE END FDCAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void FDCAN_Config(void)
{
	FDCAN_FilterTypeDef canFilter;
	uint32_t filterId = 0x01040000;		// extend frame, valid ID is low 29 bit
	uint32_t filterMask = 0x1FFF0000;	// 1 is mean need same, 0 is don't care
	
	canFilter.IdType = FDCAN_EXTENDED_ID;
	canFilter.FilterIndex = 0;
	canFilter.FilterType = FDCAN_FILTER_MASK;
	canFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	canFilter.FilterID1 = filterId;
	canFilter.FilterID2 = filterMask;
	if(HAL_FDCAN_ConfigFilter(&hfdcan1, &canFilter) != HAL_OK) {
		LOG_ERR("canfd config filter err\n");
		Error_Handler();
	}
	if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK) {
		LOG_ERR("canfd config global filter err\n");
		Error_Handler();
	}
	if(HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
		LOG_ERR("fdcan start err\n");
		Error_Handler();
	}
}

uint32_t can_getLenCode(uint8_t len)
{
	if(len == 0) {
		return FDCAN_DLC_BYTES_0;
	} else if(len == 1) {
		return FDCAN_DLC_BYTES_1;
	} else if(len == 2) {
		return FDCAN_DLC_BYTES_2;
	} else if(len == 3) {
		return FDCAN_DLC_BYTES_3;
	} else if(len == 4) {
		return FDCAN_DLC_BYTES_4;
	} else if(len == 5) {
		return FDCAN_DLC_BYTES_5;
	} else if(len == 6) {
		return FDCAN_DLC_BYTES_6;
	} else if(len == 7) {
		return FDCAN_DLC_BYTES_7;
	} else if(len == 8) {
		return FDCAN_DLC_BYTES_8;
	} else if(len <= 12) {
		return FDCAN_DLC_BYTES_12;
	} else if(len <= 16) {
		return FDCAN_DLC_BYTES_16;
	} else if(len <= 20) {
		return FDCAN_DLC_BYTES_20;
	} else if(len <= 24) {
		return FDCAN_DLC_BYTES_24;
	} else if(len <= 32) {
		return FDCAN_DLC_BYTES_32;
	} else if(len <= 48) {
		return FDCAN_DLC_BYTES_48;
	} else if(len <= 64) {
		return FDCAN_DLC_BYTES_64;
	} else {
		LOG_ERR("can send len[%d] err\n", len);
		return FDCAN_DLC_BYTES_0;
	}
}

uint8_t can_getLen(uint32_t lenCode)
{
	if(lenCode == FDCAN_DLC_BYTES_0) {
		return 0;
	} else if(lenCode == FDCAN_DLC_BYTES_1) {
		return 1;
	} else if(lenCode == FDCAN_DLC_BYTES_2) {
		return 2;
	} else if(lenCode == FDCAN_DLC_BYTES_3) {
		return 3;
	} else if(lenCode == FDCAN_DLC_BYTES_4) {
		return 4;
	} else if(lenCode == FDCAN_DLC_BYTES_5) {
		return 5;
	} else if(lenCode == FDCAN_DLC_BYTES_6) {
		return 6;
	} else if(lenCode == FDCAN_DLC_BYTES_7) {
		return 7;
	} else if(lenCode == FDCAN_DLC_BYTES_8) {
		return 8;
	} else if(lenCode == FDCAN_DLC_BYTES_12) {
		return 12;
	} else if(lenCode == FDCAN_DLC_BYTES_16) {
		return 16;
	} else if(lenCode == FDCAN_DLC_BYTES_20) {
		return 20;
	} else if(lenCode == FDCAN_DLC_BYTES_24) {
		return 24;
	} else if(lenCode == FDCAN_DLC_BYTES_32) {
		return 32;
	} else if(lenCode == FDCAN_DLC_BYTES_48) {
		return 48;
	} else if(lenCode == FDCAN_DLC_BYTES_64) {
		return 64;
	} else {
		LOG_ERR("fdcan lenCode[0x%08x] err\n", lenCode);
		return 0;
	}
}

void canSend(uint8_t isFD, uint8_t isReport, uint32_t message_id, uint8_t* data, uint8_t len)
{
	if(isReport) {
		static uint8_t replyId = 128;		// range[128~255]
		message_id |= replyId++;
		if(replyId == 0)
			replyId = 128;
	}
	
	canTxHeader.Identifier = message_id;
	canTxHeader.IdType = FDCAN_EXTENDED_ID;
	canTxHeader.TxFrameType = FDCAN_DATA_FRAME;
	canTxHeader.DataLength = can_getLenCode(len);
	canTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	canTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	canTxHeader.FDFormat = isFD ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
	canTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	canTxHeader.MessageMarker = 0;
	
	len = can_getLen(canTxHeader.DataLength);

	uint16_t cnt = 0;
	while(HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) {
		// wait tx fifo free, need timeout break.
		HAL_Delay(1);
		if(10 <= ++cnt) {
			LOG_ERR("fdcan tx fifo full, send failed\n");
			return;
		}
	}
		
	if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &canTxHeader, data) == HAL_OK) {
		if(isReport) {
			LOG_INF("report(ID=0x%08x, len=%02d):", message_id, len);
		} else {
			LOG_INF("send(ID=0x%08x, len=%02d):", message_id, len);
		}
		for(uint8_t i = 0; i < len; i++) {
			LOG_INF(" %02x", data[i]);
		}
		LOG_INF("\n");
	} else {
		LOG_ERR("fdcan send err\n");
	}
}
/* USER CODE END 1 */
