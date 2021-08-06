#include "RockBlock9603.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Lab tested with 9603
 *  - Flight tested
 */

// Status: Tested with terminal
RockBlock9603::RockBlock9603(PinName tx_pin, PinName rx_pin, PinName ri_pin) {
  _serial = new BufferedSerial(tx_pin, rx_pin, 19200);
  _at = new ATCmdParser(_serial, "\r");
  _RI = new InterruptIn(ri_pin);
  // Set initial state of flags
  ringAlert = false;
  messageAvailable = false;
  validTime = false;
  verboseLogging = false;
  iridiumStatus = true;
  startLogLength = 0;
  _at->set_timeout(AT_TIMEOUT_NORMAL);
  _at->send("AT&K0");
  _at->recv("OK");
  // at.oob("Invalid Position Fix", callback(this, &NAL9602::_oob_invalid_fix));
}

// Status: Incomplete
RockBlock9603::~RockBlock9603(void) {
  // Shutdown tasks for the RockBLOCK 9603
  /* TBC */
}

// Status: Tested with terminal
void RockBlock9603::radio_on(void) {
  _at->send("AT*R1");
  iridiumStatus = _at->recv("OK");
}

// Status: Tested with terminal
void RockBlock9603::radio_off(void) {
  _at->send("AT*R0");
  iridiumStatus = !_at->recv("OK");
}

// Status: Ready for testing
void RockBlock9603::echo_until_timeout(milliseconds listenTime) {
  Timer t;
  char buff[BUFFLENGTH];
  unsigned int i = 0;
  unsigned int j = 0;
  t.start();
  while (t.elapsed_time() <listenTime) {  // Be patient and listen for listenTime
    while ((_serial->readable()) && (i<BUFFLENGTH)) {
      buff[i] = _at->getc();
      i++;
    }
    while ((j<i) && (!_serial->readable())) {
      printf("%c",buff[j]);
      j++;
    }
    for (unsigned int k = 0; k<j; k++) {
      buff[k] = buff[j+k];
      i = i - j;
      j = 0;
    }
  }
}

void RockBlock9603::echo_until_OK() {
  char buf[80] = {0};
  unsigned int i = 0;
  char c;
  bool doneLeadingWS = false;
  bool done = false;
  while ((buf[0] != 'O') && (buf[1] != 'K')) { // does buf start "OK"?
    while (!done) { // "line" is defined to be printable-char surrounded by non-printable
      c = _at->getc();
      if (!doneLeadingWS) { // ignore leading non-printables
        if ((c >= 0x20) && (c <= 0x7E)) { // printable char range
          doneLeadingWS = true;
        }
      } else if (c < 0x20) { // done if find trailing non-printables
        done = true;
      }
      if ((c >= 0x20) && (c <= 0x7E)) {
        if (i < sizeof(buf)-1) { // don't overflow the buffer!
          buf[i] = c;
          i++;
        }
      }
    }
    buf[i] = 0; // terminate the string
    if ((buf[0] != 'O') && (buf[1] != 'K')) printf("\t%s\n", buf);
    doneLeadingWS = false;
    done =false;
    i = 0;
  }
}

// Status: Tested with terminal
void RockBlock9603::dump_manufacturer() {
  char buf[80];
  _at->send("AT+GMI");
  if (_at->recv("AT+GMI")) {
    _at->recv("%79s\r\n", buf);
    _at->recv("OK");
    printf("Manufacturer = %s\n", buf);
  } else {
    printf("Error reading manufacturer from RockBlock\n");
  }
}

// // Status: Tested with terminal
void RockBlock9603::dump_model() {
  printf("Model:");
  _at->send("AT+GMM");
  _at->recv("AT+GMM");
  // Normal recv doesn't work for some reason on model string
  echo_until_OK();
}

// // Status: Tested with terminal
void RockBlock9603::dump_revision() {
  printf("Iridium revision info\n");
  _at->send("AT+GMR");
  _at->recv("AT+GMR");
  echo_until_OK();
}

