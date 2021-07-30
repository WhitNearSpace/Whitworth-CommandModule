#include <mbed.h>
#include <unity/unity.h>
#include "cypress_fm24w256.h"

I2C i2c(p9,p10);
Cypress_FRAM fram(&i2c,0);

uint16_t generate_trial_address() {
  AnalogIn adc(p20); // Use unconnected analog in to generate some random bits
  uint16_t trial_addr;
  trial_addr = (adc.read_u16() >> 4) & 0x000F; // 1st hex digit (starting on right)
  trial_addr = trial_addr | (((adc.read_u16() >> 4) & 0x000F) << 4); // 2nd hex digit
  trial_addr = trial_addr | (((adc.read_u16() >> 4) & 0x000F) << 8); // 3rd hex digit
  trial_addr = trial_addr | (((adc.read_u16() >> 4) & 0x0007) << 12); // 4th hex digit
  return trial_addr;
}

void test_write_byte_to_address() { //testing single byte write and read
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Byte response;
  char data_byte;

  trial_addr = generate_trial_address();
  
  data_byte = 0x1B;
  code = fram.write(trial_addr, data_byte);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if the write was a success

  data_byte = 0x2c;

  code = fram.write(trial_addr + 1, data_byte);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if the write was a success
  
  response = fram.read(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if the read was a success
  TEST_ASSERT_EQUAL_UINT8(0x1B, response.data); //testing if the read is correct data value

  response = fram.read();
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if the read was a success
  TEST_ASSERT_EQUAL_UINT8(0x2c, response.data); //testing if the read is correct data value
}

void test_write_uint16_to_address(){
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Uint16 response;
  uint16_t data;

  trial_addr = generate_trial_address();
  data = 0x3fc1;
  code = fram.write_uint16(trial_addr, data);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if Uint16 write was a success

  data = 0xf532;
  code = fram.write_uint16(trial_addr + 2, data);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if Uint16 write was a success

  response = fram.read_uint16(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if uint16 read was a success
  TEST_ASSERT_EQUAL_UINT16(0x3fc1, response.data); //testing if read is the correct data value

  response = fram.read_uint16();
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if uint16 read was a success
  TEST_ASSERT_EQUAL_UINT16(0xf532, response.data);
}

void test_write_int16_to_address()
{
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Int16 response;
  int16_t data;

  trial_addr = generate_trial_address();

  data = -32;
  code = fram.write_int16(trial_addr, data);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if int16 write was a success

  data = 1002;
  code = fram.write_int16(trial_addr + 2, data);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if int16 write was a success

  response = fram.read_int16(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if int16 read was a success
  TEST_ASSERT_EQUAL_INT16(-32, response.data); //testing if read is the correct data value


  response = fram.read_int16();
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if int16 read was a success
  TEST_ASSERT_EQUAL_INT16(1002, response.data); //testing if read is the correct data value
}

void test_write_multibyte_to_address()
{
  uint16_t trial_addr;
  static const int array_length = 5;
  char write_array_1[array_length], write_array_2[array_length];
  char byte = 10;
  int code;

  trial_addr = generate_trial_address();

  for(int i = 0; i < array_length; i++) //filling 5 bytes into 1st array
  {
    write_array_1[i] = byte;
    byte += 1; 
  }

  for(int i = 0; i < array_length; i++) //filling 5 bytes into 2nd array
  {
    write_array_2[i] = byte;
    byte += 2; 
  }

  code = fram.write(trial_addr, write_array_1, sizeof(write_array_1)); //writing bytes from 1st array to specified memory address
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //test if write was a success

  code = fram.write(trial_addr + array_length, write_array_2, sizeof(write_array_2)); //writing bytes from 2nd to next memory address
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //test if write was a success

  char data_array[array_length] = {0};
  code = fram.read(trial_addr, data_array, array_length); //read 5 bytes from specified addresss
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //test if read was successful
  TEST_ASSERT_EQUAL_INT8_ARRAY(write_array_1, data_array, 5); //test if data bytes match


  data_array[array_length] = {0};
  code = fram.read(data_array, array_length); //read 5 bytes from current addresss
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //test if read was successful
  TEST_ASSERT_EQUAL_INT8_ARRAY(write_array_2, data_array, 5); //test if data bytes match

}

int main() {
  ThisThread::sleep_for(3s);
  UNITY_BEGIN();
  RUN_TEST(test_write_byte_to_address);
  RUN_TEST(test_write_uint16_to_address);
  RUN_TEST(test_write_int16_to_address);
  RUN_TEST(test_write_multibyte_to_address);
  UNITY_END();
  ThisThread::sleep_for(3s); 
}