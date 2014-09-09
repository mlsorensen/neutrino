#include <Arduino.h>
#include <JsonParser.h>
#include <JsonGenerator.h>

using namespace ArduinoJson::Generator;
using namespace ArduinoJson::Parser;

void test_relay(int pin);
void flush_serial();
void ack(bool success, char * msg);

int my_putc( char c, FILE *t) {
    Serial.write( c );
}

void setup() {
    fdevopen( &my_putc, 0);
    Serial.begin(57600);
    printf("Starting AVR\r\n");

    pinMode(2,OUTPUT);
    pinMode(3,OUTPUT);
    pinMode(4,OUTPUT);
    pinMode(5,OUTPUT);
    pinMode(9,OUTPUT);
    pinMode(10,OUTPUT);
/*    
    test_relay(2);
    test_relay(3);
    test_relay(4);
    test_relay(5);
    test_relay(9);
    test_relay(10);
*/
}

void loop() {
    String msg; 

    // fetch json message
    if(Serial.available()) {
        while(1) {
            if(Serial.available()) {
                char chr = Serial.read();
                msg += chr;
                if (chr == '}') {
                    break;
                }
            }
        }
    }

    // act on message
    if (msg.length() > 0) {
        char * cstr = new char [msg.length() + 1];
        strcpy(cstr, msg.c_str());
        
        JsonParser<32> parser;
        ArduinoJson::Parser::JsonObject incoming = parser.parse(cstr);

        if (!incoming.success()) {
            Serial.println("failed to parse json");
        }

        if (strcmp(incoming["msgtype"],"setrelay") == 0) {
            ack(1, "got setrelay");
        } else if (strcmp(incoming["msgtype"],"test") == 0) {
            ack(1, "got test");
        } else {
            ack(0, "got unknown msgtype");
        }
    }
}

void ack(bool success, char * msg) {
    ArduinoJson::Generator::JsonObject<3> outgoing;
    outgoing["msgtype"] = "ack";
    outgoing["result"] = success;
    outgoing["detail"] = msg;

    Serial.println(outgoing);
}

void test_relay(int pin) {
    printf("testing pin %d\r\n",pin);
    digitalWrite(pin,HIGH);
    delay(500);
    digitalWrite(pin,LOW);
    delay(1000);
}

void flush_serial() {
    int i = 0;
    for (i = 0; i < 10; i++) {
        Serial.read();
    }
}
