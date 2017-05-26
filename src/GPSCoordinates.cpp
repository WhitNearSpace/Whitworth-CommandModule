#include "GPSCoordinates.h"

GPSCoordinates::GPSCoordinates() {
  clearCoordinates();
}

void GPSCoordinates::clearCoordinates() {
  positionFix = false;
  latSet = false;
  longSet = false;
  altSet = false;
}

bool GPSCoordinates::validCoordinates() {
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
  if ((deg>=0)&&(deg<=90)) {
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

float GPSCoordinates::getLatitudeDecDeg() {
  float degrees = 0;
  if (validCoordinates()) {
    degrees = (float)latDeg;
    degrees += (float)latMin/60.0;
    degrees += (float)latMinDec/1000000.0;
    if (isNorth)
      return degrees;
    else
      return -degrees;
  }
  else return -999; // indicates error
}

float GPSCoordinates::getLongitudeDecDeg() {
  float degrees = 0;
  if (validCoordinates()) {
    degrees = (float)lonDeg;
    degrees += (float)lonMin/60.0;
    degrees += (float)lonMinDec/1000000.0;
    if (isEast)
      return degrees;
    else
      return -degrees;
  }
  else return -999; // indicates error
}

float GPSCoordinates::getAltitude() {
  if (validCoordinates())
    return altitude;
  else
    return -999;  // indicates error
}
