/****************************************************************************
 * Class for interfacing with a Ublox GPS module via I2C (DDC)
 * 
 * Written for Mbed OS by John M. Larkin (February 17, 2020)
 * Released under the MIT License (http://opensource.org/licenses/MIT).
 * 	
 * The MIT License (MIT)
 * Copyright (c) 2020 John M. Larkin
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
 * associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * The Mbed OS version is based on the work of Nathan Seidle @ SparkFun Electronics (September 6, 2018).
 * 
 * https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library
 * 
 * SparkFun code, firmware, and software is released under the MIT License
 * Copyright (c) 2016 SparkFun Electronics
 * MIT License can be found at (http://opensource.org/licenses/MIT) or above.
******************************************************************************/

#include "ublox.h"


Ublox_GPS::Ublox_GPS(I2C* i2c, uint8_t deviceAddress) {
  _i2c = i2c;
  commType = COMM_TYPE_I2C;
  _gpsI2Caddress = deviceAddress; // Store the I2C address from user
  currentGeofenceParams.numFences = 0; // Zero the number of geofences currently in use
  moduleQueried.versionNumber = false;
  _pollingTimer.start();
}

//Enable or disable the printing of sent/response HEX values.
//Use this in conjunction with 'Transport Logging' from the Universal Reader Assistant to see what they're doing that we're not
void Ublox_GPS::enableDebugging(void)
{
  _printDebug = true; //Should we print the commands we send? Good for debugging
}

void Ublox_GPS::disableDebugging(void)
{
  _printDebug = false; //Turn off extra print statements
}

//Safely print messages
void Ublox_GPS::debugPrint(char *message)
{
  if (_printDebug == true)
  {
    printf("%s", message);
  }
}
//Safely print messages
void Ublox_GPS::debugPrintln(const char *message)
{
  if (_printDebug == true)
  {
    printf("%s\r\n", message);
  }
}

const char* Ublox_GPS::statusString(UbloxStatus_e stat)
{
  switch (stat)
  {
  case UBLOX_STATUS_SUCCESS:
    return "Success";
    break;
  case UBLOX_STATUS_FAIL:
    return "General Failure";
    break;
  case UBLOX_STATUS_CRC_FAIL:
    return "CRC Fail";
    break;
  case UBLOX_STATUS_TIMEOUT:
    return "Timeout";
    break;
  case UBLOX_STATUS_COMMAND_UNKNOWN:
    return "Command Unknown";
    break;
  case UBLOX_STATUS_OUT_OF_RANGE:
    return "Out of range";
    break;
  case UBLOX_STATUS_INVALID_ARG:
    return "Invalid Arg";
    break;
  case UBLOX_STATUS_INVALID_OPERATION:
    return "Invalid operation";
    break;
  case UBLOX_STATUS_MEM_ERR:
    return "Memory Error";
    break;
  case UBLOX_STATUS_HW_ERR:
    return "Hardware Error";
    break;
  case UBLOX_STATUS_DATA_SENT:
    return "Data Sent";
    break;
  case UBLOX_STATUS_DATA_RECEIVED:
    return "Data Received";
    break;
  case UBLOX_STATUS_I2C_COMM_FAILURE:
    return "I2C Comm Failure";
    break;
  default:
    return "Unknown Status";
    break;
  }
  return "None";
}

void Ublox_GPS::factoryReset()
{
  // Copy default settings to permanent
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_CFG;
  packetCfg.len = 13;
  packetCfg.startingSpot = 0;
  for (uint8_t i = 0; i < 4; i++)
  {
    payloadCfg[0 + i] = 0xff; // clear mask: copy default config to permanent config
    payloadCfg[4 + i] = 0x00; // save mask: don't save current to permanent
    payloadCfg[8 + i] = 0x00; // load mask: don't copy permanent config to current
  }
  payloadCfg[12] = 0xff;     // all forms of permanent memory
  sendCommand(packetCfg, 0); // don't expect ACK
  hardReset();               // cause factory default config to actually be loaded and used cleanly
}

void Ublox_GPS::hardReset()
{
  // Issue hard reset
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RST;
  packetCfg.len = 4;
  packetCfg.startingSpot = 0;
  payloadCfg[0] = 0xff;      // cold start
  payloadCfg[1] = 0xff;      // cold start
  payloadCfg[2] = 0;         // 0=HW reset
  payloadCfg[3] = 0;         // reserved
  sendCommand(packetCfg, 0); // don't expect ACK
}

//Changes the I2C address that the Ublox module responds to
//0x42 is the default but can be changed with this command
bool Ublox_GPS::setI2CAddress(uint8_t deviceAddress, uint16_t maxWait)
{
  //Get the current config values for the I2C port
  getPortSettings(COM_PORT_I2C, maxWait); //This will load the payloadCfg array with current port settings

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 20;
  packetCfg.startingSpot = 0;

  //payloadCfg is now loaded with current bytes. Change only the ones we need to
  payloadCfg[4] = deviceAddress << 1; //DDC mode LSB

  if (sendCommand(packetCfg, maxWait) == true)
  {
    //Success! Now change our internal global.
    _gpsI2Caddress = deviceAddress; //Store the I2C address from user
    return (true);
  }
  return (false);
}

//Want to see the NMEA messages on a Serial port? Here's how
void Ublox_GPS::setNMEAOutputPort(BufferedSerial &nmeaOutputPort)
{
  _nmeaOutputPort = &nmeaOutputPort; //Store the port from user
}

//Called regularly to check for available bytes on the user' specified port
bool Ublox_GPS::checkUblox()
{
  if (commType == COMM_TYPE_I2C)
    return (checkUbloxI2C());
  // else if (commType == COMM_TYPE_SERIAL)
  //   return (checkUbloxSerial());
  return false;
}



//Polls I2C for data, passing any new bytes to process()
//Returns true if new bytes are available
bool Ublox_GPS::checkUbloxI2C()
{
  DigitalOut led(LED1);
  led = 0;
  char cmd[MAX_PAYLOAD_SIZE];
  char nak;
  char addr = _gpsI2Caddress<<1; // We need 8-bit version of address
  _i2c->lock();
  if (_pollingTimer.elapsed_time() >= i2cPollingWait)
  {
    //Get the number of bytes available from the module
    uint16_t bytesAvailable = 0;
    cmd[0] = 0xFD; //0xFD (MSB) and 0xFE (LSB) are the registers that contain number of bytes available
    nak = _i2c->write(addr, cmd, 1, true); // Do not send STOP signal
    if (nak) {
      _i2c->unlock();
      return false;
    }

    nak = _i2c->read(addr, cmd, 2);
    if (!nak) {
      if (cmd[1] == 0xFF) {
        // Nathan Seidle (SparkFun) believes this is a Ublox bug. Device should never present an 0xFF.
        debugPrintln("checkUbloxI2C: Ublox bug, no bytes available");
        _pollingTimer.reset();
        _i2c->unlock();
        return false;
      }
      bytesAvailable = cmd[0] << 8 | cmd[1];
    }

    if (bytesAvailable == 0)
    {
      debugPrintln("checkUbloxI2C: OK, zero bytes available");
      _pollingTimer.reset();
      _i2c->unlock();
      return false;
    }

    //Check for undocumented bit error. We (Nathan Seidle, SparkFun) found this doing logic scans.
    //This error is rare but if we incorrectly interpret the first bit of the two 'data available' bytes as 1
    //then we have far too many bytes to check. May be related to I2C setup time violations: https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/40
    if (bytesAvailable & ((uint16_t)1 << 15))
    {
      //Clear the MSbit
      bytesAvailable &= ~((uint16_t)1 << 15);

      if (_printDebug == true)
      {
        printf("checkUbloxI2C: Bytes available error: %d\r\n", bytesAvailable);
      }
    }

    if (bytesAvailable > 100)
    {
      if (_printDebug == true)
      {
        printf("checkUbloxI2C: Large packet of %d bytes received\r\n", bytesAvailable);
      }
    }
    else
    {
      if (_printDebug == true)
      {
        printf("checkUbloxI2C: Reading %d bytes\r\n", bytesAvailable);
      }
    }

    // mbed version
    int bytesToRead;
    bool firstRead = true;
    while (bytesAvailable) {
      if (bytesAvailable > MAX_PAYLOAD_SIZE) {
        bytesToRead = MAX_PAYLOAD_SIZE;
      } else bytesToRead = bytesAvailable;
      cmd[0] = 0xFF; //0xFF is the register to read data from
      nak = _i2c->write(addr, cmd, 1, true);
      if (nak) {
        _i2c->unlock();
        return false;
      }
      nak = _i2c->read(addr, cmd, bytesToRead);
      if (nak) {
        _i2c->unlock();
        return false;
      }
      if ((firstRead) && (cmd[0] == 0x7F)) {
        debugPrintln("checkUbloxU2C: Ublox error, module not ready with data");
        led = 1;
        _i2c->unlock();
        return false;
      }
      for (int i = 0; i < bytesToRead; i++)
        process(cmd[i]);
      firstRead = false;
      bytesAvailable -= bytesToRead;
    }
  }
  _i2c->unlock();
  return true;
} //end checkUbloxI2C()

//Processes NMEA and UBX binary sentences one byte at a time
//Take a given byte and file it into the proper array
void Ublox_GPS::process(uint8_t incoming)
{
  if (currentSentence == NONE || currentSentence == NMEA)
  {
    if (incoming == 0xB5) //UBX binary frames start with 0xB5, aka μ
    {
      //This is the start of a binary sentence. Reset flags.
      //We still don't know the response class
      ubxFrameCounter = 0;

      rollingChecksumA = 0; //Reset our rolling checksums
      rollingChecksumB = 0;

      currentSentence = UBX;
    }
    else if (incoming == '$')
    {
      currentSentence = NMEA;
    }
    else if (incoming == 0xD3) //RTCM frames start with 0xD3
    {
      rtcmFrameCounter = 0;
      currentSentence = RTCM;
    }
    else
    {
      //This character is unknown or we missed the previous start of a sentence
    }
  }

  //Depending on the sentence, pass the character to the individual processor
  if (currentSentence == UBX)
  {
    //Decide what type of response this is
    if (ubxFrameCounter == 0 && incoming != 0xB5)      //ISO 'μ'
      currentSentence = NONE;                          //Something went wrong. Reset.
    else if (ubxFrameCounter == 1 && incoming != 0x62) //ASCII 'b'
      currentSentence = NONE;                          //Something went wrong. Reset.
    else if (ubxFrameCounter == 2)                     //Class
    {
      //We can now identify the type of response
      if (incoming == UBX_CLASS_ACK)
      {
        packetAck.counter = 0;
        packetAck.valid = false;
        ubxFrameClass = CLASS_ACK;
      }
      else
      {
        packetCfg.counter = 0;
        packetCfg.valid = false;
        ubxFrameClass = CLASS_NOT_AN_ACK;
      }
    }

    ubxFrameCounter++;

    //Depending on this frame's class, pass different structs and payload arrays
    if (ubxFrameClass == CLASS_ACK)
      processUBX(incoming, &packetAck);
    else if (ubxFrameClass == CLASS_NOT_AN_ACK)
      processUBX(incoming, &packetCfg);
  }
  else if (currentSentence == NMEA)
  {
    processNMEA(incoming); //Process each NMEA character
  }
  else if (currentSentence == RTCM)
  {
    processRTCMframe(incoming); //Deal with RTCM bytes
  }
}

