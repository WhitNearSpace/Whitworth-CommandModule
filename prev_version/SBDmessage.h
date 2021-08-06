/** Whitworth Near Space SBD Message object
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 1.0
 *  @date 2021
 *  @copyright MIT License
 */

#ifndef SBDmessage_H
#define SBDmessage_H

#include <mbed.h>
#include "FlightParameters.h"
#include "GPSCoordinates.h"

#define SBD_LENGTH 340
#define POD_LENGTH 70
#define MAXPODS 6

class SBDmessage {

public:
  char podLengths[MAXPODS];
  uint32_t msgLength;
  uint32_t missionID;
  bool doneLoading;
  bool requestedPodData;
  bool updatedGPS;
  bool messageLoaded;
  bool attemptingSend;
  int selectedPod;
  Timer timeSincePodRequest;
  Timer timeSinceSbdRequest;
  float sbdTransTimeout;
  float sbdPodTimeout;

  /** Create an SBDmessage object
  */
  SBDmessage();

  ~SBDmessage();

  /** Sets mission ID portion of mission ID
  *
  * Must set missionID member variable before use
  * 
  * @param flightMode Mission ID number registered with server is
  *   positive if moving and negative if on the ground (pre or post flight)
  */
  void setMissionID(uint32_t flightMode);

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
  int loadPodBuffer(int podID, char* data);

  /** Clear message
  */
  void clearMessage();

  /** Test extraction of pod bytes
  */
  void testPodBytes();

  /** Test extraction of pod bytes
  */
  char getPodBytes(char podID, char* data);

  /** Calculate the checksum
  */
  unsigned short generateChecksum();

  void updateMsgLength();

private:
  char sbd[SBD_LENGTH];
  char podData[MAXPODS][POD_LENGTH];
  char checksum[2];

  /** Get ith byte of SBD Message
  *
  * @param i Requested byte (0-339)
  *
  * @returns ith byte (unsigned char) of SBD message
  */
  char retrieve_byte(uint32_t i);

  /** Retrieve a 16-bit signed integer from SBD message
   * 
   * @param startIndex Start index of data (0-338)
   * 
   * @returns 16-bit signed integer
   */
  int16_t retrieve_int16(uint32_t startIndex);

  /** Retrieve a 16-bit unsigned integer from SBD message
   * 
   * @param startIndex Start index of data (0-338)
   * 
   * @returns 16-bit unsigned integer
   */
  uint16_t retrieve_uint16(uint32_t startIndex);

  /** Retrieve a 32-bit signed integer from SBD message
   * 
   * @param startIndex Start index of data (0-336)
   * 
   * @returns 32-bit signed integer
   */
  int32_t retrieve_int32(int startIndex);
  
  /** Store a 16-bit signed integer to SBD message
   * @param startIndex Starting address of byte (0-338)
   * @param data The 16-bit signed integer to be stored
   */
  void store_int16(uint32_t startIndex, int16_t data);

  /** Store a 16-bit unsigned integer to SBD message
   * @param startIndex Starting address of byte (0-338)
   * @param data The 16-bit unsigned integer to be stored
   */
  void store_uint16(uint32_t startIndex, uint16_t data);

  /** Store a 32-bit signed integer to SBD message
   * @param startIndex Starting address of byte (0-336)
   * @param data The 32-bit signed integer to be stored
   */
  void store_int32(uint32_t startIndex, int32_t data);
};

#endif
