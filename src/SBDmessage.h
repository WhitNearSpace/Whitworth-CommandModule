#ifndef SBDmessage_H
#define SBDmessage_H

#include "GPSCoordinates.h"
#include "mbed.h"


#define SBD_LENGTH 340
#define POD_LENGTH 255
#define MAXPODS 6

/** Whitworth Near Space SBD Message object
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2017
 *  @copyright GNU Public License
 */
class SBDmessage {

public:
  /** Create an SBDmessage object
  *
  * @param missionID Mission ID number registered with server
  *   Positive ID = archive. Negative ID = testing (not archived)
  */
  SBDmessage(int16_t missionID);

  ~SBDmessage();

  /** Get ith byte of SBD Message
  *
  * @param i Requested byte (0-339)
  *
  * @returns ith byte (unsigned char) of SBD message
  */
  char getByte(int i);

  /** Populate portion of SBD message devoted to GPS data
  *
  * @param GPSCoordinates object
  */
  void generateGPSBytes(GPSCoordinates &gps);

  /** Populate command module portion of SBD message
  *
  * @param voltage Command module battery voltage
  * @param intTemp Temperature inside command module
  * @param extTemp External temperature
  */
  void generateCommandModuleBytes(float voltage, float intTemp, float extTemp);

  /** Loads pod bytes into SBD
  */
  void generatePodBytes();

  /** Store pod bytes in buffer
  */
  int loadPodBuffer(int podID, char numBytes, char* data);

  /** Clear message
  */
  void clearMessage();

  /** Test extraction of pod bytes
  */
  void testPodBytes();

  int32_t retrieveInt32(int startIndex);
  uint16_t retrieveUInt16(int startIndex);
  int16_t retrieveInt16(int startIndex);

private:
  char sbd[SBD_LENGTH];
  char podData[MAXPODS][POD_LENGTH];
  char podLengths[MAXPODS];
  int msgLength;
  void storeInt32(int startIndex, int32_t data);
  void storeUInt16(int startIndex, uint16_t data);
  void storeInt16(int startIndex, int16_t data);

};

#endif
