/**
  ******************************************************************************
  * @file    mx25r6435f_driver.c
  * @brief   This file includes a standard driver for the MX25R6435F QSPI
  *          memory.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "core_debug.h"
#include "mx25r6435f_driver.h"

/* Private constants --------------------------------------------------------*/
#define QSPI_QUAD_DISABLE       0x0
#define QSPI_QUAD_ENABLE        0x1

#define QSPI_HIGH_PERF_DISABLE  0x0
#define QSPI_HIGH_PERF_ENABLE   0x1

/* Private functions ---------------------------------------------------------*/
static uint8_t QSPI_ResetMemory(XSPI_HandleTypeDef *hxspi);
static uint8_t QSPI_WriteEnable(XSPI_HandleTypeDef *hxspi);
static uint8_t QSPI_AutoPollingMemReady(XSPI_HandleTypeDef *hxspi, uint32_t Timeout);
static uint8_t QSPI_QuadMode(XSPI_HandleTypeDef *hxspi, uint8_t Operation);
static uint8_t QSPI_HighPerfMode(XSPI_HandleTypeDef *hxspi, uint8_t Operation);
static uint8_t qspi_setClockPrescaler(void);

/* Exported functions ---------------------------------------------------------*/
/**
  * @brief  Select a prescaler to have a clock frequency lower than the maximum.
  *         The MX25R6435F supports a maximum frequency of 80MHz.
  *         QSPI clock is connected to AHB bus clock.
  * @retval Clock prescaler. 0 means error.
  */
static uint8_t qspi_setClockPrescaler(void)
{
  uint8_t i;

  for (i = 1; i < 255; i++) {
    if ((HAL_RCC_GetHCLKFreq() / i) <= 80000000) {
      return i;
    }
  }

  return 0;
}

