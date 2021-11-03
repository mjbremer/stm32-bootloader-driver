#include "stm32-usart-bootloader.h"

Stm32_usart_bootloader::Stm32_usart_bootloader(HardwareSerial* ser) {
  _ser = ser;
  if (_ser == NULL) {log_e("Error: bootloader controller initialized with NULL Serial instance");}
}


// Under some conditions, this function simply does not work (does not generate a response) unless called right after resetting the STM32 
// (i.e. resetting the STM32 and ESP32 almost exactly at the same time)
// I have not been able to find the underlying pattern of behavior, but I recommend that any time this function is called, 
// generate an stm32 reset immediately before
uint8_t Stm32_usart_bootloader::enter() {
  uint8_t resp = 0;
  uint8_t magic = STM32_MAGIC;

  log_i("Entering STM32 Bootloader");
  
  _ser->write(&magic,1);
  while (!_ser->available());
  _ser->read(&resp,1);
  _log_resp(resp);
  return resp;
}
uint8_t Stm32_usart_bootloader::get() {
  uint8_t resp;
  
  log_i("Getting bootloader version and commands");
  log_d("Sending get command");
  resp = _send_command(STM32_GET);
  if (resp != STM32_ACK) { return resp; }

  _ser->read(&_num_commands,1);
  log_d("Got number of commands %u",_num_commands);
  _ser->read(&_version,1);
  log_i("Bootloader version is 0x%02X",_version);

  if (_command_list != NULL) {free(_command_list);}
  _command_list = (uint8_t*)malloc(_num_commands);

  _ser->read(_command_list,_num_commands);

  for (uint8_t i=0;i<_num_commands;i++) {
    log_i("Got supported command 0x%02X",_command_list[i]);
  }

  _ser->read(&resp,1);
  _log_resp(resp);
  return resp;
}
uint8_t Stm32_usart_bootloader::getversion() {
  uint8_t resp;
  log_i("Getting bootloader version");
  
  log_d("Sending getversion command");
  resp = _send_command(STM32_GETVERSION);
  if (resp != STM32_ACK) { return resp; }
  
  _ser->read(&_version,1);
  log_i("Bootloader version is 0x%02X",_version);

  // Two empty option bytes to throw away then an ACK
  _ser->read(&resp,1);
  _ser->read(&resp,1);
  _ser->read(&resp,1);
  
  _log_resp(resp);
  return resp;
}
uint8_t Stm32_usart_bootloader::getid() {
  uint8_t resp;
  log_i("Getting chip ID");
  log_d("Sending getid command");
  resp = _send_command(STM32_GETID);
  if (resp != STM32_ACK) { return resp; }

  _ser->read(&resp,1); // always 1 for STM32, throwaway
  log_d("packet length byte is 0x%02X",resp);
  _ser->read(&resp,1);
  _chip_ID = ((uint16_t)resp) << 8; // MSB
  _ser->read(&resp,1);
  _chip_ID |= (uint16_t)resp;
  log_i("Chip ID is 0x%04X",_chip_ID); // LSB

  _ser->read(&resp,1);
  _log_resp(resp);
  return resp;
}
uint8_t Stm32_usart_bootloader::read(uint8_t* buf, size_t len, uint32_t address) {
  uint8_t resp;
  uint8_t address_pkt[5];
  uint8_t size_pkt[2];
  if (len>256) {
     log_e("Read memory: Read data length must be 256 bytes or less");
     return STM32_ERROR;
  } else if (buf == NULL) {
     log_e("Read memory: Given read buffer is NULL");
     return STM32_ERROR;
  }

  log_i("Reading %zu bytes from address 0x%08X", len, address);
  
  log_d("Sending read memory command");
  resp = _send_command(STM32_READ);
  if (resp != STM32_ACK) {return resp;}
  
  // Build address packet
  for (int i=0;i<4;i++) {address_pkt[i] = address >> (8*(3-i));}
  address_pkt[4] = _checksum(address_pkt,4);
  log_d("Sending address and checksum");
  resp = _send(address_pkt,5);
  if (resp != STM32_ACK) {return resp;}

  // Build size packet
  size_pkt[0] = (uint8_t)((len-1)&0xFF);
  size_pkt[1] = ~size_pkt[0];
  log_d("Sending read size and checksum");
  resp = _send(size_pkt,2);
  if (resp != STM32_ACK) {return resp;}

  while (!_ser->available());
  delay(100);
  _ser->read(buf,len);
  
  return STM32_ACK;
}
uint8_t Stm32_usart_bootloader::go(uint32_t address) {
  uint8_t address_pkt[5];
  uint8_t resp;

  log_i("Booting to address 0x%08X",address);
  log_d("Sending GO command");
  resp = _send_command(STM32_GO);
  if (resp != STM32_ACK) { return resp; }
  
  for (int i=0;i<4;i++) {address_pkt[i] = address >> (8*(3-i));} // Construct address packet
  address_pkt[4] = _checksum(address_pkt,4);
  log_d("Sending address and checksum");
  return _send(address_pkt,5);
}
uint8_t Stm32_usart_bootloader::write(const uint8_t*buf, size_t len, uint32_t address) {
  uint8_t address_pkt[5];
  uint8_t data_pkt[258]; // Max packet size = len (1 byte) + data {256 bytes) + checksum {1 byte)
  uint8_t resp;

  //Error checking
  if (len%4) {
    log_e("Write memory: Write data length must be a multiple of 4 bytes");
    return STM32_ERROR;
  } else if (len>256) {
     log_e("Write memory: Write data length must be 256 bytes or less");
     return STM32_ERROR;
  } else if (buf == NULL) {
     log_e("Write memory: Given write buffer is NULL");
     return STM32_ERROR;
  }

  log_i("Writing %zu bytes to address 0x%08X",len,address);
  log_d("Sending write memory command");
  resp = _send_command(STM32_WRITE);
  if (resp != STM32_ACK) {return resp;}

  // Build address packet
  for (int i=0;i<4;i++) {address_pkt[i] = address >> (8*(3-i));}
  address_pkt[4] = _checksum(address_pkt,4);
  log_d("Sending address and checksum");
  resp = _send(address_pkt,5);
  if (resp != STM32_ACK) {return resp;}

  data_pkt[0] = (uint8_t)((len-1)&0xFF);
  memcpy(&data_pkt[1],buf,len);
  data_pkt[len+1] = _checksum(data_pkt,len+1);
  
  log_d("Sending data packet");
  return _send(data_pkt,len+2);
}
uint8_t Stm32_usart_bootloader::erase() {
  // Not implemented
  return STM32_ERROR;
}

