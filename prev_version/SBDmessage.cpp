#include "SBDmessage.h"

// Status: Flight tested
SBDmessage::SBDmessage() {
  clearMessage();
  for (int i = 0; i < MAXPODS; i++)
    podLengths[i] = 0;
  attemptingSend = false;
  missionID = 0;
}

SBDmessage::~SBDmessage() {

}

void SBDmessage::clearMessage() {
  for (int i = 0; i<SBD_LENGTH; i++)
    sbd[i] = 0;
}

// Status: Ready for testing
void SBDmessage::setMissionID(uint32_t flightMode) {
  if (flightMode==FLIGHT_MODE_FLIGHT) {
    store_int16(0, missionID);
  } else {
    store_int16(0, -missionID);
  }
}

// Status: Ready for testing
char SBDmessage::retrieve_byte(uint32_t i) {
  char val = 0;
  if (i<SBD_LENGTH)
    val = sbd[i];
  else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for retrieve_byte function must be between 0 and 339");
  return val;
}

// Status: Ready for testing
int16_t SBDmessage::retrieve_int16(uint32_t startIndex) {
  int16_t result = 0;
  if (startIndex < SBD_LENGTH-1) {
    result |= sbd[startIndex]<<8;
    result |= sbd[startIndex+1];
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for retrieve_int16 function must be between 0 and 338");
  return result;
}

// Status: Ready for testing
uint16_t SBDmessage::retrieve_uint16(uint32_t startIndex) {
  uint16_t result = 0;
  if (startIndex < SBD_LENGTH-1) {
    result |= sbd[startIndex]<<8;
    result |= sbd[startIndex+1];
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for retrieve_uint16 function must be between 0 and 338");
  return result;
}

// Status: Ready for testing
int32_t SBDmessage::retrieve_int32(uint32_t startIndex) {
  int32_t result = 0;
  if (startIndex < SBD_LENGTH-3) {
    result |= sbd[startIndex]<<24;
    result |= sbd[startIndex+1]<<16;
    result |= sbd[startIndex+2]<<8;
    result |= sbd[startIndex+3];
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for retrieve_int32 function must be between 0 and 336");
  return result;
}

// Status: Ready for testing
void SBDmessage::store_int16(uint32_t startIndex, int16_t data) {
  if (startIndex < SBD_LENGTH-1) {
    sbd[startIndex] = data >> 8;
    sbd[startIndex+1] = data;
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for store_int16 function must be between 0 and 338");
}

// Status: Ready for testing
void SBDmessage::store_uint16(uint32_t startIndex, uint16_t data) {
  if (startIndex < SBD_LENGTH - 1) {
    sbd[startIndex] = data >> 8;
    sbd[startIndex+1] = data;
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for store_uint16 function must be between 0 and 338");
}

// Status: Ready for testing
void SBDmessage::store_int32(uint32_t startIndex, int32_t data) {
  if (startIndex < SBD_LENGTH - 3) {
    sbd[startIndex] = data >> 24;
    sbd[startIndex+1] = data >> 16;
    sbd[startIndex+2] = data >> 8;
    sbd[startIndex+3] = data;
  } else
    MBED_ERROR( MBED_MAKE_ERROR(MBED_MODULE_APPLICATION, MBED_ERROR_CODE_INVALID_INDEX), "SBD index for store_int32 function must be between 0 and 336");
}

// Status: Flight tested
void SBDmessage::generateGPSBytes(GPSCoordinates &gps) {
  int heading = gps.getHeading();
  bool headPos = true;
  if (heading>180) {
    heading = 360-heading;
    headPos = false;
  }
  // bitByte is a bit collection stored in byte 2 of the SBD
  char bitByte = sbd[2];  // load in current value
  bitByte |= (gps.positionFix); // bit 0 is GPS fix
  bitByte |= (char)(headPos)<<1; // bit 1 is heading sign
  sbd[2] = bitByte;

  // Time of GPS update is bytes 3-6
  store_int32(3, gps.syncTime);

  // Latitude (in 100,000th of a minute) is bytes 7-10
  store_int32(7, gps.getRawLatitude());

  // Longitude (in 100,000th of a minute) is bytes 11-14
  store_int32(11, gps.getRawLongitude());

  // Altitude (in meters) is bytes 15-16
  store_uint16(15, (uint16_t)(gps.getAltitude()));

  // Vertical velocity (in tenths of m/s) is bytes 17-18
  store_int16(17, gps.getRawVerticalVelocity());

  // Ground speed (in tenths of km/h) is bytes 19-20
  store_uint16(19, gps.getRawGroundSpeed());

  // Heading relative to true north (0-180 deg, sign in bitByte)
  sbd[21] = (char)(heading);
}

// Status: Ready for testing
void SBDmessage::generateCommandModuleBytes(float voltage, float intTemp, float extTemp) {
  // Store battery voltage in units of 0.05 V in byte 22
  sbd[22] = (char)(voltage*20);

  // Store internal temperature in units of 0.01 deg C in bytes 23-24
  store_int16(23, (int16_t)(intTemp*100));

  // Store external temperature in units of 0.01 deg C in bytes 25-26
  store_int16(25, (int16_t)(extTemp*100));
}

void SBDmessage::updateMsgLength() {
  msgLength = 27;
  for (int i = 0; i < MAXPODS; i++) {
    if (podLengths[i]>0)
      msgLength = msgLength + podLengths[i] + 1;
  }
}

/************** NEEDS UPDATING **********************/

// Status: Flight tested
unsigned short SBDmessage::generateChecksum() {
  unsigned short cs = 0;
  for (int i = 0; i<msgLength; i++) {
    cs += sbd[i];
  }
  checksum[0] = cs/256;
  checksum[1] = cs%256;
  return cs;
}



// Status: Ready for testing
int SBDmessage::loadPodBuffer(int podID, char* data) {
  char numBytes = podLengths[podID-1];
  for (int i = 0; i<numBytes; i++)
    podData[podID-1][i] = data[i];
  return 0;
}

// Status: Ready for testing
void SBDmessage::generatePodBytes() {
  /* Data format:
   *  First byte = number of bytes of data for pod i
   *  Data bytes follow
   */
  int b = 27;
  for (int i = 0; i<MAXPODS; i++) {
    if (podLengths[i]>0) {
      sbd[2] = sbd[2] | (0x01 << (2+i));
      sbd[b] = podLengths[i];
      for (int j = 0; j<podLengths[i]; j++) {
        sbd[b+j+1] = podData[i][j];
      }
      b = b + podLengths[i] + 1;
    }
  }
}

char SBDmessage::getPodBytes(char podID, char* data) {
  char numBytes = podLengths[podID-1];
  for (int i = 0; i < numBytes; i++) {
    data[i] = podData[podID-1][i];
  }
  return numBytes;
}

void SBDmessage::testPodBytes() {
  int b = 27;
  char dl;
  for (int i = 0; i<MAXPODS; i++) {
    if (sbd[2] & (0x01 << (2+i))) {
      dl = sbd[b];
      printf("Pod %d has %d data bytes\r\n\t", i+1, dl);
      for (int j = 0; j<dl; j++) {
        printf("%02X ", sbd[b+1+j]);
      }
      printf("\r\n");
      b = b + dl + 1;
    }
  }
}
