#include "mbed.h"
#include "NAL9602.h"

/** Command Module Microcontroller
 *
 * @author John M. Larkin (jlarkin@whitworth.edu)
 * @version 0.1
 * @date 2017
 * @copyright GNU Public License
 *
 * Version History:
 *  0.1 - Terminal emulation mode only for testing of NAL9602 library
 */

NAL9602 sat(USBTX,USBRX);
Serial pc(USBTX,USBRX);

int main() {
  while (true) {
    sat.zeroMessageCounter();
    wait(1);
    sat.setModeGPS(airborne_low_dynamic);
    wait(1);
    sat.setModeGPS(stationary);
    wait(1);
    sat.setModeGPS(land_vehicle);
    wait(10);
  }
 }
