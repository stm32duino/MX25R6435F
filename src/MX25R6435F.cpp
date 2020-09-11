/**
  ******************************************************************************
  * @file    MX25R6435F.c
  * @author  WI6LABS
  * @version V1.0.0
  * @date    19-July-2017
  * @brief   MX25R6435F library for STM32 Arduino
  *
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

#include "MX25R6435F.h"

MX25R6435FClass MX25R6435F;

MX25R6435FClass::MX25R6435FClass(): initDone(0)
{
}

void MX25R6435FClass::begin(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t sclk, uint8_t ssel)
{
  _qspi.pin_d0 = digitalPinToPinName(data0);
  _qspi.pin_d1 = digitalPinToPinName(data1);
  _qspi.pin_d2 = digitalPinToPinName(data2);
  _qspi.pin_d3 = digitalPinToPinName(data3);
  _qspi.pin_sclk = digitalPinToPinName(sclk);
  _qspi.pin_ssel = digitalPinToPinName(ssel);

  if (BSP_QSPI_Init(&_qspi) == MEMORY_OK) {
    initDone = 1;
  }
}

void MX25R6435FClass::end(void)
{
  BSP_QSPI_DeInit(&_qspi);
  initDone = 0;
}

uint32_t MX25R6435FClass::write(uint8_t data, uint32_t addr)
{
  return write(&data, addr, 1);
}

uint32_t MX25R6435FClass::write(uint8_t *pData, uint32_t addr, uint32_t size)
{
  if ((pData == NULL) || (initDone == 0)) {
    return 0;
  }

  if (BSP_QSPI_Write(&_qspi, pData, addr, size) != MEMORY_OK) {
    return 0;
  }

  return size;
}

uint8_t MX25R6435FClass::read(uint32_t addr)
{
  uint8_t data;

  read(&data, addr, 1);

  return data;
}

void MX25R6435FClass::read(uint8_t *pData, uint32_t addr, uint32_t size)
{
  if ((pData != NULL) && (initDone == 1)) {
    BSP_QSPI_Read(&_qspi, pData, addr, size);
  }
}

uint8_t *MX25R6435FClass::mapped(void)
{
  if (BSP_QSPI_EnableMemoryMappedMode(&_qspi) != MEMORY_OK) {
    return NULL;
  }

  return (uint8_t *)MEMORY_MAPPED_ADDRESS;
}

uint8_t MX25R6435FClass::erase(uint32_t addr)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_Erase_Block(&_qspi, addr);
}

uint8_t MX25R6435FClass::eraseChip(void)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_Erase_Chip(&_qspi);
}

uint8_t MX25R6435FClass::eraseSector(uint32_t sector)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_Erase_Sector(&_qspi, sector);
}

uint8_t MX25R6435FClass::suspendErase(void)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_SuspendErase(&_qspi);
}

uint8_t MX25R6435FClass::resumeErase(void)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_ResumeErase(&_qspi);
}

uint8_t MX25R6435FClass::sleep(void)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_EnterDeepPowerDown(&_qspi);
}

uint8_t MX25R6435FClass::wakeup(void)
{
  if (initDone == 0) {
    return MEMORY_ERROR;
  }

  return BSP_QSPI_LeaveDeepPowerDown(&_qspi);
}

uint8_t MX25R6435FClass::status(void)
{
  return BSP_QSPI_GetStatus(&_qspi);
}

uint32_t MX25R6435FClass::info(memory_info_t info)
{
  uint32_t res;
  QSPI_Info pInfo;

  BSP_QSPI_GetInfo(&pInfo);

  switch (info) {
    case MEMORY_SIZE:
      res = pInfo.FlashSize;
      break;

    case MEMORY_SECTOR_SIZE:
      res = pInfo.EraseSectorSize;
      break;

    case MEMORY_SECTOR_NUMBER:
      res = pInfo.EraseSectorsNumber;
      break;

    case MEMORY_PAGE_SIZE:
      res = pInfo.ProgPageSize;
      break;

    case MEMORY_PAGE_NUMBER:
      res = pInfo.ProgPagesNumber;
      break;

    default:
      res = 0;
      break;
  }

  return res;
}

uint32_t MX25R6435FClass::length(void)
{
  return info(MEMORY_SIZE);
}
