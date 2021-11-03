/*
 * Example program to show off some BL commands and programming capabilities - warning, this will overwrite user program space.
 * HardwareSerial device can be initialized however you want, but
 *  - must be SERIAL_8E1
 *  - must be between 2400 and 115200 baud
 *  - must call begin() before calling any bootloader functions
 */

#include "stm32-usart-bootloader.h"

#define NRST_PIN (10)

Stm32_usart_bootloader* bl;

const char dummy_data[256] = "Did you know that this string is exactly two-hundred and fifty-six characters long? Isn't that amazing? 256 is a power of 2, which makes it a really cool number. If you count these characters, you'll only see 255, because of the null terminator. Well, bye";
const char dummy_data2[256] = "Not so fast, Yugi boy. Don't you know that I am much better at children's card games than you are, even though you're possessed by an ancient pharaoh? Man, what an odd premise for an anime. Anyway, this message is also 256 characters. At least I hope so!!";
char received_data[256];

void stm_reset() {
  digitalWrite(NRST_PIN,LOW);
  delay(100);
  digitalWrite(NRST_PIN,HIGH);
  delay(100);
}

void setup() {
  pinMode(NRST_PIN,OUTPUT);
  digitalWrite(NRST_PIN,HIGH);

  
  Serial.begin(115200);
  Serial1.begin(115200,SERIAL_8E1);
  bl = new Stm32_usart_bootloader(&Serial1);

  stm_reset();
  bl->enter();
  bl->getversion();
  bl->get();
  bl->getid();

  //bl->go(0x08000000);
}

void loop() {
//  delay(5000);
//  bl->program((uint8_t*)dummy_data,256);
//  bl->read((uint8_t*)received_data,256,0x08000000);
//  Serial.println(received_data);
//  delay(5000);
//  bl->program((uint8_t*)dummy_data2,256);
//  bl->read((uint8_t*)received_data,256,0x08000000);
//  Serial.println(received_data);

 
}
