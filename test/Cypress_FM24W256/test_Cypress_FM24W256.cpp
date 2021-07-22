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

void test_write_byte_to_address() {
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Byte response;
  char data_byte;

  trial_addr = generate_trial_address();
  
  data_byte = 0x1B;
  code = fram.write(trial_addr, data_byte);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, code);
  
  response = fram.read(trial_addr);
  TEST_ASSERT_EQUAL(FRAM_SUCCESS, response.status);
  TEST_ASSERT_EQUAL_UINT8(data_byte, response.data);
}

int main() {
  ThisThread::sleep_for(3s);
  UNITY_BEGIN();
  RUN_TEST(test_write_byte_to_address);
  UNITY_END();
  ThisThread::sleep_for(3s); 
}