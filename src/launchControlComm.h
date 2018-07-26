#ifndef LAUNCH_CONTROL_COMM_H
#define LAUNCH_CONTROL_COMM_H

#include "mbed.h"
#include "NAL9602.h"
#include "TMP36.h"

/** Listener functions for Launch Control app communication
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2017
 *  @copyright GNU Public License
 */

extern TMP36 intTempSensor;
extern TMP36 extTempSensor;
extern AnalogIn batterySensor;

// Status LEDs
extern DigitalOut powerStatus;
extern DigitalOut gpsStatus;
extern DigitalOut satStatus;
extern DigitalOut podStatus;
extern DigitalOut futureStatus;

enum launchControlCommands
{
  none = 0,
  gps,
  satlink,
  podlink,
  radio,
  sleepheight,
  gpsdata,
  cmdsensors,
  poddata,
  missionid,
  podlengths
};

int parseLaunchControlInput(Serial &s, NAL9602 &sat);

int sendGPStoLaunchControl(Serial &s, NAL9602 &sat);

int sendCmdSensorsToLaunchControl(Serial &s, NAL9602 &sat);

#endif
