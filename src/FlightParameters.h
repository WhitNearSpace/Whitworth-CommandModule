#ifndef FLIGHT_PARAMETERS_H
#define FLIGHT_PARAMETERS_H

/** FlightParameters structure
 *  int mode - has four different settings
 *    0: lab mode - do not transmit SBD
 *    1: launch pad mode - transmit SBD at 5 minute intervals, wait for launch
 *    2: flight mode - transmit SBD at specified interval (15-75 s), wait for land
 *    3: landed mode - transmit SBD at 10 minute intervals
 *
 *  int transPeriod - seconds between SBD transmissions in active flight
 *
 *  float triggerHeight - trigger change to active flight (mode 2) when at this
 *                        height in meters above groundAltitude
 *
 *  float groundAltitude - altitude detected when change from mode 0 to mode 1
 */

#define PRE_TRANS_PERIOD 300
#define POST_TRANS_PERIOD 600

struct FlightParameters {
  int mode;
  int transPeriod;
  float triggerHeight;
  float groundAltitude;
};

#endif
