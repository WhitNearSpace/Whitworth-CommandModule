#ifndef NAL9602_H
#define NAL9602_H

#include "mbed.h"
#include "GPSCoordinates.h"
#include "SBDmessage.h"

#define BUFFLENGTH 800

extern Serial pc;

/** Operating modes for GPS receiver
*/
enum gpsModes
{
  stationary = 2,
  pedestrian,
  automotive,
  sea,
  airborne_low_dynamic,
  airborne_medium_dynamic,
  airborne_high_dynamic
};

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
};

/** Interface to NAL Research 9602-LP/A/AB satellite modems
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2017
 *  @copyright GNU Public License
 */
class NAL9602 {

public:
  Serial modem;
  InterruptIn RI;
  SBDmessage sbdMessage;
  bool ringAlert;
  bool messageAvailable;
  bool validTime;
  bool verboseLogging;

  /** Create a NAL9602 interface object connected to the specified pins
  *
  * @param tx_pin Serial TX pin
  * @param rx_pin Serial RX pin
  */
  NAL9602(PinName tx_pin, PinName rx_pin, PinName ri_pin = NC);

  ~NAL9602();

  /** Turn on Iridium transceiver
  */
  void satLinkOn();

  /** Turn off Iridium transceiver
  */
  void satLinkOff();

  /** Turn on GPS receiver
  */
  void gpsOn();

  /** Turn off GPS receiver
  */
  void gpsOff();

  /** Check for incoming message
  *
  * @returns 1 if there is an incoming SBD message available or 0 if none
  */
  int checkRingAlert();

  /** Report last known signal quality
  *
  * @returns number of signal "bars" (0 to 5)
  */
  int signalQuality();

  /** Get GPS data
  *
  * Updates latitude, longitude, altitude, and # of satellites
  *
  * @returns false if invalid position fix
  */
  bool gpsUpdate();

  /** Return decimal latitude
  *
  * Converts latitude reported in last gpsUpdate from degree and decimal minute
  * form to decimal degree form.  North is positive.
  *
  * @returns decimal latitude
  */
  float latitude();

  /** Return decimal longitude
  *
  * Converts longitude reported in last gpsUpdate from degree and decimal minute
  * form to decimal degree form. East is positive.
  *
  * @returns decimal longitude
  */
  float longitude();

  /** Return altitude
  *
  * Requires previous gpsUpdate()
  *
  * @returns altitude in meters
  */
  float altitude();

  /** Set GPS mode
  *
  * @param mode gives operating environment for GPS
  * 2 = stationary
  * 3 = pedestrian
  * 4 = automotive
  * 5 = sea
  * 6 = airborne, low dynamics (<1g)
  * 7 = airborne, medium dynamics (<2g)
  * 8 = airborne, high dynamics (<4g)
  */
  void setModeGPS(gpsModes mode);

  /** Reset outgoing message counter
  *
  * Sets the MOMSN (mobile originated message sequence number)
  * to zero.
  */
  void zeroMessageCounter();

  /** Send SBD message from 9602 to ground station
  * Also checks for incoming messages and loads one if available
  */
  int transmitMessage();

  /** Set the microcontroller real-time clock from GPS time
  *
  * @returns true if successful sync
  */
  bool syncTime();

  /** Listen to 9602-LP
  * Forward output of 9602 char-by-char to pc
  */
  void echoModem(int listenTime = 3);

  /** Reads 9602 response until ERROR or OK found
  * @param verbose - if true, print to pc
  */
  void scanToEnd(bool verbose = false);

  /** Connect to Iridium network
  */
  NetworkRegistration joinNetwork();

  /** Clear incoming and/or outgoing message buffer
  * @param selectedBuffer - 0 = outgoing, 1 = incoming, 2 = both
  */
  void clearBuffer(int selectedBuffer);

  /** Get status of outgoing and incoming buffers
  */
  BufferStatus getBufferStatus();

private:
  GPSCoordinates coord;
  int incomingMessageLength;

};

 #endif
