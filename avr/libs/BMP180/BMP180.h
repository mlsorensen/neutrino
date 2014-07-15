/*
  Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed, please check with the copyright holder for terms
*/
#ifndef BMP180_h
#define BMP180_h

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#else
 #include <avr/pgmspace.h>
 #include <Wire.h>
#endif

class BMP180
{
  public:
    BMP180();
    int getFahrenheitHundredths();
    int getCelsiusHundredths();
    long getPressurePascals();
    long getAltitudeCentimeters(long p);
    long getAltitudeFeet(long p);
    bool begin(int os);
    bool sensorExists();
  private:
    void _calibrate();
    int  _getInt(unsigned char regaddr);
    unsigned int _getUInt(unsigned char regaddr);
};

#endif

