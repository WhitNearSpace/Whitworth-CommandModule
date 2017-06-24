#include "mbed.h"
#include "NAL9602.h"
#include "SBDmessage.h"

/** Command Module Microcontroller
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.1
 * @date 2017
 * @copyright GNU Public License
 *
 * Version History:
 *  0.1 - Terminal emulation mode only for testing of NAL9602 library
 */

//NAL9602 sat(USBTX,USBRX);
Serial pc(USBTX,USBRX);
GPSCoordinates gps;

int main() {
  int16_t id = 1;
  time_t t;
  int32_t rawLat;
  int32_t rawLon;
  uint16_t alt;
  uint16_t gs;
  int16_t vv;
  int h;
  char podData[] = {0x01, 0x02, 0x04, 0x08, 0x10};
  SBDmessage msg(id);
  gps.positionFix = true;
  gps.syncTime = 1496150000;
  gps.setLatitudeDegMin(48,40,0,true);
  gps.setLongitudeDegMin(170,23,0,false);
  gps.setAltitude(35000);
  gps.setGroundSpeed(20.5);
  gps.setHeading(315);
  gps.setVerticalVelocity(5);
  msg.generateGPSBytes(gps);
  msg.generateCommandModuleBytes(7.38f, 38.23f, -40.0f);
  msg.loadPodBuffer(2, 5, podData);
  msg.generatePodBytes();
  while (true) {
    pc.printf("\r\nThe mission ID is %d\r\n",msg.retrieveInt16(0));
    pc.printf("The bit byte is 0x%x\r\n", msg.getByte(2));
    t = msg.retrieveInt32(3);
    pc.printf("The time is %s", ctime(&t));
    rawLat = msg.retrieveInt32(7);
    pc.printf("The latitude is %f degrees\r\n",(float)(rawLat)/60.0/100000.0);
    rawLon = msg.retrieveInt32(11);
    pc.printf("The longitude is %f degrees\r\n",(float)(rawLon)/60.0/100000.0);
    alt = msg.retrieveUInt16(15);
    pc.printf("The altitude is %u m\r\n", alt);
    vv = msg.retrieveInt16(17);
    pc.printf("The vertical velocity is %f m/s\r\n", vv/10.0);
    gs = msg.retrieveUInt16(19);
    pc.printf("The ground speed is %f km/h\r\n", gs/10.0);
    h = msg.getByte(21);
    if (!(msg.getByte(2)&0x02))
      h = -h;
    pc.printf("The heading is %d deg\r\n", h);
    pc.printf("The battery voltage is %.2f V\r\n", msg.getByte(22)/20.0);
    pc.printf("The internal temperature is %.2f deg C\r\n", msg.retrieveInt16(23)/100.0);
    pc.printf("The external temperature is %.2f deg C\r\n", msg.retrieveInt16(25)/100.0);
    msg.testPodBytes();
    wait(5);
  }
 }
