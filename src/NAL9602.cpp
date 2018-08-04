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
NAL9602::NAL9602(PinName tx_pin, PinName rx_pin, PinName ri_pin) : modem(tx_pin, rx_pin), RI(ri_pin) {
  // Set initial state of flags
  ringAlert = false;
  messageAvailable = false;
  validTime = false;
  verboseLogging = false;
  gpsStatus = false;
  iridiumStatus = false;
  startLogLength = 0;

  // Set modem baud rate
  modem.baud(19200);
}

// Status: Incomplete
NAL9602::~NAL9602(void) {
  // Detach 9602 from Iridium network
  /* TBC */
}

// Status: Tested with terminal
void NAL9602::satLinkOn(void) {
  modem.printf("AT*S1\r");
  scanToEnd();
  iridiumStatus = true;
}

// Status: Tested with terminal
void NAL9602::satLinkOff(void) {
  modem.printf("AT*S0\r");
  scanToEnd();
  iridiumStatus = false;
}

// Status: Lab tested with 9602-A
void NAL9602::gpsOn(void) {
  modem.printf("AT+PP=1\r");
  scanToEnd();
  gpsStatus = true;
}

// Status: Lab tested with 9602-A
void NAL9602::gpsOff(void) {
  modem.printf("AT+PP=0\r");
  scanToEnd();
  coord.clearCoordinates();
  gpsStatus = false;
}

// Status: Lab tested with 9602-A
int NAL9602::checkRingAlert(void) {
  int sri;
  int argFilled;

  modem.printf("AT+CRIS\r");
  // Expected response has the form +CRIS:<tri>,<sri>
  // tri is not defined for 9602
  modem.scanf(" AT+CRIS");
  argFilled = modem.scanf(" +CRIS: %*d,%d", &sri);
  scanToEnd(true);
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

  modem.printf("AT+CSQF\r");
  // Expected response has form: +CSQF:<rssi>
  modem.scanf(" AT+CSQF");
  argFilled = modem.scanf(" +CSQF:%d", &bars);
  scanToEnd(true);
  if (argFilled == 1) {
    if ((bars>=0) && (bars<=5))
      return bars;
    else
      return -2; // error: bars has invalid value
  } else
    return -1; // error: no value for bars found
}

