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
  NAL9602(PinName tx, PinName rx);

  /** Turn on/off Iridium transceiver
  */
  void satLinkOn(bool turnOn);

  /** Check for incoming message
  *
  * Returns true if there is an incoming SBD message available
  */
  bool checkRingAlert();

  /** Report last known signal quality
  *
  * Returns number of signal "bars" (0 to 5)
  */
  char signalQuality();

  /** Get GPS data
  *
  * Updates latitude, longitude, altitude, and # of satellites
  *
  * Returns false if invalid position fix
  */
  bool gpsUpdate();

  /** Return decimal latitude
  */
  float latitude();

  /** Return decimal longitude
  *
  */
  float longitude();

  /** Return altitude
  *
  * Returns altitude in meters. Requires previous gpsUpdate().
  */
  float altitude();



 }