//This is the default or generic NMEA processor. We're only going to pipe the data to serial port so we can see it.
// Need to add __weak__ attribute in header file if decide to make user-overwrite of this
// method an option. SparkFun version is like that, but I've opted to not do that (JML).
void Ublox_GPS::processNMEA(char incoming)
{
  //If user has assigned an output port then pipe the characters there
  if (_nmeaOutputPort != NULL)
    _nmeaOutputPort->write(&incoming,1); //Echo this byte to the serial port
}

//We need to be able to identify an RTCM packet and then the length
//so that we know when the RTCM message is completely received and we then start
//listening for other sentences (like NMEA or UBX)
//RTCM packet structure is very odd. I never found RTCM STANDARD 10403.2 but
//http://d1.amobbs.com/bbs_upload782111/files_39/ourdev_635123CK0HJT.pdf is good
//https://dspace.cvut.cz/bitstream/handle/10467/65205/F3-BP-2016-Shkalikava-Anastasiya-Prenos%20polohove%20informace%20prostrednictvim%20datove%20site.pdf?sequence=-1
//Lead me to: https://forum.u-blox.com/index.php/4348/how-to-read-rtcm-messages-from-neo-m8p
//RTCM 3.2 bytes look like this:
//Byte 0: Always 0xD3
//Byte 1: 6-bits of zero
//Byte 2: 10-bits of length of this packet including the first two-ish header bytes, + 6.
//byte 3 + 4 bits: Msg type 12 bits
//Example: D3 00 7C 43 F0 ... / 0x7C = 124+6 = 130 bytes in this packet, 0x43F = Msg type 1087
void Ublox_GPS::processRTCMframe(uint8_t incoming)
{
  if (rtcmFrameCounter == 1)
  {
    rtcmLen = (incoming & 0x03) << 8; //Get the last two bits of this byte. Bits 8&9 of 10-bit length
  }
  else if (rtcmFrameCounter == 2)
  {
    rtcmLen |= incoming; //Bits 0-7 of packet length
    rtcmLen += 6;        //There are 6 additional bytes of what we presume is header, msgType, CRC, and stuff
  }
  /*else if (rtcmFrameCounter == 3)
  {
    rtcmMsgType = incoming << 4; //Message Type, MS 4 bits
  }
  else if (rtcmFrameCounter == 4)
  {
    rtcmMsgType |= (incoming >> 4); //Message Type, bits 0-7
  }*/

  rtcmFrameCounter++;

  processRTCM(incoming); //Here is where we expose this byte to the user

  if (rtcmFrameCounter == rtcmLen)
  {
    //We're done!
    currentSentence = NONE; //Reset and start looking for next sentence type
  }
}

//This function is called for each byte of an RTCM frame
//Ths user can overwrite this function and process the RTCM frame as they please
//Bytes can be piped to Serial or other interface. The consumer could be a radio or the internet (Ntrip broadcaster)
void Ublox_GPS::processRTCM(uint8_t incoming)
{
  //Radio.sendReliable((String)incoming); //An example of passing this byte to a radio

  //_debugSerial->write(incoming); //An example of passing this byte out the serial port

  //Debug printing
  //  _debugSerial->print(F(" "));
  //  if(incoming < 0x10) _debugSerial->print(F("0"));
  //  if(incoming < 0x10) _debugSerial->print(F("0"));
  //  _debugSerial->print(incoming, HEX);
  //  if(rtcmFrameCounter % 16 == 0) _debugSerial->println();
}

//Given a character, file it away into the uxb packet structure
//Set valid = true once sentence is completely received and passes CRC
//The payload portion of the packet can be 100s of bytes but the max array
//size is roughly 64 bytes. startingSpot can be set so we only record
//a subset of bytes within a larger packet.
void Ublox_GPS::processUBX(uint8_t incoming, ubxPacket *incomingUBX)
{
  //Add all incoming bytes to the rolling checksum
  //Stop at len+4 as this is the checksum bytes to that should not be added to the rolling checksum
  if (incomingUBX->counter < incomingUBX->len + 4)
    addToChecksum(incoming);

  if (incomingUBX->counter == 0)
  {
    incomingUBX->cls = incoming;
  }
  else if (incomingUBX->counter == 1)
  {
    incomingUBX->id = incoming;
  }
  else if (incomingUBX->counter == 2) //Len LSB
  {
    incomingUBX->len = incoming;
  }
  else if (incomingUBX->counter == 3) //Len MSB
  {
    incomingUBX->len |= incoming << 8;
  }
  else if (incomingUBX->counter == incomingUBX->len + 4) //ChecksumA
  {
    incomingUBX->checksumA = incoming;
  }
  else if (incomingUBX->counter == incomingUBX->len + 5) //ChecksumB
  {
    incomingUBX->checksumB = incoming;

    currentSentence = NONE; //We're done! Reset the sentence to being looking for a new start char

    //Validate this sentence
    if (incomingUBX->checksumA == rollingChecksumA && incomingUBX->checksumB == rollingChecksumB)
    {
      incomingUBX->valid = true;

      if (_printDebug == true)
      {
        printf("Incoming: Size: %d Received: ", incomingUBX->len);
        printPacket(incomingUBX);

        if (packetCfg.valid == true)
          debugPrintln("packetCfg now valid");
        if (packetAck.valid == true)
          debugPrintln("packetAck now valid");
      }

      processUBXpacket(incomingUBX); //We've got a valid packet, now do something with it
    }
    else
    {
      if (_printDebug == true)
      {
        printf("Checksum failed:");
        printf(" checksumA: %#x", incomingUBX->checksumA);
        printf(" checksumB: %#x", incomingUBX->checksumB);
        printf(" rollingChecksumA: %#x", rollingChecksumA);
        printf(" rollingChecksumB: %#x\r\n", rollingChecksumB);
        printf("Failed: Size: %d Received: ", incomingUBX->len);
        printPacket(incomingUBX);
      }
    }
  }
  else //Load this byte into the payload array
  {
    //If a UBX_NAV_PVT packet comes in asynchronously, we need to fudge the startingSpot
    uint16_t startingSpot = incomingUBX->startingSpot;
    if (incomingUBX->cls == UBX_CLASS_NAV && incomingUBX->id == UBX_NAV_PVT)
      startingSpot = 0;
    //Begin recording if counter goes past startingSpot
    if ((incomingUBX->counter - 4) >= startingSpot)
    {
      //Check to see if we have room for this byte
      if (((incomingUBX->counter - 4) - startingSpot) < MAX_PAYLOAD_SIZE)         //If counter = 208, starting spot = 200, we're good to record.
        incomingUBX->payload[incomingUBX->counter - 4 - startingSpot] = incoming; //Store this byte into payload array
    }
  }

  incomingUBX->counter++;
  if (incomingUBX->counter == MAX_PAYLOAD_SIZE)
  {
    //Something has gone very wrong
    currentSentence = NONE; //Reset the sentence to being looking for a new start char
    if (_printDebug == true)
    {
      printf("processUBX: counter hit MAX_PAYLOAD_SIZE\r\n");
    }
  }
}

//Once a packet has been received and validated, identify this packet's class/id and update internal flags
void Ublox_GPS::processUBXpacket(ubxPacket *msg)
{
  switch (msg->cls)
  {
  case UBX_CLASS_ACK:
    //We don't want to store ACK packets, just set commandAck flag
    if (msg->id == UBX_ACK_ACK && msg->payload[0] == packetCfg.cls && msg->payload[1] == packetCfg.id)
    {
      //The ack we just received matched the CLS/ID of last packetCfg sent (or received)
      debugPrintln("UBX ACK: Command sent/ack'd successfully");
      commandAck = UBX_ACK_ACK;
    }
    else if (msg->id == UBX_ACK_NACK && msg->payload[0] == packetCfg.cls && msg->payload[1] == packetCfg.id)
    {
      //The ack we just received matched the CLS/ID of last packetCfg sent
      debugPrintln("UBX ACK: Not-Acknowledged");
      commandAck = UBX_ACK_NACK;
    }
    break;

  case UBX_CLASS_NAV:
    if (msg->id == UBX_NAV_PVT && msg->len == 92)
    {
      //Parse various byte fields into global vars
      constexpr int startingSpot = 0; //fixed value used in processUBX

      timeOfWeek = extractLong(0);
      gpsMillisecond = extractLong(0) % 1000; //Get last three digits of iTOW
      gpsYear = extractInt(4);
      gpsMonth = extractByte(6);
      gpsDay = extractByte(7);
      gpsHour = extractByte(8);
      gpsMinute = extractByte(9);
      gpsSecond = extractByte(10);
      gpsNanosecond = extractLong(16); //Includes milliseconds

      fixType = extractByte(20 - startingSpot);
      carrierSolution = extractByte(21 - startingSpot) >> 6; //Get 6th&7th bits of this byte
      SIV = extractByte(23 - startingSpot);
      longitude = extractLong(24 - startingSpot);
      latitude = extractLong(28 - startingSpot);
      altitude = extractLong(32 - startingSpot);
      altitudeMSL = extractLong(36 - startingSpot);
      verticalVelocity = extractLong(56 - startingSpot);
      groundSpeed = extractLong(60 - startingSpot);
      headingOfMotion = extractLong(64 - startingSpot);
      pDOP = extractLong(76 - startingSpot);

      //Mark all datums as fresh (not read before)
      moduleQueried.gpsiTOW = true;
      moduleQueried.gpsYear = true;
      moduleQueried.gpsMonth = true;
      moduleQueried.gpsDay = true;
      moduleQueried.gpsHour = true;
      moduleQueried.gpsMinute = true;
      moduleQueried.gpsSecond = true;
      moduleQueried.gpsNanosecond = true;

      moduleQueried.all = true;
      moduleQueried.longitude = true;
      moduleQueried.latitude = true;
      moduleQueried.altitude = true;
      moduleQueried.altitudeMSL = true;
      moduleQueried.SIV = true;
      moduleQueried.fixType = true;
      moduleQueried.carrierSolution = true;
      moduleQueried.verticalVelocity = true;
      moduleQueried.groundSpeed = true;
      moduleQueried.headingOfMotion = true;
      moduleQueried.pDOP = true;
    }
    else if (msg->id == UBX_NAV_HPPOSLLH && msg->len == 36)
    {
      timeOfWeek = extractLong(4);
      highResLongitude = extractLong(8);
      highResLatitude = extractLong(12);
      ellipsoid = extractLong(16);
      meanSeaLevel = extractLong(20);
      geoidSeparation = extractLong(24);
      horizontalAccuracy = extractLong(28);
      verticalAccuracy = extractLong(32);

      highResModuleQueried.all = true;
      highResModuleQueried.highResLatitude = true;
      highResModuleQueried.highResLongitude = true;
      highResModuleQueried.ellipsoid = true;
      highResModuleQueried.meanSeaLevel = true;
      highResModuleQueried.geoidSeparation = true;
      highResModuleQueried.horizontalAccuracy = true;
      highResModuleQueried.verticalAccuracy = true;
      moduleQueried.gpsiTOW = true; // this can arrive via HPPOS too.

      if (_printDebug == true)
      {
        printf("Sec: %f ", ((float)extractLong(4)) / 1000.0f);
        printf("LON: %f ", ((float)(int32_t)extractLong(8)) / 10000000.0f);
        printf("LAT: %f\r\n", ((float)(int32_t)extractLong(12)) / 10000000.0f);
        printf("ELI M: %f ", ((float)(int32_t)extractLong(16)) / 1000.0f);
        printf("MSL M: %f ", ((float)(int32_t)extractLong(20)) / 1000.0f);
        printf("GEO: %f\r\n", ((float)(int32_t)extractLong(24)) / 1000.0f);
        printf("HA 2D M: %f", ((float)extractLong(28)) / 1000.0f);
        printf("VERT M: %f\r\n", ((float)extractLong(32)) / 1000.0f);
      }
    }
    break;
  }
}

