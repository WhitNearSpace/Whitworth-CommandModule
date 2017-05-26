#ifndef NAL9602_H
#define NAL9602_H

#include "mbed.h"

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

  /** Create a NAL9602 interface object connected to the specified pins
  *
  * @param tx_pin Serial TX pin
  * @param rx_pin Serial RX pin
  */
  NAL9602(PinName tx_pin, PinName rx_pin);

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

private:




};

 #endif
