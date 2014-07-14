#include <Wire.h>
#include <bmp180.h>
#include <SI7021.h>
#include <SPI.h>
#include "RF24.h"
#include <string.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <LowPower.h>
#include <skipjack.h>

#define EEPROM_KEY_ADDR 0x00
#define EEPROM_KEY_SIZE 10

#define CHANNEL_PIN_0 0
#define CHANNEL_PIN_1 1
#define CHANNEL_PIN_2 10
#define CHANNEL_PIN_3 A3

#define ADDRESS_PIN_0 5
#define ADDRESS_PIN_1 6
#define ADDRESS_PIN_2 7

#define CHANNEL_PIN_0 0
#define CHANNEL_PIN_1 1
#define CHANNEL_PIN_2 10
#define CHANNEL_PIN_3 A3

#define ENCRYPT_PIN A7

#define RF_GOOD_LED 3
#define RF_BAD_LED 4
#define LO_BATT_LED 2

int myaddr = getMyAddr();
int mychannel = getMyChannel();
int32_t lastmillivolts = 0;

// this struct should always be a multiple of 64 bits so we can easily encrypt it (skipjack 64bit blocks)
struct sensordata {
    int8_t  addr         = myaddr;
    int8_t  placeholder1 = 0; // future var
    int16_t tempc        = 0;
    int16_t humidity     = 0;
    int16_t placeholder2 = 0; // future var
    int32_t pressurep    = 0;
    int32_t millivolts   = 0;
};

// 28 bytes. Want to keep this <= 32 bytes so that it will fit in one radio packet
struct message {
    int8_t addr = myaddr; // 1 byte
    boolean encrypted = false; // 1 byte
    byte key[EEPROM_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 10 bytes
    sensordata s; // 16 bytes
};

RF24 radio(8,9);
const uint64_t pipe = 0xFCFCFCFC00LL + myaddr;
//const uint64_t pipes[6] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; 
BMP180 bsensor;
SI7021 hsensor;
byte key[EEPROM_KEY_SIZE];

void setup() {
    // populate encryption key
    eeprom_read_block((void*)&key, (const void*)EEPROM_KEY_ADDR, sizeof(key));
    if (keyIsEmpty(key)) {
        generateKey(key);
        eeprom_write_block((const void*)&key, (void*)EEPROM_KEY_ADDR, EEPROM_KEY_SIZE);
    }
    
    // turn off analog comparator
    ACSR = B10000000;
    // turn off digital input buffers for analog pins
    DIDR0 = DIDR0 | B00111111;
    
    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS); 
    sleep_cpu ();
  
    pinMode(RF_GOOD_LED,OUTPUT);
    pinMode(RF_BAD_LED,OUTPUT);
    pinMode(LO_BATT_LED,OUTPUT);
  
    //flash lights on boot
    flash(RF_GOOD_LED);
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
    flash(RF_BAD_LED);
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
    flash(RF_GOOD_LED);
  
    //flash node address
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
    for (int i = 0 ; i < myaddr; i++) {
        pulse(RF_GOOD_LED);   
    }
  
    radio.begin();
    radio.setRetries(10,10);
    radio.setPayloadSize(16);
    radio.setChannel(mychannel);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(pipe);
    radio.startListening();
    
    hsensor.begin();
    bsensor.begin(1);
    lastmillivolts = readVcc();
}

