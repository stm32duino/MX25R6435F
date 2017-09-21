/**
  ******************************************************************************
  * @file    MX25R6435F.h
  * @author  WI6LABS
  * @version V1.0.0
  * @date    19-July-2017
  * @brief   MX25R6435F library for STM32 Arduino
  *
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

#ifndef _MX25R6435F_H_
#define _MX25R6435F_H_

#include "mx25r6435f_driver.h"

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

class MX25R6435FClass
{
  public:
    MX25R6435FClass();

    /* Initializes the memory interface. */
    void begin(void);

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
};

extern MX25R6435FClass MX25R6435F;

#endif /* _MX25R6435F_H_ */
