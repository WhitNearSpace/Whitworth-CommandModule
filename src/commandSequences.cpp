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
char send_SBD_message(NAL9602 &sat) {
  char successFlags = 0;
  bool gps_success;

  // 1.  Clear the SBD message buffers
  sat.clearBuffer(2);

  // 2.  Are there pods that should be asked to send data?
  //     If so, send data request to each pod.
  //  >>> NOT YET IMPLEMENTED <<<

  // 3.  Update GPS coordinates
  gps_success = sat.gpsUpdate();
  if (gps_success) successFlags = successFlags | 0x1;

  // 4.  Are there pods that should have sent data?
  //     If so, load each pod's data into intermediate buffer
  //  >>> NOT YET IMPLEMENTED <<<

  // 5.  Generate SBD message and send to NAL 9602 message buffer
  sat.setMessage(getBatteryVoltage(), intTempSensor.read(), extTempSensor.read());

  // 6.  Transmit SBD message
  printf("This is when an SBD transmission would occur\r\n");
  // sat.transmitMessage();

  return successFlags;
}
