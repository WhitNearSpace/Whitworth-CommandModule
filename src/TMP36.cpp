#include "TMP36.h"

TMP36::TMP36(PinName output_pin) : _pin(output_pin)
{
  cal = 3.3;  // nominal reference voltage
}

float TMP36::read()
{
  // First, convert raw analog to voltage using calibration factor
  // Second, subtract 500 mV (voltage for 0 deg C)
  // Third, multiply by 100 because scale factor is 10 mV/deg C
  return ((_pin.read()*cal)-0.500)*100.0;
}

TMP36::operator float ()
{
  return read();
}
