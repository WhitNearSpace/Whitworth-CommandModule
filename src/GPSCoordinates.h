#ifndef GPSCoordinates_H
#define GPSCoordinates_H

#include "mbed.h"

/** GPS Coordinates object
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.1
 * @date 2017
 * @copyright GNU Public License
 */

class GPSCoordinates {

public:
  bool positionFix; // Is the position valid?
  int satUsed;      // How many satellites were used?

  /** Create a GPSCoordinates object
  */
  GPSCoordinates();

  ~GPSCoordinates();

  void clearCoordinates();

  bool validCoordinates();

  void setLatitudeDegMin(int deg, int min, int mindec, bool north);

  void setLongitudeDegMin(int deg, int min, int mindec, bool east);

  void setAltitude(float alt);

  float getLatitudeDecDeg();

  float getLongitudeDecDeg();

  float getAltitude();
private:
  int latDeg;       // latitude degrees (integer portion)
  int latMin;       // latitude minutes (integer portion)
  int latMinDec;    // decimal portion of latitude minutes
  int lonDeg;       // longitude degrees (integer portion)
  int lonMin;       // longitude minutes (integer portion)
  int lonMinDec;    // decimal portion of longitude minutes
  float altitude;    // Altitude (in meters)
  bool isNorth;
  bool isEast;
  bool latSet;
  bool longSet;
  bool altSet;
};

#endif