// Status: Lab tested with 9602-A
bool NAL9602::gpsUpdate() {
  int argFilled;
  bool valid = true;
  int deg, min, decmin;
  char dir[10];
  float alt;
  char fixString[80];
  char invalidString[] = "Invalid";
  int num;
  time_t receivedTime;

  modem.printf("AT+PLOC\r");
  /* Expected response has the form:
   * +PLOC:
   * Latitude:<||>:<mm>.<nnnnn> <N/S>
   * Longitude: <ooo>:<pp>.<qqqqq> <E/W>
   * Altitude: <#> meters
   * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
   * Satellites Used=<zz>
  */
  modem.scanf(" AT+PLOC");
  modem.scanf(" +PLOC:");
  receivedTime = time(NULL);
  argFilled = modem.scanf(" Latitude=%d:%d.%d %s",&deg,&min,&decmin,&dir);
  if (argFilled == 4) {
   coord.setLatitudeDegMin(deg, min, decmin, strcmp(dir,"North")==0);
  } else valid = false;
  if (verboseLogging)
    pc.printf("Latitude = %d:%d.%d %s\r\n", deg, min, decmin, dir);

  argFilled = modem.scanf(" Longitude=%d:%d.%d %s",&deg,&min,&decmin,&dir);
  if (argFilled == 4) {
   coord.setLongitudeDegMin(deg, min, decmin, strcmp(dir,"East")==0);
  } else valid = false;
  if (verboseLogging)
    pc.printf("Longitude = %d:%d.%d %s\r\n", deg, min, decmin, dir);

  argFilled = modem.scanf(" Altitude=%f meters",&alt);
  if (argFilled == 1)
    coord.setAltitude(alt);
  else valid = false;
  if (verboseLogging)
    pc.printf("Altitude = %.2f\r\n", alt);

  modem.scanf("%79s", &fixString);
  if (strcmp(invalidString,fixString)==0)
    valid = false;
  if (verboseLogging)
    pc.printf("%s\r\n", fixString);
  while (strcmp(fixString,"Satellites")!=0) {
    modem.scanf("%79s", &fixString);
  }

  argFilled = modem.scanf(" Used=%d", &num);
  coord.satUsed = num;
  if (verboseLogging)
    pc.printf("Satellites = %d\r\n", num);
  if (valid) {
    coord.syncTime = receivedTime;
    coord.positionFix = valid;
  }
  scanToEnd();

  modem.printf("AT+PVEL\r");
  /* Expected response has the form:
  * +PVEL:
  * Ground Velocity=<#g> km/h, <#h> degrees from true North
  * Vertical Velocity=<#v> m/s
  * <Position Fix>= Invalid Position Fix, Valid Positon Fix, or Dead Reckoning
  * Satellites Used=<zz>
  */
  float v;
  float h;
  modem.scanf(" AT+PVEL");
  modem.scanf(" +PVEL:");
  argFilled = modem.scanf(" Ground Velocity=%f km/h, %f degrees from true North", &v, &h);
  if (argFilled == 2) {
    coord.setGroundSpeed(v);
    coord.setHeading((int)h);
  };
  if (verboseLogging)
    pc.printf("Ground velocity = %f km/h, %.2f deg from north\r\n", v, h);
  argFilled = modem.scanf(" Vertical Velocity=%f m/s", &v);
  if (argFilled == 1) {
    coord.setVerticalVelocity(v);
  }
  if (verboseLogging)
    pc.printf("Vertical velocity = %.2f m/s\r\n", v);
  scanToEnd();

  return valid;
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
int NAL9602::getSatsUsed() {
  return coord.satUsed;
};


// Status: Lab tested with 9602-A
void NAL9602::setModeGPS(gpsModes mode) {
  modem.printf("AT+PNAV=%d\r",mode);
  scanToEnd();
}

// Status: Lab tested with 9602-A
void NAL9602::zeroMessageCounter() {
  modem.printf("AT+SBDC\r");
  scanToEnd();
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
    modem.printf("AT+PSIXA\r");
    modem.scanf(" AT+PSIXA");
  } else {
    modem.printf("AT+PSIX\r");
    modem.scanf(" AT+PSIX");
  }
  modem.scanf(" +SBDIX:%d,%d,%d,%d,%d,%d", &outgoingStatus,
    &outgoingMessageCount, &incomingStatus, &incomingMessageCount,
    &incomingLength, &queueLength);
  if (incomingStatus == 1) {
    messageAvailable = true;
    incomingMessageLength = incomingLength;
  }
  if (queueLength == 0)
    ringAlert = false;
  scanToEnd();
  return outgoingStatus;
}