//Given a packet and payload, send everything including CRC bytes via I2C port
UbloxStatus_e Ublox_GPS::sendCommand(ubxPacket outgoingUBX, uint16_t maxWait)
{
  UbloxStatus_e retVal = UBLOX_STATUS_SUCCESS;

  calcChecksum(&outgoingUBX); //Sets checksum A and B bytes of the packet

  if (_printDebug == true)
  {
    printf("\nSending: ");
    printPacket(&outgoingUBX);
  }

  if (commType == COMM_TYPE_I2C)
  {
    retVal = sendI2cCommand(outgoingUBX, maxWait);
    if (retVal != UBLOX_STATUS_SUCCESS)
    {
      debugPrintln("Send I2C Command failed");
      return retVal;
    }
  }
  else if (commType == COMM_TYPE_SERIAL)
  {
    debugPrintln("sendCommand: Serial interface not yet implemented");
    //sendSerialCommand(outgoingUBX);
  }

  if (maxWait > 0)
  {
    //Depending on what we just sent, either we need to look for an ACK or not
    if (outgoingUBX.cls == UBX_CLASS_CFG)
    {
      debugPrintln("sendCommand: Waiting for ACK response");
      retVal = waitForACKResponse(outgoingUBX.cls, outgoingUBX.id, maxWait); //Wait for Ack response
    }
    else
    {
      debugPrintln("sendCommand: Waiting for No ACK response");
      retVal = waitForNoACKResponse(outgoingUBX.cls, outgoingUBX.id, maxWait); //Wait for Ack response
    }
  }
  return retVal;
}



//Returns false if sensor fails to respond to I2C traffic
UbloxStatus_e Ublox_GPS::sendI2cCommand(ubxPacket outgoingUBX, uint16_t maxWait)
{
  char buf[MAX_PAYLOAD_SIZE];
  char nak;
  char addr = _gpsI2Caddress<<1; // We need 8-bit version of address
  _i2c->lock(); // Lock I2C connection during this method

  //Point at 0xFF data register
  buf[0] = 0xFF;
  nak = _i2c->write(addr, buf, 1);
  if (nak) {
    _i2c->unlock();
    return UBLOX_STATUS_I2C_COMM_FAILURE;
  }

  //Write header bytes
  buf[0] = UBX_SYNCH_1;
  buf[1] = UBX_SYNCH_2;
  buf[2] = outgoingUBX.cls;
  buf[3] = outgoingUBX.id;
  buf[4] = outgoingUBX.len & 0xFF;
  buf[5] = outgoingUBX.len >> 8;
  nak = _i2c->write(addr, buf, 6, false);  
  if (nak) {
    _i2c->unlock();
    return UBLOX_STATUS_I2C_COMM_FAILURE;
  }

  //Write payload. Limit the sends into 32 byte chunks
  //This code based on ublox: https://forum.u-blox.com/index.php/20528/how-to-use-i2c-to-get-the-nmea-frames
  uint16_t bytesToSend = outgoingUBX.len;

  //"The number of data bytes must be at least 2 to properly distinguish
  //from the write access to set the address counter in random read accesses."
  uint16_t startSpot = 0;
  while (bytesToSend > 1)
  {
    uint8_t len = bytesToSend;
    if (len > I2C_BUFFER_LENGTH)
      len = I2C_BUFFER_LENGTH;

    for (uint16_t x = 0; x < len; x++) {
      buf[x] = outgoingUBX.payload[startSpot + x];
    }
    nak = _i2c->write(addr, buf, len, false);  
    if (nak) {
      _i2c->unlock();
      return UBLOX_STATUS_I2C_COMM_FAILURE;
    }

    //*outgoingUBX.payload += len; //Move the pointer forward
    startSpot += len; //Move the pointer forward
    bytesToSend -= len;
  }

  //Write checksum
  if (bytesToSend == 1) { // include trailing payload byte if exists
    buf[0] = outgoingUBX.payload[startSpot];
    buf[1] = outgoingUBX.checksumA;
    buf[2] = outgoingUBX.checksumB;
    nak = _i2c->write(addr, buf, 3);
  } else {
    buf[0] = outgoingUBX.checksumA;
    buf[1] = outgoingUBX.checksumB;
    nak = _i2c->write(addr, buf, 2);
  }
  _i2c->unlock();
  if (nak) {
    return UBLOX_STATUS_I2C_COMM_FAILURE;
  } else {
    return UBLOX_STATUS_SUCCESS;
  }
}

//Returns true if I2C device ack's
bool Ublox_GPS::isConnected()
{
  if (commType == COMM_TYPE_I2C)
  {
    char addr = _gpsI2Caddress<<1; // Create 8-bit version of address
    char buf[1];
    char nak;
    nak = _i2c->write(addr, buf, 0); // Does a zero byte "write" achieve goal?
    return (nak == 0);
  }
  else if (commType == COMM_TYPE_SERIAL)
  {
    // Query navigation rate to see whether we get a meaningful response
    // packetCfg.cls = UBX_CLASS_CFG;
    // packetCfg.id = UBX_CFG_RATE;
    // packetCfg.len = 0;
    // packetCfg.startingSpot = 0;

    // return sendCommand(packetCfg);
    return false; // Not implemented yet
  }
  return false;
}

//Given a message, calc and store the two byte "8-Bit Fletcher" checksum over the entirety of the message
//This is called before we send a command message
void Ublox_GPS::calcChecksum(ubxPacket *msg)
{
  msg->checksumA = 0;
  msg->checksumB = 0;

  msg->checksumA += msg->cls;
  msg->checksumB += msg->checksumA;

  msg->checksumA += msg->id;
  msg->checksumB += msg->checksumA;

  msg->checksumA += (msg->len & 0xFF);
  msg->checksumB += msg->checksumA;

  msg->checksumA += (msg->len >> 8);
  msg->checksumB += msg->checksumA;

  for (uint16_t i = 0; i < msg->len; i++)
  {
    msg->checksumA += msg->payload[i];
    msg->checksumB += msg->checksumA;
  }
}

//Given a message and a byte, add to rolling "8-Bit Fletcher" checksum
//This is used when receiving messages from module
void Ublox_GPS::addToChecksum(uint8_t incoming)
{
  rollingChecksumA += incoming;
  rollingChecksumB += rollingChecksumA;
}

//Pretty prints the current ubxPacket
void Ublox_GPS::printPacket(ubxPacket *packet)
{
  if (_printDebug == true)
  {
    printf("CLS:");
    if (packet->cls == UBX_CLASS_NAV) //1
      printf("NAV");
    else if (packet->cls == UBX_CLASS_ACK) //5
      printf("ACK");
    else if (packet->cls == UBX_CLASS_CFG) //6
      printf("CFG");
    else if (packet->cls == UBX_CLASS_MON) //0x0A
      printf("MON");
    else
      printf("%#x", packet->cls);

    printf(" ID:");
    if (packet->cls == UBX_CLASS_NAV && packet->id == UBX_NAV_PVT)
      printf("PVT");
    else if (packet->cls == UBX_CLASS_CFG && packet->id == UBX_CFG_RATE)
      printf("RATE");
    else if (packet->cls == UBX_CLASS_CFG && packet->id == UBX_CFG_CFG)
      printf("SAVE");
    else
      printf("%#x",packet->id);

    printf(" Len: %#x", packet->len);

    printf(" Payload:");
    for (int x = 0; x < packet->len; x++)
    {
      printf(" %x", packet->payload[x]);
    }
    printf("\r\n");
  }
}


//=-=-=-=-=-=-=-= Specific commands =-=-=-=-=-=-=-==-=-=-=-=-=-=-=
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//When messages from the class CFG are sent to the receiver, the receiver will send an "acknowledge"(UBX - ACK - ACK) or a
//"not acknowledge"(UBX-ACK-NAK) message back to the sender, depending on whether or not the message was processed correctly.
//Some messages from other classes also use the same acknowledgement mechanism.

//If the packetCfg len is 1, then we are querying the device for data
//If the packetCfg len is >1, then we are sending a new setting