/**
  * @brief  Initializes the QSPI interface.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Init(QSPI_t *obj)
{
  if (obj == NULL) {
    return QSPI_ERROR;
  }
  XSPI_HandleTypeDef *handle = &(obj->handle);

  /* Determine the XSPI to use */
  XSPI_TypeDef *xspi_d0 = pinmap_peripheral(obj->pin_d0, PinMap_XSPI_DATA0);
  XSPI_TypeDef *xspi_d1 = pinmap_peripheral(obj->pin_d1, PinMap_XSPI_DATA1);
  XSPI_TypeDef *xspi_d2 = pinmap_peripheral(obj->pin_d2, PinMap_XSPI_DATA2);
  XSPI_TypeDef *xspi_d3 = pinmap_peripheral(obj->pin_d3, PinMap_XSPI_DATA3);

  XSPI_TypeDef *xspi_sclk = pinmap_peripheral(obj->pin_sclk, PinMap_XSPI_SCLK);
  XSPI_TypeDef *xspi_ssel = pinmap_peripheral(obj->pin_ssel, PinMap_XSPI_SSEL);

  /* Pins Dx/SSEL/SCLK must not be NP. */
  if (xspi_d0 == NP || xspi_d1 == NP || xspi_d2 == NP || xspi_sclk == NP || xspi_ssel == NP) {
    core_debug("ERROR: at least one QSPI pin has no peripheral\n");
    return QSPI_ERROR;
  }

  XSPI_TypeDef *spi_d01 = pinmap_merge_peripheral(xspi_d0, xspi_d1);
  XSPI_TypeDef *spi_d23 = pinmap_merge_peripheral(xspi_d2, xspi_d3);
  XSPI_TypeDef *spi_dx = pinmap_merge_peripheral(spi_d01, spi_d23);
  XSPI_TypeDef *spi_sxxx = pinmap_merge_peripheral(xspi_sclk, xspi_ssel);

  obj->qspi = pinmap_merge_peripheral(spi_dx, spi_sxxx);

  /* Are all pins connected to the same SPI instance? */
  if (obj->qspi == NP) {
    core_debug("ERROR: QSPI pins mismatch\n");
    return QSPI_ERROR;
  }

  handle->Instance = obj->qspi;

  /* Call the DeInit function to reset the driver */
  if (HAL_XSPI_DeInit(handle) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* System level initialization */
  BSP_QSPI_MspInit(obj);

#ifdef OCTOSPI
  /* OSPI initialization */
  handle->Init.FifoThreshold         = 4;
  handle->Init.DualQuad              = HAL_OSPI_DUALQUAD_DISABLE;
  handle->Init.MemoryType            = HAL_OSPI_MEMTYPE_MACRONIX;
  handle->Init.DeviceSize            = POSITION_VAL(MX25R6435F_FLASH_SIZE);
  handle->Init.ChipSelectHighTime    = 1;
  handle->Init.FreeRunningClock      = HAL_OSPI_FREERUNCLK_DISABLE;
  handle->Init.ClockMode             = HAL_OSPI_CLOCK_MODE_0;
  handle->Init.ClockPrescaler        = 4; /* QSPI clock = 110MHz / ClockPrescaler = 27.5 MHz */
  handle->Init.SampleShifting        = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  handle->Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
  handle->Init.ChipSelectBoundary    = 0;
  handle->Init.DelayBlockBypass      = HAL_OSPI_DELAY_BLOCK_USED;
#else /* OCTOSPI */
  /* QSPI initialization */
  /* High performance mode clock is limited to 80 MHz */
  handle->Init.ClockPrescaler     = qspi_setClockPrescaler() + 1; /* QSPI clock = systemCoreClock / (ClockPrescaler+1) */
  handle->Init.FifoThreshold      = 4;
  handle->Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE;
  handle->Init.FlashSize          = POSITION_VAL(MX25R6435F_FLASH_SIZE) - 1;
  handle->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  handle->Init.ClockMode          = QSPI_CLOCK_MODE_0;
#endif

  if (HAL_XSPI_Init(handle) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* QSPI memory reset */
  if (QSPI_ResetMemory(handle) != QSPI_OK) {
    return QSPI_NOT_SUPPORTED;
  }

  /* QSPI quad enable */
  if (QSPI_QuadMode(handle, QSPI_QUAD_ENABLE) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* High performance mode enable */
  if (QSPI_HighPerfMode(handle, QSPI_HIGH_PERF_ENABLE) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Re-configure the clock for the high performance mode */
  /* High performance mode clock is limited to 80 MHz */
  handle->Init.ClockPrescaler = qspi_setClockPrescaler(); /* QSPI clock = systemCoreClock / (ClockPrescaler+1) */

  if (HAL_XSPI_Init(handle) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Init(handle) != HAL_OK) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_DeInit(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

  /* Call the DeInit function to reset the driver */
  if (HAL_XSPI_DeInit(handle) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* System level De-initialization */
  BSP_QSPI_MspDeInit(obj);

  return QSPI_OK;
}

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @param  pData    : Pointer to data to be read
  * @param  ReadAddr : Read start address
  * @param  Size     : Size of data to read
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Read(QSPI_t *obj, uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the read command */
  sCommand.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction           = QUAD_INOUT_READ_CMD;
  sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address               = ReadAddr;
  sCommand.AddressMode           = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize           = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytes        = MX25R6435F_ALT_BYTES_NO_PE_MODE;
  sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
  sCommand.DataMode              = HAL_OSPI_DATA_4_LINES;
  sCommand.NbData                = Size;
  sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles           = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
  sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;

#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the read command */
  sCommand.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction        = QUAD_INOUT_READ_CMD;
  sCommand.AddressMode        = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
  sCommand.Address            = ReadAddr;
  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytes     = MX25R6435F_ALT_BYTES_NO_PE_MODE;
  sCommand.DataMode           = QSPI_DATA_4_LINES;
  sCommand.DummyCycles        = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
  sCommand.NbData             = Size;
  sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Configure the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Reception of the data */
  if (HAL_XSPI_Receive(handle, pData, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @param  pData     : Pointer to data to be written
  * @param  WriteAddr : Write start address
  * @param  Size      : Size of data to write
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Write(QSPI_t *obj, uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);
  uint32_t end_addr, current_size, current_addr;

  /* Calculation of the size between the write address and the end of the page */
  current_size = MX25R6435F_PAGE_SIZE - (WriteAddr % MX25R6435F_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size) {
    current_size = Size;
  }

  /* Initialize the adress variables */
  current_addr = WriteAddr;
  end_addr = WriteAddr + Size;

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the program command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the program command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = QUAD_PAGE_PROG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Perform the write page by page */
  do {
    sCommand.Address = current_addr;
    sCommand.NbData  = current_size;

    /* Enable write operations */
    if (QSPI_WriteEnable(handle) != QSPI_OK) {
      return QSPI_ERROR;
    }

    /* Configure the command */
    if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return QSPI_ERROR;
    }

    /* Transmission of the data */
    if (HAL_XSPI_Transmit(handle, pData, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for end of program */
    if (QSPI_AutoPollingMemReady(handle, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
      return QSPI_ERROR;
    }

    /* Update the address and size variables for next page programming */
    current_addr += current_size;
    pData += current_size;
    current_size = ((current_addr + MX25R6435F_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MX25R6435F_PAGE_SIZE;
  } while (current_addr < end_addr);

  return QSPI_OK;
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @param  BlockAddress : Block address to erase
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Erase_Block(QSPI_t *obj, uint32_t BlockAddress)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = BLOCK_ERASE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = BlockAddress;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = BLOCK_ERASE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = BlockAddress;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Enable write operations */
  if (QSPI_WriteEnable(handle) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Send the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure automatic polling mode to wait for end of erase */
  if (QSPI_AutoPollingMemReady(handle, MX25R6435F_BLOCK_ERASE_MAX_TIME) != QSPI_OK) {
    return QSPI_ERROR;
  }
  return QSPI_OK;
}

/**
  * @brief  Erases the specified sector of the QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @param  Sector : Sector address to erase (0 to 255)
  * @retval QSPI memory status
  * @note This function is non blocking meaning that sector erase
  *       operation is started but not completed when the function
  *       returns. Application has to call BSP_QSPI_GetStatus()
  *       to know when the device is available again (i.e. erase operation
  *       completed).
  */
uint8_t BSP_QSPI_Erase_Sector(QSPI_t *obj, uint32_t Sector)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

  if (Sector >= (uint32_t)(MX25R6435F_FLASH_SIZE / MX25R6435F_SECTOR_SIZE)) {
    return QSPI_ERROR;
  }

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = SECTOR_ERASE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = (Sector * MX25R6435F_SECTOR_SIZE);
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = SECTOR_ERASE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = (Sector * MX25R6435F_SECTOR_SIZE);
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Enable write operations */
  if (QSPI_WriteEnable(handle) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Send the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  Erases the entire QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Erase_Chip(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = CHIP_ERASE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = CHIP_ERASE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Enable write operations */
  if (QSPI_WriteEnable(handle) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Send the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure automatic polling mode to wait for end of erase */
  if (QSPI_AutoPollingMemReady(handle, MX25R6435F_CHIP_ERASE_MAX_TIME) != QSPI_OK) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  Reads current status of the QSPI memory.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_GetStatus(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);
  uint8_t reg;
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the read security register command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_SEC_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData             = 1;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the read security register command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_SEC_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Configure the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Reception of the data */
  if (HAL_XSPI_Receive(handle, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Check the value of the register */
  if ((reg & (MX25R6435F_SECR_P_FAIL | MX25R6435F_SECR_E_FAIL)) != 0) {
    return QSPI_ERROR;
  } else if ((reg & (MX25R6435F_SECR_PSB | MX25R6435F_SECR_ESB)) != 0) {
    return QSPI_SUSPENDED;
  }

  /* Initialize the read status register command */
  sCommand.Instruction = READ_STATUS_REG_CMD;

  /* Configure the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Reception of the data */
  if (HAL_XSPI_Receive(handle, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Check the value of the register */
  if ((reg & MX25R6435F_SR_WIP) != 0) {
    return QSPI_BUSY;
  } else {
    return QSPI_OK;
  }
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo : pointer on the configuration structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_GetInfo(QSPI_Info *pInfo)
{
  /* Configure the structure with the memory configuration */
  pInfo->FlashSize          = MX25R6435F_FLASH_SIZE;
  pInfo->EraseSectorSize    = MX25R6435F_SECTOR_SIZE;
  pInfo->EraseSectorsNumber = (MX25R6435F_FLASH_SIZE / MX25R6435F_SECTOR_SIZE);
  pInfo->ProgPageSize       = MX25R6435F_PAGE_SIZE;
  pInfo->ProgPagesNumber    = (MX25R6435F_FLASH_SIZE / MX25R6435F_PAGE_SIZE);

  return QSPI_OK;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_EnableMemoryMappedMode(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef      sCommand;
  OSPI_MemoryMappedTypeDef sMemMappedCfg;

  /* Configure the command for the read instruction */
  sCommand.OperationType         = HAL_OSPI_OPTYPE_READ_CFG;
  sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction           = QUAD_INOUT_READ_CMD;
  sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode           = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize           = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytes        = MX25R6435F_ALT_BYTES_NO_PE_MODE;
  sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
  sCommand.DataMode              = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles           = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
  sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure the command for the program instruction */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
  sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DummyCycles        = 0;

  /* Configure the command */
  if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure the memory mapped mode */
  sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_OSPI_MemoryMapped(handle, &sMemMappedCfg) != HAL_OK) {
    return QSPI_ERROR;
  }
#else /* OCTOSPI */
  QSPI_CommandTypeDef      sCommand;
  QSPI_MemoryMappedTypeDef sMemMappedCfg;

  /* Configure the command for the read instruction */
  sCommand.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction        = QUAD_INOUT_READ_CMD;
  sCommand.AddressMode        = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytes     = MX25R6435F_ALT_BYTES_NO_PE_MODE;
  sCommand.DataMode           = QSPI_DATA_4_LINES;
  sCommand.DummyCycles        = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
  sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the memory mapped mode */
  sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_QSPI_MemoryMapped(handle, &sCommand, &sMemMappedCfg) != HAL_OK) {
    return QSPI_ERROR;
  }
#endif /* OCTOSPI */

  return QSPI_OK;
}

/**
  * @brief  This function suspends an ongoing erase command.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_SuspendErase(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

  /* Check whether the device is busy (erase operation is
  in progress).
  */
  if (BSP_QSPI_GetStatus(obj) == QSPI_BUSY) {
#ifdef OCTOSPI
    OSPI_RegularCmdTypeDef sCommand;

    /* Initialize the suspend command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = PROG_ERASE_SUSPEND_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
    QSPI_CommandTypeDef sCommand;

    /* Initialize the erase command */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = PROG_ERASE_SUSPEND_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */
    /* Send the command */
    if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return QSPI_ERROR;
    }

    if (BSP_QSPI_GetStatus(obj) == QSPI_SUSPENDED) {
      return QSPI_OK;
    }

    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  This function resumes a paused erase command.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_ResumeErase(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

  /* Check whether the device is in suspended state */
  if (BSP_QSPI_GetStatus(obj) == QSPI_SUSPENDED) {
#ifdef OCTOSPI
    OSPI_RegularCmdTypeDef sCommand;

    /* Initialize the resume command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = PROG_ERASE_RESUME_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
    QSPI_CommandTypeDef sCommand;

    /* Initialize the erase command */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = PROG_ERASE_RESUME_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */
    /* Send the command */
    if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return QSPI_ERROR;
    }
    /*
    When this command is executed, the status register write in progress bit is set to 1, and
    the flag status register program erase controller bit is set to 0. This command is ignored
    if the device is not in a suspended state.
    */

    if (BSP_QSPI_GetStatus(obj) == QSPI_BUSY) {
      return QSPI_OK;
    }

    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  This function enter the QSPI memory in deep power down mode.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_EnterDeepPowerDown(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the deep power down command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = DEEP_POWER_DOWN_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the deep power down command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = DEEP_POWER_DOWN_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Send the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* ---          Memory takes 10us max to enter deep power down          --- */
  /* --- At least 30us should be respected before leaving deep power down --- */

  return QSPI_OK;
}

/**
  * @brief  This function leave the QSPI memory from deep power down mode.
  * @param  obj : pointer to QSPI_t structure
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_LeaveDeepPowerDown(QSPI_t *obj)
{
  XSPI_HandleTypeDef *handle = &(obj->handle);

#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = NO_OPERATION_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = NO_OPERATION_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Send the command */
  if (HAL_XSPI_Command(handle, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* --- A NOP command is sent to the memory, as the nCS should be low for at least 20 ns --- */
  /* ---                  Memory takes 35us min to leave deep power down                  --- */

  return QSPI_OK;
}

/**
  * @brief  Initializes the QSPI MSP.
  * @param  obj : pointer to QSPI_t structure
  * @retval None
  */
__weak void BSP_QSPI_MspInit(QSPI_t *obj)
{
#ifdef OCTOSPI

  /* Enable the OctoSPI memory interface clock */
  /* Reset the OctoSPI memory interface */
#if defined(OCTOSPI1)
  if (obj->qspi == OCTOSPI1) {
    __HAL_RCC_OSPI1_CLK_ENABLE();

    __HAL_RCC_OSPI1_FORCE_RESET();
    __HAL_RCC_OSPI1_RELEASE_RESET();
  }
#endif
#if defined(OCTOSPI2)
  if (obj->qspi == OCTOSPI2) {
    __HAL_RCC_OSPI2_CLK_ENABLE();

    __HAL_RCC_OSPI2_FORCE_RESET();
    __HAL_RCC_OSPI2_RELEASE_RESET();
  }
#endif
#else /* OCTOSPI */
  /* Enable the QuadSPI memory interface clock */
  __HAL_RCC_QSPI_CLK_ENABLE();

  /* Reset the QuadSPI memory interface */
  __HAL_RCC_QSPI_FORCE_RESET();
  __HAL_RCC_QSPI_RELEASE_RESET();
#endif /* OCTOSPI */

  /* Configure QSPI GPIO pins */
  pinmap_pinout(obj->pin_d0, PinMap_XSPI_DATA0);
  pinmap_pinout(obj->pin_d1, PinMap_XSPI_DATA1);
  pinmap_pinout(obj->pin_d2, PinMap_XSPI_DATA2);
  pinmap_pinout(obj->pin_d3, PinMap_XSPI_DATA3);
  pinmap_pinout(obj->pin_sclk, PinMap_XSPI_SCLK);
  pinmap_pinout(obj->pin_ssel, PinMap_XSPI_SSEL);
}

/**
  * @brief  De-Initializes the QSPI MSP.
  * @param  obj : pointer to QSPI_t structure
  * @retval None
  */
__weak void BSP_QSPI_MspDeInit(QSPI_t *obj)
{
  /* QSPI CLK, CS, D0-D3 GPIO pins de-configuration  */

  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_d0), STM_GPIO_PIN(obj->pin_d0));
  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_d1), STM_GPIO_PIN(obj->pin_d1));
  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_d2), STM_GPIO_PIN(obj->pin_d2));
  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_d3), STM_GPIO_PIN(obj->pin_d3));
  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_sclk), STM_GPIO_PIN(obj->pin_sclk));
  HAL_GPIO_DeInit((GPIO_TypeDef *)STM_PORT(obj->pin_ssel), STM_GPIO_PIN(obj->pin_ssel));

#ifdef OCTOSPI
#if defined(OCTOSPI1)
  /* Reset the OctoSPI memory interface */
  /* Disable the OctoSPI memory interface clock */
  if (obj->qspi == OCTOSPI1) {
    __HAL_RCC_OSPI1_FORCE_RESET();
    __HAL_RCC_OSPI1_RELEASE_RESET();

    __HAL_RCC_OSPI1_CLK_DISABLE();
  }
#endif
#if defined(OCTOSPI2)
  if (obj->qspi == OCTOSPI2) {
    __HAL_RCC_OSPI2_FORCE_RESET();
    __HAL_RCC_OSPI2_RELEASE_RESET();

    __HAL_RCC_OSPI2_CLK_DISABLE();
  }
#endif
#else /* OCTOSPI */
  /* Reset the QuadSPI memory interface */
  __HAL_RCC_QSPI_FORCE_RESET();
  __HAL_RCC_QSPI_RELEASE_RESET();

  /* Disable the QuadSPI memory interface clock */
  __HAL_RCC_QSPI_CLK_DISABLE();
#endif /* OCTOSPI */
}

/**
  * @brief  This function reset the QSPI memory.
  * @param  hxspi : QSPI handle
  * @retval None
  */
static uint8_t QSPI_ResetMemory(XSPI_HandleTypeDef *hxspi)
{
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Initialize the reset enable command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = RESET_ENABLE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Initialize the reset enable command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = RESET_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  /* Send the command */
  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Send the reset memory command */
  sCommand.Instruction = RESET_MEMORY_CMD;
  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure automatic polling mode to wait the memory is ready */
  if (QSPI_AutoPollingMemReady(hxspi, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hxspi : QSPI handle
  * @retval None
  */
static uint8_t QSPI_WriteEnable(XSPI_HandleTypeDef *hxspi)
{
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;
  OSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = WRITE_ENABLE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure automatic polling mode to wait for write enabling */
  sConfig.Match         = MX25R6435F_SR_WEL;
  sConfig.Mask          = MX25R6435F_SR_WEL;
  sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval      = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  sCommand.Instruction  = READ_STATUS_REG_CMD;
  sCommand.DataMode     = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData       = 1;
  sCommand.DataDtrMode  = HAL_OSPI_DATA_DTR_DISABLE;

  if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_OSPI_AutoPolling(hxspi, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }
#else /* OCTOSPI */
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hxspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Configure automatic polling mode to wait for write enabling */
  sConfig.Match           = MX25R6435F_SR_WEL;
  sConfig.Mask            = MX25R6435F_SR_WEL;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  sCommand.Instruction    = READ_STATUS_REG_CMD;
  sCommand.DataMode       = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(hxspi, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }
#endif /* OCTOSPI */

  return QSPI_OK;
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hxspi   : QSPI handle
  * @param  Timeout : Timeout for auto-polling
  * @retval None
  */
static uint8_t QSPI_AutoPollingMemReady(XSPI_HandleTypeDef *hxspi, uint32_t Timeout)
{
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;
  OSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData             = 1;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match         = 0;
  sConfig.Mask          = MX25R6435F_SR_WIP;
  sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval      = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_OSPI_AutoPolling(hxspi, &sConfig, Timeout) != HAL_OK) {
    return QSPI_ERROR;
  }
#else /* OCTOSPI */
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match           = 0;
  sConfig.Mask            = MX25R6435F_SR_WIP;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling(hxspi, &sCommand, &sConfig, Timeout) != HAL_OK) {
    return QSPI_ERROR;
  }
#endif /* OCTOSPI */

  return QSPI_OK;
}

/**
  * @brief  This function enables/disables the Quad mode of the memory.
  * @param  hxspi     : QSPI handle
  * @param  Operation : QSPI_QUAD_ENABLE or QSPI_QUAD_DISABLE mode
  * @retval None
  */
static uint8_t QSPI_QuadMode(XSPI_HandleTypeDef *hxspi, uint8_t Operation)
{
  uint8_t reg;
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Read status register */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.NbData             = 1;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Read status register */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Receive(hxspi, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Enable write operations */
  if (QSPI_WriteEnable(hxspi) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Activate/deactivate the Quad mode */
  if (Operation == QSPI_QUAD_ENABLE) {
    SET_BIT(reg, MX25R6435F_SR_QE);
  } else {
    CLEAR_BIT(reg, MX25R6435F_SR_QE);
  }

  sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Transmit(hxspi, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Wait that memory is ready */
  if (QSPI_AutoPollingMemReady(hxspi, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Check the configuration has been correctly done */
  sCommand.Instruction = READ_STATUS_REG_CMD;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Receive(hxspi, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if ((((reg & MX25R6435F_SR_QE) == 0) && (Operation == QSPI_QUAD_ENABLE)) ||
      (((reg & MX25R6435F_SR_QE) != 0) && (Operation == QSPI_QUAD_DISABLE))) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

/**
  * @brief  This function enables/disables the high performance mode of the memory.
  * @param  hxspi     : QSPI handle
  * @param  Operation : QSPI_HIGH_PERF_ENABLE or QSPI_HIGH_PERF_DISABLE high performance mode
  * @retval None
  */
static uint8_t QSPI_HighPerfMode(XSPI_HandleTypeDef *hxspi, uint8_t Operation)
{
  uint8_t reg[3];
#ifdef OCTOSPI
  OSPI_RegularCmdTypeDef sCommand;

  /* Read status register */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.NbData             = 1;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
#else /* OCTOSPI */
  QSPI_CommandTypeDef sCommand;

  /* Read status register */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
#endif /* OCTOSPI */

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Receive(hxspi, &(reg[0]), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Read configuration registers */
  sCommand.Instruction = READ_CFG_REG_CMD;
  sCommand.NbData      = 2;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Receive(hxspi, &(reg[1]), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Enable write operations */
  if (QSPI_WriteEnable(hxspi) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Activate/deactivate the Quad mode */
  if (Operation == QSPI_HIGH_PERF_ENABLE) {
    SET_BIT(reg[2], MX25R6435F_CR2_LH_SWITCH);
  } else {
    CLEAR_BIT(reg[2], MX25R6435F_CR2_LH_SWITCH);
  }

  sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
  sCommand.NbData      = 3;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Transmit(hxspi, &(reg[0]), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  /* Wait that memory is ready */
  if (QSPI_AutoPollingMemReady(hxspi, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
    return QSPI_ERROR;
  }

  /* Check the configuration has been correctly done */
  sCommand.Instruction = READ_CFG_REG_CMD;
  sCommand.NbData      = 2;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if (HAL_XSPI_Receive(hxspi, &(reg[0]), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return QSPI_ERROR;
  }

  if ((((reg[1] & MX25R6435F_CR2_LH_SWITCH) == 0) && (Operation == QSPI_HIGH_PERF_ENABLE)) ||
      (((reg[1] & MX25R6435F_CR2_LH_SWITCH) != 0) && (Operation == QSPI_HIGH_PERF_DISABLE))) {
    return QSPI_ERROR;
  }

  return QSPI_OK;
}

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
