#include "mbed.h"
#include "NAL9602.h"
#include "GPSCoordinates.h"

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

NAL9602 sat(USBTX,USBRX);
GPSCoordinates coord();
DigitalOut heartBeat(LED1);

int main() {
  int incomingMessageFlag;
  while (true) {
    heartBeat = 1;
    incomingMessageFlag = sat.signalQuality();
    wait(10);
    heartBeat = 0;
    sat.modem.printf("Satellite has %d bars\r\n",incomingMessageFlag);
    wait(5);
  }
 }
