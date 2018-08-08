#include "commandSequences.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Lab tested with 9602-A
 *  - Flight tested
 */

// Status:  Ready for testing
char send_SBD_message(RN41 &bt, NAL9602 &sat) {
  char successFlags = 0;
  bool gps_success;
  bool msg_err;
  bool transmit_success;

  // 1.  Are there pods that should be asked to send data?
  //     If so, send data request to each pod.
  //  >>> NOT YET IMPLEMENTED <<<

  // 2.  Update GPS coordinates
  bt.modem.printf("Updating GPS...");
  gps_success = sat.gpsUpdate();
  if (gps_success) successFlags = successFlags | 0x1;
  if (gps_success) {
    bt.modem.printf("success\r\n");
  } else {
    bt.modem.printf("failure\r\n");
  }

  // 3.  Are there pods that should have sent data?
  //     If so, load each pod's data into intermediate buffer
  //  >>> NOT YET IMPLEMENTED <<<

  // 4.  Generate SBD message and send to NAL 9602 message buffer
  bt.modem.printf("Setting message...");
  msg_err = sat.setMessage(getBatteryVoltage(), 0, 0);
  if (!msg_err) successFlags = successFlags | 0x2;
  if (!msg_err) {
    bt.modem.printf("success\r\n");
  } else {
    bt.modem.printf("failure\r\n");
  }

  // 5.  Transmit SBD message
  transmit_success = sat.transmitMessage();
  if(transmit_success) successFlags = successFlags | 0x4;
  bt.modem.printf("Transmit code: %i\r\n", transmit_success);
  return successFlags;
}
