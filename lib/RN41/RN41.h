/** Interface to RN-41 Bluetooth module
 *
 *  @author John M. Larkin (jlarkin@whitworth.edu)
 *  @version 0.1
 *  @date 2018
 *  @copyright MIT License
 */

 #ifndef RN41_H
 #define RN41_H

 #include "mbed.h"

class RN41 {

public:
  FILE* modem;
  bool connected;
  bool shutdownRequest;

  /** Create an RN-41 interface object connected to the specified pins
  *
  * @param bt_serial_ptr pointer to BufferedSerial object
  */
  RN41(BufferedSerial* bt_serial_ptr);

  ~RN41();

  void initiateShutdown();

  void processShutdown();

  void queueCmd();

private:
  int cmdStep;
  Timeout cmdSequencer;
  bool cmdReady;
};

#endif
