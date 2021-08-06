#include "RN41.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Lab tested with 9602-A
 *  - Flight tested
 */

// Status: Tested with terminal
RN41::RN41(BufferedSerial* bt_serial_ptr) {
  // Set initial state of flags
  connected = false;
  shutdownRequest = false;
  cmdStep = 0;
  cmdReady = false;

  // Set the baud rate to the default speed of the RN41 module
  bt_serial_ptr->set_baud(115200);

  // Make file-like connection to BufferedSerial object
  modem = fdopen(bt_serial_ptr, "r+");
}

// Status: Tested with terminal
RN41::~RN41(void) {
  // Is there anything to do?
}

// Status: Ready for testing
void RN41::initiateShutdown() {
  shutdownRequest = true;
  connected = false;
  cmdReady = false;
  cmdStep = 0;
  cmdSequencer.attach(callback(this, &RN41::queueCmd), 1250ms);
}

// Status: Ready for testing
void RN41::processShutdown() {
  if (cmdReady) {
    switch(cmdStep) {
      case 1:
        fprintf(modem, "$$$");
        cmdSequencer.attach(callback(this, &RN41::queueCmd), 1250ms);
        cmdReady = false;
        break;
      case 2:
        fprintf(modem, " K,\r\n");
        cmdSequencer.attach(callback(this, &RN41::queueCmd), 1250ms);
        cmdReady = false;
        break;
      case 3:
        fprintf(modem, "Z\r\n");
        shutdownRequest = false;
        cmdReady = false;
        break;
    }
  }
}

// Status: Ready for testing
void RN41::queueCmd() {
  cmdStep++;
  cmdReady = true;
}
