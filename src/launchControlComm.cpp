#include "launchControlComm.h"

int parseLaunchControlInput(Serial &s, NAL9602 &sat) {
  char cmd[80];
  char strOpt[80];
  int numOpt;
  int status = 0;
  s.scanf("%79s", &cmd);
  //s.printf("Match GPS? %d\r\n", strcmp(cmd,"GPS"));
  //s.printf("Match GPSDATA? %d\r\n", strcmp(cmd,"GPSDATA"));
  if (strcmp(cmd,"GPS")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      sat.gpsOn();
    } else if (strcmp(strOpt,"OFF")==0) {
      sat.gpsOff();
    } else status = -2;
  } else if (strcmp(cmd,"SATLINK")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      sat.satLinkOn();
    } else if (strcmp(strOpt,"OFF")==0) {
      sat.satLinkOff();
    } else status = -2;
  } else if (strcmp(cmd,"PODLINK")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      // Not yet implemented
    } else if (strcmp(strOpt,"OFF")==0) {
      // Not yet implemented
    } else status = -1;
  } else if (strcmp(cmd,"RADIO")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      // Not yet implemented
    } else if (strcmp(strOpt,"OFF")==0) {
      // Not yet implemented
    } else status = -1;
  } else if (strcmp(cmd,"SLEEPHEIGHT")==0) {
    // Not yet implemented
  } else if (strcmp(cmd,"GPSDATA")==0) {
    status = sendGPStoLaunchControl(s, sat);
  } else if (strcmp(cmd,"CMDSENSORS")==0) {
    // Not yet implemented
  } else if (strcmp(cmd,"PODDATA")==0) {
    // Not yet implemented
  } else if (strcmp(cmd,"MISSIONID")==0) {
    // Not yet implemented
  } else if (strcmp(cmd,"PODLENGTHS")==0) {
    // Not yet implemented
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
