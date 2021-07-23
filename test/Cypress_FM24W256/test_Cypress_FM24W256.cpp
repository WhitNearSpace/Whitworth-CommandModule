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
  
  response = fram.read(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if the read was a success
  TEST_ASSERT_EQUAL_UINT8(data_byte, response.data); //testing if the read is correct data value
}

void test_write_uint16_to_address(){
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Uint16 response;
  uint16_t data_;

  trial_addr = generate_trial_address();
  data_ = 0xe34a;
  code = fram.write_uint16(trial_addr, data_);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if Uint16 write was a success

  response = fram.read_uint16(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if uint16 read was a success
  TEST_ASSERT_EQUAL_UINT16(data_, response.data); //testing if read is the correct data value
}

void test_write_int16_to_address()
{
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Int16 response;
  int16_t data_;

  trial_addr = generate_trial_address();

  data_ = 0x7621;
  code = fram.write_int16(trial_addr, data_);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code); //testing if int16 write was a success

  response = fram.read_int16(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status); //testing if int16 read was a success
  TEST_ASSERT_EQUAL_INT16(data_, response.data); //testing if read is the correct data value
}

int main() {
  ThisThread::sleep_for(3s);
  UNITY_BEGIN();
  RUN_TEST(test_write_byte_to_address);
  RUN_TEST(test_write_uint16_to_address);
  RUN_TEST(test_write_int16_to_address);
  UNITY_END();
  ThisThread::sleep_for(3s); 
}