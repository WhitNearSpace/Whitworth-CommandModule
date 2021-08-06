#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <mbed.h>
#include <rtos.h>

// Status mode variable
enum LED_Modes {
  off,
  steady,
  quick_flash,
  slow_flash,
  breathing
};

// Class for a status LED (threaded)
class StatusLED {

public:
  StatusLED(PinName output_pin);
  void mode(LED_Modes newMode);
  void operator= (LED_Modes newMode);

private:
  PwmOut _pwm;
  volatile LED_Modes _mode; // volatile because modified outside of _updateLED thread
  Thread _led_thread;
  float _delta; // duty cycle step size for breathing mode
  void _updateLED();   
};

#endif