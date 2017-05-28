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
NAL9602::NAL9602(PinName tx_pin, PinName rx_pin) : modem(tx_pin, rx_pin) {
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
    if ( (sri==0) || (sri==1) )
      return sri;
    else
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
