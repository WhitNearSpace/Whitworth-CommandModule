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

int flightTransPeriod = 60;  // time between SBD transmissions (in s) during flight
Timer timeSinceTrans;  // time since last SBD transmission
int flightMode; // flag for mode (lab, pre-liftoff, moving, landed)

int main() {
  flightMode = 0;  // start in "lab" mode on powerup
  const int preTransPeriod = 300; // before "liftoff" transmit once per 5 minutes
  const int postTransPeriod = 600; // after "landing" transmit once per 10 minutes
  gpsModes currentGPSmode = stationary;
  NetworkRegistration regResponse;
  BufferStatus buffStatus;
  Timer pauseTime;
  time_t t;
  int err;
  bool success;

  pc.baud(115200);
  pc.printf("\r\n\r\n--------------------------------------------------\r\n");
  pc.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
  pc.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
  led1 = 0;
  pauseTime.start();
  while (!sat.modem.readable() && pauseTime<5) {
  }
  pauseTime.stop();
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
  while (true) {
    switch (flightMode) {

      case 0:  // Lab mode (no SBD transmission)
        if (pc.readable()) {
          parseLaunchControlInput(pc, sat);
        }
        break;

      case 1: // Flight mode, pre-liftoff
        if (timeSinceTrans > preTransPeriod) {
          timeSinceTrans.reset();
        }
        if (pc.readable()) {
          parseLaunchControlInput(pc, sat);
        }
        break;

      case 2: // Flight mode, moving!
        break;
    }
  }
 }