// Status: Lab tested with 9602-A
bool NAL9602::syncTime() {
  struct tm t;
  validTime = false;
  unsigned int month, day, year;


  // Get UTC date from GPS
  modem.printf("AT+PD\r");
  modem.scanf(" AT+PD");
  modem.scanf(" +PD:");
  modem.scanf(" UTC Date=%u-%u-%u", &month, &day, &year);
  t.tm_mon = month - 1;  // January = 0, not 1
  t.tm_mday = day;
  t.tm_year = year - 1900; // Years since 1900 is required
  scanToEnd();
  if (verboseLogging)
    pc.printf("UTC Date: %d-%d-%d\t", t.tm_mon+1, t.tm_mday, t.tm_year+1900);

  // Get UTC time from GPS
  modem.printf("AT+PT\r");
  modem.scanf(" AT+PT");
  modem.scanf(" +PT:");
  modem.scanf(" UTC Time=%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
  scanToEnd();
  if (verboseLogging)
    pc.printf("UTC Time: %.2d:%.2d:%.2d\r\n\r\n", t.tm_hour, t.tm_min, t.tm_sec);

  // Set RTC and associated flag
  set_time(mktime(&t));
  time_t seconds = time(NULL);
  if (seconds > 1496150000)
    validTime = true;
  return validTime;
}

// Status: Lab tested with 9602-A
NetworkRegistration NAL9602::joinNetwork() {
  char s[80];
  NetworkRegistration reg;
  modem.printf("AT+PSREG\r");
  modem.scanf(" AT+PSREG");
  modem.scanf("%79s", &s);
  if (strcmp(s,"No")==0) { // No GPS fix
    reg.status = -1;
    reg.err = -1;
  } else {
    modem.scanf(" +SBDREG:%i,%i", &reg.status, &reg.err);
  }
  if (verboseLogging)
  scanToEnd();
  return reg;
}

// Status: Ready for testing
void NAL9602::clearBuffer(int selectedBuffer) {
  modem.printf("AT+SBDD%d\r", selectedBuffer);
  scanToEnd();
}

// Status:  Lab tested with 9602-A
BufferStatus NAL9602::getBufferStatus() {
  BufferStatus bs;
  modem.printf("AT+SBDS\r");
  modem.scanf(" AT+SBDS");
  modem.scanf(" +SBDS: %d,%d,%d,%d", &bs.outgoingFlag, &bs.outgoingMsgNum, &bs.incomingFlag, &bs.incomingMsgNum);
  scanToEnd();
  return bs;
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
int NAL9602::setMessage(float mission_id, int flightMode, float voltage, float intTemp, float extTemp) {
  char status[80];
  int err;
  sbdMessage.setMissionID(mission_id, flightMode);
  sbdMessage.generateGPSBytes(coord);
  sbdMessage.generateCommandModuleBytes(voltage, intTemp, extTemp);
  sbdMessage.generatePodBytes();
  int n = sbdMessage.msgLength;
  if (n > 340) {
    return -1;
  } else {
    modem.printf("AT+SBDWB=%d\r", n);
    modem.scanf("%79s", &status);
    while ((strcmp(status,"READY")!=0)) {
      modem.scanf("%79s", &status);
    }
    for (int i = 0; i < n; i++) {
      modem.printf("%c", sbdMessage.getByte(i));
    }
    unsigned short checksum = sbdMessage.generateChecksum();
    modem.printf("%c%c", (char)(checksum/256), (char)(checksum%256));
    modem.scanf(" %d",&err);
    scanToEnd();
    return err;
  }
}

// Status: Lab tested with 9602-A
void NAL9602::echoModem(int listenTime) {
  Timer t;
  char buff[BUFFLENGTH];
  unsigned int i = 0;
  unsigned int j = 0;
  t.start();
  while (t<listenTime) {  // Be patient and listen for listenTime (in seconds)
    while ((modem.readable()) && (i<BUFFLENGTH)) {
      buff[i] = modem.getc();
      i++;
    }
    while ((j<i) && (!modem.readable())) {
      if (verboseLogging)
        pc.putc(buff[j]);
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
      modemStartLog[startLogLength] = modem.getc();
      startLogLength++;
    }
  }
}

void NAL9602::echoStartLog() {
  for (unsigned int i = 0; i < startLogLength; i++) {
    pc.putc(modemStartLog[i]);
  }
}

// Status: Lab tested with 9602-A
void NAL9602::scanToEnd(bool verbose) {
  char status[80];
  modem.scanf("%79s", &status);
  while (((strcmp(status,"ERROR")!=0))&&(strcmp(status,"OK")!=0)) {
    if (verbose)
      pc.printf("\t%s\r\n", status);
    modem.scanf("%79s", &status);
  }
  if (verbose)
    pc.printf("\tStatus = %s\r\n", status);
}
