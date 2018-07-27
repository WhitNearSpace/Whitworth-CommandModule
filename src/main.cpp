#include "mbed.h"
#include "NAL9602.h"
#include "SBDmessage.h"
#include "launchControlComm.h"
#include "TMP36.h"

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

Serial pc(p9,p10);
NAL9602 sat(p28,p27);
TMP36 intTempSensor(p18);
TMP36 extTempSensor(p20);
AnalogIn batterySensor(p19);

// Status LEDs
DigitalOut powerStatus(p24);
DigitalOut gpsStatus(p22);
DigitalOut satStatus(p21);
DigitalOut podStatus(p23);
DigitalOut futureStatus(p25);

int main() {
  gpsModes currentGPSmode = stationary;
  NetworkRegistration regResponse;
  BufferStatus buffStatus;
  Timer pauseTime;
  time_t t;
  int err;
  bool success;

  /** flightMode
   *  0: lab mode - do not transmit SBD
   *  1: preflight mode - transmit SBD at 5 minute intervals, wait for launch
   *  2: flight mode - transmit SBD at specified interval (15-75 s), wait for land
   *  3: postflight mode - transmit SBD at 10 minute intervals
   */
  int flightMode = 0;

  // Start-up LED sequence
  powerStatus = 0;
  gpsStatus = 0;
  satStatus = 0;
  podStatus = 0;
  for (int i = 0; i<5; i++) {
      futureStatus = 0;
      powerStatus = 1;
      wait(0.2);
      powerStatus = 0;
      gpsStatus = 1;
      wait(0.2);
      gpsStatus = 0;
      satStatus = 1;
      wait(0.2);
      satStatus = 0;
      podStatus = 1;
      wait(0.2);
      podStatus = 0;
      futureStatus = 1;
      wait(0.2);
  }
  futureStatus = 0;
  powerStatus = 1;

  pc.baud(115200);
  pc.printf("\r\n\r\n----------------------------------------------------------------------------------------------------\r\n");
  pc.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
  pc.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
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
