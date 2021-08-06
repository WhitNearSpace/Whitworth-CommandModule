#include <mbed.h>
#include <rtos.h>
#include "NAL9602.h"
#include "RN41.h"
#include <CM_to_FC.h>
#include "StatusLED.h"
#include "launchControlComm.h"
#include "FlightParameters.h"
#include "commandSequences.h"


/** Command Module Microcontroller
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 3.0.b1
 * @date 2020
 * @copyright MIT License
 *
 * Version History:
 *  0.1 - Terminal emulation mode only for testing of NAL9602 library
 *  0.2 - Interface with Launch Control app and more testing
 *  1.0 - Successful first flight
 *  2.0 - Pod communication added, transitioned to RTOS
 *  3.0b0 - Begin rewrite for modular hardware and RockBlock vs NAL 9602
 *  3.0b1 - Make changes caused by update to Mbed OS 6
 *    - console access (Serial pc references need to be changed)
 *    - method for creating delays (wait --> ThisThread::sleep_for) 
 */

char versionString[] = "3.0.b1";
char dateString[] = "7/5/2021";

// LPC1768 connections (active)
SPI spi(p5, p6, p7);              // Shared SPI master
I2C i2c(p9,p10);                  // Shared I2C master
DigitalIn serialAlert(p11);       // Alert: there is inbound serial communication from multiplexer
DigitalOut podRadioWake(p12);     // Send wake signal to XBee pod link
StatusLED ledA(p21);                 // External indicator LED A
StatusLED ledB(p22);                 // External indicator LED B
StatusLED ledC(p23);                 // External indicator LED C
DigitalOut audioAlert(p24);       // Audio alert control

/*************************************************************************************************
 * LPC1768 connections (inactive)
 * 
 * p8 is currently unassigned
 * CM_to_FC podRadio(p13, p14);      // XBee 802.15.4 (2.4 GHz) interface for pod communications
 * DigitalOut groundRadioWake(p15);  // Send wake signal to XBee ground link
 * AnalogIn analogIn0(p16);
 * AnalogIn analogIn1(p17);
 * AnalogIn analogIn2(p18);          // Could also be an analog output
 * PwmOut pwm(p25);
 * RB9603 sat(p28, p27, p19, p20, p30, p26, p29); // RockBlock 9603 interface object  
*************************************************************************************************/

// Shared connections
// RN41 bt(i2c)

/*************************************************************************************************
 * old LPC1768 connections
 * RN41 bt(p9,p10);              // Bluetooth connection via RN-41
 * NAL9602 sat(p28,p27);         // NAL 9602 modem interface object
 * CM_to_FC podRadio(p13, p14);  // XBee 802.15.4 (2.4 GHz) interface for pod communications
 * DigitalOut podRadioWake(p15); // Signal XBee to move from sleep to wake mode
 * TMP36 intTempSensor(p18);     // Internal temperature sensor
 * TMP36 extTempSensor(p20);     // External temperature sensor
 * AnalogIn batterySensor(p19);  // Command module battery monitor
 * DigitalOut powerStatus(p24);  // red (command module powered)
 * DigitalOut gpsStatus(p22);    // green (GPS unit powered)
 * DigitalOut satStatus(p21);    // blue (Iridium radio powered)
 * DigitalOut podStatus(p23);    // amber, clear (XBee connection to pods)
 * DigitalOut futureStatus(p25); // amber, opaque (currently used to indicate when parsing command from BT)
*************************************************************************************************/

// Flight state and settings
struct FlightParameters flight;

// Timing objects
Timer timeSinceTrans;     // time since last SBD transmission
Timer checkTime;          // timer in pending mode to do checks increasing altitude


