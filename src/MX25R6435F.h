/**
  ******************************************************************************
  * @file    MX25R6435F.h
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

#ifndef _MX25R6435F_H_
#define _MX25R6435F_H_

#include "Arduino.h"
#include "mx25r6435f_driver.h"

/*
 * For backward compatibility define the xSPI pins used by:
 * B-L475E-IOT01A and B-L4S5I-IOT01A
 */
#ifndef MX25R6435F_D0
  #define MX25R6435F_D0           PE12
#endif
#ifndef MX25R6435F_D1
  #define MX25R6435F_D1           PE13
#endif
#ifndef MX25R6435F_D2
  #define MX25R6435F_D2           PE14
#endif
#ifndef MX25R6435F_D3
  #define MX25R6435F_D3           PE15
#endif
#ifndef MX25R6435F_SCLK
  #define MX25R6435F_SCLK         PE10
#endif
#ifndef MX25R6435F_SSEL
  #define MX25R6435F_SSEL         PE11
#endif

/* Memory configuration paremeters */
typedef enum {
  MEMORY_SIZE,
  MEMORY_SECTOR_SIZE,
  MEMORY_SECTOR_NUMBER,
  MEMORY_PAGE_SIZE,
  MEMORY_PAGE_NUMBER
} memory_info_t;

/* Memory Error codes */
#define MEMORY_OK             QSPI_OK
#define MEMORY_ERROR          QSPI_ERROR
#define MEMORY_BUSY           QSPI_BUSY
#define MEMORY_NOT_SUPPORTED  QSPI_NOT_SUPPORTED
#define MEMORY_SUSPENDED      QSPI_SUSPENDED

/* Base address of the memory in mapped mode */
#define MEMORY_MAPPED_ADDRESS ((uint32_t)0x90000000)

class MX25R6435FClass {
  public:
    MX25R6435FClass();

    // setDx/SCLK/SSEL have to be called before begin()
    void setDx(uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3)
    {
      _qspi.pin_d0 = digitalPinToPinName(data0);
      _qspi.pin_d1 = digitalPinToPinName(data1);
      _qspi.pin_d2 = digitalPinToPinName(data2);
      _qspi.pin_d3 = digitalPinToPinName(data3);
    };
    void setSCLK(uint32_t sclk)
    {
      _qspi.pin_sclk = digitalPinToPinName(sclk);
    };
    void setSSEL(uint32_t ssel)
    {
      _qspi.pin_ssel = digitalPinToPinName(ssel);
    };

    void setDx(PinName data0, PinName data1, PinName data2, PinName data3)
    {
      _qspi.pin_d0 = (data0);
      _qspi.pin_d1 = (data1);
      _qspi.pin_d2 = (data2);
      _qspi.pin_d3 = (data3);
    };
    void setSCLK(PinName sclk)
    {
      _qspi.pin_sclk = (sclk);
    };
    void setSSEL(PinName ssel)
    {
      _qspi.pin_ssel = (ssel);
    };

    /* Initializes the memory interface. */
    void begin(uint8_t data0 = MX25R6435F_D0, uint8_t data1 = MX25R6435F_D1, uint8_t data2 = MX25R6435F_D2, uint8_t data3 = MX25R6435F_D3, uint8_t sclk = MX25R6435F_SCLK, uint8_t ssel = MX25R6435F_SSEL);

    /* De-Initializes the memory interface. */
    void end(void);

    /**
      * @brief  Writes one byte to the memory.
      * @param  data      : data to be written
      * @param  addr      : Write start address
      * @retval number of data written. 0 indicates a failure.
      */
    uint32_t write(uint8_t data, uint32_t addr);

    /**
      * @brief  Writes an amount of data to the memory.
      * @param  pData     : Pointer to data to be written
      * @param  addr : Write start address
      * @param  size      : Size of data to write
      * @retval number of data written. 0 indicates a failure.
      */
    uint32_t write(uint8_t *pData, uint32_t addr, uint32_t size);

    /**
      * @brief  Reads one byte from the memory.
      * @param  addr : Read start address
      * @retval data read
      */
    uint8_t read(uint32_t addr);

    /**
      * @brief  Reads an amount of data from the memory.
      * @param  pData    : Pointer to data to be read
      * @param  addr     : Read start address
      * @param  size     : Size of data to read
      */
    void read(uint8_t *pData, uint32_t addr, uint32_t size);

    /**
      * @brief  Configure the memory in mapped mode
      * @retval pointer to the memory
      */
    uint8_t *mapped(void);

    /**
      * @brief  Erases the specified block of the memory.
      * @param  addr : Block address to erase
      * @retval memory status
      */
    uint8_t erase(uint32_t addr);

    /**
      * @brief  Erases the entire memory.
      * @retval memory status
      */
    uint8_t eraseChip(void);

    /**
      * @brief  Erases the specified sector of the memory.
      * @param  sector : Sector address to erase
      * @retval memory status
      * @note This function is non blocking meaning that sector erase
      *       operation is started but not completed when the function
      *       returns. Application has to call BSP_QSPI_GetStatus()
      *       to know when the device is available again (i.e. erase operation
      *       completed).
      */
    uint8_t eraseSector(uint32_t sector);

    /**
      * @brief  This function suspends an ongoing eraseSector command.
      * @retval memory status
      */
    uint8_t suspendErase(void);

    /**
      * @brief  This function resumes a paused eraseSector command.
      * @retval memory status
      */
    uint8_t resumeErase(void);

    /**
      * @brief  This function enter the memory in deep power down mode.
      * @retval memory status
      */
    uint8_t sleep(void);

    /**
      * @brief  This function leave the memory from deep power down mode.
      * @retval memory status
      */
    uint8_t wakeup(void);

    /* Reads current status of the memory.*/
    uint8_t status(void);

    /**
      * @brief  Return the configuration of the memory.
      * @param  info :  value to read. This parameter should be a value of
      *                 memory_info_t.
      * @retval configuration value read
      */
    uint32_t info(memory_info_t info);

    /* Return the total size of the memory */
    uint32_t length(void);

  private:
    uint8_t initDone;
    QSPI_t _qspi;
};

extern MX25R6435FClass MX25R6435F;

#endif /* _MX25R6435F_H_ */
