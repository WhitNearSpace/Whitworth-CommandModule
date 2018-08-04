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
  Serial modem;
  bool connected;
  bool shutdownRequest;

  /** Create an RN-41 interface object connected to the specified pins
  *
  * @param tx_pin Serial TX pin
  * @param rx_pin Serial RX pin
  */
  RN41(PinName tx_pin, PinName rx_pin);

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
