#include "NAL9602.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Tested with emulator
 *  - Lab tested with 9602-LP
 *  - Flight tested
 */

// Status: Incomplete
NAL9602::NAL9602(PinName tx_pin, PinName rx_pin, PinName ri_pin) : modem(tx_pin, rx_pin), RI(ri_pin) {
  // Set initial state of flags
  ringAlert = false;
  messageAvailable = false;
  validTime = false;

  // Start in "quiet" mode
  satLinkOff();
  gpsOff();
}

// Status: Incomplete
NAL9602::~NAL9602(void) {
  // Detach 9602 from Iridium network
  /* TBC */

  // Shut down receivers
  satLinkOff();
  gpsOff();
}

// Status: Tested with terminal
void NAL9602::satLinkOn(void) {
  modem.printf("AT*S1\n\r");
}

// Status: Tested with terminal
void NAL9602::satLinkOff(void) {
  modem.printf("AT*S0\n\r");
}

// Status: Tested with terminal
void NAL9602::gpsOn(void) {
  modem.printf("AT+PP=1\n\r");
}

// Status: Tested with terminal
void NAL9602::gpsOff(void) {
  modem.printf("AT+PP=0\n\r");
  coord.clearCoordinates();
}

// Status: Tested with terminal
int NAL9602::checkRingAlert(void) {
  int sri;
  int argFilled;

  modem.printf("AT+CRIS\n\r");
  // Expected response has the form +CRIS:<tri>,<sri>
  // tri is not defined for 9602
  argFilled = modem.scanf("+CRIS: %*d,%d", &sri);
  if (argFilled == 1) {
    if ( (sri==0) || (sri==1) ) {
      ringAlert = (sri==1);
      return sri;
    } else
      return -2; // error: sri has invalid value
  } else
    return -1;  // error: no value for sri found
}

// Status: Tested with terminal
int NAL9602::signalQuality() {
  int bars;
  int argFilled;

  modem.printf("AT+CSQF\n\r");
  // Expected response has form: +CSQF:<rssi>
  argFilled = modem.scanf("+CSQF:%d", &bars);
  if (argFilled == 1) {
    if ((bars>=0) && (bars<=5))
      return bars;
    else
      return -2; // error: bars has invalid value
  } else
    return -1; // error: no value for bars found
}

// Status: Tested with terminal
bool NAL9602::gpsUpdate() {
  int argFilled;
  bool valid = true;
  int deg, min, decmin;
  char dir;
  float alt;
  char fixString[80];
  char invalidString[] = "Invalid";
  int num;
  time_t receivedTime;

  modem.printf("AT+PLOC\n\r");
  /* Expected response has the form:
   * +PLOC:
   * Latitude:<||>:<mm>.<nnnnn> <N/S>
   * Longitude: <ooo>:<pp>.<qqqqq> <E/W>
   * Altitude: <#> meters
   * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
   * Satellites Used=<zz>
   */
   modem.scanf("+PLOC:");
   receivedTime = time(NULL);
   argFilled = modem.scanf(" Latitude:%d:%d.%d %c",&deg,&min,&decmin,&dir);
   if (argFilled == 4) {
     coord.setLatitudeDegMin(deg, min, decmin, dir=='N');
   } else valid = false;

   argFilled = modem.scanf(" Longitude:%d:%d.%d %c",&deg,&min,&decmin,&dir);
   if (argFilled == 4) {
     coord.setLongitudeDegMin(deg, min, decmin, dir=='E');
   } else valid = false;

   argFilled = modem.scanf(" Altitude:%f meters",&alt);
   if (argFilled == 1)
    coord.setAltitude(alt);
   else valid = false;

   modem.scanf("%79s", &fixString);
   if (strcmp(invalidString,fixString)==0)
    valid = false;

   argFilled = modem.scanf("[^S]Satellites Used=%d", &num);
   coord.satUsed = num;
   if (valid)
    coord.syncTime = receivedTime;
   coord.positionFix = valid;

   modem.printf("AT+PVEL\n\r");
   /* Expected response has the form:
    * +PVEL:
    * Ground Velocity=<#g> km/h, <#h> degrees from true North
    * Vertical Velocity=<#v> m/s
    * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
    * Satellites Used=<zz>
    */
    float v;
    int h;
    modem.scanf("+PVEL:");
    argFilled = modem.scanf(" Ground Velocity=%f km/h, %d", &v, &h);
    if (argFilled == 2) {
      coord.setGroundSpeed(v);
      coord.setHeading(h);
    };
    argFilled = modem.scanf("[^V]Vertical Velocity=%f", &v);
    if (argFilled == 1) {
      coord.setVerticalVelocity(v);
    }
    argFilled = modem.scanf("[^S]Satellites Used=%*d");

   return valid;
}

// Status: Ready for testing
float NAL9602::latitude() {
  return coord.getLatitudeDecDeg();
}

// Status: Ready for testing
float NAL9602::longitude() {
  return coord.getLongitudeDecDeg();
}

// Status: Ready for testing
float NAL9602::altitude() {
  return coord.getAltitude();
}

// Status: Tested with terminal
void NAL9602::setModeGPS(gpsModes mode) {
  modem.printf("AT+PNAV=%d\n\r",mode);
}

// Status: Tested with terminal
void NAL9602::zeroMessageCounter() {
  modem.printf("AT+SBDC\n\r");
}

// Status: Ready for testing
int NAL9602::transmitMessage() {
  int outgoingStatus;
  int outgoingMessageCount;
  int incomingStatus;
  int incomingMessageCount;
  int incomingLength;
  int queueLength;
  if ((RI==1)||ringAlert) {
    modem.printf("AT+PSIXA\n\r");
  } else {
    modem.printf("AT+PSIX\n\r");
  }
  modem.scanf("+SBDIX:%d,%d,%d,%d,%d,%d", &outgoingStatus,
    &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
    &incomingLength, &queueLength);
  if (incomingStatus == 1) {
    messageAvailable = true;
    incomingMessageLength = incomingLength;
  }
  if (queueLength == 0)
    ringAlert = false;
  return outgoingStatus;
}

// Status: Ready for testing
bool NAL9602::syncTime() {
  struct tm t;
  validTime = false;

  // Get UTC date from GPS
  modem.printf("AT+PD\n\r");
  modem.scanf("+PD:");
  modem.scanf(" UTC Date = %u-%u-%u", &t.tm_mon, &t.tm_mday,
    &t.tm_year);
  t.tm_year = t.tm_year - 1900; // Years since 1900 is required
  t.tm_mon = t.tm_mon - 1;  // January = 0, not 1
  modem.scanf("[^S]Satellites Used=%*d");  // scan to end of response

  // Get UTC time from GPS
  modem.printf("AT+PT\n\r");
  modem.scanf("+PT:");
  modem.scanf(" UTC Time=%u:%u:%u.%*u", &t.tm_hour, &t.tm_min, &t.tm_sec);
  modem.scanf("[^S]Satellites Used=%*d");  // scan to end of response

  // Set RTC and associated flag
  set_time(mktime(&t));
  time_t seconds = time(NULL);
  if (seconds > 1496150000)
    validTime = true;
  return validTime;
}
