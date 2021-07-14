#include "StatusLED.h"

#define DELTA_SIZE 0.025

StatusLED::StatusLED(PinName output_pin) : _pwm(output_pin)
{
  _pwm.period(0.01);
  _pwm = 0;
  _mode = off;
  _delta = DELTA_SIZE;
  _led_thread.start(callback(this, &StatusLED::_updateLED));
  _led_thread.set_priority(osPriorityLow);
}

void StatusLED::mode(LED_Modes newMode)
{
  _mode = newMode;
  switch(newMode) {
    case steady:
      _pwm = 1;
      break;
    case quick_flash:
      _pwm = 1;
      break;
    case slow_flash:
      _pwm = 1;
      break;
    case breathing:
      _pwm = 0;
      _delta = DELTA_SIZE;
      break;
    default:
      _pwm = 0;
  }
}

void StatusLED::_updateLED()
{
  int wait_time; // a time in milliseconds
  while (true) {
    switch (_mode) {
      case quick_flash:
        _pwm = !_pwm;
        wait_time = 100;
        break;
      case slow_flash:
        _pwm = !_pwm;
        wait_time = 1000;
        break;
      case breathing:
        if (_pwm<=0) _delta = DELTA_SIZE;
        if (_pwm>=1) _delta = -DELTA_SIZE;
        _pwm = _pwm + _delta;
        wait_time = 50;
        break;
      default:
        wait_time = 100;
    }
    ThisThread::sleep_for(std::chrono::milliseconds(wait_time));
  }
}

void StatusLED::operator= (LED_Modes newMode) {
  this->mode(newMode);
} 