#ifndef COMMAND_SEQUENCES_H
#define COMMAND_SEQUENCES_H

#include "NAL9602.h"
#include "RN41.h"
#include "TMP36.h"
#include "launchControlComm.h"

char send_SBD_message(RN41 &bt, NAL9602 &sat);

#endif
