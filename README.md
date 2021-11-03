# STM32 USART Bootloader Driver for Arduino/ESP32
This project is a "quick-and-dirty" implementation of what I call a "bootloader driver" for the STM32 family of microprocessors, written for Arduino. These STM32s come with builtin bootloaders in system memory, with a list of commands that can be used to do a variety of things, but primarily program the microcontroller without using an ST-LINK.

### Motivation

The primary use for this library is to allow an Arduino-compatible microcontroller to flash a program onto an STM32 chip. On the surface this might not seem that useful, however when combined with wireless capabilities (i.e. ESP32) it allows for an STM32 to be programmable (although not necessarily debuggable) entirely over-the-air. This OTA programming was my base motivation for this library.

### Notes

This library was developed and tested using an ESP32C3 dev board, and an STM32F407VG discovery board. It has not been tested using any other boards.

The library uses ESP logging functions that may not be available for your board. In this case you will need to implement these functions yourself as some sprintf/Serial.print combination (or just delete them if you're brave).

This library was developed using ST Application Note AN3155, and you should consider this the main documentation for this library. The only public function provided that is not a builtin bootloader command is program(), which allows a simple interface to flash a bin file directly into user flash, starting at 0x08000000 (currently hardcoded).

### TODO

Currently unimplemented commands (testable):
* write_protect
* write_unprotect
* readout_protect
* readout_unprotect

Currently unimplemented commands (untestable - not available on STM32F407 BL version 3.1):
* erase
* special
* extended_special
* get_checksum
