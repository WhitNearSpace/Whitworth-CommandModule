#ifndef COMMAND_SEQUENCES_H
#define COMMAND_SEQUENCES_H

#include "NAL9602.h"
#include "RN41.h"
#include "TMP36.h"
#include <CM_to_FC.h>
#include "launchControlComm.h"

char send_SBD_message(RN41 &bt, NAL9602 &sat, CM_to_FC &podRadio);

void transfer_pod_data_to_SBD(CM_to_FC &podRadio, NAL9602 &sat);

#endif
