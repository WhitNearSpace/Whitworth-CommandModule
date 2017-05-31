#include "SBDmessage.h"

// Status: Ready for testing
SBDmessage::SBDmessage(int16_t missionID) {
  sbd[0] = missionID>>8;
  sbd[1] = missionID;
  for (int i = 2; i<SBD_LENGTH; i++) {
    sbd[i] = 0;
  }
}

// Status: Ready for testing
char SBDmessage::getByte(int i) {
  if ((i>=0)&&(i<SBD_LENGTH))
    return sbd[i];
}

void SBDmessage::generateGPSBytes(GPSCoordinates &gps) {
  char miscByte = 0;
  int16_t t;

  // miscByte is a bit collection
  miscByte |= (gps.positionFix);  // bit 0 = GPS position fix 
}
