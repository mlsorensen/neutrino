#include <Arduino.h>
#include <JsonParser.h>
#include <JsonGenerator.h>

#define HEAT_PIN     2
#define COOL_PIN     3
#define FAN_PIN      4
#define HUMIDIFY_PIN 5
#define HEAT2_PIN    9
#define COOL2_PIN    10

using namespace ArduinoJson::Generator;
using namespace ArduinoJson::Parser;

void test_relay(int pin);
void ack(bool success, char * msg);
bool update_relays();

struct Relaystates {
    bool heat     = 0;
    bool cool     = 0;
    bool fan      = 0;
    bool humidify = 0;
    bool heat2    = 0;
    bool cool2    = 0;
} relaystates;

int my_putc( char c, FILE *t) {
    Serial.write( c );
}

void setup() {
    fdevopen( &my_putc, 0);
    Serial.begin(57600);
    printf("Starting AVR\r\n");

    pinMode(HEAT_PIN, OUTPUT);
    pinMode(COOL_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(HUMIDIFY_PIN, OUTPUT);
    pinMode(HEAT2_PIN, OUTPUT);
    pinMode(COOL2_PIN, OUTPUT);
}

void loop() {
    String msg; 

    // fetch json message
    if(Serial.available()) {
        while(1) {
            if(Serial.available()) {
                char chr = Serial.read();
                if (chr == '\r') {
                    break;
                }
                msg += chr;
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
            ack(0,"json failed to parse");
            return;
        }

        if (strcmp(incoming["msgtype"],"setrelays") == 0) {
            if ((bool)incoming["relaystates"]["cool"] == 1) {
                relaystates.heat     = 0;
                relaystates.cool     = 1;
                relaystates.fan      = 1;
                relaystates.humidify = 0;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0;
            } else if ((bool)incoming["relaystates"]["heat"] == 1) {
                relaystates.heat     = 1;
                relaystates.cool     = 0;
                relaystates.fan      = 1;
                relaystates.humidify = 0;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0; 
            }
            update_relays();
            ack(1, "got relay states");
        } else if (strcmp(incoming["msgtype"],"clearrelays") == 0) {
            relaystates.heat     = 0;
            relaystates.cool     = 0;
            relaystates.fan      = 0;
            relaystates.humidify = 0;
            relaystates.heat2    = 0;
            relaystates.cool2    = 0;
            update_relays();
            ack(1, "got clear relays"); 
        } else if (strcmp(incoming["msgtype"],"test") == 0) {
            ack(1, "got test");
        } else {
            ack(0, "got unknown msgtype");
        }
    }
}

bool update_relays() {
    digitalWrite(HEAT_PIN, relaystates.heat);
    digitalWrite(COOL_PIN, relaystates.cool);
    digitalWrite(FAN_PIN, relaystates.fan);
    digitalWrite(HUMIDIFY_PIN, relaystates.humidify);
    digitalWrite(HEAT2_PIN, relaystates.heat2);
    digitalWrite(COOL2_PIN, relaystates.cool2);
    return true;
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
