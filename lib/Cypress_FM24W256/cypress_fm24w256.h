/** Class for interfacing with a Cypress FM24W256 FRAM via I2C
 *  
 * Written for Mbed OS 6
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 1.0
 *  @date 2021
 *  @copyright MIT License
 */

#ifndef CYPRESS_FM24W256_H
#define CYPRESS_FM24W256_H

#include <mbed.h>

#define FRAM_SUCCESS 0
#define FRAM_ERROR_NO_SLAVE -1
#define FRAM_ERROR_BUS_BUSY -2
#define FRAM_ERROR_INVALID_MEMORY_ADDRESS -3

struct FRAM_Response_Read_Byte {
  int status;
  char data;
};

struct FRAM_Response_Read_Uint16 {
  int status;
  uint16_t data;
};

struct FRAM_Response_Read_Int16 {
  int status;
  int16_t data;
};


class Cypress_FRAM
{
public:
  Cypress_FRAM(I2C* i2c, uint8_t device3BitAddress = 0);
  const uint16_t maxAddress = 0x7FFF; // maximum memory address     
  
  // write functions
  int write(uint16_t mem_addr, char data);
  int write_uint16(uint16_t mem_addr, uint16_t data);
  //int write_uint16(uint16_t data);
  int write_int16(uint16_t mem_addr, int16_t data);
  // int write_int16(int16_t data);
  int write(uint16_t mem_addr, const char* data, int length);
  // int write(char data);
  // int write(const char* data, int length);


  // read functions
  FRAM_Response_Read_Byte read(uint16_t mem_addr);
  FRAM_Response_Read_Byte read();
  FRAM_Response_Read_Uint16 read_uint16(uint16_t mem_addr);
  FRAM_Response_Read_Uint16 read_uint16();
  FRAM_Response_Read_Int16 read_int16(int16_t mem_addr);
  // FRAM_Response_Read_Int16 read_int16();
  int read(uint16_t mem_addr, char* data, int length);
  // int read(char* data, int length);

private:
  I2C* _i2c;  // Shared connection to I2C
  char _addr; // 8-bit I2C address

};

#endif
