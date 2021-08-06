#ifndef GPSCoordinates_H
#define GPSCoordinates_H

#include "mbed.h"

/** GPS Coordinates object
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.1
 * @date 2017
 * @copyright MIT License
 */

class GPSCoordinates {

public:
  bool positionFix; // Is the position valid?
  int satUsed;      // How many satellites were used?
  time_t syncTime; // Time of last GPS update (in seconds since 1/1/1970)

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

  int getRawLatitude();

  int getRawLongitude();

  void setGroundSpeed(float v);

  void setVerticalVelocity(float v);

  void setHeading(int h);

  int getRawGroundSpeed();

  int getRawVerticalVelocity();

  int getHeading();

private:
  int latDeg;       // latitude degrees (integer portion)
  int latMin;       // latitude minutes (integer portion)
  int latMinDec;    // decimal portion of latitude minutes
  int lonDeg;       // longitude degrees (integer portion)
  int lonMin;       // longitude minutes (integer portion)
  int lonMinDec;    // decimal portion of longitude minutes
  float altitude;    // Altitude (in meters)
  bool latSet;
  bool longSet;
  bool altSet;
  bool isNorth;
  bool isEast;
  int grndSpeed;  // ground speed in units of tenths of km/h
  int vertVel; // vertical velocity in units of tenths of m/s
  int heading; // degrees from true north

};

#endif
