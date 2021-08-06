#include <mbed.h>
#include "ADT7410.h"
#include <unity/unity.h>

I2C i2c(p9,p10);
ADT7410 tempsensor(&i2c,0x90);
DigitalOut led1(LED1), led2(LED2);

void test_temperatureconversion() 
{
    float tempC;
    tempC = tempsensor.temperatureconversion(0x1c90);
    TEST_ASSERT_FLOAT_WITHIN (.5, -55, tempC);

    tempC = tempsensor.temperatureconversion(0x1ce0);
    TEST_ASSERT_FLOAT_WITHIN (.5, -50, tempC);

    tempC = tempsensor.temperatureconversion(0x1e70);
    TEST_ASSERT_FLOAT_WITHIN (.5, -25, tempC);

    tempC = tempsensor.temperatureconversion(0x1fff);
    TEST_ASSERT_FLOAT_WITHIN (.5, -.0625, tempC);

    tempC = tempsensor.temperatureconversion(0x0000);
    TEST_ASSERT_FLOAT_WITHIN (.5, 0, tempC);

    tempC = tempsensor.temperatureconversion(0x001);
    TEST_ASSERT_FLOAT_WITHIN (.5, .0625, tempC);

    tempC = tempsensor.temperatureconversion(0x190);
    TEST_ASSERT_FLOAT_WITHIN (.5, 25, tempC);

    tempC = tempsensor.temperatureconversion(0x320);
    TEST_ASSERT_FLOAT_WITHIN (.5, 50, tempC);

    tempC = tempsensor.temperatureconversion(0x7d0);
    TEST_ASSERT_FLOAT_WITHIN (.5, 125, tempC);

    tempC = tempsensor.temperatureconversion(0x960);
    TEST_ASSERT_FLOAT_WITHIN (.5, 150, tempC);
}

void test_room_temperature()
{
    float tempC;
    tempC = tempsensor.oneShotRead();
    TEST_ASSERT_FLOAT_WITHIN (15, 25, tempC);
    
}

int main() {
    ThisThread::sleep_for(3s);
    UNITY_BEGIN();
    RUN_TEST(test_temperatureconversion);
    RUN_TEST(test_room_temperature);
    UNITY_END();
    ThisThread::sleep_for(3s);

}