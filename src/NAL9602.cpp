#include "NAL9602.h"

NAL9602::NAL9602(PinName tx_pin, PinName rx_pin, PinName ri_pin) : modem(tx_pin, rx_pin), _ri(ri_pin) {
  // Start in "quiet" mode
  satLinkOff();
  gpsOff();

  /* TBC */
}

NAL9602::~NAL9602(void) {
  // Detach 9602 from Iridium network
  /* TBC */

  // Shut down receivers
  satLinkOff();
  gpsOff();

  /* TBC */
}

void NAL9602::satLinkOn(void) {
  modem.printf("AT*S1\n\r");
}

void NAL9602::satLinkOff(void) {
  modem.printf("AT*S0\n\r");
}

void NAL9602::gpsOn(void) {
  modem.printf("AT+PP=1\n\r");
}

void NAL9602::gpsOff(void) {
  modem.printf("AT+PP=0\n\r");
}

int NAL9602::checkRingAlert(void) {
  int sri;

  modem.printf("AT+CRIS\n\r");
  // Expected response has the form +CRIS:<tri>,<sri>
  // tri is not defined for 9602
  modem.scanf("+CRIS: %*d, %d", &sri);
  return sri;
}
