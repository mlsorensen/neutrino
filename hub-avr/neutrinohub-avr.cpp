#include <Arduino.h>

void test_relay(int pin);

void setup() {
    Serial.begin(57600);
    Serial.println("Starting AVR\n");

    pinMode(2,OUTPUT);
    pinMode(3,OUTPUT);
    pinMode(4,OUTPUT);
    pinMode(5,OUTPUT);
    pinMode(9,OUTPUT);
    pinMode(10,OUTPUT);
    
    test_relay(2);
    test_relay(3);
    test_relay(4);
    test_relay(5);
    test_relay(9);
    test_relay(10);
}

void loop() {
    delay(1000);
}

void test_relay(int pin) {
    Serial.print("testing pin");
    Serial.println(pin);
    digitalWrite(pin,HIGH);
    delay(500);
    digitalWrite(pin,LOW);
    delay(1000);
}