//Returns true if we got the following:
//* If packetCfg len is 1 and we got and ACK and a valid packetCfg (module is responding with register content)
//* If packetCfg len is >1 and we got an ACK (no valid packetCfg needed, module absorbs new register data)
//Returns false if we timed out, got a NACK (command unknown), or had a CLS/ID mismatch
UbloxStatus_e Ublox_GPS::waitForACKResponse(uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
  commandAck = UBX_ACK_NONE; //Reset flag
  packetCfg.valid = false;   //This will go true when we receive a response to the packet we sent
  packetAck.valid = false;

  _pollingTimer.reset();
  while (_pollingTimer.elapsed_time() < std::chrono::milliseconds(maxTime))
  {
    if (checkUblox() == true) //See if new data is available. Process bytes as they come in.
    {
      //First we verify the ACK. commandAck will only go UBX_ACK_ACK if CLS/ID matches
      if (commandAck == UBX_ACK_ACK)
      {
        if (_printDebug == true)
        {
          printf("waitForACKResponse: ACK received after %d ms\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
        }

        //Are we expecting data back or just an ACK?
        if (packetCfg.len == 1)
        {
          //We are expecting a data response so now we verify the response packet was valid
          if (packetCfg.valid == true)
          {
            if (packetCfg.cls == requestedClass && packetCfg.id == requestedID)
            {
              if (_printDebug == true)
              {
                printf("waitForACKResponse: CLS/ID match after %d ms\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
              }
              return (UBLOX_STATUS_DATA_RECEIVED); //Received a data and a correct ACK!
            }
            else
            {
              //Reset packet and continue checking incoming data for matching cls/id
              debugPrintln("waitForACKResponse: CLS/ID mismatch, continue to wait...");
              packetCfg.valid = false; //This will go true when we receive a response to the packet we sent
            }
          }
          else
          {
            //We were expecting data but didn't get a valid config packet
            debugPrintln("waitForACKResponse: Invalid config packet");
            return (UBLOX_STATUS_FAIL); //We got an ACK, we're never going to get valid config data
          }
        }
        else
        {
          //We have sent new data. We expect an ACK but no return config packet.
          debugPrintln("waitForACKResponse: New data successfully sent");
          return (UBLOX_STATUS_DATA_SENT); //New data successfully sent
        }
      }
      else if (commandAck == UBX_ACK_NACK)
      {
        if (_printDebug == true)
        {
          printf("waitForACKResponse: NACK received after %d ms\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
        }
        return (UBLOX_STATUS_COMMAND_UNKNOWN); //Received a NACK
      }
    } //checkUblox == true

    wait_us(500);
  } //while (_pollingTimer.elapsed_time() < std::chrono::milliseconds(maxTime))

  //TODO add check here if config went valid but we never got the following ack
  //Through debug warning, This command might not get an ACK
  if (packetCfg.valid == true)
  {
    debugPrintln("waitForACKResponse: Config was valid but ACK not received");
  }

  if (_printDebug == true)
  {
    printf("waitForACKResponse: timeout after %d ms. No ack packet received.\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
  }

  return (UBLOX_STATUS_TIMEOUT);
}

//For non-CFG queries no ACK is sent so we use this function
//Returns true if we got a config packet full of response data that has CLS/ID match to our query packet
//Returns false if we timed out
UbloxStatus_e Ublox_GPS::waitForNoACKResponse(uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
  packetCfg.valid = false; //This will go true when we receive a response to the packet we sent
  packetAck.valid = false;
  packetCfg.cls = 255;
  packetCfg.id = 255;

  _pollingTimer.reset();
  while (_pollingTimer.elapsed_time() < std::chrono::milliseconds(maxTime))
  {
    if (checkUblox() == true) //See if new data is available. Process bytes as they come in.
    {
      //Did we receive a config packet that matches the cls/id we requested?
      if (packetCfg.cls == requestedClass && packetCfg.id == requestedID)
      {
        //This packet might be good or it might be CRC corrupt
        if (packetCfg.valid == true)
        {
          if (_printDebug == true)
          {
            printf("waitForNoACKResponse: CLS/ID match after %d ms\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
          }
          return (UBLOX_STATUS_DATA_RECEIVED); //We have new data to act upon
        }
        else
        {
          debugPrintln("waitForNoACKResponse: CLS/ID match but failed CRC");
          return (UBLOX_STATUS_CRC_FAIL); //We got the right packet but it was corrupt
        }
      }
      else if (packetCfg.cls < 255 && packetCfg.id < 255)
      {
        //Reset packet and continue checking incoming data for matching cls/id
        if (_printDebug == true)
        {
          printf("waitForNoACKResponse: CLS/ID mismatch: ");
          printf("CLS: %#x", packetCfg.cls);
          printf(" ID: %#x\r\n", packetCfg.id);
        }

        packetCfg.valid = false; //This will go true when we receive a response to the packet we sent
        packetCfg.cls = 255;
        packetCfg.id = 255;
      }
    }

    wait_us(500);
  }

  if (_printDebug == true)
  {
    printf("waitForNoACKResponse: timeout after %d ms. No packet received.\r\n", std::chrono::duration_cast<milliseconds>(_pollingTimer.elapsed_time()).count());
  }

  return (UBLOX_STATUS_TIMEOUT);
}

//Save current configuration to flash and BBR (battery backed RAM)
//This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool Ublox_GPS::saveConfiguration(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_CFG;
  packetCfg.len = 12;
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  packetCfg.payload[4] = 0xFF; //Set any bit in the saveMask field to save current config to Flash and BBR

  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  return (true);
}

//Reset module to factory defaults
//This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool Ublox_GPS::factoryDefault(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_CFG;
  packetCfg.len = 12;
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  packetCfg.payload[0] = 0xFF; //Set any bit in the clearMask field to clear saved config
  packetCfg.payload[8] = 0xFF; //Set any bit in the loadMask field to discard current config and rebuild from lower non-volatile memory layers

  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  return (true);
}


//Given a group, ID and size, return the value of this config spot
//The 32-bit key is put together from group/ID/size. See other getVal to send key directly.
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::getVal8(uint16_t group, uint16_t id, uint8_t size, uint8_t layer, uint16_t maxWait)
{
  //Create key
  uint32_t key = 0;
  key |= (uint32_t)id;
  key |= (uint32_t)group << 16;
  key |= (uint32_t)size << 28;

  if (_printDebug == true)
  {
    printf("key: %#lx\r\n", key);
  }

  return getVal8(key, layer, maxWait);
}

//Given a key, return its value
//This function takes a full 32-bit key
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::getVal8(uint32_t key, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALGET;
  packetCfg.len = 4 + 4 * 1; //While multiple keys are allowed, we will send only one key at a time
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  if (_printDebug == true)
  {
    printf("key: %#lx\r\n", key);
  }

  //Send VALGET command with this key
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //Verify the response is the correct length as compared to what the user called (did the module respond with 8-bits but the user called getVal32?)
  //Response is 8 bytes plus cfg data
  //if(packet->len > 8+1)

  //Pull the requested value from the response
  //Response starts at 4+1*N with the 32-bit key so the actual data we're looking for is at 8+1*N
  return (extractByte(8));
}

//Given a key, set a 16-bit value
//This function takes a full 32-bit key
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::setVal(uint32_t key, uint16_t value, uint8_t layer, uint16_t maxWait)
{
  return setVal16(key, value, layer, maxWait);
}

//Given a key, set a 16-bit value
//This function takes a full 32-bit key
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::setVal16(uint32_t key, uint16_t value, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 2; //4 byte header, 4 byte key ID, 2 bytes of value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value >> 8 * 0; //Value LSB
  payloadCfg[9] = value >> 8 * 1;

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Given a key, set an 8-bit value
//This function takes a full 32-bit key
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::setVal8(uint32_t key, uint8_t value, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 1; //4 byte header, 4 byte key ID, 1 byte value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value; //Value

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Given a key, set a 32-bit value
//This function takes a full 32-bit key
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::setVal32(uint32_t key, uint32_t value, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 4; //4 byte header, 4 byte key ID, 4 bytes of value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value >> 8 * 0; //Value LSB
  payloadCfg[9] = value >> 8 * 1;
  payloadCfg[10] = value >> 8 * 2;
  payloadCfg[11] = value >> 8 * 3;

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Start defining a new UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 32-bit value
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::newCfgValset32(uint32_t key, uint32_t value, uint8_t layer)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 4; //4 byte header, 4 byte key ID, 4 bytes of value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < MAX_PAYLOAD_SIZE; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value >> 8 * 0; //Value LSB
  payloadCfg[9] = value >> 8 * 1;
  payloadCfg[10] = value >> 8 * 2;
  payloadCfg[11] = value >> 8 * 3;

  //All done
  return (true);
}

//Start defining a new UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 16-bit value
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::newCfgValset16(uint32_t key, uint16_t value, uint8_t layer)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 2; //4 byte header, 4 byte key ID, 2 bytes of value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < MAX_PAYLOAD_SIZE; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value >> 8 * 0; //Value LSB
  payloadCfg[9] = value >> 8 * 1;

  //All done
  return (true);
}

//Start defining a new UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 8-bit value
//Default layer is BBR
//Configuration of modern Ublox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
uint8_t Ublox_GPS::newCfgValset8(uint32_t key, uint8_t value, uint8_t layer)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + 1; //4 byte header, 4 byte key ID, 1 byte value
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint16_t x = 0; x < MAX_PAYLOAD_SIZE; x++)
    packetCfg.payload[x] = 0;

  payloadCfg[0] = 0;     //Message Version - set to 0
  payloadCfg[1] = layer; //By default we ask for the BBR layer

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //Load user's value
  payloadCfg[8] = value; //Value

  //All done
  return (true);
}

//Add another keyID and value to an existing UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 32-bit value
uint8_t Ublox_GPS::addCfgValset32(uint32_t key, uint32_t value)
{
  //Load key into outgoing payload
  payloadCfg[packetCfg.len + 0] = key >> 8 * 0; //Key LSB
  payloadCfg[packetCfg.len + 1] = key >> 8 * 1;
  payloadCfg[packetCfg.len + 2] = key >> 8 * 2;
  payloadCfg[packetCfg.len + 3] = key >> 8 * 3;

  //Load user's value
  payloadCfg[packetCfg.len + 4] = value >> 8 * 0; //Value LSB
  payloadCfg[packetCfg.len + 5] = value >> 8 * 1;
  payloadCfg[packetCfg.len + 6] = value >> 8 * 2;
  payloadCfg[packetCfg.len + 7] = value >> 8 * 3;

  //Update packet length: 4 byte key ID, 4 bytes of value
  packetCfg.len = packetCfg.len + 4 + 4;

  //All done
  return (true);
}

//Add another keyID and value to an existing UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 16-bit value
uint8_t Ublox_GPS::addCfgValset16(uint32_t key, uint16_t value)
{
  //Load key into outgoing payload
  payloadCfg[packetCfg.len + 0] = key >> 8 * 0; //Key LSB
  payloadCfg[packetCfg.len + 1] = key >> 8 * 1;
  payloadCfg[packetCfg.len + 2] = key >> 8 * 2;
  payloadCfg[packetCfg.len + 3] = key >> 8 * 3;

  //Load user's value
  payloadCfg[packetCfg.len + 4] = value >> 8 * 0; //Value LSB
  payloadCfg[packetCfg.len + 5] = value >> 8 * 1;

  //Update packet length: 4 byte key ID, 2 bytes of value
  packetCfg.len = packetCfg.len + 4 + 2;

  //All done
  return (true);
}

//Add another keyID and value to an existing UBX-CFG-VALSET ubxPacket
//This function takes a full 32-bit key and 8-bit value
uint8_t Ublox_GPS::addCfgValset8(uint32_t key, uint8_t value)
{
  //Load key into outgoing payload
  payloadCfg[packetCfg.len + 0] = key >> 8 * 0; //Key LSB
  payloadCfg[packetCfg.len + 1] = key >> 8 * 1;
  payloadCfg[packetCfg.len + 2] = key >> 8 * 2;
  payloadCfg[packetCfg.len + 3] = key >> 8 * 3;

  //Load user's value
  payloadCfg[packetCfg.len + 4] = value; //Value

  //Update packet length: 4 byte key ID, 1 byte value
  packetCfg.len = packetCfg.len + 4 + 1;

  //All done
  return (true);
}

//Add a final keyID and value to an existing UBX-CFG-VALSET ubxPacket and send it
//This function takes a full 32-bit key and 32-bit value
uint8_t Ublox_GPS::sendCfgValset32(uint32_t key, uint32_t value, uint16_t maxWait)
{
  //Load keyID and value into outgoing payload
  addCfgValset32(key, value);

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Add a final keyID and value to an existing UBX-CFG-VALSET ubxPacket and send it
//This function takes a full 32-bit key and 16-bit value
uint8_t Ublox_GPS::sendCfgValset16(uint32_t key, uint16_t value, uint16_t maxWait)
{
  //Load keyID and value into outgoing payload
  addCfgValset16(key, value);

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Add a final keyID and value to an existing UBX-CFG-VALSET ubxPacket and send it
//This function takes a full 32-bit key and 8-bit value
uint8_t Ublox_GPS::sendCfgValset8(uint32_t key, uint8_t value, uint16_t maxWait)
{
  //Load keyID and value into outgoing payload
  addCfgValset8(key, value);

  //Send VALSET command with this key and value
  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //All done
  return (true);
}

//Get the current TimeMode3 settings - these contain survey in statuses
bool Ublox_GPS::getSurveyMode(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_TMODE3;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  return (sendCommand(packetCfg, maxWait));
}

//Control Survey-In for NEO-M8P
bool Ublox_GPS::setSurveyMode(uint8_t mode, uint16_t observationTime, float requiredAccuracy, uint16_t maxWait)
{
  if (getSurveyMode(maxWait) == false) //Ask module for the current TimeMode3 settings. Loads into payloadCfg.
    return (false);

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_TMODE3;
  packetCfg.len = 40;
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  //payloadCfg should be loaded with poll response. Now modify only the bits we care about
  payloadCfg[2] = mode; //Set mode. Survey-In and Disabled are most common.

  payloadCfg[24] = observationTime & 0xFF; //svinMinDur in seconds
  payloadCfg[25] = observationTime >> 8;   //svinMinDur in seconds

  uint32_t svinAccLimit = requiredAccuracy * 10000; //Convert m to 0.1mm
  payloadCfg[28] = svinAccLimit & 0xFF;             //svinAccLimit in 0.1mm increments
  payloadCfg[29] = svinAccLimit >> 8;
  payloadCfg[30] = svinAccLimit >> 16;

  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Begin Survey-In for NEO-M8P
bool Ublox_GPS::enableSurveyMode(uint16_t observationTime, float requiredAccuracy, uint16_t maxWait)
{
  return (setSurveyMode(SVIN_MODE_ENABLE, observationTime, requiredAccuracy, maxWait));
}

//Stop Survey-In for NEO-M8P
bool Ublox_GPS::disableSurveyMode(uint16_t maxWait)
{
  return (setSurveyMode(SVIN_MODE_DISABLE, 0, 0, maxWait));
}

//Reads survey in status and sets the global variables
//for status, position valid, observation time, and mean 3D StdDev
//Returns true if commands was successful
bool Ublox_GPS::getSurveyStatus(uint16_t maxWait)
{
  //Reset variables
  svin.active = false;
  svin.valid = false;
  svin.observationTime = 0;
  svin.meanAccuracy = 0;

  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_SVIN;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //We got a response, now parse the bits into the svin structure
  svin.observationTime = extractLong(8);

  uint32_t tempFloat = extractLong(28);
  svin.meanAccuracy = tempFloat / 10000.0; //Convert 0.1mm to m

  svin.valid = payloadCfg[36];
  svin.active = payloadCfg[37]; //1 if survey in progress, 0 otherwise

  return (true);
}

//Loads the payloadCfg array with the current protocol bits located the UBX-CFG-PRT register for a given port
bool Ublox_GPS::getPortSettings(uint8_t portID, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 1;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = portID;

  return (sendCommand(packetCfg, maxWait));
}

//Configure a given port to output UBX, NMEA, RTCM3 or a combination thereof
//Port 0=I2c, 1=UART1, 2=UART2, 3=USB, 4=SPI
//Bit:0 = UBX, :1=NMEA, :5=RTCM3
bool Ublox_GPS::setPortOutput(uint8_t portID, uint8_t outStreamSettings, uint16_t maxWait)
{
  //Get the current config values for this port ID
  if (getPortSettings(portID, maxWait) == false)
    return (false); //Something went wrong. Bail.

  if (commandAck != UBX_ACK_ACK)
  {
    debugPrintln("setPortOutput failed to ACK");
    return (false);
  }

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 20;
  packetCfg.startingSpot = 0;

  //payloadCfg is now loaded with current bytes. Change only the ones we need to
  payloadCfg[14] = outStreamSettings; //OutProtocolMask LSB - Set outStream bits

  return (sendCommand(packetCfg, maxWait));
}

//Configure a given port to input UBX, NMEA, RTCM3 or a combination thereof
//Port 0=I2c, 1=UART1, 2=UART2, 3=USB, 4=SPI
//Bit:0 = UBX, :1=NMEA, :5=RTCM3
bool Ublox_GPS::setPortInput(uint8_t portID, uint8_t inStreamSettings, uint16_t maxWait)
{
  //Get the current config values for this port ID
  //This will load the payloadCfg array with current port settings
  if (getPortSettings(portID, maxWait) == false)
    return (false); //Something went wrong. Bail.

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_PRT;
  packetCfg.len = 20;
  packetCfg.startingSpot = 0;

  //payloadCfg is now loaded with current bytes. Change only the ones we need to
  payloadCfg[12] = inStreamSettings; //InProtocolMask LSB - Set inStream bits

  return (sendCommand(packetCfg, maxWait));
}

//Configure a port to output UBX, NMEA, RTCM3 or a combination thereof
bool Ublox_GPS::setI2COutput(uint8_t comSettings, uint16_t maxWait)
{
  return (setPortOutput(COM_PORT_I2C, comSettings, maxWait));
}
// boolean SFE_UBLOX_GPS::setUART1Output(uint8_t comSettings, uint16_t maxWait)
// {
//   return (setPortOutput(COM_PORT_UART1, comSettings, maxWait));
// }
// boolean SFE_UBLOX_GPS::setUART2Output(uint8_t comSettings, uint16_t maxWait)
// {
//   return (setPortOutput(COM_PORT_UART2, comSettings, maxWait));
// }
// boolean SFE_UBLOX_GPS::setUSBOutput(uint8_t comSettings, uint16_t maxWait)
// {
//   return (setPortOutput(COM_PORT_USB, comSettings, maxWait));
// }
// boolean SFE_UBLOX_GPS::setSPIOutput(uint8_t comSettings, uint16_t maxWait)
// {
//   return (setPortOutput(COM_PORT_SPI, comSettings, maxWait));
// }

//Set the rate at which the module will give us an updated navigation solution
//Expects a number that is the updates per second. For example 1 = 1Hz, 2 = 2Hz, etc.
//Max is 40Hz(?!)
bool Ublox_GPS::setNavigationFrequency(uint8_t navFreq, uint16_t maxWait)
{

  //Adjust the I2C polling timeout based on update rate
  i2cPollingWait = std::chrono::milliseconds(1000 / (navFreq * 4)); //This is the time to wait between checks for new I2C data

  //Query the module for the latest lat/long
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RATE;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false) //This will load the payloadCfg array with current settings of the given register
    return (false);                             //If command send fails then bail

  uint16_t measurementRate = 1000 / navFreq;

  //payloadCfg is now loaded with current bytes. Change only the ones we need to
  payloadCfg[0] = measurementRate & 0xFF; //measRate LSB
  payloadCfg[1] = measurementRate >> 8;   //measRate MSB

  return (sendCommand(packetCfg, maxWait));
}

//Get the rate at which the module is outputting nav solutions
uint8_t Ublox_GPS::getNavigationFrequency(uint16_t maxWait)
{
  //Query the module for the latest lat/long
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RATE;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false) //This will load the payloadCfg array with current settings of the given register
    return (0);                                 //If command send fails then bail

  uint16_t measurementRate = 0;

  //payloadCfg is now loaded with current bytes. Get what we need
  measurementRate = extractInt(0); //Pull from payloadCfg at measRate LSB

  measurementRate = 1000 / measurementRate; //This may return an int when it's a float, but I'd rather not return 4 bytes
  return (measurementRate);
}

//In case no config access to the GPS is possible and PVT is send cyclically already
//set config to suitable parameters
bool Ublox_GPS::assumeAutoPVT(bool enabled, bool implicitUpdate)
{
  bool changes = autoPVT != enabled || autoPVTImplicitUpdate != implicitUpdate;
  if (changes)
  {
    autoPVT = enabled;
    autoPVTImplicitUpdate = implicitUpdate;
  }
  return changes;
}

//Enable or disable automatic navigation message generation by the GPS. This changes the way getPVT
//works.
bool Ublox_GPS::setAutoPVT(bool enable, uint16_t maxWait)
{
  return setAutoPVT(enable, true, maxWait);
}

//Enable or disable automatic navigation message generation by the GPS. This changes the way getPVT
//works.
bool Ublox_GPS::setAutoPVT(bool enable, bool implicitUpdate, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_MSG;
  packetCfg.len = 3;
  packetCfg.startingSpot = 0;
  payloadCfg[0] = UBX_CLASS_NAV;
  payloadCfg[1] = UBX_NAV_PVT;
  payloadCfg[2] = enable ? 1 : 0; // rate relative to navigation freq.

  bool ok = sendCommand(packetCfg, maxWait);
  if (ok)
  {
    autoPVT = enable;
    autoPVTImplicitUpdate = implicitUpdate;
  }
  moduleQueried.all = false;
  return ok;
}

//Configure a given message type for a given port (UART1, I2C, SPI, etc)
bool Ublox_GPS::configureMessage(uint8_t msgClass, uint8_t msgID, uint8_t portID, uint8_t sendRate, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_MSG;
  packetCfg.len = 8;
  packetCfg.startingSpot = 0;

  //Clear packet payload
  for (uint8_t x = 0; x < packetCfg.len; x++)
    packetCfg.payload[x] = 0;

  packetCfg.payload[0] = msgClass;
  packetCfg.payload[1] = msgID;
  packetCfg.payload[2 + portID] = sendRate; //Send rate is relative to the event a message is registered on. For example, if the rate of a navigation message is set to 2, the message is sent every 2nd navigation solution.

  return (sendCommand(packetCfg, maxWait));
}

//Enable a given message type, default of 1 per update rate (usually 1 per second)
bool Ublox_GPS::enableMessage(uint8_t msgClass, uint8_t msgID, uint8_t portID, uint8_t rate, uint16_t maxWait)
{
  return (configureMessage(msgClass, msgID, portID, rate, maxWait));
}

//Disable a given message type on a given port
bool Ublox_GPS::disableMessage(uint8_t msgClass, uint8_t msgID, uint8_t portID, uint16_t maxWait)
{
  return (configureMessage(msgClass, msgID, portID, 0, maxWait));
}

bool Ublox_GPS::enableNMEAMessage(uint8_t msgID, uint8_t portID, uint8_t rate, uint16_t maxWait)
{
  return (configureMessage(UBX_CLASS_NMEA, msgID, portID, rate, maxWait));
}

bool Ublox_GPS::disableNMEAMessage(uint8_t msgID, uint8_t portID, uint16_t maxWait)
{
  return (enableNMEAMessage(msgID, portID, 0, maxWait));
}

//Given a message number turns on a message ID for output over a given portID (UART, I2C, SPI, USB, etc)
//To disable a message, set secondsBetween messages to 0
//Note: This function will return false if the message is already enabled
//For base station RTK output we need to enable various sentences

//NEO-M8P has four:
//1005 = 0xF5 0x05 - Stationary RTK reference ARP
//1077 = 0xF5 0x4D - GPS MSM7
//1087 = 0xF5 0x57 - GLONASS MSM7
//1230 = 0xF5 0xE6 - GLONASS code-phase biases, set to once every 10 seconds

//ZED-F9P has six:
//1005, 1074, 1084, 1094, 1124, 1230

//Much of this configuration is not documented and instead discerned from u-center binary console
bool Ublox_GPS::enableRTCMmessage(uint8_t messageNumber, uint8_t portID, uint8_t sendRate, uint16_t maxWait)
{
  return (configureMessage(UBX_RTCM_MSB, messageNumber, portID, sendRate, maxWait));
}

//Disable a given message on a given port by setting secondsBetweenMessages to zero
bool Ublox_GPS::disableRTCMmessage(uint8_t messageNumber, uint8_t portID, uint16_t maxWait)
{
  return (enableRTCMmessage(messageNumber, portID, 0, maxWait));
}

//Add a new geofence using UBX-CFG-GEOFENCE
bool Ublox_GPS::addGeofence(int32_t latitude, int32_t longitude, uint32_t radius, uint8_t confidence, uint8_t pinPolarity, uint8_t pin, uint16_t maxWait)
{
  if (currentGeofenceParams.numFences >= 4)
    return (false); // Quit if we already have four geofences defined

  // Store the new geofence parameters
  currentGeofenceParams.lats[currentGeofenceParams.numFences] = latitude;
  currentGeofenceParams.longs[currentGeofenceParams.numFences] = longitude;
  currentGeofenceParams.rads[currentGeofenceParams.numFences] = radius;
  currentGeofenceParams.numFences = currentGeofenceParams.numFences + 1; // Increment the number of fences

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_GEOFENCE;
  packetCfg.len = (currentGeofenceParams.numFences * 12) + 8;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0;                               // Message version = 0x00
  payloadCfg[1] = currentGeofenceParams.numFences; // numFences
  payloadCfg[2] = confidence;                      // confLvl = Confidence level 0-4 (none, 68%, 95%, 99.7%, 99.99%)
  payloadCfg[3] = 0;                               // reserved1
  if (pin > 0)
  {
    payloadCfg[4] = 1; // enable PIO combined fence state
  }
  else
  {
    payloadCfg[4] = 0; // disable PIO combined fence state
  }
  payloadCfg[5] = pinPolarity; // PIO pin polarity (0 = low means inside, 1 = low means outside (or unknown))
  payloadCfg[6] = pin;         // PIO pin
  payloadCfg[7] = 0;           //reserved2
  payloadCfg[8] = currentGeofenceParams.lats[0] & 0xFF;
  payloadCfg[9] = currentGeofenceParams.lats[0] >> 8;
  payloadCfg[10] = currentGeofenceParams.lats[0] >> 16;
  payloadCfg[11] = currentGeofenceParams.lats[0] >> 24;
  payloadCfg[12] = currentGeofenceParams.longs[0] & 0xFF;
  payloadCfg[13] = currentGeofenceParams.longs[0] >> 8;
  payloadCfg[14] = currentGeofenceParams.longs[0] >> 16;
  payloadCfg[15] = currentGeofenceParams.longs[0] >> 24;
  payloadCfg[16] = currentGeofenceParams.rads[0] & 0xFF;
  payloadCfg[17] = currentGeofenceParams.rads[0] >> 8;
  payloadCfg[18] = currentGeofenceParams.rads[0] >> 16;
  payloadCfg[19] = currentGeofenceParams.rads[0] >> 24;
  if (currentGeofenceParams.numFences >= 2)
  {
    payloadCfg[20] = currentGeofenceParams.lats[1] & 0xFF;
    payloadCfg[21] = currentGeofenceParams.lats[1] >> 8;
    payloadCfg[22] = currentGeofenceParams.lats[1] >> 16;
    payloadCfg[23] = currentGeofenceParams.lats[1] >> 24;
    payloadCfg[24] = currentGeofenceParams.longs[1] & 0xFF;
    payloadCfg[25] = currentGeofenceParams.longs[1] >> 8;
    payloadCfg[26] = currentGeofenceParams.longs[1] >> 16;
    payloadCfg[27] = currentGeofenceParams.longs[1] >> 24;
    payloadCfg[28] = currentGeofenceParams.rads[1] & 0xFF;
    payloadCfg[29] = currentGeofenceParams.rads[1] >> 8;
    payloadCfg[30] = currentGeofenceParams.rads[1] >> 16;
    payloadCfg[31] = currentGeofenceParams.rads[1] >> 24;
  }
  if (currentGeofenceParams.numFences >= 3)
  {
    payloadCfg[32] = currentGeofenceParams.lats[2] & 0xFF;
    payloadCfg[33] = currentGeofenceParams.lats[2] >> 8;
    payloadCfg[34] = currentGeofenceParams.lats[2] >> 16;
    payloadCfg[35] = currentGeofenceParams.lats[2] >> 24;
    payloadCfg[36] = currentGeofenceParams.longs[2] & 0xFF;
    payloadCfg[37] = currentGeofenceParams.longs[2] >> 8;
    payloadCfg[38] = currentGeofenceParams.longs[2] >> 16;
    payloadCfg[39] = currentGeofenceParams.longs[2] >> 24;
    payloadCfg[40] = currentGeofenceParams.rads[2] & 0xFF;
    payloadCfg[41] = currentGeofenceParams.rads[2] >> 8;
    payloadCfg[42] = currentGeofenceParams.rads[2] >> 16;
    payloadCfg[43] = currentGeofenceParams.rads[2] >> 24;
  }
  if (currentGeofenceParams.numFences >= 4)
  {
    payloadCfg[44] = currentGeofenceParams.lats[3] & 0xFF;
    payloadCfg[45] = currentGeofenceParams.lats[3] >> 8;
    payloadCfg[46] = currentGeofenceParams.lats[3] >> 16;
    payloadCfg[47] = currentGeofenceParams.lats[3] >> 24;
    payloadCfg[48] = currentGeofenceParams.longs[3] & 0xFF;
    payloadCfg[49] = currentGeofenceParams.longs[3] >> 8;
    payloadCfg[50] = currentGeofenceParams.longs[3] >> 16;
    payloadCfg[51] = currentGeofenceParams.longs[3] >> 24;
    payloadCfg[52] = currentGeofenceParams.rads[3] & 0xFF;
    payloadCfg[53] = currentGeofenceParams.rads[3] >> 8;
    payloadCfg[54] = currentGeofenceParams.rads[3] >> 16;
    payloadCfg[55] = currentGeofenceParams.rads[3] >> 24;
  }
  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Clear all geofences using UBX-CFG-GEOFENCE
bool Ublox_GPS::clearGeofences(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_GEOFENCE;
  packetCfg.len = 8;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0; // Message version = 0x00
  payloadCfg[1] = 0; // numFences
  payloadCfg[2] = 0; // confLvl
  payloadCfg[3] = 0; // reserved1
  payloadCfg[4] = 0; // disable PIO combined fence state
  payloadCfg[5] = 0; // PIO pin polarity (0 = low means inside, 1 = low means outside (or unknown))
  payloadCfg[6] = 0; // PIO pin
  payloadCfg[7] = 0; //reserved2

  currentGeofenceParams.numFences = 0; // Zero the number of geofences currently in use

  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Clear the antenna control settings using UBX-CFG-ANT
//This function is hopefully redundant but may be needed to release
//any PIO pins pre-allocated for antenna functions
bool Ublox_GPS::clearAntPIO(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_ANT;
  packetCfg.len = 4;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0x10; // Antenna flag mask: set the recovery bit
  payloadCfg[1] = 0;
  payloadCfg[2] = 0xFF; // Antenna pin configuration: set pinSwitch and pinSCD to 31
  payloadCfg[3] = 0xFF; // Antenna pin configuration: set pinOCD to 31, set reconfig bit

  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Returns the combined geofence state using UBX-NAV-GEOFENCE
bool Ublox_GPS::getGeofenceState(geofenceState &currentGeofenceState, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_GEOFENCE;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false) //Ask module for the geofence status. Loads into payloadCfg.
    return (false);

  currentGeofenceState.status = payloadCfg[5];    // Extract the status
  currentGeofenceState.numFences = payloadCfg[6]; // Extract the number of geofences
  currentGeofenceState.combState = payloadCfg[7]; // Extract the combined state of all geofences
  if (currentGeofenceState.numFences > 0)
    currentGeofenceState.states[0] = payloadCfg[8]; // Extract geofence 1 state
  if (currentGeofenceState.numFences > 1)
    currentGeofenceState.states[1] = payloadCfg[10]; // Extract geofence 2 state
  if (currentGeofenceState.numFences > 2)
    currentGeofenceState.states[2] = payloadCfg[12]; // Extract geofence 3 state
  if (currentGeofenceState.numFences > 3)
    currentGeofenceState.states[3] = payloadCfg[14]; // Extract geofence 4 state

  return (true);
}

//Power Save Mode
//Enables/Disables Low Power Mode using UBX-CFG-RXM
bool Ublox_GPS::powerSaveMode(bool power_save, uint16_t maxWait)
{
  // Let's begin by checking the Protocol Version as UBX_CFG_RXM is not supported on the ZED (protocol >= 27)
  uint8_t protVer = getProtocolVersionHigh(maxWait);
  /*
  if (_printDebug == true)
  {
    _debugSerial->print(F("Protocol version is "));
    _debugSerial->println(protVer);
  }
  */
  if (protVer >= 27)
  {
    debugPrintln("powerSaveMode (UBX-CFG-RXM) is not supported by this protocol version");
    return (false);
  }

  // Now let's change the power setting using UBX-CFG-RXM
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RXM;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false) //Ask module for the current power management settings. Loads into payloadCfg.
    return (false);

  if (power_save)
  {
    payloadCfg[1] = 1; // Power Save Mode
  }
  else
  {
    payloadCfg[1] = 0; // Continuous Mode
  }

  packetCfg.len = 2;
  packetCfg.startingSpot = 0;

  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Change the dynamic platform model using UBX-CFG-NAV5
//Possible values are:
//PORTABLE,STATIONARY,PEDESTRIAN,AUTOMOTIVE,SEA,
//AIRBORNE1g,AIRBORNE2g,AIRBORNE4g,WRIST,BIKE
//WRIST is not supported in protocol versions less than 18
//BIKE is supported in protocol versions 19.2
bool Ublox_GPS::setDynamicModel(DynamicModel_e newDynamicModel, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_NAV5;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false) //Ask module for the current navigation model settings. Loads into payloadCfg.
    return (false);

  payloadCfg[0] = 0x01;            // mask: set only the dyn bit (0)
  payloadCfg[1] = 0x00;            // mask
  payloadCfg[2] = newDynamicModel; // dynModel

  packetCfg.len = 36;
  packetCfg.startingSpot = 0;

  return (sendCommand(packetCfg, maxWait)); //Wait for ack
}

//Given a spot in the payload array, extract four bytes and build a long
uint32_t Ublox_GPS::extractLong(uint8_t spotToStart)
{
  uint32_t val = 0;
  val |= (uint32_t)payloadCfg[spotToStart + 0] << 8 * 0;
  val |= (uint32_t)payloadCfg[spotToStart + 1] << 8 * 1;
  val |= (uint32_t)payloadCfg[spotToStart + 2] << 8 * 2;
  val |= (uint32_t)payloadCfg[spotToStart + 3] << 8 * 3;
  return (val);
}

//Given a spot in the payload array, extract two bytes and build an int
uint16_t Ublox_GPS::extractInt(uint8_t spotToStart)
{
  uint16_t val = 0;
  val |= (uint16_t)payloadCfg[spotToStart + 0] << 8 * 0;
  val |= (uint16_t)payloadCfg[spotToStart + 1] << 8 * 1;
  return (val);
}

//Given a spot, extract byte the payload
uint8_t Ublox_GPS::extractByte(uint8_t spotToStart)
{
  return (payloadCfg[spotToStart]);
}

//Get the current year
uint16_t Ublox_GPS::getYear(uint16_t maxWait)
{
  if (moduleQueried.gpsYear == false)
    getPVT(maxWait);
  moduleQueried.gpsYear = false; //Since we are about to give this to user, mark this data as stale
  return (gpsYear);
}

//Get the current month
uint8_t Ublox_GPS::getMonth(uint16_t maxWait)
{
  if (moduleQueried.gpsMonth == false)
    getPVT(maxWait);
  moduleQueried.gpsMonth = false; //Since we are about to give this to user, mark this data as stale
  return (gpsMonth);
}

//Get the current day
uint8_t Ublox_GPS::getDay(uint16_t maxWait)
{
  if (moduleQueried.gpsDay == false)
    getPVT(maxWait);
  moduleQueried.gpsDay = false; //Since we are about to give this to user, mark this data as stale
  return (gpsDay);
}

//Get the current hour
uint8_t Ublox_GPS::getHour(uint16_t maxWait)
{
  if (moduleQueried.gpsHour == false)
    getPVT(maxWait);
  moduleQueried.gpsHour = false; //Since we are about to give this to user, mark this data as stale
  return (gpsHour);
}

//Get the current minute
uint8_t Ublox_GPS::getMinute(uint16_t maxWait)
{
  if (moduleQueried.gpsMinute == false)
    getPVT(maxWait);
  moduleQueried.gpsMinute = false; //Since we are about to give this to user, mark this data as stale
  return (gpsMinute);
}

//Get the current second
uint8_t Ublox_GPS::getSecond(uint16_t maxWait)
{
  if (moduleQueried.gpsSecond == false)
    getPVT(maxWait);
  moduleQueried.gpsSecond = false; //Since we are about to give this to user, mark this data as stale
  return (gpsSecond);
}

//Get the current millisecond
uint16_t Ublox_GPS::getMillisecond(uint16_t maxWait)
{
  if (moduleQueried.gpsiTOW == false)
    getPVT(maxWait);
  moduleQueried.gpsiTOW = false; //Since we are about to give this to user, mark this data as stale
  return (gpsMillisecond);
}

//Get the current nanoseconds - includes milliseconds
int32_t Ublox_GPS::getNanosecond(uint16_t maxWait)
{
  if (moduleQueried.gpsNanosecond == false)
    getPVT(maxWait);
  moduleQueried.gpsNanosecond = false; //Since we are about to give this to user, mark this data as stale
  return (gpsNanosecond);
}

//Get the latest Position/Velocity/Time solution and fill all global variables
bool Ublox_GPS::getPVT(uint16_t maxWait)
{
  if (autoPVT && autoPVTImplicitUpdate)
  {
    //The GPS is automatically reporting, we just check whether we got unread data
    debugPrintln("getPVT: Autoreporting");
    checkUblox();
    return moduleQueried.all;
  }
  else if (autoPVT && !autoPVTImplicitUpdate)
  {
    //Someone else has to call checkUblox for us...
    debugPrintln("getPVT: Exit immediately");
    return (false);
  }
  else
  {
    debugPrintln("getPVT: Polling");

    //The GPS is not automatically reporting navigation position so we have to poll explicitly
    packetCfg.cls = UBX_CLASS_NAV;
    packetCfg.id = UBX_NAV_PVT;
    packetCfg.len = 0;
    //packetCfg.startingSpot = 20; //Begin listening at spot 20 so we can record up to 20+MAX_PAYLOAD_SIZE = 84 bytes Note:now hard-coded in processUBX

    //The data is parsed as part of processing the response
    UbloxStatus_e retVal = sendCommand(packetCfg, maxWait);

    if (retVal == UBLOX_STATUS_DATA_RECEIVED)
      return (true);

    if (_printDebug == true)
    {
      printf("getPVT retVal: %s\r\n", statusString(retVal));
    }
    return (false);
  }
}

uint32_t Ublox_GPS::getTimeOfWeek(uint16_t maxWait /* = 250*/)
{
  if (moduleQueried.gpsiTOW == false)
    getPVT(maxWait);
  moduleQueried.gpsiTOW = false; //Since we are about to give this to user, mark this data as stale
  return (timeOfWeek);
}

bool Ublox_GPS::hasHighPrecisionReceiver(uint16_t maxWait)
{
  if (moduleQueried.versionNumber == false)
    getProtocolVersion(maxWait);
  return highPrecisionReceiver;
}

int32_t Ublox_GPS::getHighResLatitude(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.highResLatitude == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.highResLatitude = false; //Since we are about to give this to user, mark this data as stale
  return (highResLatitude);
}

int32_t Ublox_GPS::getHighResLongitude(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.highResLongitude == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.highResLongitude = false; //Since we are about to give this to user, mark this data as stale
  return (highResLongitude);
}

int32_t Ublox_GPS::getEllipsoid(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.ellipsoid == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.ellipsoid = false; //Since we are about to give this to user, mark this data as stale
  return (ellipsoid);
}

int32_t Ublox_GPS::getMeanSeaLevel(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.meanSeaLevel == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.meanSeaLevel = false; //Since we are about to give this to user, mark this data as stale
  return (meanSeaLevel);
}

int32_t Ublox_GPS::getGeoidSeparation(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.geoidSeparation == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.geoidSeparation = false; //Since we are about to give this to user, mark this data as stale
  return (geoidSeparation);
}

uint32_t Ublox_GPS::getHorizontalAccuracy(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.horizontalAccuracy == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.horizontalAccuracy = false; //Since we are about to give this to user, mark this data as stale
  return (horizontalAccuracy);
}

uint32_t Ublox_GPS::getVerticalAccuracy(uint16_t maxWait /* = 250*/)
{
  if (highResModuleQueried.verticalAccuracy == false)
    getHPPOSLLH(maxWait);
  highResModuleQueried.verticalAccuracy = false; //Since we are about to give this to user, mark this data as stale
  return (verticalAccuracy);
}

bool Ublox_GPS::getHPPOSLLH(uint16_t maxWait)
{
  if (hasHighPrecisionReceiver()) {
    //The GPS is not automatically reporting navigation position so we have to poll explicitly
    packetCfg.cls = UBX_CLASS_NAV;
    packetCfg.id = UBX_NAV_HPPOSLLH;
    packetCfg.len = 0;

    return sendCommand(packetCfg, maxWait);
  } else {
    if (_printDebug) {
      printf("getHPPOSLLH Error: not connected to a high precision GNSS receiver\r\n");
    }
  }
  return (false); //If command send fails then bail
}

//Get the current 3D high precision positional accuracy - a fun thing to watch
//Returns a long representing the 3D accuracy in millimeters
uint32_t Ublox_GPS::getPositionAccuracy(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_HPPOSECEF;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false)
    return (0); //If command send fails then bail

  uint32_t tempAccuracy = extractLong(24); //We got a response, now extract a long beginning at a given position

  if ((tempAccuracy % 10) >= 5)
    tempAccuracy += 5; //Round fraction of mm up to next mm if .5 or above
  tempAccuracy /= 10;  //Convert 0.1mm units to mm

  return (tempAccuracy);
}

//Get the current latitude in degrees
//Returns a long representing the number of degrees *10^-7
int32_t Ublox_GPS::getLatitude(uint16_t maxWait)
{
  if (moduleQueried.latitude == false)
    getPVT(maxWait);
  moduleQueried.latitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (latitude);
}

//Get the current longitude in degrees
//Returns a long representing the number of degrees *10^-7
int32_t Ublox_GPS::getLongitude(uint16_t maxWait)
{
  if (moduleQueried.longitude == false)
    getPVT(maxWait);
  moduleQueried.longitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (longitude);
}

//Get the current altitude in mm according to ellipsoid model
int32_t Ublox_GPS::getAltitude(uint16_t maxWait)
{
  if (moduleQueried.altitude == false)
    getPVT(maxWait);
  moduleQueried.altitude = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (altitude);
}

//Get the current altitude in mm according to mean sea level
//Ellipsoid model: https://www.esri.com/news/arcuser/0703/geoid1of3.html
//Difference between Ellipsoid Model and Mean Sea Level: https://eos-gnss.com/elevation-for-beginners/
int32_t Ublox_GPS::getAltitudeMSL(uint16_t maxWait)
{
  if (moduleQueried.altitudeMSL == false)
    getPVT(maxWait);
  moduleQueried.altitudeMSL = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (altitudeMSL);
}

//Get the number of satellites used in fix
uint8_t Ublox_GPS::getSIV(uint16_t maxWait)
{
  if (moduleQueried.SIV == false)
    getPVT(maxWait);
  moduleQueried.SIV = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (SIV);
}

//Get the current fix type
//0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix
uint8_t Ublox_GPS::getFixType(uint16_t maxWait)
{
  if (moduleQueried.fixType == false)
  {
    getPVT(maxWait);
  }
  moduleQueried.fixType = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (fixType);
}

//Get the carrier phase range solution status
//Useful when querying module to see if it has high-precision RTK fix
//0=No solution, 1=Float solution, 2=Fixed solution
uint8_t Ublox_GPS::getCarrierSolutionType(uint16_t maxWait)
{
  if (moduleQueried.carrierSolution == false)
    getPVT(maxWait);
  moduleQueried.carrierSolution = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (carrierSolution);
}

// Get the vertical velocity in mm/s, positive = down (added by John Larkin)
int32_t Ublox_GPS::getVerticalVelocity(uint16_t maxWait)
{
  if (moduleQueried.verticalVelocity == false) {
    getPVT(maxWait);
  }
  moduleQueried.verticalVelocity = false; // Mark this data as stale
  return (verticalVelocity);
}

//Get the ground speed in mm/s
int32_t Ublox_GPS::getGroundSpeed(uint16_t maxWait)
{
  if (moduleQueried.groundSpeed == false)
    getPVT(maxWait);
  moduleQueried.groundSpeed = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (groundSpeed);
}

//Get the heading of motion (as opposed to heading of car) in degrees * 10^-5
int32_t Ublox_GPS::getHeading(uint16_t maxWait)
{
  if (moduleQueried.headingOfMotion == false)
    getPVT(maxWait);
  moduleQueried.headingOfMotion = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (headingOfMotion);
}

//Get the positional dillution of precision * 10^-2
uint16_t Ublox_GPS::getPDOP(uint16_t maxWait)
{
  if (moduleQueried.pDOP == false)
    getPVT(maxWait);
  moduleQueried.pDOP = false; //Since we are about to give this to user, mark this data as stale
  moduleQueried.all = false;

  return (pDOP);
}

//Get the current protocol version of the Ublox module we're communicating with
//This is helpful when deciding if we should call the high-precision Lat/Long (HPPOSLLH) or the regular (POSLLH)
uint8_t Ublox_GPS::getProtocolVersionHigh(uint16_t maxWait)
{
  if (moduleQueried.versionNumber == false)
    getProtocolVersion(maxWait);
  return (versionHigh);
}

//Get the current protocol version of the Ublox module we're communicating with
//This is helpful when deciding if we should call the high-precision Lat/Long (HPPOSLLH) or the regular (POSLLH)
uint8_t Ublox_GPS::getProtocolVersionLow(uint16_t maxWait)
{
  if (moduleQueried.versionNumber == false)
    getProtocolVersion(maxWait);
  return (versionLow);
}

//Get the current protocol version of the Ublox module we're communicating with
//This is helpful when deciding if we should call the high-precision Lat/Long (HPPOSLLH) or the regular (POSLLH)
bool Ublox_GPS::getProtocolVersion(uint16_t maxWait)
{
  //Send packet with only CLS and ID, length of zero. This will cause the module to respond with the contents of that CLS/ID.
  packetCfg.cls = UBX_CLASS_MON;
  packetCfg.id = UBX_MON_VER;

  packetCfg.len = 0;
  packetCfg.startingSpot = 40; //Start at first "extended software information" string

  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //Payload should now contain ~220 characters (depends on module type)

  if (_printDebug == true)
  {
    printf("MON VER Payload: ");
    for (int location = 0; location < packetCfg.len; location++)
    {
      if (location % 30 == 0)
        printf("\r\n\t");
      if (!payloadCfg[location]) printf("%c", payloadCfg[location]);
    }
    printf("\r\n\r\n");
  }

  highPrecisionReceiver = false;
  //We will step through the payload looking at each extension field of 30 bytes
  for (uint8_t extensionNumber = 0; extensionNumber < 10; extensionNumber++)
  {
    // Look for firmware field as well
    if (payloadCfg[(30*extensionNumber) + 0] == 'F' && payloadCfg[(30 * extensionNumber) + 1] == 'W') {
      if (payloadCfg[(30*extensionNumber) + 6] == 'H')
        highPrecisionReceiver = true;
    }
    //Now we need to find "PROTVER=18.00" in the incoming byte stream
    if (payloadCfg[(30 * extensionNumber) + 0] == 'P' && payloadCfg[(30 * extensionNumber) + 6] == 'R')
    {
      versionHigh = (payloadCfg[(30 * extensionNumber) + 8] - '0') * 10 + (payloadCfg[(30 * extensionNumber) + 9] - '0');  //Convert '18' to 18
      versionLow = (payloadCfg[(30 * extensionNumber) + 11] - '0') * 10 + (payloadCfg[(30 * extensionNumber) + 12] - '0'); //Convert '00' to 00
      moduleQueried.versionNumber = true;                                                                                  //Mark this data as new

      if (_printDebug == true)
      {
        if (highPrecisionReceiver)
          printf("High Precision GNSS\r\n");
        else
          printf("Standard Precision GNSS\r\n");
        printf("Protocol version: %d.%d\r\n", versionHigh, versionLow);
      }
      return (true); //Success!
    }
  }

  return (false); //We failed
}

//Mark all the PVT data as read/stale. This is handy to get data alignment after CRC failure
void Ublox_GPS::flushPVT()
{
  //Mark all datums as stale (read before)
  moduleQueried.gpsiTOW = false;
  moduleQueried.gpsYear = false;
  moduleQueried.gpsMonth = false;
  moduleQueried.gpsDay = false;
  moduleQueried.gpsHour = false;
  moduleQueried.gpsMinute = false;
  moduleQueried.gpsSecond = false;
  moduleQueried.gpsNanosecond = false;

  moduleQueried.all = false;
  moduleQueried.longitude = false;
  moduleQueried.latitude = false;
  moduleQueried.altitude = false;
  moduleQueried.altitudeMSL = false;
  moduleQueried.SIV = false;
  moduleQueried.fixType = false;
  moduleQueried.carrierSolution = false;
  moduleQueried.verticalVelocity = false;
  moduleQueried.groundSpeed = false;
  moduleQueried.headingOfMotion = false;
  moduleQueried.pDOP = false;
}

//Relative Positioning Information in NED frame
//Returns true if commands was successful
bool Ublox_GPS::getRELPOSNED(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_RELPOSNED;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  if (sendCommand(packetCfg, maxWait) == false)
    return (false); //If command send fails then bail

  //We got a response, now parse the bits

  uint16_t refStationID = extractInt(2);
  //_debugSerial->print(F("refStationID: "));
  //_debugSerial->println(refStationID));

  int32_t tempRelPos;

  tempRelPos = extractLong(8);
  relPosInfo.relPosN = tempRelPos / 100.0; //Convert cm to m

  tempRelPos = extractLong(12);
  relPosInfo.relPosE = tempRelPos / 100.0; //Convert cm to m

  tempRelPos = extractLong(16);
  relPosInfo.relPosD = tempRelPos / 100.0; //Convert cm to m

  relPosInfo.relPosLength = extractLong(20);
  relPosInfo.relPosHeading = extractLong(24);

  relPosInfo.relPosHPN = payloadCfg[32];
  relPosInfo.relPosHPE = payloadCfg[33];
  relPosInfo.relPosHPD = payloadCfg[34];
  relPosInfo.relPosHPLength = payloadCfg[35];

  uint32_t tempAcc;

  tempAcc = extractLong(36);
  relPosInfo.accN = tempAcc / 10000.0; //Convert 0.1 mm to m

  tempAcc = extractLong(40);
  relPosInfo.accE = tempAcc / 10000.0; //Convert 0.1 mm to m

  tempAcc = extractLong(44);
  relPosInfo.accD = tempAcc / 10000.0; //Convert 0.1 mm to m

  uint8_t flags = payloadCfg[60];

  relPosInfo.gnssFixOk = flags & (1 << 0);
  relPosInfo.diffSoln = flags & (1 << 1);
  relPosInfo.relPosValid = flags & (1 << 2);
  relPosInfo.carrSoln = (flags & (0b11 << 3)) >> 3;
  relPosInfo.isMoving = flags & (1 << 5);
  relPosInfo.refPosMiss = flags & (1 << 6);
  relPosInfo.refObsMiss = flags & (1 << 7);

  return (true);
}
