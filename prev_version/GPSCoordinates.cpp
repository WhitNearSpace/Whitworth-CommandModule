#include "GPSCoordinates.h"

GPSCoordinates::GPSCoordinates(void) {
  clearCoordinates();
}

GPSCoordinates::~GPSCoordinates() {

}

void GPSCoordinates::clearCoordinates(void) {
  positionFix = false;
  latSet = false;
  longSet = false;
  altSet = false;
  syncTime = 0;
}

bool GPSCoordinates::validCoordinates(void) {
  return (positionFix && latSet && longSet && altSet);
}

void GPSCoordinates::setLatitudeDegMin(int deg, int min, int mindec, bool north) {
  bool valid = true;
  isNorth = north;
  if ((deg>=0)&&(deg<=90)) {
    latDeg = deg;
  } else valid = false;

  if ((min>=0)&&(min<=59))
    latMin = min;
  else valid = false;

  if ((mindec>=0)&&(mindec<=99999))
    latMinDec = mindec;
  else valid = false;

  latSet = valid;
}

void GPSCoordinates::setLongitudeDegMin(int deg, int min, int mindec, bool east) {
  bool valid = true;
  isEast = east;
  if ((deg>=0)&&(deg<=179)) {
    lonDeg = deg;
  } else valid = false;

  if ((min>=0)&&(min<=59))
    lonMin = min;
  else valid = false;

  if ((mindec>=0)&&(mindec<=99999))
    lonMinDec = mindec;
  else valid = false;

  longSet = valid;
}

void GPSCoordinates::setAltitude(float alt) {
  if ((alt>-415) && (alt<200000)) {
    altSet = true;
    altitude = alt;
  } else altSet = false;
}

float GPSCoordinates::getLatitudeDecDeg(void) {
  float degrees = 0;
  if (validCoordinates()) {
    degrees = (float)latDeg;
    degrees += (float)latMin/60.0;
    degrees += (float)latMinDec/100000.0/60.0;
    if (isNorth)
      return degrees;
    else
      return -degrees;
  }
  else return -999; // indicates error
}

int GPSCoordinates::getRawLatitude(void) {
  int angle; // angle in units of 10,000th of a minute
  if (validCoordinates()) {
    angle = latMinDec;
    angle += latMin*100000;
    angle += latDeg*100000*60;
    if (isNorth)
      return angle;
    else
      return -angle;
  } else return 0;
}

float GPSCoordinates::getLongitudeDecDeg(void) {
  float degrees = 0;
  if (validCoordinates()) {
    degrees = (float)lonDeg;
    degrees += (float)lonMin/60.0;
    degrees += (float)lonMinDec/100000.0/60.0;
    if (isEast)
      return degrees;
    else
      return -degrees;
  }
  else return -999; // indicates error
}

int GPSCoordinates::getRawLongitude(void) {
  int angle; // angle in units of 10,000th of a minute
  if (validCoordinates()) {
    angle = lonMinDec;
    angle += lonMin*100000;
    angle += lonDeg*100000*60;
    if (isEast)
      return angle;
    else
      return -angle;
  } else return 0;
}

float GPSCoordinates::getAltitude(void) {
  if (validCoordinates())
    return altitude;
  else
    return -999;  // indicates error
}

void GPSCoordinates::setGroundSpeed(float v) {
  if (v>=0)
    grndSpeed = (int)(v*10);
}

int GPSCoordinates::getRawGroundSpeed(void) {
  return grndSpeed;
}

void GPSCoordinates::setVerticalVelocity(float v) {
  vertVel = (int)(v*10);
}

int GPSCoordinates::getRawVerticalVelocity(void) {
  return vertVel;
}

void GPSCoordinates::setHeading(int h) {
  if ((h>=0)&&(h<=360))
    heading = h;
  else
    heading = 0;
}

int GPSCoordinates::getHeading(void) {
  return heading;
}
