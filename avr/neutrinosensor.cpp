#include <Arduino.h>
#include <Wire.h>
#include <BMP180.h>
#include <SI7021.h>
#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <LowPower.h>
#include <skipjack.h>
#include <hmac-md5.h>
#include <sha1.h>

#define EEPROM_ENC_KEY_ADDR 0x00
#define EEPROM_ENC_KEY_SIZE 10
#define EEPROM_SIG_KEY_ADDR 0x0a
#define EEPROM_SIG_KEY_SIZE 4

#define SIG_SIZE 6

#define PAYLOAD_SIZE 17

#define CHANNEL_PIN_0 10
#define CHANNEL_PIN_1 A3
#define CHANNEL_PIN_2 A2

#define ADDRESS_PIN_0 5
#define ADDRESS_PIN_1 6
#define ADDRESS_PIN_2 7
#define ADDRESS_PIN_3 0
#define ADDRESS_PIN_4 1

#define PROXIMITY_INT 0
#define PROXIMITY_PIN 2

#define PAIRING_INT 1
#define PAIRING_PIN 3

#define BOUNCE_DURATION 60

#define RF_GOOD_LED A0
#define RF_BAD_LED 4
#define LO_BATT_LED A1

//prototypes

void flash(int pin);
void pulse(int pin);
void rflog(char * msg);
long readVcc();
int getMyAddr();
int getMyChannel();
bool keyIsEmpty(byte * key, int size);
void generateKey(byte * key, int size);
void proximityTrigger();
void pairingTrigger();
void sendDataMessage();
void sendPairingMessage();

int myaddr = getMyAddr();
// commented out until hardware supports channel pins
int mychannel = getMyChannel();
int32_t lastmillivolts = 0;

volatile bool proximitytrigger = false;
volatile bool pairingtrigger = false;
volatile unsigned long bouncetime=0;

// this struct should always be a multiple of 64 bits so we can easily encrypt it (skipjack 64bit blocks)
// data is passed as integers, therefore decimals are shifted where noted to maintain precision
struct sensordata {
    uint8_t  addr         = myaddr;
    bool     proximity    = true; // sensor closed or open
    int16_t  tempc        = 0; // temp in centicelsius
    int16_t  humidity     = 0; // humidity in basis points (percent of percent)
    uint16_t pressuredp   = 0; // pressure in decapascals
    uint16_t millivolts   = 0; // battery voltage millivolts
    uint8_t  signature[SIG_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //truncated md5 of sensor data
};

struct pairingdata {
    byte enckey[EEPROM_ENC_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 10 bytes
    byte sigkey[EEPROM_SIG_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00}; // 4 bytes
    int16_t padding = 0; // 2 bytes
};

/* keep messages small, increases radio range. Currently 17 bytes (PAYLOAD_SIZE).

header is one byte that represents the following:

MSB  00000000  LSB
     RAAAAAMM

MM - message type
     00 - contains sensor data message
     01 - pairing, contains keys message
     10 - reserved
     11 - reserved

AAAAA - sensor address (if applicable)

R - reserved
*/

struct sensordatamessage {
    int8_t header = (myaddr << 2) + 0x00; // 1 byte
    sensordata data; // 16 bytes, will encrypt
};

struct sensorpairingmessage {
    int8_t header = (myaddr << 2) + 0x01; // 1 byte
    pairingdata data; // 16 bytes unencrypted
};

RF24 radio(8,9);
const uint64_t pipe = 0xFCFCFCFC00LL + (mychannel << 8) + (myaddr % 6);
//const uint64_t pipes[6] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; 
BMP180 bsensor;
SI7021 hsensor;
byte encryptionkey[EEPROM_ENC_KEY_SIZE];
byte signaturekey[EEPROM_SIG_KEY_SIZE];

void setup() {
    // populate encryption key
    eeprom_read_block((void*)&encryptionkey, (const void*)EEPROM_ENC_KEY_ADDR, EEPROM_ENC_KEY_SIZE);
    if (keyIsEmpty(encryptionkey, EEPROM_ENC_KEY_SIZE)) {
        generateKey(encryptionkey, EEPROM_ENC_KEY_SIZE);
        eeprom_write_block((const void*)&encryptionkey, (void*)EEPROM_ENC_KEY_ADDR, EEPROM_ENC_KEY_SIZE);
    }

    // populate signature key
    eeprom_read_block((void*)&signaturekey, (const void*)EEPROM_SIG_KEY_ADDR, EEPROM_SIG_KEY_SIZE);
    if (keyIsEmpty(signaturekey, EEPROM_SIG_KEY_SIZE)) {
        generateKey(signaturekey, EEPROM_SIG_KEY_SIZE);
        eeprom_write_block((const void*)&signaturekey, (void*)EEPROM_SIG_KEY_ADDR, EEPROM_SIG_KEY_SIZE);
    }
    
    // turn off analog comparator
    ACSR = B10000000;
    // turn off digital input buffers for analog pins
    //DIDR0 = DIDR0 | B00111111;
    
    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS); 
    sleep_cpu();
  
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
    radio.setRetries(6, 4);
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.setChannel(mychannel);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(pipe);
    radio.startListening();
    
