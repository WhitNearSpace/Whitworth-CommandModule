#include "NAL9602.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Lab tested with 9602-A
 *  - Flight tested
 */

// Status: Incomplete
NAL9602::NAL9602(PinName tx_pin, PinName rx_pin, PinName ri_pin) : modem(tx_pin, rx_pin, 19200), at(&modem,"\r"), RI(ri_pin) {
  // Set initial state of flags
  ringAlert = false;
  messageAvailable = false;
  validTime = false;
  verboseLogging = false;
  gpsStatus = false;
  iridiumStatus = false;
  startLogLength = 0;

  at.oob("Invalid Position Fix", callback(this, &NAL9602::_oob_invalid_fix));
}

// Status: Incomplete
NAL9602::~NAL9602(void) {
  // Detach 9602 from Iridium network
  /* TBC */
}

// Status: Tested with terminal
void NAL9602::satLinkOn(void) {
  at.send("AT*S1");
  if (!at.recv("OK")) printf("satLinkOn failure\r\n");
  iridiumStatus = true;
}

// Status: Tested with terminal
void NAL9602::satLinkOff(void) {
  at.send("AT*S0") && at.recv("OK");
  iridiumStatus = false;
}

// Status: Lab tested with 9602-A
void NAL9602::gpsOn(void) {
  at.send("AT+PP=1") && at.recv("OK");
  gpsStatus = true;
}

// Status: Lab tested with 9602-A
void NAL9602::gpsOff(void) {
  at.send("AT+PP=0") && at.recv("OK");
  coord.clearCoordinates();
  gpsStatus = false;
}

