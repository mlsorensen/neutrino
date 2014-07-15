/*
Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms
*/
#include "Arduino.h"
#include "BMP180.h"
#include <Wire.h>

#define I2C_ADDR 0x77

struct calibration {
    int ac1;
    int ac2; 
    int ac3; 
    unsigned int ac4;
    unsigned int ac5;
    unsigned int ac6;
    int b1; 
    int b2;
    int mb;
    int mc;
    int md;
} _cdata;

unsigned char _oversampling = 0;
bool _bosch_exists = false;
long b5;

BMP180::BMP180() {
}

// pass oversampling rate (0 lowest, 3 highest)
bool BMP180::begin(int os) {
    Wire.begin();
    Wire.beginTransmission(I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        _oversampling = os;
        _calibrate();
        _bosch_exists = true;
    }
    return _bosch_exists;
}

bool BMP180::sensorExists() {
    return _bosch_exists;
}

void BMP180::_calibrate() {
    _cdata.ac1 = _getInt(0xAA);
    _cdata.ac2 = _getInt(0xAC);
    _cdata.ac3 = _getInt(0xAE);
    _cdata.ac4 = _getUInt(0xB0);
    _cdata.ac5 = _getUInt(0xB2);
    _cdata.ac6 = _getUInt(0xB4);
    _cdata.b1 = _getInt(0xB6);
    _cdata.b2 = _getInt(0XB8);
    _cdata.mb = _getInt(0xBA);
    _cdata.mc = _getInt(0xBC);
    _cdata.md = _getInt(0xBE);
}

int BMP180::_getInt(unsigned char regaddr) {
    
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(regaddr);
    Wire.endTransmission();
    
    Wire.requestFrom(I2C_ADDR, 2);
    while(Wire.available() < 2) {
    }
    
    unsigned char byte1, byte2;
    byte1 = Wire.read();
    byte2 = Wire.read();
    int ret = (int)byte1 << 8 | byte2;
    
    return ret;
}

unsigned int BMP180::_getUInt(unsigned char regaddr) {
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(regaddr);
    Wire.endTransmission();
    
    Wire.requestFrom(I2C_ADDR, 2);
    while(Wire.available() < 2) {
    }
    
    unsigned char byte1, byte2;
    byte1 = Wire.read();
    byte2 = Wire.read();
    unsigned int ret = (unsigned int)byte1 << 8 | byte2;
    
    return ret;
}

// Fahrenheit in hundredths of a degree
int BMP180::getFahrenheitHundredths() {
    int temp = (getCelsiusHundredths()* 1.8) + 3200;
    return temp;
}

// Celsius in hundredths of a degree
int BMP180::getCelsiusHundredths() {
    // first, get uncompensated temperature
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xF4);
    Wire.write(0x2E);
    Wire.endTransmission();
    delay(5);
    unsigned int uncomptemp = _getInt(0xF6);
  
    //see datasheet for these formulas
    long x1, x2;
    x1 = (((long)uncomptemp - (long)_cdata.ac6) * (long)_cdata.ac5) >> 15;
    x2 = ((long)_cdata.mc << 11)/(x1 + _cdata.md);
    b5 = x1 + x2;

    return ((b5 + 8)  >> 4) * 10;
}

//result in centimeters
long BMP180::getAltitudeCentimeters(long p) {
    // see datacheet for this formula
    long ac = (float)4433000 * (1 - pow(((float) p/(float)101325), 0.190295));
    return ac;
}

// decimal shifted two places
long BMP180::getAltitudeFeet(long p) {
    long a = getAltitudeCentimeters(p);
    
    return a * 3.2808;
}

//refactor
long BMP180::getPressurePascals() {
    
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xF4);
    Wire.write(0x34 + (_oversampling << 6));
    Wire.endTransmission();
    delay(2 + (3 << _oversampling));
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xF6);
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDR, 3);
    
    while(Wire.available() < 3) {
    }
    
    unsigned char byte1 = Wire.read();
    unsigned char byte2 = Wire.read();
    unsigned char byte3 = Wire.read();
    
    unsigned long uncomppressure = (((unsigned long) byte1 << 16) | ((unsigned long) byte2 << 8) | (unsigned long) byte3) >> (8 - _oversampling);
    
    // see datasheet for formula
    long x1, x2, x3, b3, b6, p;
    unsigned long b4, b7;
    
    b6 = b5 - 4000;
    x1 = (_cdata.b2 * (b6 * b6)>>12)>>11;
    x2 = (_cdata.ac2 * b6)>>11;
    x3 = x1 + x2;
    b3 = (((((long)_cdata.ac1) * 4 + x3) << _oversampling) + 2)>>2;
    
    x1 = (_cdata.ac3 * b6) >> 13;
    x2 = (_cdata.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (_cdata.ac4 * (unsigned long)(x3 + 32768)) >> 15;
    
    b7 = ((unsigned long)(uncomppressure - b3) * (50000 >> _oversampling));
    if (b7 < 0x80000000)
        p = (b7 << 1) / b4;
    else
        p = (b7 / b4) << 1;
    
    x1 = (p >> 8) * ( p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p += (x1 + x2 + 3791) >> 4;
    
    return p;
    
}