// TODO: include special cases for global erases
uint8_t Stm32_usart_bootloader::extended_erase(uint16_t num_pages, uint16_t* pagenumbers) {
  uint8_t resp;
  uint8_t* pkt;
  const size_t pktsize = (2*num_pages)+3;// Two extra for size, one for checksum

  log_i("Erasing %u pages",num_pages);
  log_d("Sending extended erase command");
  resp = _send_command(STM32_EXTENDED_ERASE);
  if (resp != STM32_ACK) {return resp;}

  pkt = (uint8_t*)malloc(pktsize); 

  pkt[0] = (uint8_t)(((num_pages-1)>>8) & 0xFF);
  pkt[1] = (uint8_t)((num_pages-1) & 0xFF);

  for (int i=0;i<num_pages;i++) {
    pkt[(2*i)+2] = (uint8_t)((pagenumbers[i]>>8) & 0xFF);
    pkt[(2*i)+3] = (uint8_t)(pagenumbers[i] & 0xFF);
  }

  pkt[pktsize-1] = _checksum(pkt,pktsize-1);

  resp = _send(pkt,pktsize);

  free(pkt);
  return resp;
}
uint8_t Stm32_usart_bootloader::special() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::extended_special() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::write_protect() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::write_unprotect() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::readout_protect() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::readout_unprotect() {
  // Not implemented
  return STM32_ERROR;
}
uint8_t Stm32_usart_bootloader::get_checksum() {
  // Not implemented
  return STM32_ERROR;
}

uint8_t Stm32_usart_bootloader::program(uint8_t* buf, size_t len) {
  if (buf == NULL) {
    log_e("Bootloader program buffer is NULL");
    return STM32_NACK;
  } else if (len%256) {
    log_e("Bootloader program size must be a multiple of 256");
    return STM32_NACK;
  } else if (len == 0) {
    log_e("Bootloader program size is zero");
    return STM32_NACK;
  }
  
  size_t fpointer = 0;
  uint8_t iterations = len/256;
  size_t base_address = 0x08000000;

  log_i("Programming STM32...");
  log_d("Erasing flash program space");
  erase_program_space(len);
  log_d("Writing flash pages");

  for (int i=0;i<iterations;i++) {
    write(&buf[256*i],256, base_address+(256*i));
  }
  
  

  return STM32_ACK;
}

uint8_t Stm32_usart_bootloader::_erase_program_space(size_t program_size) {
  uint16_t num_pages = 1+(program_size/256);
  uint16_t* pagenumbers = (uint16_t*)malloc(2*num_pages);
  for (int i=0;i<num_pages;i++) {pagenumbers[i] = i;}
  extended_erase(num_pages,pagenumbers);

  free(pagenumbers);
  return STM32_ACK;  
}



uint8_t Stm32_usart_bootloader::_send(const uint8_t* buf, size_t len) {
  uint8_t resp = 0;
  _ser->write(buf,len);
  while (!_ser->available());
  _ser->read(&resp,1);
  
  _log_resp(resp);

  return resp;
}

uint8_t Stm32_usart_bootloader::_send_command(uint8_t cmd) {
  uint8_t pkt[2];
  pkt[0] = cmd;
  pkt[1] = ~pkt[0];
  return _send(pkt,2);
}

uint8_t Stm32_usart_bootloader::_checksum(uint8_t* buf, size_t len) {
  uint8_t resp = 0;
  for (size_t i = 0; i < len; i++) {resp ^= buf[i];}
  return resp;
}

void Stm32_usart_bootloader::_log_resp(uint8_t resp) {
  switch (resp) {
    case STM32_ACK:
      log_d("ACK received");
      break;
    case STM32_NACK:
      log_e("NACK received");
      break;
    default:
      log_e("Unknown response 0x%02X received",resp);
      break;
  }
}
