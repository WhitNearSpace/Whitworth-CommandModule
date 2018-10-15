#include "launchControlComm.h"

int parseLaunchControlInput(Serial &s, NAL9602 &sat) {
  char cmd[80];
  char strOpt[80];
  char c;
  bool reachedEnd = false;
  int numOpt, numOpt2, numOpt3, numOpt4, numOpt5, numOpt6;
  int status = 0;
  s.scanf("%79s", &cmd);
  // GPS commands
  if (strcmp(cmd,"GPS")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      sat.gpsOn();
    } else if (strcmp(strOpt,"OFF")==0) {
      sat.gpsOff();
    } else status = -2;

  // SATLINK commands
  } else if (strcmp(cmd,"SATLINK")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      sat.satLinkOn();
    } else if (strcmp(strOpt,"OFF")==0) {
      sat.satLinkOff();
    } else status = -2;

  // PODLINK commands
  } else if (strcmp(cmd,"PODLINK")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      // Not yet implemented
    } else if (strcmp(strOpt,"OFF")==0) {
      // Not yet implemented
    } else status = -2;

  // RADIO commands
  } else if (strcmp(cmd,"RADIO")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      // Not yet implemented
    } else if (strcmp(strOpt,"OFF")==0) {
      // Not yet implemented
    } else status = -2;

  // TRIGGERHEIGHT command
  } else if (strcmp(cmd,"TRIGGERHEIGHT")==0) {
    s.scanf(" %i", &numOpt);
    if ((numOpt>10) && (numOpt<200)) {
      flight.triggerHeight = (float)(numOpt);
    } else {
      status = -2;
    }

  // GPSDATA request
  } else if (strcmp(cmd,"GPSDATA")==0) {
    sendGPStoLaunchControl(s, sat);
    status=1;

  // CMDSENSORS request
  } else if (strcmp(cmd,"CMDSENSORS")==0) {
    status = sendCmdSensorsToLaunchControl(s, sat);

  // PODDATA command
  } else if (strcmp(cmd,"PODDATA")==0) {
    // Not yet implemented

  // MISSIONID command
  } else if (strcmp(cmd,"MISSIONID")==0) {
    s.scanf(" %i", &numOpt);
    if ((numOpt>0) && (numOpt<32768)) {
      sat.sbdMessage.missionID = numOpt;
    } else {
      status = -2;
    }

  // PODLENGTHS command
  } else if (strcmp(cmd,"PODLENGTHS")==0) {
    s.scanf(" %i %i %i %i %i %i\r\n", &numOpt, &numOpt2, &numOpt3, &numOpt4,
      &numOpt5, &numOpt6);
    sat.sbdMessage.podLengths[0] = numOpt;
    sat.sbdMessage.podLengths[1] = numOpt2;
    sat.sbdMessage.podLengths[2] = numOpt3;
    sat.sbdMessage.podLengths[3] = numOpt4;
    sat.sbdMessage.podLengths[4] = numOpt5;
    sat.sbdMessage.podLengths[5] = numOpt6;
    sat.sbdMessage.updateMsgLength();

  // FLIGHT_MODE commands
  } else if (strcmp(cmd,"FLIGHT_MODE?")==0) {
    s.printf("MODE=%i\r\n", flight.mode);

  } else if (strcmp(cmd,"FLIGHT_MODE")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      if (flight.mode<2) {
        status = changeModeToPending(sat);
      } else {
        status = -3;
      }
    } else if (strcmp(strOpt,"OFF")==0) {
      if (flight.mode<2) {
        status = changeModeToLab(sat);
      } else {
        status = -3;
      }
    } else status = -2;

  // TRANSPERIOD command
  } else if (strcmp(cmd,"TRANSPERIOD")==0) {
    s.scanf(" %i", &numOpt);
    if ((numOpt>=15) && (numOpt<=90)) {
      flight.transPeriod = numOpt;
    } else status = -2;

  // HELLO command
  } else if (strcmp(cmd, "HELLO")==0) {
    s.printf("COMMAND MODULE READY\r\n");


  } else status = -1;

  // Clear the buffer until reach \n
  while (!reachedEnd) {
    c = s.getc();
    if (c == '\n') reachedEnd = true;
  }
  if (status==0) {
    s.printf("OK\r\n");
  } else if (status<0) {
    s.printf("ERROR\r\n");
  }
  return status;
}