// Status: Tested with terminal
void RockBlock9603::dump_IMEI() {
  uint64_t imei;
  _at->send("AT+GSN");
  _at->recv("AT+GSN");
  _at->recv("%llu\r\n", &imei);
  _at->recv("OK");
  printf("IMEI = %llu\n", imei);
}

// Status: Tested with terminal
uint64_t RockBlock9603::get_IMEI() {
  uint64_t imei;
  _at->send("AT+GSN");
  _at->recv("AT+GSN");
  _at->recv("%llu\r\n", &imei);
  _at->recv("OK");
  return imei;
}

// Status: Ready for testing
int RockBlock9603::get_last_signal_quality() {
  int bars;
  bool argFilled;
  _at->send("AT+CSQF");
  // Expected response has form: +CSQF:<rssi>
  _at->recv("AT+CSQF");
  argFilled = _at->recv("+CSQF:%d\r\n", &bars);
  if (argFilled) {
    if ((bars>=0) && (bars<=5))
      return bars;
    else
      return -2; // error: bars has invalid value
  } else
    return -1; // error: no value for bars found
}

// Status: Ready for testing
int RockBlock9603::get_new_signal_quality() {
  int bars;
  bool argFilled;
  _at->send("AT+CSQ");
  // Expected response has form: +CSQ:<rssi>
  _at->recv("AT+CSQ");
  argFilled = _at->recv("+CSQ:%d\r\n", &bars);
  if (argFilled) {
    if ((bars>=0) && (bars<=5))
      return bars;
    else
      return -2; // error: bars has invalid value
  } else
    return -1; // error: no value for bars found
}

// Status:  Ready for testing
BufferStatus RockBlock9603::get_buffer_status() {
  BufferStatus bs;
  _at->send("AT+SBDSX");
  _at->recv("AT+SBDSX");
  _at->recv("+SBDSX: %d,%d,%d,%d,%d,%d\r\n", &bs.outgoingFlag, &bs.outgoingMsgNum,
    &bs.incomingFlag, &bs.incomingMsgNum, &bs.raFlag, &bs.numMsgWaiting);
  _at->recv("OK");
  return bs;
}





// // Status: Ready for testing
// int NAL9602::transmitMessage() {
//   int outgoingStatus;
//   int outgoingMessageCount;
//   int incomingStatus;
//   int incomingMessageCount;
//   int incomingLength;
//   int queueLength;
//   at.set_timeout(AT_TIMEOUT_LONG);
//   at.send("AT+SBDI");
//   at.recv("AT+SBDI");
//   at.recv("+SBDI:%d,%d,%d,%d,%d,%d\r\n", &outgoingStatus,
//     &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
//     &incomingLength, &queueLength);
//   if (incomingStatus == 1) {
//     messageAvailable = true;
//     incomingMessageLength = incomingLength;
//   }
//   if (queueLength == 0)
//     ringAlert = false;
//   at.recv("OK");
//   /* Outgoing (MO) codes
//    *   0 = no SBD message to send from the 9602
//    *   1 = SBD message successfully sent
//    *   2 = error occurred while attempting to send SBD message
//    *
//    */
//   if (outgoingStatus==1) {
//     printf("Transmission success!\r\n\r\n");
//   } else {
//     printf("Transmission error\r\n");
//   }
//   at.set_timeout(AT_TIMEOUT_NORMAL);
//   return outgoingStatus;
// }



// // Status: Ready for testing
// void NAL9602::clearBuffer(int selectedBuffer) {
//   at.send("AT+SBDD%d", selectedBuffer);
//   at.recv("OK");
// }

// // Status: Lab tested with 9602-A
// void NAL9602::addMessageGPS() {
//   sbdMessage.generateGPSBytes(coord);
// }

// // Status: Lab tested with 9602-A
// void NAL9602::loadPodData(int podID, char* data) {
//   sbdMessage.loadPodBuffer(podID, data);
// }