void loop() {
    message m;
    // humidity, temp
    if (hsensor.sensorExists()) {
        si7021_env sidata = hsensor.getHumidityAndTemperature();
        m.s.humidity        = sidata.humidityBasisPoints;
        m.s.tempc           = sidata.celsiusHundredths;
    }
    
    // pressure
    if (bsensor.sensorExists()) {
        // fall back to bosch sensor for temperature if necessary
        if(! hsensor.sensorExists()) {
            m.s.tempc = bsensor.getCelsiusHundredths();
        }
        m.s.pressurep = bsensor.getPressurePascals();
        
    }
    
    // voltage was read after last radio send to get reading after high power draw
    m.s.millivolts = lastmillivolts;
    
    // when encryption is disabled, we send the key. when it is enabled, we send the empty key
    if (shouldEncrypt()) {
        m.encrypted = true;
        // encrypt memory blocks containing sensor data. skipjack is 64 bit (8 byte) blocks
        for (int i = 0; i < sizeof(m.s); i += 8) {
            int ptrshift = i * 8;
            skipjack_enc((&m.s + ptrshift),&key);
        }
    } else {
        memcpy((void*)&m.key, (void*)&key, EEPROM_KEY_SIZE * sizeof(byte));
    }
    
    radio.powerUp();
    delay(2);
    radio.stopListening();
    bool ok = radio.write(&m, sizeof m);
    if (ok) {
        flash(RF_GOOD_LED);
    } else {
        flash(RF_BAD_LED); 
    }
    lastmillivolts = readVcc();
    radio.startListening();
    radio.powerDown();
    
    if (m.s.millivolts < 2200) {
        flash(LO_BATT_LED);
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
    flash(RF_GOOD_LED);
  } else {
    flash(RF_BAD_LED); 
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
    pinMode(ADDRESS_PIN_0, INPUT_PULLUP);
    pinMode(ADDRESS_PIN_1, INPUT_PULLUP);
    pinMode(ADDRESS_PIN_2, INPUT_PULLUP);
    
    if(!digitalRead(ADDRESS_PIN_2)) {
        result = result + 4;  
    }
    
    if(!digitalRead(ADDRESS_PIN_1)) {
        result = result + 2;  
    }
    
    if(!digitalRead(ADDRESS_PIN_0)) {
        result = result + 1 ;  
    }
    
    // save power
    pinMode(ADDRESS_PIN_0, OUTPUT);
    digitalWrite(ADDRESS_PIN_0, LOW);
    pinMode(ADDRESS_PIN_1, OUTPUT);
    digitalWrite(ADDRESS_PIN_1, LOW);
    pinMode(ADDRESS_PIN_2, OUTPUT);
    digitalWrite(ADDRESS_PIN_2, LOW);
    
    return result; 
}

int getMyChannel() {
    int result = 0;
    pinMode(CHANNEL_PIN_0, INPUT_PULLUP);
    pinMode(CHANNEL_PIN_1, INPUT_PULLUP);
    pinMode(CHANNEL_PIN_2, INPUT_PULLUP);
    pinMode(CHANNEL_PIN_3, INPUT_PULLUP);
    
    if(!digitalRead(CHANNEL_PIN_3)) {
        result = result + 8;  
    }
    
    if(!digitalRead(CHANNEL_PIN_2)) {
        result = result + 4;  
    }
    
    if(!digitalRead(CHANNEL_PIN_1)) {
        result = result + 2;  
    }
    
    if(!digitalRead(CHANNEL_PIN_0)) {
        result = result + 1 ;  
    }
    
    // save power
    pinMode(CHANNEL_PIN_0, OUTPUT);
    digitalWrite(CHANNEL_PIN_0, LOW);
    pinMode(CHANNEL_PIN_1, OUTPUT);
    digitalWrite(CHANNEL_PIN_1, LOW);
    pinMode(CHANNEL_PIN_2, OUTPUT);
    digitalWrite(CHANNEL_PIN_2, LOW);
    pinMode(CHANNEL_PIN_3, OUTPUT);
    digitalWrite(CHANNEL_PIN_3, LOW);
    
    return result;
}

boolean shouldEncrypt() {
    boolean result = false;
    pinMode(ENCRYPT_PIN, INPUT_PULLUP);
    
    if(!digitalRead(ENCRYPT_PIN)) {
        result = true;   
    }
    
    pinMode(ENCRYPT_PIN, OUTPUT);
    digitalWrite(ENCRYPT_PIN, LOW);
    
    return result;
}

bool keyIsEmpty(byte * key) {
    for (int i = 0; i < EEPROM_KEY_SIZE; i++) {
        if (key[i] != 0xff) {
            return false;   
        }
    }
    return true;
}

void generateKey(byte * key) {
   randomSeed(analogRead(0));
   for (int i = 0; i < EEPROM_KEY_SIZE; i++) {
       key[i] = (unsigned char) random(0,255);  
   } 
}
