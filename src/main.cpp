#include <mbed.h>
#include "ublox.h"

I2C i2c(p9, p10);
DigitalOut led1(LED1);

int main() {
  int latitude, longitude;
  int altitude;
  char SIV;
  int groundSpeed, verticalVelocity;
  i2c.frequency(400000);
  Ublox_GPS myGPS(&i2c);
  ThisThread::sleep_for(2s);
  led1 = myGPS.isConnected();

  myGPS.disableDebugging();

  while (true) {
    ThisThread::sleep_for(5s);
    latitude = myGPS.getLatitude();
    longitude = myGPS.getLongitude();
    altitude = myGPS.getAltitude();
    SIV = myGPS.getSIV();
    groundSpeed = myGPS.getGroundSpeed();
    verticalVelocity = myGPS.getVerticalVelocity();
    printf("Lat: %.6f", latitude*1e-7);
    printf(" Lon: %.6f", longitude*1e-7);
    printf(" Alt: %.1f m", altitude/1000.0);
    printf(" \tSatellites: %d\r\n", SIV);
    printf("Ground speed: %f m/s", groundSpeed/1000.0);
    printf(" Vertical velocity: %f m/s\r\n", verticalVelocity/1000.0);
    printf("---------------------------------------------------------------\n");
  } 
}
