#include "cypress_fm24w256.h"

Cypress_FRAM::Cypress_FRAM(I2C* i2c, uint8_t device3BitAddress) {
  _i2c = i2c;
  if (device3BitAddress < 8) { // Valid, no bits beyond 0-2 set
    _addr = 0xA0 | (device3BitAddress << 1);
  } else { // Invalid 3-bit address, unset bits 3-7
    _addr = 0xA0 | ((device3BitAddress & 0x07) << 1);
  }
  printf("FRAM I2C address (8-bit) is %x\n", _addr);
}

int Cypress_FRAM::write(uint16_t mem_addr, char data) //writes single byte to specified address
{
  char payload[3];
  int mb_ack; // multi-byte ack is different than single byte ack coding
  int status = 0;
  if (mem_addr <= maxAddress) { // Valid memory address
    payload[0] = mem_addr >> 8; // MSB byte of memory address
    payload[1] = mem_addr & 0x00FF; // LSB byte of memory address
    payload[2] = data;
    mb_ack = _i2c->write(_addr, payload, 3);
  } else {
    mb_ack = 3;
  }
  switch (mb_ack) {
    case 0: status = FRAM_SUCCESS; break;
    case 1: status = FRAM_ERROR_NO_SLAVE; break;
    case 2: status = FRAM_ERROR_BUS_BUSY; break;
    case 3: status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return status;
}

int Cypress_FRAM::write_uint16(uint16_t mem_addr, uint16_t data) //writes uint16 to selected address
{
  char payload[4];
  int uint16_ack; // multi-byte ack is different than single byte ack coding
  int status = 0;
  if (mem_addr <= maxAddress) { // Valid memory address
    payload[0] = mem_addr >> 8; // MSB byte of memory address
    payload[1] = mem_addr & 0x00FF; // LSB byte of memory address
    payload[2] = data >> 8; //MSB byte of uint16 data
    payload[3] = data & 0x00FF; //LSB byte of uint16 data
    uint16_ack = _i2c->write(_addr, payload, 4);
  } else {
    uint16_ack = 3;
  }
  switch (uint16_ack) {
    case 0: status = FRAM_SUCCESS; break;
    case 1: status = FRAM_ERROR_NO_SLAVE; break;
    case 2: status = FRAM_ERROR_BUS_BUSY; break;
    case 3: status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return status;
}

int Cypress_FRAM::write_int16(uint16_t mem_addr, int16_t data) //rights int16 to selected address
{
  char payload[4];
  int int16_ack; //acknowlegdement bit
  int status = 0;
  if (mem_addr <= maxAddress) { // Valid memory address
    payload[0] = mem_addr >> 8; // MSB byte of memory address
    payload[1] = mem_addr & 0x00FF; // LSB byte of memory address
    payload[2] = data >> 8; //MSB byte of int16 data
    payload[3] = data & 0x00FF; //LSB byte of int16 data
    int16_ack = _i2c->write(_addr, payload, 4);
  } else {
    int16_ack = 3;
  }
  switch (int16_ack) {
    case 0: status = FRAM_SUCCESS; break;
    case 1: status = FRAM_ERROR_NO_SLAVE; break;
    case 2: status = FRAM_ERROR_BUS_BUSY; break;
    case 3: status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return status;

}

FRAM_Response_Read_Byte Cypress_FRAM::read(uint16_t mem_addr) //reads single byte from selected address
{
  int ack;
  FRAM_Response_Read_Byte response;
  if (mem_addr > maxAddress) {  // Invalid memory address
    ack = 3;
  } else { // Perform PARTIAL write to set memory location (no data, no stop)
    _i2c->start();
    ack = _i2c->write(_addr); // send slave address (in write mode)
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr >> 8); // send MSB byte of memory address
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr & 0x00FF); // send LSB byte of memory address
  }

  if (ack == 1) { // success so continue
    _i2c->start(); // abort write by sending START, leaving memory at current loc
    ack = _i2c->write(_addr | 1); // send slave address (in read mode)
  }
  if (ack == 1) { // success so continue
    response.data = _i2c->read(0); // no acknowledge so end of read
  }
  _i2c->stop(); // send stop signal
  switch (ack) {
    case 0: response.status = FRAM_ERROR_NO_SLAVE; break;
    case 1: response.status = FRAM_SUCCESS; break;
    case 2: response.status = FRAM_ERROR_BUS_BUSY; break;
    case 3: response.status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return response;
}

FRAM_Response_Read_Byte Cypress_FRAM::read() //reads single byte from current address
{
  int ack;
  FRAM_Response_Read_Byte response;
    _i2c->start();
    ack = _i2c->write(_addr | 1); // bit 0 = high --> read operation
    response.data = _i2c->read(0); // no acknowledge means end of read
    _i2c->stop();
    switch (ack) {
    case 0: response.status = FRAM_ERROR_NO_SLAVE; break;
    case 1: response.status = FRAM_SUCCESS; break;
    case 2: response.status = FRAM_ERROR_BUS_BUSY; break;
    case 3: response.status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
    return response;
  
}

FRAM_Response_Read_Uint16 Cypress_FRAM::read_uint16(uint16_t mem_addr){ //reads uint_16 from selected address
  int ack;
  char byte1,byte2;
  FRAM_Response_Read_Uint16 response;
  if (mem_addr > maxAddress) {  // Invalid memory address
    ack = 3;
  } else { // Perform PARTIAL write to set memory location (no data, no stop)
    _i2c->start();
    ack = _i2c->write(_addr); // send slave address (in write mode)
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr >> 8); // send MSB byte of memory address
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr & 0x00FF); // send LSB byte of memory address
  }

  if (ack == 1) { // success so continue
    _i2c->start(); // abort write by sending START, leaving memory at current loc
    ack = _i2c->write(_addr | 1); // send slave address (in read mode)
  }
  if (ack == 1) { // success so continue
    byte1 = _i2c->read(1); //acknowledge so continue
  }
  if (ack == 1) {
    byte2 = _i2c->read(0); //no acknowledge so stop
    response.data = (byte1 << 8) | byte2;
  }
  _i2c->stop(); // send stop signal
  switch (ack) {
    case 0: response.status = FRAM_ERROR_NO_SLAVE; break;
    case 1: response.status = FRAM_SUCCESS; break;
    case 2: response.status = FRAM_ERROR_BUS_BUSY; break;
    case 3: response.status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return response;
}
FRAM_Response_Read_Uint16 Cypress_FRAM::read_uint16() //reads uint16 at current address
{
  int ack;
  uint16_t byte1,byte2;
  FRAM_Response_Read_Uint16 response;
    _i2c->start();
    ack = _i2c->write(_addr | 1); // bit 0 = high --> read operation
    if(ack == 1)
      byte1 = _i2c->read(1); //acknowledge means keep reading
    if(ack == 1) 
    { 
      byte2 = _i2c->read(0); // no acknowledge means end of read
      response.data = (byte1 << 8) | byte2;
    }
    _i2c->stop();
    switch (ack) {
    case 0: response.status = FRAM_ERROR_NO_SLAVE; break;
    case 1: response.status = FRAM_SUCCESS; break;
    case 2: response.status = FRAM_ERROR_BUS_BUSY; break;
    case 3: response.status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
    return response;
}

FRAM_Response_Read_Int16 Cypress_FRAM::read_int16(int16_t mem_addr) //reads int_16 from selected address
{ 
  int ack;
  char byte1,byte2;
  FRAM_Response_Read_Int16 response;
  if (mem_addr > maxAddress) {  // Invalid memory address
    ack = 3;
  } else { // Perform PARTIAL write to set memory location (no data, no stop)
    _i2c->start();
    ack = _i2c->write(_addr); // send slave address (in write mode)
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr >> 8); // send MSB byte of memory address
  }
  if (ack == 1) { // success so continue
    ack = _i2c->write(mem_addr & 0x00FF); // send LSB byte of memory address
  }

  if (ack == 1) { // success so continue
    _i2c->start(); // abort write by sending START, leaving memory at current loc
    ack = _i2c->write(_addr | 1); // send slave address (in read mode)
  }
  if (ack == 1) { // success so continue
    byte1 = _i2c->read(1); //acknowledge so continue
  }
  if (ack == 1) {
    byte2 = _i2c->read(0); //no acknowledge so stop
    response.data = (byte1 << 8) | byte2;
  }
  _i2c->stop(); // send stop signal
  switch (ack) {
    case 0: response.status = FRAM_ERROR_NO_SLAVE; break;
    case 1: response.status = FRAM_SUCCESS; break;
    case 2: response.status = FRAM_ERROR_BUS_BUSY; break;
    case 3: response.status = FRAM_ERROR_INVALID_MEMORY_ADDRESS; break;
  }
  return response;
}