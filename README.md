# MX25R6435F
Arduino library to support the Quad-SPI NOR Flash memory MX25R6435F

## API

The library provides the following API:

* begin()
* end()
* write()
* read()
* mapped()
* erase()
* eraseChip()
* eraseSector()
* suspendErase()
* resumeErase()
* sleep()
* wakeup()
* status()
* info()
* length()

## Examples

3 sketches provide basic examples to show how to use the library API.  
demo.ino uses basic read/write functions.  
eraseChip.ino erases all data present in the memory.  
memoryMappedMode.ino shows how to use the mapped mode.  

## Documentation

You can find the source files at  
https://github.com/stm32duino/MX25R6435F

The MX25R6435F datasheet is available at  
http://www.mxic.com.tw/Lists/Datasheet/Attachments/6270/MX25R6435F,%20Wide%20Range,%2064Mb,%20v1.4.pdf
