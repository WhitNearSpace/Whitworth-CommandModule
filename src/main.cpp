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

NAL9602 sat(p28,p27);
Serial pc(USBTX,USBRX);
DigitalOut led1(LED1);

int main() {
  Timer pauseTime;
  time_t t;
  sat.verboseLogging = true;
  pc.baud(115200);
  led1 = 0;
  pauseTime.start();
  while (!sat.modem.readable() && pauseTime<15) {
  }
  sat.echoModem();
  while (!sat.validTime) {
    wait(15);
    led1 = 1;
    sat.syncTime();
    led1 = 0;
  }
  time(&t);
  printf("%s\r\n", ctime(&t));
  sat.gpsUpdate();
  while (true) {
  }
 }
