/** Class for interfacing with a ADT7410 Temp Sensor via I2C
 *  
 * Written for Mbed OS 6
 *
 *  @author Grant B. McDonald (gmcdonald24@my.whitworth.edu)
 *  @version 1.0
 *  @date 2021
 *  @copyright MIT License
 */

#ifndef ADT7410_H
#define ADT7410_H

#include <mbed.h>


class ADT7410 {
    public:
        typedef enum{
            continuous = 0x00, //continuous mode
            oneshot = 0x01, // one shot mode
            onesps = 0x02 //one shot per second
        } mode_t;
        ADT7410(I2C *i2c, int addr); // constructor function
        void setMode(mode_t modecode);
        float oneShotRead();
        float temperatureconversion(uint16_t rawData);
    private:
        I2C* _i2c; 
        int _addr;
};

#endif