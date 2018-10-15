#ifndef LAUNCH_CONTROL_COMM_H
#define LAUNCH_CONTROL_COMM_H

#include "mbed.h"
#include "projectGlobals.h"
#include "NAL9602.h"
#include "RN41.h"
#include "TMP36.h"
#include "FlightParameters.h"

/** Listener functions for Launch Control app communication
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2017
 *  @copyright MIT License
 */

int parseLaunchControlInput(Serial &s, NAL9602 &sat);

int sendGPStoLaunchControl(Serial &s, NAL9602 &sat);

int sendCmdSensorsToLaunchControl(Serial &s, NAL9602 &sat);

float getBatteryVoltage();

void updateStatusLED();

int changeModeToPending(NAL9602 &sat);

int changeModeToLab(NAL9602 &sat);

int changeModeToFlight(RN41 &bt, NAL9602 &sat);

int changeModeToLanded(RN41 &bt, NAL9602 &sat);

#endif
