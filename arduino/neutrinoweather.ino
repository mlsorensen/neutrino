#include <Wire.h>
#include <bmp180.h>
#include <SI7021.h>
#include <SPI.h>
#include "RF24.h"
#include <string.h>
#include <avr/sleep.h>
#include <LowPower.h>

int rfgoodled = 3;
int rfbadled  = 4;
int lobattled = 2;

int myaddr = getMyAddr();

struct weather {
    int8_t  addr = myaddr;
    int16_t tempc = 0;
    int16_t humidity = 0;
    int32_t pressurep = 0;
    int32_t millivolts = 0;
};

RF24 radio(8,9);
const uint64_t pipe = 0xFCFCFCFC00LL + myaddr;
//const uint64_t pipes[6] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; 
BMP180 bsensor;
SI7021 hsensor;

void setup() {
    // turn off analog comparator
    ACSR = B10000000;
    // turn off digital input buffers for analog pins
    DIDR0 = DIDR0 | B00111111;
    
    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS); 
    sleep_cpu ();
  
    pinMode(rfgoodled,OUTPUT);
    pinMode(rfbadled,OUTPUT);
    pinMode(lobattled,OUTPUT);
  
    //flash lights on boot
    flash(rfgoodled);
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
    flash(rfbadled);
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
    flash(rfgoodled);
  
    //flash node address
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
    for (int i = 0 ; i < myaddr; i++) {
        pulse(rfgoodled);   
    }
  
    radio.begin();
    radio.setRetries(10,10);
    radio.setPayloadSize(16);
    radio.setChannel(0x4c);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(pipe);
    radio.startListening();
    
    hsensor.begin();
    bsensor.begin(1);
}

void loop() {
    weather w;
    // humidity, temp
    if (hsensor.sensorExists()) {
        si7021_env sidata = hsensor.getHumidityAndTemperature();
        w.humidity        = sidata.humidityBasisPoints;
        w.tempc           = sidata.celsiusHundredths;
    }
    
    // pressure
    if (bsensor.sensorExists()) {
        // fall back to bosch sensor for temperature if necessary
        if(! hsensor.sensorExists()) {
            w.tempc = bsensor.getCelsiusHundredths();
        }
        w.pressurep = bsensor.getPressurePascals();
        
    }
    
    // voltage
    w.millivolts = readVcc();
    
    radio.powerUp();
    delay(2);
    radio.stopListening();
    bool ok = radio.write(&w, sizeof w);
    if (ok) {
        flash(rfgoodled);
    } else {
        flash(rfbadled); 
    }
    radio.startListening();
    radio.powerDown();
    
    if (w.millivolts < 2200) {
        flash(lobattled);
    }
    
    // power down for 56 seconds
    for (int i = 0; i < 7; i++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
    // power down for 4 seconds (60 total)
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    
}

void flash(int pin) {
  digitalWrite(pin, HIGH);
  delay(2);
  digitalWrite(pin, LOW);
  delay(2);
}

void pulse(int pin) {
   // software pwm, flash light for ~1 second with short duty cycle
   for (int i = 0; i < 50; i++) {
       digitalWrite(pin, HIGH);
       delay(1);
       digitalWrite(pin,LOW);
       delay(9);
   }
   digitalWrite(pin,LOW);
   LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);   
}

// used for debugging, sending message output over rf
void rflog(char * msg) {
  radio.stopListening();
  
  bool ok = radio.write(msg, 32);
  if (ok) {
    flash(rfgoodled);
  } else {
    flash(rfbadled); 
  }
  radio.startListening();
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
  
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

// read jumpers and get node address/id
int getMyAddr() {
    int result = 0;
    pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    
    if(!digitalRead(7)) {
        result = result + 4;  
    }
    
    if(!digitalRead(6)) {
        result = result + 2;  
    }
    
    if(!digitalRead(5)) {
        result = result + 1 ;  
    }
    
    // save power
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    pinMode(7, OUTPUT);
    digitalWrite(7, LOW);
    
    return result; 
}