    hsensor.begin();
    bsensor.begin(1);
    lastmillivolts = readVcc();

    // proximity switch interrupt
    pinMode(PROXIMITY_PIN, INPUT_PULLUP);
    attachInterrupt(PROXIMITY_INT, proximityTrigger, HIGH);

    // pairing button interrupt
    pinMode(PAIRING_PIN, INPUT_PULLUP);
    attachInterrupt(PAIRING_INT, pairingTrigger, LOW);
}

void proximityTrigger() {
    if (abs(millis() - bouncetime) > BOUNCE_DURATION) {
        proximitytrigger = true;
        bouncetime = millis();
    }
}

void pairingTrigger() {
    if (abs(millis() - bouncetime) > BOUNCE_DURATION) {
        pairingtrigger = true;
        bouncetime = millis();
    }
}

void loop() {
    if (digitalRead(PAIRING_PIN) == LOW) {
        sendPairingMessage();
        pairingtrigger = false;
    } else {
        sendDataMessage();
    }

    // power down for 56 seconds
    for (int i = 0; i < 6; i++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
}

void sendDataMessage() {
    sensordatamessage m;
    // humidity, temp
    if (hsensor.sensorExists()) {
        si7021_env sidata = hsensor.getHumidityAndTemperature();
        m.data.humidity        = sidata.humidityBasisPoints;
        m.data.tempc           = sidata.celsiusHundredths;
    }

    // pressure
    if (bsensor.sensorExists()) {
        // fall back to bosch sensor for temperature if necessary
        if(! hsensor.sensorExists()) {
            m.data.tempc = bsensor.getCelsiusHundredths();
        }
        m.data.pressuredp = bsensor.getPressurePascals() / 10;

    }

    // voltage was read after last radio send to get reading after high power draw
    m.data.millivolts = lastmillivolts;

    if (digitalRead(PROXIMITY_PIN) == LOW) {
        m.data.proximity = true;
    } else {
        m.data.proximity = false;
    }
    proximitytrigger = false;

    uint8_t hmac[16];
    hmac_md5(&hmac, &signaturekey, EEPROM_SIG_KEY_SIZE * 8, &m.data, 80);
    memcpy(&m.data.signature, &hmac, SIG_SIZE);
    // encrypt memory blocks containing sensor data. skipjack is 64 bit (8 byte) blocks
    for (int i = 0; i < sizeof(m.data); i += 8) {
        int ptrshift = i * 8;
        skipjack_enc((&m.data + ptrshift),&encryptionkey);
    }

    radio.powerUp();
    delay(2);
    radio.stopListening();
    bool ok = radio.write(&m, sizeof m);
    radio.startListening();
    radio.powerDown();

    if (ok) {
        flash(RF_GOOD_LED);
    } else {
        flash(RF_BAD_LED);
    }
    lastmillivolts = readVcc();

    if (m.data.millivolts < 2200) {
        flash(LO_BATT_LED);
    }
}

void sendPairingMessage() {
    sensorpairingmessage m;
    memcpy(&m.data.enckey, &encryptionkey, EEPROM_ENC_KEY_SIZE);
    memcpy(&m.data.sigkey, &signaturekey, EEPROM_SIG_KEY_SIZE);

    radio.powerUp();
    delay(2);
    radio.stopListening();
    bool ok = radio.write(&m, sizeof m);
    radio.startListening();
    radio.powerDown();

    if (ok) {
        flash(RF_GOOD_LED);
    } else {
        flash(RF_BAD_LED);
    }
}

void flash(int pin) {
  digitalWrite(pin, HIGH);
  delay(3);
  digitalWrite(pin, LOW);
  delay(1);
}

void pulse(int pin) {
   // software pwm, flash light for ~1 second with short duty cycle
   for (int i = 0; i < 40; i++) {
       digitalWrite(pin, HIGH);
       delay(1);
       digitalWrite(pin,LOW);
       delay(4);
   }
   digitalWrite(pin,LOW);
   LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);   
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
    pinMode(ADDRESS_PIN_3, INPUT_PULLUP);
    pinMode(ADDRESS_PIN_4, INPUT_PULLUP);

    if(!digitalRead(ADDRESS_PIN_4)) {
        result = result + 16;
    }

    if(!digitalRead(ADDRESS_PIN_3)) {
        result = result + 8;
    }
    
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
    pinMode(ADDRESS_PIN_3, OUTPUT);
    digitalWrite(ADDRESS_PIN_3, LOW);
    pinMode(ADDRESS_PIN_4, OUTPUT);
    digitalWrite(ADDRESS_PIN_4, LOW);
    
    return result; 
}

int getMyChannel() {
    int result = 0;
    pinMode(CHANNEL_PIN_0, INPUT_PULLUP);
    pinMode(CHANNEL_PIN_1, INPUT_PULLUP);
    pinMode(CHANNEL_PIN_2, INPUT_PULLUP);
    
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

    // we use channels 60-75
    result += 60;

    return result;
}

bool keyIsEmpty(byte * key, int size) {
    for (int i = 0; i < size; i++) {
        if (key[i] != 0xff) {
            return false;   
        }
    }
    return true;
}

void generateKey(byte * key, int keySize) {
    randomSeed(analogRead(A0)); // random() will be used only to set the delays between samples
    uint32_t seedSize = keySize * 100;

    unsigned char seed[seedSize];
    int e = 0; // entropy
    int r = 0; // remaining unused bits, because 8 doesn't divide neatly into 10
    digitalWrite(RF_BAD_LED, HIGH);
    digitalWrite(LO_BATT_LED, HIGH);
    for (int i = 0; i < seedSize; ++i) {
        seed[i] = (unsigned char)(e >> (keySize - r));
        e = analogRead(A0);
        seed[i] = (unsigned char)(seed[i] | (e & (0x3FF >> r)));
        r = 10 - r;
        delay(random(1, 20)); // The correct delay range is hardware-dependent; this is just an example.
    }
    digitalWrite(RF_BAD_LED, LOW);
    digitalWrite(LO_BATT_LED, LOW);

    unsigned char hash[20]; // SHA-1 produces a 160-bit hash.
    sha1(hash, seed, seedSize);

    // Hash the hash, because we actually want an 80-bit key.
    for(int i = 0; i < keySize; ++i) {
        key[i] = (unsigned char)(hash[2*i] ^ hash[2*i +1]); // bitwise XOR, the poor man's hash
    }
    pulse(RF_GOOD_LED);
}
