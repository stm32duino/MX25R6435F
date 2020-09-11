# MX25R6435F
Arduino library to support the Quad-SPI NOR Flash memory MX25R6435F using the Quad SPI flash memories interface. Since library version 2.0.0 and [STM32 core](https://github.com/stm32duino/Arduino_Core_STM32) version 2.0.0 the OctoSPI Flash memories interface can also be used.

## API

The library provides the following API:

* `begin()`
* `end()`
* `write()`
* `read()`
* `mapped()`
* `erase()`
* `eraseChip()`
* `eraseSector()`
* `suspendErase()`
* `resumeErase()`
* `sleep()`
* `wakeup()`
* `status()`
* `info()`
* `length()`

Since library version 2.0.0, xSPI pins can be defined at sketch level, using:

* To redefine the default one before call of `begin()`:

  * `setDx(uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3)`
  * `setDx(PinName data0, PinName data1, PinName data2, PinName data3)`
  * `setSSEL(uint32_t ssel)`
  * `setSSEL(PinName ssel)`
  * `setSCLK(uint32_t sclk)`
  * `setSCLK(PinName sclk)`

  *Code snippet:*
```C++
  MX25R6435F.setDx(PE12, PE13, PE14, PE15); // using pin number
  MX25R6435F.setSCLK(PE10);
  MX25R6435F.setSSEL(PE_11); // using PinName
  MX25R6435F.begin();
```

* or using the `begin()` method:

  * `MX25R6435FClass(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t clk, uint8_t ssel)`

  *Code snippet:*
```C++
  MX25R6435F.begin(PE12, PE13, PE14, PE15, PE10, PE11);
```

* or by redefining the default pins definition (using [build_opt.h](https://github.com/stm32duino/wiki/wiki/Customize-build-options-using-build_opt.h) or [hal_conf_extra.h](https://github.com/stm32duino/wiki/wiki/HAL-configuration#customize-hal-or-variant-definition)):

  * `MX25R6435F_D0`
  * `MX25R6435F_D1`
  * `MX25R6435F_D2`
  * `MX25R6435F_D3`
  * `MX25R6435F_SCLK`
  * `MX25R6435F_SSEL`

## Examples

3 sketches provide basic examples to show how to use the library API:
* `demo.ino` uses basic read/write functions.
* `eraseChip.ino` erases all data present in the memory.
* `memoryMappedMode.ino` shows how to use the mapped mode.

## Documentation

You can find the source files at  
https://github.com/stm32duino/MX25R6435F

The MX25R6435F datasheet is available at  
http://www.mxic.com.tw/Lists/Datasheet/Attachments/6270/MX25R6435F,%20Wide%20Range,%2064Mb,%20v1.4.pdf
