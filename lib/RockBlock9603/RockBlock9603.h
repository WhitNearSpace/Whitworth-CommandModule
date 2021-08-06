/** Interface to RockBLOCK 9603 Iridium satellite modem
 *  
 * Update for Mbed OS 6 (written in 2021)
 * Based on Mbed OS 5 class for NAL Research 9602-LP/A/AB satellite modems (written in 2017)
 *   Note: NAL Research 9602-A includes a GPS receiver but the RockBLOCK 9603 does not
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 1.0
 *  @date 2021
 *  @copyright MIT License
 */

#ifndef ROCKBLOCK9603_H
#define ROCKBLOCK9603_H

#include "mbed.h"
// #include "SBDmessage.h"
// #include "FlightParameters.h"

using namespace std::chrono;

// FlightParameter structure
// extern FlightParameters flight;  // needs to be global because used in ISR

#define BUFFLENGTH 400
#define LOG_BUFF_LENGTH 400

#define AT_TIMEOUT_NORMAL 5000
#define AT_TIMEOUT_LONG   30000

struct NetworkRegistration
{
  int status;
  int err;
};

struct BufferStatus
{
  int outgoingFlag;
  int outgoingMsgNum;
  int incomingFlag;
  int incomingMsgNum;
  int raFlag;
  int numMsgWaiting;
};


class RockBlock9603 {

public:
  // SBDmessage sbdMessage;
  bool ringAlert;
  bool messageAvailable;
  bool validTime;
  bool verboseLogging;
  bool iridiumStatus;

  /** Create a RockBlock9603 interface object connected to the specified pins
  *
  * @param tx_pin Serial TX pin
  * @param rx_pin Serial RX pin
  */
  RockBlock9603(PinName tx_pin, PinName rx_pin, PinName ri_pin = NC);

  ~RockBlock9603();

  /** Listen to 9603
  * Forward output of 9603 char-by-char to console
  */
  void echoModem(std::chrono::duration listenTime = 3s);

  // /** Check for incoming message
  // *
  // * @returns 1 if there is an incoming SBD message available or 0 if none
  // */
  // int checkRingAlert();

  // /** Report last known signal quality
  // *
  // * @returns number of signal "bars" (0 to 5)
  // */
  // int signalQuality();


 

  // void dumpManufacturer();
  // void dumpModel();
  // void dumpRevision();
  // void dumpIMEI();

  // /** Reset outgoing message counter
  // *
  // * Sets the MOMSN (mobile originated message sequence number)
  // * to zero.
  // */
  // void zeroMessageCounter();




  // /** Listen to 9602-LP
  // * Save output of 9602 to buffer
  // */
  // void saveStartLog(int listenTime = 15);

  // /** Print saved log to Serial connection
  // */
  // void echoStartLog(Serial &s);

  // /** Reads 9602 response until ERROR or OK found
  // * @param verbose - if true, print to "console"
  // */
  // // void scanToEnd(bool verbose = false);

  // /** Connect to Iridium network
  // */
  // NetworkRegistration joinNetwork();

  // /** Clear incoming and/or outgoing message buffer
  // * @param selectedBuffer - 0 = outgoing, 1 = incoming, 2 = both
  // */
  // void clearBuffer(int selectedBuffer);

  // /** Get status of outgoing and incoming buffers
  // */
  // BufferStatus getBufferStatus();

  // /** Add GPS bytes to SBD message
  // */
  // void addMessageGPS();

  // /** Store pod data bytes for inclusion into SBD message
  // *
  // * @param int podID
  // * @param char data[] - array of raw bytes
  // */
  // void loadPodData(int podID, char* data);

  // /** Load outgoing message buffer
  // */
  // int setMessage(float voltage, float intTemp, float extTemp);

  // /** Send SBD message from 9602 to ground station
  // * Also checks for incoming messages and loads one if available
  // */
  // int transmitMessage();

  // int transmitMessageWithRingAlert();

private:
  BufferedSerial* _serial;
  ATCmdParser* _at;
  InterruptIn* _RI;
  // GPSCoordinates coord;
  int incomingMessageLength;
  char modemStartLog[LOG_BUFF_LENGTH];
  unsigned int startLogLength;

  void _oob_invalid_fix();
};

 #endif
