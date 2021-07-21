#include <mbed.h>
#include "cypress_fm24w256.h"

DigitalOut led1(LED1);

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

int main() {
  uint16_t trial_addr;
  int code;
  FRAM_Response_Read_Byte response;
  FRAM_Response_Read_Uint16 uint16_response;
  FRAM_Response_Read_Int16 int16_response;
  ThisThread::sleep_for(100ms);
  trial_addr = generate_trial_address();
  char data_byte = 0x1B;
  printf("Attempting to write %#02x to address %04x\n", data_byte, trial_addr);
  code = fram.write(trial_addr, data_byte);
  printf("Response code is %d\n\n", code);

  data_byte = 0x3C; //next byte
  printf("Attempting to write %#02x to next address\n", data_byte);
  code = fram.write((trial_addr + 1), data_byte); //next byte write
  printf("Response code is %d\n\n", code);

  printf("Attempting to read from address %04x\n", trial_addr);
  response = fram.read(trial_addr);
  printf("Response code is %d\n", response.status);
  if (response.status == FRAM_SUCCESS)
    printf("Retrieved data is %#02x\n", response.data);
  printf("\n");

  printf("Attempting to read from next adress\n");
  response = fram.read();
  printf("Response code is %d\n", response.status);
  if (response.status == FRAM_SUCCESS)
    printf("Retrieved data is %#02x\n", response.data);
  printf("\n");

  //uint16t data write test
  uint16_t uint16_data = 234;
  printf("Attempting to write %u to address %04x\n", uint16_data, trial_addr);
  code = fram.write_uint16(trial_addr, uint16_data);
  printf("Response code is %d\n\n", code);

  uint16_data = 10321;
  code = fram.write_uint16(trial_addr + 2, uint16_data);



//uint16 read test (specified address)
  printf("Attempting to read from address %04x\n", trial_addr);
  uint16_response = fram.read_uint16(trial_addr);
  printf("Response code is %d\n", uint16_response.status);
  if (uint16_response.status == FRAM_SUCCESS)
    printf("Retrieved data is %u\n", uint16_response.data);
  printf("\n");

//uint16 read test (current address)
  printf("Attempting to read from next address\n");
  uint16_response = fram.read_uint16();
  printf("Response code is %d\n", uint16_response.status);
  if (uint16_response.status == FRAM_SUCCESS)
    printf("Retrieved data is %u\n", uint16_response.data);
  printf("\n");

//int16 write test
  int16_t int16_data= 565;
  printf("Attempting to write %i to address %04x\n", int16_data, trial_addr);
  code = fram.write_int16(trial_addr, int16_data);
  printf("Response code is %d\n\n", code);

//int16 read test
  printf("Attempting to read from address %04x\n", trial_addr);
  int16_response = fram.read_int16(trial_addr);
  printf("Response code is %d\n", int16_response.status);
  if (int16_response.status == FRAM_SUCCESS)
    printf("Retrieved data is %i\n", int16_response.data);
  printf("\n");

  while (true) {
    led1 = !led1;
    ThisThread::sleep_for(2s);
  }
}


/*********************************************************************************
 *  Ublox test code
 ********************************************************************************/
// I2C i2c(p9,p10);
// int main() {
//   int latitude, longitude;
//   int altitude;
//   char SIV;
//   int groundSpeed, verticalVelocity;
//   i2c.frequency(400000);
//   Ublox_GPS myGPS(&i2c);
//   ThisThread::sleep_for(2s);
//   led1 = myGPS.isConnected();

//   myGPS.disableDebugging();

//   while (true) {
//     ThisThread::sleep_for(5s);
//     latitude = myGPS.getLatitude();
//     longitude = myGPS.getLongitude();
//     altitude = myGPS.getAltitude();
//     SIV = myGPS.getSIV();
//     groundSpeed = myGPS.getGroundSpeed();
//     verticalVelocity = myGPS.getVerticalVelocity();
//     printf("Lat: %.6f", latitude*1e-7);
//     printf(" Lon: %.6f", longitude*1e-7);
//     printf(" Alt: %.1f m", altitude/1000.0);
//     printf(" \tSatellites: %d\r\n", SIV);
//     printf("Ground speed: %f m/s", groundSpeed/1000.0);
//     printf(" Vertical velocity: %f m/s\r\n", verticalVelocity/1000.0);
//     printf("---------------------------------------------------------------\n");
//   } 
// }
