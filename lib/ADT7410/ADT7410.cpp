#include <mbed.h>
#include "ADT7410.h"

#define DATA_REG_ADDR 0x00
#define CONFIG_REG_ADDR 0x03
#define CONFIG_BYTE_DEFAULT 0x00


ADT7410::ADT7410(I2C *i2c, int addr)
{
    _i2c = i2c;
    _addr = addr;
}

void ADT7410::setMode(mode_t modecode)
{
    char buff[2]; 
    buff[0] = CONFIG_REG_ADDR; // loading buffer for write
    buff[1] = CONFIG_BYTE_DEFAULT | (modecode << 5);
    _i2c->write(_addr, buff, 2); //writing to adt4710 i2c address to set the mode
    ThisThread::sleep_for(300ms);
}

float ADT7410::oneShotRead() //reads temp a single time
{
    char buff[3];
    uint16_t rawData; //thisw will store the bytes that the temp sensor sends back
    buff[0] = CONFIG_REG_ADDR; 
    buff[1] = CONFIG_BYTE_DEFAULT | 0x20; //setting bits 6:5 of the defualt mode configuration byte to 01
    _i2c->write(_addr, buff, 2); 
    ThisThread::sleep_for(300ms); //must sleep for at least 240ms to allow temp senser to complete temperature conversion

    
    buff[0] = DATA_REG_ADDR; //load buffer with data register for a data read
    _i2c->write(_addr, buff, 1); //perform a write to the register you wish to read from
    _i2c->read(_addr, buff, 2); //read data from data register address


    rawData = (buff[0] << 8) | buff[1]; // put the two bytes of temperature data into one int16 byte
    rawData = rawData >> 3; //shift right 3 to get temperature data bits in the right spot.
    return temperatureconversion(rawData);
}

float ADT7410::temperatureconversion(uint16_t rawData){ //converts raw data from adt7410 to a temperature in celcius
    float tempC; //this will hold the temperature in celcius
    if (rawData & (1 << 12)) { //tests to see if rawData is representing a negative number (bit 12 after bit shift is high)
        tempC = (float)(rawData - 8192) / 16;
    } else {
        tempC = (float)(rawData) / 16;
    }
    return tempC;
}