// Status: Lab tested with 9602-A
int sendGPStoLaunchControl(Serial &s, NAL9602 &sat) {
  int status = 0;
  bool fix;
  bool logSetting = sat.verboseLogging;
  sat.verboseLogging = false;
  fix = sat.gpsUpdate();
  if (fix) {
    s.printf("FIX=YES\r\n");
    s.printf("LAT=%.5f\r\n",sat.latitude());
    s.printf("LON=%.5f\r\n",sat.longitude());
    s.printf("ALT=%.5f\r\n",sat.altitude());
  } else {
    s.printf("FIX=NO\r\n");
    s.printf("LAT=0\r\n");
    s.printf("LON=0\r\n");
    s.printf("ALT=0\r\n");
  }
  s.printf("OK\r\n");
  sat.verboseLogging = logSetting;
  return status;
}

// Status: Lab tested with 9602-A
int sendCmdSensorsToLaunchControl(Serial &s, NAL9602 &sat) {
  int status = 0;
  s.printf("BAT=%.2f\r\n", getBatteryVoltage());
  s.printf("EXT_TEMP=%.1f\r\n", extTempSensor.read());
  s.printf("INT_TEMP=%.1f\r\n", intTempSensor.read());
  return status;
}

// Status: Lab tested with 9602-A
void updateStatusLED() {
  switch (flight.mode) {
    case 0:
      powerStatus = 1;
      gpsStatus = sat.gpsStatus;
      satStatus = sat.iridiumStatus;
      podStatus = 0; // link to pods is not yet implemented so must be off
      //futureStatus = 0; // off for now...
      break;
    case 1:
      powerStatus = !powerStatus;
      gpsStatus = sat.gpsStatus && (!powerStatus);
      satStatus = sat.iridiumStatus && (!powerStatus);
      podStatus = 0; // link to pods is not yet implemented so must be off
      //futureStatus = 0; // off for now...
      break;
    case 2:
      powerStatus = 0;
      gpsStatus = 0;
      satStatus = 0;
      podStatus = 0;
      //futureStatus = 0;
      break;
    case 3:
      powerStatus = !powerStatus;
      gpsStatus = sat.gpsStatus && powerStatus;
      satStatus = sat.iridiumStatus && powerStatus;
      podStatus = 0; // link to pods is not yet implemented so must be off
      //futureStatus = 0; // off for now...
      break;
    default:
      powerStatus = 1;
      futureStatus = !futureStatus;
  }
}

// Status: Lab tested with 9602-A
int changeModeToLab(NAL9602 &sat) {
  int status = 0;
  sat.setModeGPS(stationary);
  flight.mode = 0;
  sat.verboseLogging = true;
  return status;
}

// Status: Lab tested with 9602-A
int changeModeToPending(NAL9602 &sat) {
  sat.verboseLogging = false;
  int status = 0;
  sat.setModeGPS(pedestrian);
  wait(1);
  if (!sat.gpsStatus) {
    sat.gpsOn();  // Flight mode requires GPS
    wait(1);
  }
  if (!sat.iridiumStatus) {
    sat.satLinkOn();  // Flight mode requires sat
    wait(1);
  }
  bool fix;
  int fixSats = 0;
  while (fixSats<4) {
    fix = sat.gpsUpdate();
    if (fix) fixSats = sat.getSatsUsed();
    if (fixSats<4) {
      wait(5);
    }
  }
  flight.groundAltitude = sat.altitude();
  flight.mode = 1;
  sat.sbdMessage.sbdTransTimeout = 0.9*PRE_TRANS_PERIOD;
  timeSinceTrans.reset();
  timeSinceTrans.start();
  checkTime.reset();
  checkTime.start();
  return status;
}

int changeModeToFlight(RN41 &bt, NAL9602 &sat) {
  int status = 0;
  sat.setModeGPS(airborne_low_dynamic);
  flight.mode = 2;
  bt.initiateShutdown();
  sat.sbdMessage.sbdTransTimeout = 0.9*flight.transPeriod;
  return status;
}

int changeModeToLanded(RN41 &bt, NAL9602 &sat) {
  int status = 0;
  sat.setModeGPS(pedestrian);
  flight.mode = 3;
  sat.sbdMessage.sbdTransTimeout = 0.9*POST_TRANS_PERIOD;
  return status;
}


float getBatteryVoltage() {
  float sum = 0;
  int n = 10;
  for (int i = 0; i < n; i++)
    sum = sum + batterySensor.read();
  return sum/n*13.29;
};
