#ifndef NAL9602_H
#define NAL9602_H

#include "mbed.h"
#include "GPSCoordinates.h"

/** Operating modes for GPS receiver
*/
enum gpsModes
{
  stationary = 1,
  walking,
  land_vehicle,
  at_sea,
  airborne_low_dynamic,
  airborne_medium_dynamic,
  airborne_high_dynamic
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
  bool ringAlert;
  bool messageAvailable;
  bool validTime;

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
  * 1 = stationary
  * 2 = walking
  * 3 = land vehicle
  * 4 = at sea
  * 5 = airborne, low dynamics (<1g)
  * 6 = airborne, medium dynamics (<2g)
  * 7 = airborne, high dynamics (<4g)
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

private:
  GPSCoordinates coord;
  int incomingMessageLength;

};

 #endif
