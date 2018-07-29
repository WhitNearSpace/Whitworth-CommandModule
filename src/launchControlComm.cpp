#include "launchControlComm.h"

int parseLaunchControlInput(Serial &s, NAL9602 &sat) {
  char cmd[80];
  char strOpt[80];
  int numOpt;
  int status = 0;
  s.scanf("%79s", &cmd);
  //s.printf("Match GPS? %d\r\n", strcmp(cmd,"GPS"));
  //s.printf("Match GPSDATA? %d\r\n", strcmp(cmd,"GPSDATA"));

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

  // SLEEPHEIGHT command
  } else if (strcmp(cmd,"SLEEPHEIGHT")==0) {
    // Not yet implemented

  // GPSDATA request
  } else if (strcmp(cmd,"GPSDATA")==0) {
    status = sendGPStoLaunchControl(s, sat);

  // CMDSENSORS request
  } else if (strcmp(cmd,"CMDSENSORS")==0) {
    // Not yet implemented

  // PODDATA request
  } else if (strcmp(cmd,"PODDATA")==0) {
    // Not yet implemented

  // MISSIONID command
  } else if (strcmp(cmd,"MISSIONID")==0) {
    // Not yet implemented

  // PODLENGTHS command
  } else if (strcmp(cmd,"PODLENGTHS")==0) {
    // Not yet implemented

  // FLIGHT_MODE commands
  } else if (strcmp(cmd,"FLIGHT_MODE")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      if (flightMode<2) {
        flightMode = 1;
        timeSinceTrans.start();
      } else {
        status = -3;
      }
    } else if (strcmp(strOpt,"OFF")==0) {
      if (flightMode<2) {
        flightMode = 0;
        timeSinceTrans.stop();
        timeSinceTrans.reset();
      } else {
        status = -3;
      }
    } else if (strcmp(strOpt,"?")==0) {
      s.printf("MODE=%i\r\n", flightMode);
    } else status = -2;

  // TRANSPERIOD command
  } else if (strcmp(cmd,"TRANSPERIOD")==0) {
    s.scanf(" = %i", &numOpt);
    if ((numOpt>=15) && (numOpt<=90)) {
      flightTransPeriod = numOpt;
      status = 0;
    } else status = -2;

  } else status = -1;
  if (status==0) {
    s.printf("OK\r\n");
  } else {
    s.printf("ERROR\r\n");
  }
  return status;
}

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
    if (fix) {
      s.printf("FIX=NO\r\n");
      s.printf("LAT=0\r\n");
      s.printf("LON=0\r\n");
      s.printf("ALT=0\r\n");
    }
  }
  sat.verboseLogging = logSetting;
  return status;
}
