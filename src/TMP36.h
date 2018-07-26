#ifndef TMP36_H
#define TMP36_H

#include "mbed.h"

// Class for TMP36 temperature sensor
class TMP36 {

public:
    float cal;

    TMP36(PinName output_pin);

    /** Overload float conversion so can get temperature without .read()
    */
    operator float ();

    /** Returns temperature in Celsius
    */
    float read();
private:
    AnalogIn _pin;
};

#endif
