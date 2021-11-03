#ifndef STM32_USART_BOOTLOADER_H
#define STM32_USART_BOOTLOADER_H

#include <Arduino.h>
#include <HardwareSerial.h>

// Standard STM32 responses
#define STM32_ACK (0x79)
#define STM32_NACK (0x1F)
#define STM32_ERROR (0x01)

// Command to launch the USART bootloader
#define STM32_MAGIC (0x7F)

// All USART bootloader commands (to be send with their complement)
#define STM32_GET (0x00)
#define STM32_GETVERSION (0x01)
#define STM32_GETID (0x02)
#define STM32_READ (0x11)
#define STM32_GO (0x21)
#define STM32_WRITE (0x31)
#define STM32_ERASE (0x43)
#define STM32_EXTENDED_ERASE (0x44)
#define STM32_SPECIAL (0x50)
#define STM32_EXTENDED_SPECIAL (0x51)
#define STM32_WRITE_PROTECT (0x63)
#define STM32_WRITE_UNPROTECT (0x73)
#define STM32_READOUT_PROTECT (0x82)
#define STM32_READOUT_UNPROTECT (0x92)
#define STM32_GET_CHECKSUM (0xA1)

/*
 * TODO
 * extended_erase
 * erase_program_space
 * program
 * 
 * 
 */

class Stm32_usart_bootloader {
  public:
    Stm32_usart_bootloader(HardwareSerial* ser);

    // STM32 USART Bootloader commands - read ST Application Note AN3155
    uint8_t enter();
    uint8_t get();
    uint8_t getversion();
    uint8_t getid();
    uint8_t read(uint8_t* buf, size_t len, uint32_t address);
    uint8_t go(uint32_t address);
    uint8_t write(const uint8_t*buf, size_t len, uint32_t address);
    uint8_t erase();
    uint8_t extended_erase(uint16_t num_pages, uint16_t* pagenumbers);
    uint8_t special();
    uint8_t extended_special();
    uint8_t write_protect();
    uint8_t write_unprotect();
    uint8_t readout_protect();
    uint8_t readout_unprotect();
    uint8_t get_checksum();


    // Functions built on top of base commands 
    uint8_t program(uint8_t* buf, size_t len);
    uint8_t erase_program_space(size_t program_size);
  private:
    HardwareSerial* _ser;
    uint8_t _send(const uint8_t* buf, size_t len);
    uint8_t _send_command(uint8_t cmd);
    uint8_t _checksum(uint8_t* buf, size_t len);
    void _log_resp(uint8_t resp);

    // Things we can read from the bootloader
    uint8_t _version = 0x00;
    uint8_t _num_commands = 0;
    uint8_t* _command_list = NULL;
    uint16_t _chip_ID = 0;
    

};

#endif
