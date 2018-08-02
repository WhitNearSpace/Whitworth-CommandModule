#include "mbed.h"
#include "NAL9602.h"
#include "SBDmessage.h"
#include "launchControlComm.h"
#include "TMP36.h"

/** Command Module Microcontroller
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.2
 * @date 2018
 * @copyright MIT License
 *
 * Version History:
 *  0.1 - Terminal emulation mode only for testing of NAL9602 library
 *  0.2 - Interface with Launch Control app and more testing
 */

char versionString[] = "0.2";
char dateString[] = "8/01/2018";

Serial pc(p9,p10);
NAL9602 sat(p28,p27);
TMP36 intTempSensor(p18);
TMP36 extTempSensor(p20);
AnalogIn batterySensor(p19);

// Status LEDs
DigitalOut powerStatus(p24);  // red
DigitalOut gpsStatus(p22);    // green
DigitalOut satStatus(p21);    // blue
DigitalOut podStatus(p23);    // amber, clear
DigitalOut futureStatus(p25); // amber, opaque (below switch)
// futureStatus is currently used to indicate when parsing command from BT

// Comm flags
bool btFound = false;

// Mission details
int missionID;
int flightTransPeriod = 60;  // time between SBD transmissions (in s) during flight

// GPS and flight variables
gpsModes currentGPSmode = stationary;  // initial value but change for flight
float groundAltitude;
float triggerHeight = 100;
int flightMode; // flag for mode (lab, pre-liftoff, moving, landed)

// Timing objects
Timer timeSinceTrans;  // time since last SBD transmission
Timer pauseTime;
Timeout cmdSequence;

int main() {
  pc.baud(115200);
  NetworkRegistration regResponse;
  BufferStatus buffStatus;

  time_t t;
  int err;
  bool success;
  float currentAltitude;

  Ticker statusTicker;

  /** flightMode
   *  0: lab mode - do not transmit SBD
   *  1: preflight mode - transmit SBD at 5 minute intervals, wait for launch
   *  2: flight mode - transmit SBD at specified interval (15-75 s), wait for land
   *  3: postflight mode - transmit SBD at 10 minute intervals
   */
  flightMode = 0;
  const int preTransPeriod = 300; // before "liftoff" transmit once per 5 minutes
  const int postTransPeriod = 600; // after "landing" transmit once per 10 minutes

  // Satellite modem startup
  pauseTime.start();
  while (!sat.modem.readable() && pauseTime<5) {
  }
  pauseTime.stop();
  pauseTime.reset();
  sat.saveStartLog(5);

  // Bluetooth start-up sequence
  powerStatus = 0;
  gpsStatus = 0;
  satStatus = 0;
  podStatus = 0;
  pauseTime.start();
  while (!pc.readable() && pauseTime < 60) {
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
  if (pauseTime < 60) {
    btFound = true;
  }
  pauseTime.stop();
  pauseTime.reset();
  futureStatus = 0;
  powerStatus = 1;

  if (btFound) {
    pc.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    pc.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
    pc.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
    pc.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    sat.echoStartLog();
    pc.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    pc.printf("Battery = %0.2f V\r\n", batterySensor*13.29);
    pc.printf(" \r\nSynchronizing clock with satellites...\r\n");
  }

  sat.verboseLogging = false;
  sat.setModeGPS(currentGPSmode);
  while (!sat.validTime) {
    sat.syncTime();
    if (!sat.validTime)
      wait(15);
  }
  time(&t);
  if (btFound) {
    pc.printf("%s\r\n", ctime(&t));
    pc.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
  }

  if (pc.readable()) {
    parseLaunchControlInput(pc, sat); // really should just be handshake detect but I'm lazy (for now)
  }
  statusTicker.attach(&updateStatusLED, 1.0);

  while (true) {
    switch (flightMode) {
      /************************************************************************
       *  Lab mode (no SBD transmission)
       *
       *  Can be promoted to mode 1 by Launch Control
       ***********************************************************************/
      case 0:
        if (pc.readable()) {
          futureStatus = 1;
          parseLaunchControlInput(pc, sat);
          futureStatus = 0;
        }
        break;

      /************************************************************************
       *  Flight mode, pre-liftoff (SBD transmissions, but not too frequent)
       *
       *  Can be demoted to mode 0 by Launch Control
       *  Can be promoted to mode 2 if altitude crosses threshold
       ***********************************************************************/
      case 1: // Flight mode, pre-liftoff
        if (timeSinceTrans > preTransPeriod) {
          timeSinceTrans.reset();
          /* carryout procedure for transmitting SBD */
          pc.printf("This is when an SBD transmission would occur\r\n");
        } else if (pauseTime > 15) {
          pauseTime.reset();
          success = sat.gpsUpdate();
          if (success) {
            currentAltitude = sat.altitude();
            if (currentAltitude > (groundAltitude+triggerHeight)) {
              pc.printf("Changing to flight mode 2\r\n");
              changeModeToFlight(pc, sat);
            }
          }
        }
        if (pc.readable()) {
          futureStatus = 1;
          parseLaunchControlInput(pc, sat);
          futureStatus = 0;
        }
        wait(0.2);

        break;

      case 2: // Flight mode, moving!
        break;

      case 3: // Flight mode, landed
        break;
    }
  }
 }
