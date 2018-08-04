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

// LPC1768 connections
Serial bt(p9,p10);            // Bluetooth connection via RN-41
NAL9602 sat(p28,p27);         // NAL 9602 modem interface object
TMP36 intTempSensor(p18);     // Internal temperature sensor
TMP36 extTempSensor(p20);     // External temperature sensor
AnalogIn batterySensor(p19);  // Command module battery monitor
DigitalOut powerStatus(p24);  // red (command module powered)
DigitalOut gpsStatus(p22);    // green (GPS unit powered)
DigitalOut satStatus(p21);    // blue (Iridium radio powered)
DigitalOut podStatus(p23);    // amber, clear (XBee connection to pods)
DigitalOut futureStatus(p25); // amber, opaque (currently used to indicate when parsing command from BT)


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
  bt.baud(115200);
  NetworkRegistration regResponse;
  BufferStatus buffStatus;

  time_t t;
  int err;
  bool success;
  bool gps_success;
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
  while (!bt.readable() && pauseTime < 60) {
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
    bt.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    bt.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
    bt.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
    bt.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    sat.echoStartLog();
    bt.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    bt.printf("Battery = %0.2f V\r\n", getBatteryVoltage());
    bt.printf(" \r\nSynchronizing clock with satellites...\r\n");
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
    bt.printf("%s\r\n", ctime(&t));
    bt.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
  }

  if (bt.readable()) {
    parseLaunchControlInput(bt, sat); // really should just be handshake detect but I'm lazy (for now)
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
        if (bt.readable()) {
          futureStatus = 1;
          parseLaunchControlInput(bt, sat);
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
          /********************************************************************
           * Send SBD message
           *******************************************************************/
          // 1.  Reset transmission counter
          timeSinceTrans.reset();

          // 2.  Clear the SBD message buffers
          sat.clearBuffer(2);

          // 3.  Are there pods that should be asked to send data?
          //     If so, send data request to each pod.
          //  >>> NOT YET IMPLEMENTED <<<

          // 4.  Update GPS coordinates
          gps_success = sat.gpsUpdate();

          // 5.  Are there pods that should have sent data?
          //     If so, load each pod's data into intermediate buffer
          //  >>> NOT YET IMPLEMENTED <<<

          // 6.  Generate SBD message and send to NAL 9602 message buffer
          sat.setMessage(missionID, flightMode, getBatteryVoltage(), intTempSensor.read(), extTempSensor.read());

          // 7.  Transmit SBD message
          bt.printf("This is when an SBD transmission would occur\r\n");
          // sat.transmitMessage();
          /********************* END - Send SBD message ****************/

        }
        if (pauseTime > 15) {
          pauseTime.reset();
          if (!gps_success)
            gps_success = sat.gpsUpdate();
          if (gps_success) {
            currentAltitude = sat.altitude();
            if (currentAltitude > (groundAltitude+triggerHeight)) {
              bt.printf("Changing to flight mode 2\r\n");
              changeModeToFlight(bt, sat);
            }
          }
        }
        if (bt.readable()) {
          futureStatus = 1;
          parseLaunchControlInput(bt, sat);
          futureStatus = 0;
        }
        gps_success = false;
        break;

      case 2: // Flight mode, moving!
        break;

      case 3: // Flight mode, landed
        break;
    }
  }
 }
