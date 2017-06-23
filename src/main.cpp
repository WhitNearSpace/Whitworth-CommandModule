#include "mbed.h"
#include "NAL9602.h"
#include "SBDmessage.h"

/** Command Module Microcontroller
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.1
 * @date 2017
 * @copyright GNU Public License
 *
 * Version History:
 *  0.1 - Terminal emulation mode only for testing of NAL9602 library
 */

NAL9602 sat(p28,p27);
Serial pc(USBTX,USBRX);
DigitalOut led1(LED1);

int main() {
  gpsModes currentGPSmode = stationary;
  NetworkRegistration regResponse;
  BufferStatus buffStatus;
  Timer pauseTime;
  time_t t;
  pc.baud(115200);
  pc.printf("\r\n\r\n----------------------------------------\r\n");
  pc.printf("LPC1768 restarting...\r\n\r\n");
  led1 = 0;
  pauseTime.start();
  while (!sat.modem.readable() && pauseTime<5) {
  }
  sat.echoModem();
  // End start-up procedure
  sat.setModeGPS(currentGPSmode);
  while (!sat.validTime) {
    led1 = 1;
    sat.syncTime();
    led1 = 0;
    if (!sat.validTime)
      wait(15);
  }
  time(&t);
  printf("%s\r\n", ctime(&t));
  sat.verboseLogging = true;
  sat.sbdMessage.setMissionID(1);
  buffStatus = sat.getBufferStatus();
  pc.printf("Incoming message: %d, %d\r\n", buffStatus.incomingFlag, buffStatus.incomingMsgNum);
  pc.printf("Outgoing message: %d, %d\r\n", buffStatus.outgoingFlag, buffStatus.outgoingMsgNum);
  while (true) {
  }
 }
