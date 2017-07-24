/*
 * memoryMappedMode
 *
 * This sketch show how to use the mx25r6435f driver and the memory mapped mode.
 *
 */

#include <MX25R6435F.h>

#define BUFFER_SIZE         ((uint32_t)0x0100)
#define WRITE_READ_ADDR     ((uint32_t)0x0050)

uint8_t aTxBuffer[BUFFER_SIZE];
uint8_t aRxBuffer[BUFFER_SIZE];

static void     Fill_Buffer (uint8_t *pBuffer, uint32_t uwBufferLength, uint32_t uwOffset);

void setup() {
  Serial.begin(9600);
}

void loop() {
  /* QSPI info structure */
  uint8_t status, read_status = 0;
  uint16_t index = 0;
  uint8_t *mem_addr;

  Serial.println("*****************************************************************");
  Serial.println("********************* Memory mapped Test ************************");
  Serial.println("*****************************************************************");

  MX25R6435F.end();

  /*##-1- Configure the device ##########################################*/
  /* Device configuration */
  MX25R6435F.begin();
  status = MX25R6435F.status();

  if ((status == MEMORY_ERROR) || (status == MEMORY_SUSPENDED) || (status == MEMORY_BUSY))
  {
    Serial.println("Init : FAILED, Test Aborted");
  }
  else
  {
    Serial.println("Init : OK");

    /*##-2- Read & check the info #######################################*/
    if( (MX25R6435F.info(MEMORY_SIZE) != 0x800000)        ||
        (MX25R6435F.info(MEMORY_SECTOR_SIZE) != 0x1000)   ||
        (MX25R6435F.info(MEMORY_PAGE_SIZE) != 0x100)      ||
        (MX25R6435F.info(MEMORY_SECTOR_NUMBER) != 2048)   ||
        (MX25R6435F.info(MEMORY_PAGE_NUMBER) != 32768))
    {
      Serial.println("GET INFO : FAILED, Test Aborted");
    }
    else
    {
      Serial.println("GET INFO : OK");

      /*##-3- Erase memory ################################################*/
      if(MX25R6435F.erase(WRITE_READ_ADDR) != MEMORY_OK)
      {
        Serial.println("ERASE : FAILED, Test Aborted");
      }
      else
      {
        Serial.println("ERASE : OK");

        /*##-4- memory read/write access  #################################*/
        /* Fill the buffer to write */
        Fill_Buffer(aTxBuffer, BUFFER_SIZE, 0xD20F);

        for(uint32_t i = 0; i < 1; i++)
        {
          if(MX25R6435F.write(aTxBuffer, (i * 0x100), BUFFER_SIZE) != BUFFER_SIZE)
          {
            Serial.println("WRITE : FAILED, Test Aborted");
          }
        }

        /* Read back data from the QSPI memory */
        mem_addr = MX25R6435F.mapped();

        if(mem_addr != NULL) {
          for(uint32_t i = 0; i < 1; i++)
          {
            mem_addr += (i * 0x100);
            for (index = 0; index < BUFFER_SIZE; index++)
            {
              if (*mem_addr != aTxBuffer[index])
              {
                read_status++;
                Serial.println(read_status);
              }
              mem_addr++;
            }
          }


          if(read_status != 0)
          {
            Serial.println("READ : FAILED");
            Serial.println("Test Aborted");
          }
          else
          {
            Serial.println("READ :  OK");
            Serial.println("Test : OK");

          }
        } else {
          Serial.println("MAPPED MODE :  FAILED");
          Serial.println("Test Aborted");
        }
      }
    }
  }

  delay(5000);
}

static void Fill_Buffer(uint8_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset)
{
  uint32_t tmpIndex = 0;

  /* Put in global buffer different values */
  for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ )
  {
    pBuffer[tmpIndex] = tmpIndex + uwOffset;
  }
}
