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
char send_SBD_message(RN41 &bt, NAL9602 &sat) {
  char successFlags = 0;
  bool gps_success = false;
  bool msg_err = false;
  bool transmit_success = false;
  int transmit_code;
  int bars = 0;
  time_t t;  // Time structure
  FILE* fp;

  // 1.  Check to see if pod data needs to be requested
  while (!(sat.sbdMessage.doneLoading || sat.sbdMessage.requestedPodData)) {
    // Is this pod expected to send data?  If not, move to next pod
    if (sat.sbdMessage.podLengths[sat.sbdMessage.selectedPod]) {
      // If a request has not been sent, send it now
      if (!sat.sbdMessage.requestedPodData) {
        // >>> NOT YET IMPLEMENTED <<<
        sat.sbdMessage.requestedPodData = true;
        sat.sbdMessage.timeSincePodRequest.start();
        sat.sbdMessage.timeSincePodRequest.reset();
        #ifdef DEV_MODE_LOGGING
          time(&t);
          fp = fopen("/local/devLog.txt", "a");
          fprintf(fp, "%s \t Requested data from pod %i\r\n", ctime(&t), sat.sbdMessage.selectedPod);
          fclose(fp);
        #endif
      }
    } else {
      sat.sbdMessage.selectedPod++;
      sat.sbdMessage.requestedPodData = false;
      if (sat.sbdMessage.selectedPod == MAXPODS) {
        sat.sbdMessage.selectedPod = 0;
        sat.sbdMessage.doneLoading = true;
      }
    }
  }


  // 2.  Have the GPS coordinates been updated for this SBD?  If not, update.
  if (!sat.sbdMessage.updatedGPS) {
    gps_success = sat.gpsUpdate();
    if (gps_success) {
      sat.sbdMessage.updatedGPS = true;
      #ifdef DEV_MODE_LOGGING
        time(&t);
        fp = fopen("/local/devLog.txt", "a");
        fprintf(fp, "%s \t GPS update success\r\n", ctime(&t));
        fclose(fp);
      #endif
    } else {
      // Log a failure message for post-flight diagnostics
      time(&t);
      fp = fopen("/local/log.txt", "a");
      fprintf(fp, "%s \t GPS update failed\r\n", ctime(&t));
      fclose(fp);
    }
  }
  if (sat.sbdMessage.updatedGPS) successFlags = successFlags | 1;

  // 3.  Has a pod been requested to send data?
  //     If so, check for availability and load into intermediate buffer
  if (sat.sbdMessage.requestedPodData) {
    //  >>> NOT YET IMPLEMENTED <<<
    //  Should have time out feature (using timeSincePodRequest)
  }
  if (sat.sbdMessage.doneLoading) successFlags = successFlags | 2;

  // 4.  Has GPS been updated and pod data received?  If so, it is time to load
  //     the SBD message been loaded into the NAL 9602 buffer if not already done.
  if ((sat.sbdMessage.doneLoading) && (sat.sbdMessage.updatedGPS) && (!sat.sbdMessage.messageLoaded)) {
    msg_err = sat.setMessage(getBatteryVoltage(), 0, 0);
    if (!msg_err) sat.sbdMessage.messageLoaded = true;
  }
  if (sat.sbdMessage.messageLoaded) successFlags = successFlags | 4;

  // 5.  If message is loaded, check for satellite signal
  if (sat.sbdMessage.messageLoaded) {
    bars = sat.signalQuality();
    if (bars) successFlags = successFlags | 8;
  }

  // 6.  If message is loaded and have good satellite signal, transmit message
  if (bars) {
    transmit_code = sat.transmitMessage();
    if (transmit_code == 0) transmit_success = true;
  }

  // 7.  If transmission was successful, flag and reset.  If not, check for timeout.
  if (transmit_success) {
    successFlags = successFlags | 16;
    sat.sbdMessage.doneLoading = false;
    sat.sbdMessage.requestedPodData = false;
    sat.sbdMessage.updatedGPS = false;
    sat.sbdMessage.messageLoaded = false;
    sat.sbdMessage.selectedPod = 0;
    sat.sbdMessage.timeSinceSbdRequest.stop();
  } else {
    // Log a failure message for post-flight diagnostics
    time(&t);
    fp = fopen("/local/log.txt", "a");
    fprintf(fp, "%s \t SBD transmit failure, code = %i\r\n", ctime(&t), transmit_code);

    if (sat.sbdMessage.timeSinceSbdRequest > sat.sbdMessage.sbdTransTimeout) {
      // Tried to transmit for too long and failed
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
