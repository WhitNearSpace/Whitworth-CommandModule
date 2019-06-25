#include "commandSequences.h"

/* Development Status Notes
 *
 * Each function should be preceded by a note on its status:
 *  - Incomplete
 *  - Ready for testing
 *  - Tested with terminal
 *  - Lab tested with 9602-A
 *  - Flight tested
 */

// Status:  Ready for testing
char send_SBD_message(RN41 &bt, NAL9602 &sat, CM_to_FC &podRadio) {
  char successFlags = 0;
  bool gps_fix = false;
  bool msg_err = false;
  bool transmit_success = false;
  int transmit_code;
  int bars = 0;
  int numSats = 0;
  time_t t;  // Time structure
  FILE* fp;

  // 0.  Start the clock the first time
  if (!sat.sbdMessage.attemptingSend) {
    sat.sbdMessage.timeSinceSbdRequest.reset();
    sat.sbdMessage.timeSinceSbdRequest.start();
    sat.sbdMessage.attemptingSend = true;
  }

  // 1.  Check to see if pod data needs to be requested
  if (!sat.sbdMessage.requestedPodData) {
    podRadio.request_data_all();
    sat.sbdMessage.requestedPodData = true;
    sat.sbdMessage.timeSincePodRequest.start();
    sat.sbdMessage.timeSincePodRequest.reset();
  }

  // 2.  Have the GPS coordinates been updated for this SBD?  If not, update.
  if (!sat.sbdMessage.updatedGPS) {
    gps_fix = sat.gpsUpdate();
    if (gps_fix) {
      numSats = sat.getSatsUsed();
      if (numSats>3) {
        sat.sbdMessage.updatedGPS = true;
        #ifdef DEV_MODE_LOGGING
          time(&t);
          fp = fopen("/local/devLog.txt", "a");
          fprintf(fp, "%s \t GPS update success with %i satellites\r\n", ctime(&t), numSats);
          fclose(fp);
        #endif
      } else {
        #ifdef DEV_MODE_LOGGING
          time(&t);
          fp = fopen("/local/devLog.txt", "a");
          fprintf(fp, "%s \t GPS update failed, only %i satellites used\r\n", ctime(&t), numSats);
          fclose(fp);
        #endif
      }
    } else {
      // Log a failure message for post-flight diagnostics
      time(&t);
      fp = fopen("/local/log.txt", "a");
      fprintf(fp, "%s \t GPS - no fix\r\n", ctime(&t));
      fclose(fp);
    }
  }
  if (sat.sbdMessage.updatedGPS) successFlags = successFlags | 1;

  // 3.  Check for pod data availability and load into intermediate buffer
  if ((sat.sbdMessage.requestedPodData) && (!sat.sbdMessage.doneLoading)) {
    if ((podRadio.is_all_data_updated()) || (sat.sbdMessage.timeSincePodRequest>sat.sbdMessage.sbdPodTimeout)) {
      transfer_pod_data_to_SBD(podRadio, sat);
      sat.sbdMessage.doneLoading = true;
    }
  }
  if (sat.sbdMessage.doneLoading) successFlags = successFlags | 2;

  // 4.  Has GPS been updated and pod data received?  If so, it is time to load
  //     the SBD message into the NAL 9602 buffer if not already done.
  if ((sat.sbdMessage.doneLoading) && (sat.sbdMessage.updatedGPS) && (!sat.sbdMessage.messageLoaded)) {
    msg_err = sat.setMessage(getBatteryVoltage(), intTempSensor.read(), extTempSensor.read());
    if (!msg_err) {
      sat.sbdMessage.messageLoaded = true;
      #ifdef DEV_MODE_LOGGING
        time(&t);
        fp = fopen("/local/devLog.txt", "a");
        fprintf(fp, "%s \t SBD message loaded into buffer\r\n", ctime(&t));
        fclose(fp);
      #endif
    }
  }
  if (sat.sbdMessage.messageLoaded) successFlags = successFlags | 4;

  // 5.  If message is loaded, attempt transmission
  if (sat.sbdMessage.messageLoaded) {
    transmit_code = sat.transmitMessage();
    if (transmit_code & 1) {
      transmit_success = true;
      #ifdef DEV_MODE_LOGGING
        time(&t);
        fp = fopen("/local/devLog.txt", "a");
        fprintf(fp, "%s \t SBD message transmitted\r\n", ctime(&t));
        fclose(fp);
      #endif
    }
  }

  // 6.  If transmission was successful, flag and reset.  If not, check for problems.
  if (transmit_success) {
    successFlags = successFlags | 16;
    sat.sbdMessage.doneLoading = false;
    sat.sbdMessage.requestedPodData = false;
    sat.sbdMessage.updatedGPS = false;
    sat.sbdMessage.messageLoaded = false;
    sat.sbdMessage.selectedPod = 0;
    sat.sbdMessage.timeSinceSbdRequest.stop();
  } else {
    // If message loaded and no successful transmission then there is a problem
    if (sat.sbdMessage.messageLoaded) {
      // Log a failure message for post-flight diagnostics
      time(&t);
      bars = sat.signalQuality();
      fp = fopen("/local/log.txt", "a");
      fprintf(fp, "%s \t SBD transmit failure, signal quality = %i", ctime(&t), bars);
      if (bars) {
        fprintf(fp, ", transmit code = %i", transmit_code);
      }
      fprintf(fp,"\r\n");
      fclose(fp);
      if (bars) {
        wait(5*(float)rand()/RAND_MAX);  // if transmit failed, wait 0-5 sec (per manufacturer)
      }
    }

    // Tried to transmit for too long and failed
    if (sat.sbdMessage.timeSinceSbdRequest > sat.sbdMessage.sbdTransTimeout) {
      successFlags = successFlags | 128;  // Actually a failure flag bit
      fprintf(fp, "%s \t SBD timeout\r\n", ctime(&t));
      sat.sbdMessage.doneLoading = false;
      sat.sbdMessage.requestedPodData = false;
      sat.sbdMessage.updatedGPS = false;
      sat.sbdMessage.messageLoaded = false;
      sat.sbdMessage.selectedPod = 0;
      sat.sbdMessage.timeSinceSbdRequest.stop();
    }
    fclose(fp);
  }
  return successFlags;
}

void transfer_pod_data_to_SBD(CM_to_FC &podRadio, NAL9602 &sat) {
  char data[MAX_POD_DATA_BYTES];
  char len;
  char n;
  for (int i = 0; i < podRadio.registry_length(); i++) {
    n = podRadio.pod_index(i);
    len = podRadio.get_pod_data(n, data);
    sat.sbdMessage.podLengths[n-1] = len;
    sat.sbdMessage.loadPodBuffer(n,data);
  }
  sat.sbdMessage.updateMsgLength();
}