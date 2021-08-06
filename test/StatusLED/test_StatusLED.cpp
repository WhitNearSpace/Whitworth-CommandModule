#include <mbed.h>
#include <unity/unity.h>
#include "StatusLED.h"

using namespace std::chrono;

StatusLED led(LED1);
uint32_t led1_bit_mask = 1 << 18; // LED1 is GPIO1 port bit 18
// Thread period_timer_thread;

void test_mode_off(void) {
  led = off;
  ThisThread::sleep_for(1s); // it can take up to 1 s for mode update
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT_BITS_LOW(led1_bit_mask, LPC_GPIO1->FIOPIN);
    ThisThread::sleep_for(milliseconds((rand() % 100)+1));
  }
}

void test_mode_steady(void) {
  led = steady;
  ThisThread::sleep_for(1s); // it can take up to 1 s for mode update
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT_BITS_HIGH(led1_bit_mask, LPC_GPIO1->FIOPIN);
    ThisThread::sleep_for(milliseconds((rand() % 100)+1));
  }
}

void test_mode_slow_flash(void) {
  Timer t;
  led = slow_flash;
  ThisThread::sleep_for(1s); // it can take up to 1 s for mode update
  // Is the duty cycle approximately 50%?
  int on_count = 0;
  for (int i = 0; i < 20; i++) {
    if (led1_bit_mask & LPC_GPIO1->FIOPIN) on_count++;
    ThisThread::sleep_for(500ms);
  }
  TEST_ASSERT_INT_WITHIN_MESSAGE(2,10,on_count,"Duty cycle should be 50%");

  // Is the period approximately 2000 ms?
  int avg_period = 0;
  t.stop();
  for (int i = 0; i < 5; i++) {
    t.reset(); 
    while (led1_bit_mask & LPC_GPIO1->FIOPIN) {
      ThisThread::sleep_for(5ms);
    }
    t.start();
    while (!(led1_bit_mask & LPC_GPIO1->FIOPIN)) {
      ThisThread::sleep_for(5ms);
    }
    while (led1_bit_mask & LPC_GPIO1->FIOPIN) {
      ThisThread::sleep_for(5ms);
    }
    t.stop();
    avg_period = avg_period + duration_cast<milliseconds>(t.elapsed_time()).count();
  }
  avg_period = avg_period/5;
  TEST_ASSERT_INT64_WITHIN_MESSAGE(200,2000,avg_period,"Slow flash period should be 2000 +/- 200 ms.");
}

void test_mode_quick_flash(void) {
  Timer t;
  led = quick_flash;
  ThisThread::sleep_for(1s); // it can take up to 1 s for mode update
  // Is the duty cycle approximately 50%?
  int on_count = 0;
  for (int i = 0; i < 20; i++) {
    if (led1_bit_mask & LPC_GPIO1->FIOPIN) on_count++;
    ThisThread::sleep_for(50ms);
  }
  TEST_ASSERT_INT_WITHIN_MESSAGE(2,10,on_count,"Duty cycle should be 50%");

  // Is the period approximately 200 ms?
  int avg_period = 0;
  t.stop();
  for (int i = 0; i < 5; i++) {
    t.reset(); 
    while (led1_bit_mask & LPC_GPIO1->FIOPIN) {
      ThisThread::sleep_for(5ms);
    }
    t.start();
    while (!(led1_bit_mask & LPC_GPIO1->FIOPIN)) {
      ThisThread::sleep_for(5ms);
    }
    while (led1_bit_mask & LPC_GPIO1->FIOPIN) {
      ThisThread::sleep_for(5ms);
    }
    t.stop();
    avg_period = avg_period + duration_cast<milliseconds>(t.elapsed_time()).count();
  }
  avg_period = avg_period/5;
  TEST_ASSERT_INT_WITHIN_MESSAGE(20,200,avg_period,"Quick flash period should be 200 +/- 20 ms.");
}

void test_mode_breathing(void) {
  led = breathing;
  ThisThread::sleep_for(1s);

  uint32_t mr_max = LPC_PWM1->MR0;
  uint32_t mr1;
  int dutycycle_midrange = 0;
  for (int i = 0; i < 10; i++) {
    mr1 = LPC_PWM1->MR1;
    if ((mr1>0) && (mr1 < mr_max)) dutycycle_midrange++;
    ThisThread::sleep_for(100ms);
  }
  TEST_ASSERT_INT_WITHIN_MESSAGE(2,10,dutycycle_midrange,"Duty cycle should fall between extremes most of the time.");
}

int main() {
  ThisThread::sleep_for(3s);
  UNITY_BEGIN();
  RUN_TEST(test_mode_off);
  RUN_TEST(test_mode_steady);
  RUN_TEST(test_mode_slow_flash);
  RUN_TEST(test_mode_quick_flash);
  RUN_TEST(test_mode_breathing);
  UNITY_END();
  ThisThread::sleep_for(3s); 
}