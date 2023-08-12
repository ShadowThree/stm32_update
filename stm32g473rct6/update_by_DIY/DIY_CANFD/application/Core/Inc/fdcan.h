/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fdcan.h
  * @brief   This file contains all the function prototypes for
  *          the fdcan.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FDCAN_H__
#define __FDCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern FDCAN_HandleTypeDef hfdcan1;

/* USER CODE BEGIN Private defines */
#define FDCAN_BUF_LEN	64

#define CANFD   1
#define REPORT  1

extern FDCAN_TxHeaderTypeDef   canTxHeader;
extern FDCAN_RxHeaderTypeDef   canRxHeader;
extern uint8_t                 canTxBuf[FDCAN_BUF_LEN];
extern uint8_t                 canRxBuf[FDCAN_BUF_LEN];
/* USER CODE END Private defines */

void MX_FDCAN1_Init(void);

/* USER CODE BEGIN Prototypes */
void FDCAN_Config(void);
uint8_t can_getLen(uint32_t lenCode);
void canSend(uint8_t isFD, uint8_t isReport, uint32_t message_id, uint8_t* data, uint8_t len);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __FDCAN_H__ */

