#include "SBDmessage.h"

// Status: Tested with terminal
SBDmessage::SBDmessage() {
  for (int i = 0; i<MAX_SBD_LENGTH; i++) {
    sbd[i] = 0;
  }
  msgLength = 22;
}

SBDmessage::~SBDmessage() {

}

void SBDmessage::setMissionID(int16_t missionID) {
  storeInt16(0, missionID);
}

// Status: Tested with terminal
char SBDmessage::getByte(int i) {
  if ((i>=0)&&(i<MAX_SBD_LENGTH))
    return sbd[i];
}

// Status: Tested with terminal
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
  storeInt32(3, gps.syncTime);

  // Latitude (in 100,000th of a minute) is bytes 7-10
  storeInt32(7, gps.getRawLatitude());

  // Longitude (in 100,000th of a minute) is bytes 11-14
  storeInt32(11, gps.getRawLongitude());

  // Altitude (in meters) is bytes 15-16
  storeUInt16(15, (uint16_t)(gps.getAltitude()));

  // Vertical velocity (in tenths of m/s) is bytes 17-18
  storeInt16(17, gps.getRawVerticalVelocity());

  // Ground speed (in tenths of km/h) is bytes 19-20
  storeUInt16(19, gps.getRawGroundSpeed());

  // Heading relative to true north (0-180 deg, sign in bitByte)
  sbd[21] = (char)(heading);
}

void SBDmessage::appendPodBytes(char podData[], int dataLength) {
  for (int i = 0; i < dataLength; i++) {
    sbd[i+msgLength] = podData[i];
  }
  msgLength += dataLength;
}

int SBDmessage::getMsgLength() {
  return msgLength;
}

// Status: Tested with terminal
unsigned short SBDmessage::generateChecksum() {
  unsigned short cs = 0;
  for (int i = 0; i<msgLength; i++) {
    cs += sbd[i];
  }
  checksum[0] = cs/256;
  checksum[1] = cs%256;
  return cs;
}

void SBDmessage::loadTestMsg() {
  sbd[0] = 'h';
  sbd[1] = 'e';
  sbd[2] = 'l';
  sbd[3] = 'l';
  sbd[4] = 'o';
  msgLength = 5;
}

void SBDmessage::storeInt32(int startIndex, int32_t data) {
  sbd[startIndex] = data >> 24;
  sbd[startIndex+1] = data >> 16;
  sbd[startIndex+2] = data >> 8;
  sbd[startIndex+3] = data;
}

void SBDmessage::storeInt16(int startIndex, int16_t data) {
  sbd[startIndex] = data >> 8;
  sbd[startIndex+1] = data;
}

void SBDmessage::storeUInt16(int startIndex, uint16_t data) {
  sbd[startIndex] = data >> 8;
  sbd[startIndex+1] = data;
}

int32_t SBDmessage::retrieveInt32(int startIndex) {
  int32_t result = 0;
  result |= sbd[startIndex]<<24;
  result |= sbd[startIndex+1]<<16;
  result |= sbd[startIndex+2]<<8;
  result |= sbd[startIndex+3];
  return result;
}

uint16_t SBDmessage::retrieveUInt16(int startIndex) {
  uint16_t result = 0;
  result |= sbd[startIndex]<<8;
  result |= sbd[startIndex+1];
  return result;
}

int16_t SBDmessage::retrieveInt16(int startIndex) {
  int16_t result = 0;
  result |= sbd[startIndex]<<8;
  result |= sbd[startIndex+1];
  return result;
}
