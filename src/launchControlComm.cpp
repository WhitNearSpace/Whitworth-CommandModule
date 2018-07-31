#include "launchControlComm.h"

int parseLaunchControlInput(Serial &s, NAL9602 &sat) {
  char cmd[80];
  char strOpt[80];
  int numOpt, numOpt2, numOpt3, numOpt4, numOpt5, numOpt6;
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
    status = sendCmdSensorsToLaunchControl(s, sat);

  // PODDATA command
  } else if (strcmp(cmd,"PODDATA")==0) {
    // Not yet implemented

  // MISSIONID command
  } else if (strcmp(cmd,"MISSIONID")==0) {
    s.scanf("=%i", &numOpt);
    if ((numOpt>0) && (numOpt<32768)) {
      missionID = numOpt;
    } else {
      status = -2;
    }

  // PODLENGTHS command
  } else if (strcmp(cmd,"PODLENGTHS")==0) {
    s.scanf("=%i %i %i %i %i %i", &numOpt, &numOpt2, &numOpt3, &numOpt4,
      &numOpt5, &numOpt6);
    sat.sbdMessage.podLengths[0] = numOpt;
    sat.sbdMessage.podLengths[1] = numOpt2;
    sat.sbdMessage.podLengths[2] = numOpt3;
    sat.sbdMessage.podLengths[3] = numOpt4;
    sat.sbdMessage.podLengths[4] = numOpt5;
    sat.sbdMessage.podLengths[5] = numOpt6;
    sat.sbdMessage.updateMsgLength();

  // FLIGHT_MODE commands
  } else if (strcmp(cmd,"FLIGHT_MODE")==0) {
    s.scanf(" %79s", &strOpt);
    if (strcmp(strOpt,"ON")==0) {
      if (flightMode<2) {
        if (!sat.gpsStatus) sat.gpsOn();  // Flight mode requires GPS
        if (!sat.iridiumStatus) sat.satLinkOn();  // Flight mode requires sat
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
    } else status = -2;

  // HELLO command
  } else if (strcmp(cmd, "HELLO")==0) {
    s.printf("COMMAND MODULE READY\r\n");


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

int sendCmdSensorsToLaunchControl(Serial &s, NAL9602 &sat) {
  int status = 0;
  s.printf("BAT=%.2f\r\n", batterySensor.read()*13.29);
  s.printf("EXT_TEMP=%.1f\r\n", extTempSensor.read());
  s.printf("INT_TEMP=%.1f\r\n", intTempSensor.read());
  return status;
}