// // Status: Lab tested with 9602-A
// int NAL9602::setMessage(float voltage, float intTemp, float extTemp) {
//   char status[80];
//   int err;
//   sbdMessage.clearMessage();
//   sbdMessage.setMissionID(flight.mode);
//   sbdMessage.generateGPSBytes(coord);
//   sbdMessage.generateCommandModuleBytes(voltage, intTemp, extTemp);
//   sbdMessage.generatePodBytes();
//   sbdMessage.updateMsgLength();
//   int n = sbdMessage.msgLength;
//   if (n > 340) {
//     return -1;
//   } else {
//     at.send("AT+SBDWB=%d", n);
//     at.recv("READY");
//     for (int i = 0; i < n; i++) {
//       at.putc(sbdMessage.getByte(i));
//     }
//     unsigned short checksum = sbdMessage.generateChecksum();
//     at.putc((char)(checksum/256));
//     at.putc((char)(checksum%256));
//     at.recv("%d\r\n", &err);
//     at.recv("OK");
//     switch(err) {
//       case 0:
//         pc.printf("SBD message successfully written to 9602\r\n");
//         break;
//       case 1:
//         pc.printf("SBD message write timeout\r\n");
//         break;
//       case 2:
//         pc.printf("SBD message checksum does not match\r\n");
//         break;
//       case 3:
//         pc.printf("SBD message size is not correct\r\n");
//         break;
//       default:
//         pc.printf("Unknown error received following SBDWB command\r\n");
//     }
//     return err;
//   }
// }


// void NAL9602::_oob_invalid_fix() {
//   coord.positionFix = false;
// }

// // Status: Lab tested with 9602-A
// int NAL9602::checkRingAlert(void) {
//   int sri;
//   bool argFilled;

//   at.send("AT+CRIS");
//   // Expected response has the form +CRIS:<tri>,<sri>
//   // tri is not defined for 9602
//   at.recv("AT+CRIS");
//   argFilled = at.recv("+CRIS: %*d,%d\r\n", &sri);

//   if (argFilled) {
//     if ( (sri==0) || (sri==1) ) {
//       ringAlert = (sri==1);
//       return sri;
//     } else
//       return -2; // error: sri has invalid value
//   } else
//     return -1;  // error: no value for sri found
// }

// // Status: Lab tested with 9602-A
// void NAL9602::zeroMessageCounter() {
//   at.send("AT+SBDC");
//   at.recv("OK");
// }

// // Status: Ready for testing
// int NAL9602::transmitMessageWithRingAlert() {
//   int outgoingStatus;
//   int outgoingMessageCount;
//   int incomingStatus;
//   int incomingMessageCount;
//   int incomingLength;
//   int queueLength;
//   // if ((RI==1)||ringAlert) {
//   //   modem.printf("AT+PSIXA\r");
//   //   modem.scanf(" AT+PSIXA");
//   // } else {
//   at.send("AT+PSIX");
//   at.recv("AT+PSIX");
//   // }
//   at.recv("+SBDIX:%d,%d,%d,%d,%d,%d\r\n", &outgoingStatus,
//     &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
//     &incomingLength, &queueLength);
//   if (incomingStatus == 1) {
//     messageAvailable = true;
//     incomingMessageLength = incomingLength;
//   }
//   if (queueLength == 0)
//     ringAlert = false;
//   at.recv("OK");
//   switch(outgoingStatus) {
//     case 0:
//       pc.printf("MO message transferred successfully\r\n");
//       break;
//     default:
//       pc.printf("Outgoing status code: %i\r\n", outgoingStatus);
//   }
//   return outgoingStatus;
// }

// // Status: Needs testing
// NetworkRegistration NAL9602::joinNetwork() {
//   char s[80];
//   NetworkRegistration reg;
//   at.send("AT+PSREG");
//   at.recv("AT+PSREG");
//   at.recv("%79s\r\n", s);
//   if (strcmp(s,"No")==0) { // No GPS fix
//     reg.status = -1;
//     reg.err = -1;
//   } else {
//     at.recv(" +SBDREG:%i,%i\r\n", &reg.status, &reg.err);
//   }
//   at.recv("OK");
//   return reg;
// }