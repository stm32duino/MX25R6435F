/*
 * demo
 *
 * This sketch show how to use the mx25r6435f driver.
 *
 */

#include <MX25R6435F.h>

#define BUFFER_SIZE         ((uint32_t)0x0100)
#define WRITE_READ_ADDR     ((uint32_t)0x0050)

uint8_t aTxBuffer[BUFFER_SIZE];
uint8_t aRxBuffer[BUFFER_SIZE];

static void     Fill_Buffer (uint8_t *pBuffer, uint32_t uwBufferLength, uint32_t uwOffset);
static uint8_t  Buffercmp   (uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength);

void setup() {
  Serial.begin(9600);
}

void loop() {
  uint8_t status;

  Serial.println("*****************************************************************");
  Serial.println("*********************** Memory Test *****************************");
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

        /* Write data to the QSPI memory */
        if(MX25R6435F.write(aTxBuffer, WRITE_READ_ADDR, BUFFER_SIZE) == 0)
        {
          Serial.println("WRITE : FAILED, Test Aborted");
        }
        else
        {
          Serial.println("WRITE : OK");

          /* Read back data from the QSPI memory */
          MX25R6435F.read(aRxBuffer, WRITE_READ_ADDR, BUFFER_SIZE);

          /*##-5- Checking data integrity ############################################*/
          if(Buffercmp(aRxBuffer, aTxBuffer, BUFFER_SIZE) > 0)
          {
            Serial.println("COMPARE : FAILED, Test Aborted");
          }
          else
          {
            Serial.println("Test : OK");
          }
        }
      }
    }
  }

  // Enables the deep power mode
  MX25R6435F.sleep();
  delay(5000);
  MX25R6435F.wakeup();
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

static uint8_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer1 != *pBuffer2)
    {
      return 1;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}