int main() {
  time_t t;  // Time structure
  bool savedFlightParameters = false;
  bool gps_success;  // Was GPS update a success?
  bool transmit_success;  // Was SBD transmit a success?
  bool transmit_timeout;  // Did the SBD transmission fail to complete in time?
  char sbdFlags; // byte of flags (bit 0 = gps, 1 = lo )
  Timer pauseTime;  // wait for things to respond but if not, move on
  Timer podInviteTime;   // time since last pod invitation sent

  flight.mode = 0;              // flag for mode (0 = lab)
  flight.transPeriod = 60;      // time between SBD transmissions (in s) during flight
  flight.triggerHeight = 40;    // trigger active flight if this many meters above ground
  int landedIndicator = 0;      // number of times has been flagged as landed
  const float podInviteInterval = 60;  // Send invitations to pods (when in lab or launch mode) at this interval (in seconds)

  // Satellite modem startup
  // sat.saveStartLog(5);  // Gather any start-up output from satellite modem for 5 seconds
  // sat.verboseLogging = false;

  // Initiate LED Status Lights
  ledA = quick_flash; // waiting for Bluetooth
  ledB = off;         // No GPS activity
  ledC = off;         // No pod link activity

  // Bluetooth start-up sequence
  // Wait for Bluetooth link. If not found in 60 seconds, check for saved flight parameters.
  // Repeat until either linked or flight parameters found.
  pauseTime.start();
  while ((!bt.connected) && (!savedFlightParameters)) {
    while ((!bt.modem.readable()) && (pauseTime < 60)) {
      ThisThread::sleep_for(200ms);
    }
    if (pauseTime < 60) {
      bt.connected = true;
      ledA = steady;
    } else {
      // No Bluetooth connnection so should try to load previous flight parameters
    }
    pauseTime.reset();
  }
  pauseTime.stop();

  if (bt.connected) {
    bt.modem.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    bt.modem.printf("Near Space Command Module, v. %s (%s)\r\n", versionString, dateString);
    bt.modem.printf("John M. Larkin, Department of Engineering and Physics\r\nWhitworth University\r\n\r\n");
    bt.modem.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    bt.modem.printf("NAL 9602 power-up log:\r\n");
    sat.echoStartLog(bt.modem);
    sat.gpsNoSleep();
    sat.setModeGPS(stationary);
    bt.modem.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
    bt.modem.printf("Battery = %0.2f V\r\n", getBatteryVoltage());
    bt.modem.printf(" \r\nSynchronizing clock with satellites...\r\n");
  }

  while (!sat.validTime) {
    sat.syncTime();
    if (!sat.validTime)
      ThisThread::sleep_for(15s);
  }
  time(&t);
  srand(time(NULL)); // seed the random number generator with the current time (used for transmit retry delay)
  if (bt.connected) {
    bt.modem.printf("%s (UTC)\r\n", ctime(&t));
    bt.modem.printf("\r\n----------------------------------------------------------------------------------------------------\r\n");
  } else { // No Bluetooth connection so set default
  // In a future version I would like to read in saved mission settings from some persistent storage
      //   int err = readFlightInfo(sat, podRadio);
      //   if (!err) {
      //     changeModeToPending(sat);
      //   }
  // The "better than nothing" version will use default settings so minimal tracking would still take place in case of power cycle mid-flight
    sat.sbdMessage.missionID = 1;  // Default mission ID
    changeModeToPending(sat);
  }

  if (bt.modem.readable()) {
    parseLaunchControlInput(bt.modem, sat); // really should just be handshake detect but I'm lazy (for now)
  }
  
  sat.verboseLogging = false;  // "true" is causing system to hang during gpsUpdate
  podInviteTime.start();

  while (true) {
    switch (flight.mode) {
      /************************************************************************
       *  Lab mode (no SBD transmission)
       *
       *  Can be promoted to mode 1 by Launch Control
       ***********************************************************************/
      case 0:
        if (bt.modem.readable()) {
          futureStatus = 1; // Use "future LED" to signal processing command
          parseLaunchControlInput(bt.modem, sat);
          futureStatus = 0;
        }
        if (podInviteTime > podInviteInterval) {
          podInviteTime.reset();
          podRadio.invite();
        }
        break;

      /************************************************************************
       *  Launch pad mode, pre-liftoff (SBD transmissions, but not too frequent)
       *
       *  Can be demoted to mode 0 by Launch Control
       *  Can be promoted to mode 2 if altitude crosses threshold
       ***********************************************************************/
      case 1:
        if ((timeSinceTrans > PRE_TRANS_PERIOD) || (sat.sbdMessage.attemptingSend)) {
          // If haven't already started send process, reset clock
          if (!sat.sbdMessage.attemptingSend) {
            timeSinceTrans.reset();
          }
          sbdFlags = send_SBD_message(bt, sat, podRadio);
          gps_success = sbdFlags & 1;
          transmit_success = sbdFlags & 16;
          transmit_timeout = sbdFlags & 128;
          if (transmit_success || transmit_timeout) {
            sat.sbdMessage.attemptingSend = false;
          }
        }
        if ((checkTime > 15) && (!sat.sbdMessage.attemptingSend)) { // Periodic GPS update
          checkTime.reset();
          if (!gps_success)
            gps_success = sat.gpsUpdate();
          if (gps_success) {
            if (sat.altitude() > (flight.groundAltitude + flight.triggerHeight)) {
              bt.modem.printf("Changing to flight mode. Good bye!\r\n");
              changeModeToFlight(bt, sat);
            }
          }
        }
        if (bt.modem.readable()) {
          futureStatus = 1;
          parseLaunchControlInput(bt.modem, sat);
          futureStatus = 0;
        }
        if (podInviteTime > podInviteInterval) {
          podInviteTime.reset();
          podRadio.sync_registry();
          podRadio.printRegistry();
          podRadio.invite();
          podRadio.test_all_clocks();
        }
        gps_success = false;
        transmit_success = false;
        break;

      /************************************************************************
       *  Flight mode
       *
       *  Can be promoted to mode 3 if meets landing condition
       ***********************************************************************/
      case 2:
        if ((timeSinceTrans > flight.transPeriod) || (sat.sbdMessage.attemptingSend)) {
          // If haven't already started send process, reset clock
          if (!sat.sbdMessage.attemptingSend) {
            timeSinceTrans.reset();
          }
          sbdFlags = send_SBD_message(bt, sat, podRadio);
          gps_success = sbdFlags & 1;
          transmit_success = sbdFlags & 16;
          transmit_timeout = sbdFlags & 128;
          if (transmit_success || transmit_timeout) {
            sat.sbdMessage.attemptingSend = false;
          }
        }
        if ((checkTime > 15) && (!sat.sbdMessage.attemptingSend)) {
          checkTime.reset();
          if (!gps_success)
            gps_success = sat.gpsUpdate();
          if (gps_success) {
            if ((sat.altitude() < 5000) && (sat.altitude() > 0)) { // Is altitude plausible for landing?
              if (fabs(sat.verticalVelocity())<1.0) // Is vertical velocity small?
                landedIndicator++;
              if (landedIndicator > 2*(flight.transPeriod/15)) // If enough landing confirmations, transition to Landed mode
                changeModeToLanded(bt, sat);
            }
          }
        }
        break;

      /************************************************************************
       *  Landed mode
       *
       *  Send SBD messages infrequently
       ***********************************************************************/
      case 3: // Landed mode
        if ((timeSinceTrans > POST_TRANS_PERIOD) || (sat.sbdMessage.attemptingSend)) {
          // If haven't already started send process, reset clock
          if (!sat.sbdMessage.attemptingSend) {
            timeSinceTrans.reset();
          }
          sbdFlags = send_SBD_message(bt, sat, podRadio);
          gps_success = sbdFlags & 1;
          transmit_success = sbdFlags & 16;
          transmit_timeout = sbdFlags & 128;
          if (transmit_success || transmit_timeout) {
            sat.sbdMessage.attemptingSend = false;
          }
        }
        if ((checkTime > 15) && (!sat.sbdMessage.attemptingSend)) {
          checkTime.reset();
          if (getBatteryVoltage()<6.4) {
            // Battery is running low so shut down systems
            sat.satLinkOff();
            sat.gpsOff();
            statusLightTicker.detach();
            powerStatus = 0;
            gpsStatus = 0;
            satStatus = 0;
            podStatus = 0;
            timeSinceTrans.stop();
            checkTime.stop();
            while (1) {
              sleep();
            }
          }
        }
        break;
    }
  }
 }
