/**
  ******************************************************************************
  * @file    mx25r6435f_driver.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    21-April-2017
  * @brief   This file contains the common defines and functions prototypes for
  *          the mx25r6435f_driver.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _MX25R6435F_DRIVER_H
#define _MX25R6435F_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32_def.h"
#include "PeripheralPins.h"
#include "mx25r6435f_desc.h"


/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32L475E_IOT01
  * @{
  */

/** @addtogroup STM32L475E_IOT01_QSPI
  * @{
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup STM32L475E_IOT01_QSPI_Exported_Constants QSPI Exported Constants
  * @{
  */

#if defined(OCTOSPI1) || defined(OCTOSPI2)
#define OCTOSPI
#define XSPI_HandleTypeDef  OSPI_HandleTypeDef
#define XSPI_TypeDef        OCTOSPI_TypeDef
#define PinMap_XSPI_DATA0   PinMap_OCTOSPI_DATA0
#define PinMap_XSPI_DATA1   PinMap_OCTOSPI_DATA1
#define PinMap_XSPI_DATA2   PinMap_OCTOSPI_DATA2
#define PinMap_XSPI_DATA3   PinMap_OCTOSPI_DATA3
#define PinMap_XSPI_SCLK    PinMap_OCTOSPI_SCLK
#define PinMap_XSPI_SSEL    PinMap_OCTOSPI_SSEL
#define HAL_XSPI_Init       HAL_OSPI_Init
#define HAL_XSPI_DeInit     HAL_OSPI_DeInit
#define HAL_XSPI_TIMEOUT_DEFAULT_VALUE HAL_OSPI_TIMEOUT_DEFAULT_VALUE
#define HAL_XSPI_Command    HAL_OSPI_Command
#define HAL_XSPI_Transmit   HAL_OSPI_Transmit
#define HAL_XSPI_Receive    HAL_OSPI_Receive
#elif defined(QUADSPI)
#define XSPI_HandleTypeDef  QSPI_HandleTypeDef
#define XSPI_TypeDef        QUADSPI_TypeDef
#define PinMap_XSPI_DATA0   PinMap_QUADSPI_DATA0
#define PinMap_XSPI_DATA1   PinMap_QUADSPI_DATA1
#define PinMap_XSPI_DATA2   PinMap_QUADSPI_DATA2
#define PinMap_XSPI_DATA3   PinMap_QUADSPI_DATA3
#define PinMap_XSPI_SCLK    PinMap_QUADSPI_SCLK
#define PinMap_XSPI_SSEL    PinMap_QUADSPI_SSEL
#define HAL_XSPI_Init       HAL_QSPI_Init
#define HAL_XSPI_DeInit     HAL_QSPI_DeInit
#define HAL_XSPI_TIMEOUT_DEFAULT_VALUE HAL_QPSI_TIMEOUT_DEFAULT_VALUE
#define HAL_XSPI_Command    HAL_QSPI_Command
#define HAL_XSPI_Transmit   HAL_QSPI_Transmit
#define HAL_XSPI_Receive    HAL_QSPI_Receive
#else
#error "QSPI feature not available. MX25R6435F library compilation failed."
#endif /* OCTOSPIx */

/* QSPI Error codes */
#define QSPI_OK            ((uint8_t)0x00)
#define QSPI_ERROR         ((uint8_t)0x01)
#define QSPI_BUSY          ((uint8_t)0x02)
#define QSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define QSPI_SUSPENDED     ((uint8_t)0x08)

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup STM32L475E_IOT01_QSPI_Exported_Types QSPI Exported Types
  * @{
  */
/* QSPI Info */
typedef struct {
  uint32_t FlashSize;          /*!< Size of the flash */
  uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
  uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} QSPI_Info;


typedef struct {
  XSPI_HandleTypeDef handle;
  XSPI_TypeDef *qspi;
  PinName pin_d0;
  PinName pin_d1;
  PinName pin_d2;
  PinName pin_d3;
  PinName pin_sclk;
  PinName pin_ssel;
} QSPI_t;

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup STM32L475E_IOT01_QSPI_Exported_Functions QSPI Exported Functions
  * @{
  */
uint8_t BSP_QSPI_Init                  (QSPI_t *obj);
uint8_t BSP_QSPI_DeInit                (QSPI_t *obj);
uint8_t BSP_QSPI_Read                  (QSPI_t *obj, uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t BSP_QSPI_Write                 (QSPI_t *obj, uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t BSP_QSPI_Erase_Block           (QSPI_t *obj, uint32_t BlockAddress);
uint8_t BSP_QSPI_Erase_Sector          (QSPI_t *obj, uint32_t Sector);
uint8_t BSP_QSPI_Erase_Chip            (QSPI_t *obj);
uint8_t BSP_QSPI_GetStatus             (QSPI_t *obj);
uint8_t BSP_QSPI_GetInfo               (QSPI_Info* pInfo);
uint8_t BSP_QSPI_EnableMemoryMappedMode(QSPI_t *obj);
uint8_t BSP_QSPI_SuspendErase          (QSPI_t *obj);
uint8_t BSP_QSPI_ResumeErase           (QSPI_t *obj);
uint8_t BSP_QSPI_EnterDeepPowerDown    (QSPI_t *obj);
uint8_t BSP_QSPI_LeaveDeepPowerDown    (QSPI_t *obj);

void BSP_QSPI_MspInit(QSPI_t *obj);
void BSP_QSPI_MspDeInit(QSPI_t *obj);
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* _MX25R6435F_DRIVER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
