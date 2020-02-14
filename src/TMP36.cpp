#include "TMP36.h"

TMP36::TMP36(PinName output_pin) : _pin(output_pin)
{
  ref_voltage = 3.3;  // nominal reference voltage
  offset = 0.500;     // data sheet claims 500 mV at 0 deg C
  scale = 0.010;       // data sheet claims 10 mV/deg C
}

float TMP36::read()
{
  // First, convert input value to voltage
  // Second, subtract offset (voltage for 0 deg C)
  // Third, divide by scale factor (### V/deg C)
  return ((_pin.read()*ref_voltage)-offset)/scale;
}

TMP36::operator float ()
{
  return read();
}
