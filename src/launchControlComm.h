#ifndef LAUNCH_CONTROL_COMM_H
#define LAUNCH_CONTROL_COMM_H

#include "mbed.h"
#include "projectGlobals.h"
#include "NAL9602.h"
#include "TMP36.h"

/** Listener functions for Launch Control app communication
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2017
 *  @copyright MIT License
 */

extern int flightTransPeriod;
extern int flightMode;
extern int missionID;

extern Timeout cmdSequence;
extern Timer timeSinceTrans;
extern Timer pauseTime;

extern gpsModes currentGPSmode;
extern float groundAltitude;
extern float triggerHeight;

enum launchControlCommands
{
  none = 0,
  gps,
  satlink,
  podlink,
  radio,
  triggerheight,
  gpsdata,
  cmdsensors,
  poddata,
  missionid,
  podlengths
};

int parseLaunchControlInput(Serial &s, NAL9602 &sat);

int sendGPStoLaunchControl(Serial &s, NAL9602 &sat);

int sendCmdSensorsToLaunchControl(Serial &s, NAL9602 &sat);

float getBatteryVoltage();

void updateStatusLED();

int changeModeToPending(NAL9602 &sat);

int changeModeToLab(Serial &s, NAL9602 &sat);

int changeModeToFlight(Serial &s, NAL9602 &sat);

void shutdownBT();

#endif