// Status: Lab tested with 9602-A
int NAL9602::checkRingAlert(void) {
  int sri;
  bool argFilled;

  at.send("AT+CRIS");
  // Expected response has the form +CRIS:<tri>,<sri>
  // tri is not defined for 9602
  at.recv("AT+CRIS");
  argFilled = at.recv("+CRIS: %*d,%d\r\n", &sri);

  if (argFilled) {
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
  bool argFilled;

  at.send("AT+CSQF");
  // Expected response has form: +CSQF:<rssi>
  at.recv("AT+CSQF");
  argFilled = at.recv("+CSQF:%d\r\n", &bars);
  if (argFilled) {
    if ((bars>=0) && (bars<=5))
      return bars;
    else
      return -2; // error: bars has invalid value
  } else
    return -1; // error: no value for bars found
}

// Status: Lab tested with 9602-A
bool NAL9602::gpsUpdate() {
  bool argFilled;
  int deg, min, decmin;
  char dir[10];
  float alt;
  char fixString[80];
  int num;
  time_t receivedTime;

  coord.positionFix = true;
  bool valid = true;

  at.send("AT+PLOC");
  /* Expected response has the form:
   * +PLOC:
   * Latitude:<||>:<mm>.<nnnnn> <N/S>
   * Longitude: <ooo>:<pp>.<qqqqq> <E/W>
   * Altitude: <#> meters
   * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
   * Satellites Used=<zz>
  */

  at.recv("AT+PLOC");
  at.recv("+PLOC:");
  receivedTime = time(NULL);
  argFilled = at.recv("Latitude=%d:%d.%d %s\r\n",&deg,&min,&decmin,dir);
  if (argFilled) {
   coord.setLatitudeDegMin(deg, min, decmin, strcmp(dir,"North")==0);
  } else valid = false;

  if (valid) {
    argFilled = at.recv("Longitude=%d:%d.%d %s\r\n",&deg,&min,&decmin,dir);
    if (argFilled) {
     coord.setLongitudeDegMin(deg, min, decmin, strcmp(dir,"East")==0);
    } else valid = false;
  }

  if (valid) {
    argFilled = at.recv("Altitude=%f meters",&alt);
    if (argFilled) {
      coord.setAltitude(alt);
    } else valid = false;
  }

  if (valid) {
    argFilled = at.recv("Satellites Used=%d\r\n", &num);
    at.recv("OK");
    if (argFilled) {
      coord.satUsed = num;
    } else valid = false;
  }

  coord.positionFix = valid && coord.positionFix && (coord.satUsed > 3);
  if (coord.positionFix) {
    coord.syncTime = receivedTime;  
  } 

  at.send("AT+PVEL");
  /* Expected response has the form:
  * +PVEL:
  * Ground Velocity=<#g> km/h, <#h> degrees from true North
  * Vertical Velocity=<#v> m/s
  * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
  * Satellites Used=<zz>
  */
  float v;
  float h;
  at.recv("AT+PVEL");
  at.recv("+PVEL:");
  argFilled = at.recv("Ground Velocity=%f km/h, %f degrees from true North", &v, &h);
  if (argFilled) {
    coord.setGroundSpeed(v);
    coord.setHeading((int)h);
    if (verboseLogging)
      printf("Ground velocity = %f km/h, %.2f deg from north\r\n", v, h);
  };

  argFilled = at.recv("Vertical Velocity=%f m/s", &v);
  if (argFilled) {
    coord.setVerticalVelocity(v);
    if (verboseLogging)
      printf("Vertical velocity = %.2f m/s\r\n", v);
  }
  at.recv("OK");
  return coord.positionFix;
}

// Status: Lab tested with 9602-A
float NAL9602::latitude() {
  return coord.getLatitudeDecDeg();
}

// Status: Lab tested with 9602-A
float NAL9602::longitude() {
  return coord.getLongitudeDecDeg();
}

// Status: Lab tested with 9602-A
float NAL9602::altitude() {
  return coord.getAltitude();
}

// Status: Ready for testing
float NAL9602::verticalVelocity() {
  return coord.getRawVerticalVelocity()*0.1;
}

// Status: Ready for testing
int NAL9602::getSatsUsed() {
  return coord.satUsed;
};


// Status: Lab tested with 9602-A
void NAL9602::setModeGPS(gpsModes mode) {
  at.send("AT+PNAV=%d",mode);
  at.recv("OK");
}

// Status: Lab tested with 9602-A
void NAL9602::zeroMessageCounter() {
  at.send("AT+SBDC");
  at.recv("OK");
}

// Status: Ready for testing
int NAL9602::transmitMessageWithRingAlert() {
  int outgoingStatus;
  int outgoingMessageCount;
  int incomingStatus;
  int incomingMessageCount;
  int incomingLength;
  int queueLength;
  // if ((RI==1)||ringAlert) {
  //   modem.printf("AT+PSIXA\r");
  //   modem.scanf(" AT+PSIXA");
  // } else {
  at.send("AT+PSIX");
  at.recv("AT+PSIX");
  // }
  at.recv("+SBDIX:%d,%d,%d,%d,%d,%d\r\n", &outgoingStatus,
    &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
    &incomingLength, &queueLength);
  if (incomingStatus == 1) {
    messageAvailable = true;
    incomingMessageLength = incomingLength;
  }
  if (queueLength == 0)
    ringAlert = false;
  at.recv("OK");
  switch(outgoingStatus) {
    case 0:
      pc.printf("MO message transferred successfully\r\n");
      break;
    default:
      pc.printf("Outgoing status code: %i\r\n", outgoingStatus);
  }
  return outgoingStatus;
}

// Status: Ready for testing
int NAL9602::transmitMessage() {
  int outgoingStatus;
  int outgoingMessageCount;
  int incomingStatus;
  int incomingMessageCount;
  int incomingLength;
  int queueLength;
  at.send("AT+SBDI");
  at.recv("AT+SBDI");
  at.recv("+SBDI:%d,%d,%d,%d,%d,%d\r\n", &outgoingStatus,
    &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
    &incomingLength, &queueLength);
  if (incomingStatus == 1) {
    messageAvailable = true;
    incomingMessageLength = incomingLength;
  }
  if (queueLength == 0)
    ringAlert = false;
  at.recv("OK");
  /* Outgoing (MO) codes
   *   0 = no SBD message to send from the 9602
   *   1 = SBD message successfully sent
   *   2 = error occurred while attempting to send SBD message
   *
   */
  if (outgoingStatus==1) {
    printf("Transmission success!\r\n\r\n");
  } else {
    printf("Transmission error\r\n");
  }
  return outgoingStatus;
}

// Status: Lab tested with 9602-A
bool NAL9602::syncTime() {
  struct tm t;
  validTime = false;
  int month, day, year;


  // Get UTC date from GPS
  at.send("AT+PD");
  at.recv("AT+PD");
  at.recv("+PD:");
  at.recv("UTC Date=%d-%d-%d\r\n", &month, &day, &year);
  t.tm_mon = month - 1;  // January = 0, not 1
  t.tm_mday = day;
  t.tm_year = year - 1900; // Years since 1900 is required
  at.recv("OK");

  // Get UTC time from GPS
  at.send("AT+PT");
  at.recv("AT+PT");
  at.recv("+PT:");
  at.recv("UTC Time=%d:%d:%d.", &t.tm_hour, &t.tm_min, &t.tm_sec);
  at.recv("OK");

  // Set RTC and associated flag
  set_time(mktime(&t));
  time_t seconds = time(NULL);
  if (seconds > 1496150000)
    validTime = true;
  return validTime;
}

// Status: Needs testing
NetworkRegistration NAL9602::joinNetwork() {
  char s[80];
  NetworkRegistration reg;
  at.send("AT+PSREG");
  at.recv("AT+PSREG");
  at.recv("%79s\r\n", s);
  if (strcmp(s,"No")==0) { // No GPS fix
    reg.status = -1;
    reg.err = -1;
  } else {
    at.recv(" +SBDREG:%i,%i\r\n", &reg.status, &reg.err);
  }
  at.recv("OK");
  return reg;
}

// Status: Ready for testing
void NAL9602::clearBuffer(int selectedBuffer) {
  at.send("AT+SBDD%d", selectedBuffer);
  at.recv("OK");
}

// Status:  Lab tested with 9602-A
BufferStatus NAL9602::getBufferStatus() {
  BufferStatus bs;
  at.send("AT+SBDSX");
  at.recv("AT+SBDSX");
  at.recv("+SBDSX: %d,%d,%d,%d,%d,%d\r\n", &bs.outgoingFlag, &bs.outgoingMsgNum,
    &bs.incomingFlag, &bs.incomingMsgNum, &bs.raFlag, &bs.numMsgWaiting);
  at.recv("OK");
  return bs;
}

// Status: Ready for testing
void NAL9602::getGpsModes() {
  at.send("AT+PNAV=?");
  echoModem(pc, 15);
}

// Status: Ready for testing
void NAL9602::dumpManufacturer() {
  at.send("AT+GMI");
  echoModem(pc, 15);
}

// Status: Ready for testing
void NAL9602::dumpModel() {
  at.send("AT+GMM");
  echoModem(pc, 15);
}

// Status: Ready for testing
void NAL9602::dumpRevision() {
  at.send("AT+GMR");
  echoModem(pc, 15);
}

// Status: Ready for testing
void NAL9602::dumpIMEI() {
  at.send("AT+GSN");
  echoModem(pc, 15);
}

void NAL9602::gpsNoSleep() {
  at.send("AT^GAO1");
  at.recv("AT^GAO1");
  at.recv("OK");
}

// Status: Lab tested with 9602-A
void NAL9602::addMessageGPS() {
  sbdMessage.generateGPSBytes(coord);
}

// Status: Lab tested with 9602-A
void NAL9602::loadPodData(int podID, char* data) {
  sbdMessage.loadPodBuffer(podID, data);
}

// Status: Lab tested with 9602-A
int NAL9602::setMessage(float voltage, float intTemp, float extTemp) {
  char status[80];
  int err;
  pc.printf("Inside setMessage\r\n");
  pc.printf("Setting mission ID with %i and %i\r\n", sbdMessage.missionID, flight.mode);
  sbdMessage.setMissionID(flight.mode);
  pc.printf("Generating GPS bytes\r\n");
  sbdMessage.generateGPSBytes(coord);
  pc.printf("Generating command module bytes\r\n");
  sbdMessage.generateCommandModuleBytes(voltage, intTemp, extTemp);
  pc.printf("Generating pod bytes\r\n");
  sbdMessage.generatePodBytes();
  sbdMessage.updateMsgLength();
  int n = sbdMessage.msgLength;
  pc.printf("SBD message length is %i\r\n", n);
  if (n > 340) {
    return -1;
  } else {
    at.send("AT+SBDWB=%d", n);
    at.recv("READY");
    // modem.printf("AT+SBDWB=%d\r", n);
    // modem.scanf("%79s", status);
    // while ((strcmp(status,"READY")!=0)) {
    //   modem.scanf("%79s", status);
    // }
    for (int i = 0; i < n; i++) {
      at.putc(sbdMessage.getByte(i));
      // modem.printf("%c", sbdMessage.getByte(i));
    }
    unsigned short checksum = sbdMessage.generateChecksum();
    at.putc((char)(checksum/256));
    at.putc((char)(checksum%256));
    // modem.printf("%c%c", (char)(checksum/256), (char)(checksum%256));
    at.recv("%d\r\n", &err);
    at.recv("OK");
    // modem.scanf(" %d",&err);
    // scanToEnd();
    switch(err) {
      case 0:
        pc.printf("SBD message successfully written to 9602\r\n");
        break;
      case 1:
        pc.printf("SBD message write timeout\r\n");
        break;
      case 2:
        pc.printf("SBD message checksum does not match\r\n");
        break;
      case 3:
        pc.printf("SBD message size is not correct\r\n");
        break;
      default:
        pc.printf("Unknown error received following SBDWB command\r\n");
    }
    return err;
  }
}

// Status: Lab tested with 9602-A
void NAL9602::echoModem(Serial &s, int listenTime) {
  Timer t;
  char buff[BUFFLENGTH];
  unsigned int i = 0;
  unsigned int j = 0;
  t.start();
  while (t<listenTime) {  // Be patient and listen for listenTime (in seconds)
    while ((modem.readable()) && (i<BUFFLENGTH)) {
      buff[i] = at.getc();
      // buff[i] = modem.getc();
      i++;
    }
    while ((j<i) && (!modem.readable())) {
      s.putc(buff[j]);
      j++;
    }
    for (unsigned int k = 0; k<j; k++) {
      buff[k] = buff[j+k];
      i = i - j;
      j = 0;
    }
  }
}

void NAL9602::saveStartLog(int listenTime) {
  Timer t;
  startLogLength = 0;
  t.start();
  while (t<listenTime) {
    while ((modem.readable()) && (startLogLength < LOG_BUFF_LENGTH)) {
      modemStartLog[startLogLength] = at.getc();
      startLogLength++;
    }
  }
}

void NAL9602::echoStartLog(Serial &s) {
  for (unsigned int i = 0; i < startLogLength; i++) {
    s.putc(modemStartLog[i]);
  }
  if (startLogLength == LOG_BUFF_LENGTH)
    s.printf("\r\nStart output exceeded %d character buffer\r\n", LOG_BUFF_LENGTH);
}

void NAL9602::_oob_invalid_fix() {
  coord.positionFix = false;
}