#include "mbed.h"
#include "NAL9602.h"
#include "SBDmessage.h"
#include "launchControlComm.h"

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

char versionString[] = "0.1";
char dateString[] = "6/23/2017";

NAL9602 sat(p28,p27);
Serial pc(USBTX,USBRX);
DigitalOut led1(LED1);

int main() {
  gpsModes currentGPSmode = stationary;
  NetworkRegistration regResponse;
  BufferStatus buffStatus;
  Timer pauseTime;
  time_t t;
  int err;
  bool success;
  int flightMode = 0;
  pc.baud(115200);
  pc.printf("\r\n\r\n--------------------------------------------------\r\n");
  pc.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
  pc.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
  led1 = 0;
  pauseTime.start();
  while (!sat.modem.readable() && pauseTime<5) {
  }
  sat.verboseLogging = true;
  sat.echoModem();
  sat.setModeGPS(currentGPSmode);
  // End start-up procedure
  while (!sat.validTime) {
    sat.syncTime();
    if (!sat.validTime)
      wait(15);
  }
  time(&t);
  printf("%s\r\n", ctime(&t));
  while (flightMode == 0) {
    if (pc.readable()) {
      parseLaunchControlInput(pc, sat);
    }
  }
 }
