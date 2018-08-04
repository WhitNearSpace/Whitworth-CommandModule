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

// Status: Ready for testing
RN41::RN41(PinName tx_pin, PinName rx_pin) : modem(tx_pin, rx_pin) {
  // Set initial state of flags
  connected = false;
  shutdownRequest = false;
  cmdStep = 0;
  cmdReady = false;

  // Set modem baud rate
  modem.baud(115200);
}

// Status: Ready for testing
RN41::~RN41(void) {
  // Is there anything to do?
}

void RN41::initiateShutdown() {
  shutdownRequest = true;
  connected = false;
  cmdReady = false;
  cmdStep = 0;
  cmdSequencer.attach(callback(this, &RN41::queueCmd), 1.25);
}

void RN41::processShutdown() {
  if (cmdReady) {
    switch(cmdStep) {
      case 1:
        modem.printf("$$$");
        cmdSequencer.attach(callback(this, &RN41::queueCmd), 1.25);
        cmdReady = false;
        break;
      case 2:
        modem.printf(" K,\r\n");
        cmdSequencer.attach(callback(this, &RN41::queueCmd), 1.25);
        cmdReady = false;
        break;
      case 3:
        modem.printf("Z\r\n");
        shutdownRequest = false;
        cmdReady = false;
        break;
    }
  }
}

void RN41::queueCmd() {
  cmdStep++;
  cmdReady = true;
}
