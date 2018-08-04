/******************************************************************************
 * Global variables declared in main.cpp and also used elsewhere
 *****************************************************************************/

#ifndef CMD_MODULE_GLOBALS_H
#define CMD_MODULE_GLOBALS_H

// Header files needed to define these variables
#include "mbed.h"
#include "NAL9602.h"
#include "TMP36.h"

 // LPC1768 connections
 extern Serial bt;                    // Bluetooth connection via RN-41
 extern NAL9602 sat;                  // NAL 9602 modem interface object
 extern TMP36 intTempSensor;          // Internal temperature sensor
 extern TMP36 extTempSensor;          // External temperature sensor
 extern AnalogIn batterySensor;       // Command module battery monitor
 extern DigitalOut powerStatus;       // LED indicating power status
 extern DigitalOut gpsStatus;         // LED indicating GPS turned on
 extern DigitalOut satStatus;         // LED indicating Iridium radio turned on
 extern DigitalOut podStatus;         // LED indicating XBee connection to pods
 extern DigitalOut futureStatus;      // LED currently used to indicate when parsing command from BT

// LPC timing objects (Timer, Timeout, and Ticker)

#endif
