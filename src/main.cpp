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
  SBDmessage msg(id);
  gps.positionFix = true;
  gps.setLatitudeDegMin(48,40,0,true);
  gps.setLongitudeDegMin(170,23,0,false);
  gps.setAltitude(35000);
  msg.generateGPSBytes(gps);
  while (true) {
    pc.printf("The mission ID is %u%u\r\n",msg.getByte(0),msg.getByte(1));
    pc.printf("The status byte is 0x%x\r\n", msg.getByte(2));
    pc.printf("\tGPS fix: %d\r\n", msg.getByte(2)&0x01);
    pc.printf("\tNorth: %d\r\n", msg.getByte(2)&0x02);
    pc.printf("\tEast: %d\r\n", msg.getByte(2)&0x04);
    wait(5);
  }
 }
