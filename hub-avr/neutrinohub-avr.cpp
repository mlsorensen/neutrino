#include <Arduino.h>
#include <JsonParser.h>
#include <JsonGenerator.h>
#include <Adafruit_NeoPixel.h>

#define HEAT_PIN     9
#define COOL_PIN     10
#define FAN_PIN      2
#define HUMIDIFY_PIN 3
#define HEAT2_PIN    4
#define COOL2_PIN    5

#define LEDPIN       6
#define LEDCOUNT     12

#define JSON_BUFFER_SIZE 128

using namespace ArduinoJson::Generator;
using namespace ArduinoJson::Parser;

void test_relay(int pin);
void ack(bool success, const char * msg);
bool update_relays();
void render_leds();
void blank_ledcolors();
bool is_idle();
void animate(const char * animation, int color[3]);
void smiley_face();
void dual_sweep_up(int r, int g, int b, int wait);
void dual_sweep_down(int r, int g, int b, int wait);
void fade_in_two(int ledleft, int ledright, int up, int r, int g, int b, int wait);
void fade_out(int*, int, int, int, int, int);
void fade_in(int*, int, int, int, int, int);
void fan();

struct Relaystates {
    bool heat     = 0;
    bool cool     = 0;
    bool fan      = 0;
    bool humidify = 0;
    bool heat2    = 0;
    bool cool2    = 0;
} relaystates;

int ledcolors [LEDCOUNT][3];
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LEDCOUNT, LEDPIN, NEO_GRB + NEO_KHZ800);

int my_putc( char c, FILE *t) {
    Serial.write( c );
}

void setup() {
    fdevopen( &my_putc, 0);
    Serial.begin(57600);
    printf("starting avr\r\n");

    pinMode(HEAT_PIN, OUTPUT);
    pinMode(COOL_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(HUMIDIFY_PIN, OUTPUT);
    pinMode(HEAT2_PIN, OUTPUT);
    pinMode(COOL2_PIN, OUTPUT);

    leds.begin();
    blank_ledcolors();
    render_leds();

    // startup animation
    int color[] = {0, 20, 90};
    animate("smiley_face",color);
    blank_ledcolors();
    delay(1000);
    render_leds();
}

void loop() {
    char jsonmsg[JSON_BUFFER_SIZE]; 
    int index = 0;

    // fetch json message
    if(Serial.available()) {
        while(1) {
            if(Serial.available()) {
                char single = Serial.read();
                if (single == '\r') {
                    break;
                }
                jsonmsg[index] = single;
                index++;
                jsonmsg[index] = '\0';
                if (index >= JSON_BUFFER_SIZE) {
                    // this is an error, and just a sanity check that we don't buffer overflow
                    break;
                }
            }
        }
    }

    if (sizeof(jsonmsg)) {
        JsonParser<32> jsonparser;
        ArduinoJson::Parser::JsonObject incoming = jsonparser.parse(jsonmsg);

        if (!incoming.success()) {
            ack(0,(const char*)"json failed to parse");
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
                update_relays();
                ack(1, (const char*)"cooling");
            } else if ((bool)incoming["relaystates"]["heat"] == 1) {
                relaystates.heat     = 1;
                relaystates.cool     = 0;
                relaystates.fan      = 1;
                relaystates.humidify = 0;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0;
                update_relays();
                ack(1, (const char*)"heating");
            } else if ((bool)incoming["relaystates"]["fan"] == 1) {
                relaystates.heat     = 0;
                relaystates.cool     = 0;
                relaystates.fan      = 1;
                relaystates.humidify = 0;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0;
                update_relays();
                ack(1, (const char*)"fan running");
            } else if ((bool)incoming["relaystates"]["humidify"] == 1) {
                relaystates.heat     = 0;
                relaystates.cool     = 0;
                relaystates.fan      = 1;
                relaystates.humidify = 1;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0;
                update_relays();
                ack(1, (const char*)"humidifying");
            } else if ((bool)incoming["relaystates"]["heat_humidify"] == 1) {
                relaystates.heat     = 1;
                relaystates.cool     = 0;
                relaystates.fan      = 1;
                relaystates.humidify = 1;
                relaystates.heat2    = 0;
                relaystates.cool2    = 0;
                update_relays();
                ack(1, (const char*)"heating");
            } else if ((bool)incoming["relaystates"]["idle"] == 1) {
                if (is_idle()) {
                    ack(1, (const char*)"already idle");
                } else {
                    relaystates.heat     = 0;
                    relaystates.cool     = 0;
                    relaystates.fan      = 1;
                    relaystates.humidify = 0;
                    relaystates.heat2    = 0;
                    relaystates.cool2    = 0;
                    update_relays();
                    delay(60000);
                    ack(1, (const char*)"idling, waited 60 seconds");
                    relaystates.fan      = 0;
                    update_relays();
                }
            } else {
                ack(0, (const char*)"provided unknown relay state");
            }
        } else if (strcmp(incoming["msgtype"],"test") == 0) {
            ack(1, (const char*)"got test");
        } else if (strcmp(incoming["msgtype"],"ledcolor") == 0) {
            for (int i = 0; i < LEDCOUNT; i++) {
                for (int j = 0; j < 3; j++) {
                    ledcolors[i][j] = atoi((char*)incoming["colors"][i][j]);
                }
            }
            render_leds();
            ack(1, (const char*)"set led color");
        } else if (strcmp(incoming["msgtype"],"renderleds") == 0) {
            render_leds();
            ack(1, (const char*)"rendered leds");
        } else if (strcmp(incoming["msgtype"],"animation") == 0) {
            int color[] = {atoi((char*)incoming["color"][0]),
                           atoi((char*)incoming["color"][1]),
                           atoi((char*)incoming["color"][2])};
            animate((const char*)incoming["name"],color);
            ack(1, "ran animation");
        }else {
            ack(0, (const char*)"got unknown msgtype");
        }
    }
}

bool is_idle() {
    return relaystates.heat == 0 && relaystates.cool == 0 && relaystates.fan == 0 && relaystates.humidify == 0 && relaystates.heat2 == 0 && relaystates.cool2 == 0;
}

void animate(const char * animation, int color[3]) {
    if (strcmp(animation,"smiley_face") == 0) {
        smiley_face();
    } else if (strcmp(animation, "dual_sweep_up") == 0) {
        dual_sweep_up(color[0], color[1], color[2], 1000);
    } else if (strcmp(animation, "dual_sweep_down") == 0) {
        dual_sweep_down(color[0], color[1], color[2], 1000);
    } else if (strcmp(animation, "fan") == 0) {
        fan();
    }
}

void smiley_face() {
    blank_ledcolors();
    int lit_leds1[] = {5,7};
    for (int i = 0; i < sizeof(lit_leds1)/sizeof(int); i++) {
        ledcolors[lit_leds1[i]][1] = 100;    
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds1[i]][j] = 100;
        }
    }
    render_leds();
    delay(1000);
    int lit_leds2[] = {0,5,7};
    for (int i = 0; i < sizeof(lit_leds2)/sizeof(int); i++) {
        ledcolors[lit_leds2[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds2[i]][j] = 100;
        }
    }
    render_leds();
    delay(100);
    int lit_leds3[] = {0,1,5,7,11};
    for (int i = 0; i < sizeof(lit_leds3)/sizeof(int); i++) {
        ledcolors[lit_leds3[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds3[i]][j] = 100;
        }
    }
    render_leds();
    delay(100);
    int lit_leds4[] = {0,1,2,5,7,10,11};
    for (int i = 0; i < sizeof(lit_leds4)/sizeof(int); i++) {
        ledcolors[lit_leds4[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds4[i]][j] = 100;
        }
    }
    render_leds();
    delay(1000);
    blank_ledcolors();
    int lit_leds5[5] = {0,1,2,10,11};
    for (int i = 0; i < sizeof(lit_leds5)/sizeof(int); i++) {
        ledcolors[lit_leds5[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds5[i]][j] = 100;
        }
    }
    render_leds();
    delay(200);
    for (int i = 0; i < sizeof(lit_leds4)/sizeof(int); i++) {
        ledcolors[lit_leds4[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds4[i]][j] = 100;
        }
    }
    render_leds();
    delay(400);
    blank_ledcolors();
    for (int i = 0; i < sizeof(lit_leds5)/sizeof(int); i++) {
        ledcolors[lit_leds5[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds5[i]][j] = 100;
        }
    }
    render_leds();
    delay(200);
    for (int i = 0; i < sizeof(lit_leds4)/sizeof(int); i++) {
        ledcolors[lit_leds4[i]][1] = 100;
        for (int j=0; j < 3; j++) {
            ledcolors[lit_leds4[i]][j] = 100;
        }
    }
    render_leds();
}

void dual_sweep_up(int r, int g, int b, int wait) {
    blank_ledcolors();
    render_leds();
    for (int i = 0; i <= LEDCOUNT/2; i++) {
        fade_in_two(i, LEDCOUNT - i, 1, r, g, b, wait);
    }
    blank_ledcolors();
    render_leds();
}

void dual_sweep_down(int r, int g, int b, int wait) {
    blank_ledcolors();
    render_leds();
    for (int i = LEDCOUNT/2; i >= 0; i--) {
        fade_in_two(LEDCOUNT - i, i, 0, r, g, b, wait);
    }
    blank_ledcolors();
    render_leds();
}

void fade_in_two(int ledleft, int ledright, int up, int r, int g, int b, int wait) {
    // multiply by 100 for finer resolution without floats
    r = r * 100;
    g = g * 100;
    b = b * 100;
    
    // save starting values
    int rstart = r;
    int gstart = g;
    int bstart = b;
    
    // divide by 255 to get increments
    int rinc = rstart / 128;
    int ginc = gstart / 128;
    int binc = bstart / 128;
    
    r = 0;
    g = 0;
    b = 0;
    
    int rold = rstart;
    int gold = gstart;
    int bold = bstart;
    
    int oldledleft  = -1;
    int oldledright = 13;
    
    if (up == 1) {
        oldledleft  = ledleft - 1;
        oldledright = ledright + 1;
    } else {
        oldledleft  = ledleft;
        ledleft = oldledleft + 1;
        oldledright = ledright;
        ledright = oldledright -1;
    }
    
    for (int i = 255; i > 0; i -= 2) {
        leds.setPixelColor(ledleft, leds.Color(r/100, g/100, b/100));
        leds.setPixelColor(ledright, leds.Color(r/100, g/100, b/100));
        if (oldledleft >= 0) {
           leds.setPixelColor(oldledleft, leds.Color(rold/100, gold/100, bold/100));     
        }
        if (oldledright <= LEDCOUNT) {
           leds.setPixelColor(oldledright, leds.Color(rold/100, gold/100, bold/100)); 
        }    
        leds.show();
        delayMicroseconds(wait);
        r = r + rinc;
        g = g + ginc;
        b = b + binc;
        rold = rold - rinc;
        gold = gold - ginc;
        bold = bold - binc;
    }
    
    // fractions of increments sometimes mean that the leds don't get turned all the way off.
    if (oldledleft >= 0) {
        leds.setPixelColor(oldledleft, leds.Color(0,0,0));
    }
    if (oldledright <= LEDCOUNT) {
        leds.setPixelColor(oldledright, leds.Color(0,0,0)); 
    } 
    leds.show();
}

void fan() {
    int led1[3] = {0,4,8};
    int led2[3] = {1,5,9};
    int led3[3] = {2,6,10};
    int led4[3] = {3,7,11};
    int r = 50;
    int g = 50;
    int b = 10;
    int wait = 400;

    fade_in(led1, 3, r, g, b, wait);
    fade_in(led2, 3, r, g, b, wait);
    fade_out(led1, 3, r, g, b, wait);
    fade_in(led3, 3, r, g, b, wait);
    fade_out(led2, 3, r, g, b, wait);
    fade_in(led4, 3, r, g, b, wait);
    fade_out(led3, 3, r, g, b, wait);
    fade_in(led1, 3, r, g, b, wait);
    fade_out(led4, 3, r, g, b, wait);
    fade_in(led2, 3, r, g, b, wait);
    fade_out(led1, 3, r, g, b, wait);
    fade_in(led3, 3, r, g, b, wait);
    fade_out(led2, 3, r, g, b, wait);
    fade_in(led4, 3, r, g, b, wait);
    fade_out(led3, 3, r, g, b, wait);
    fade_out(led4, 3, r, g, b, wait);

    blank_ledcolors();
    render_leds(); 
}

void fade_out(int * led, int size, int r, int g, int b, int wait) {
    // multiply by 100 for finer resolution without floats
    r = r * 100;
    g = g * 100;
    b = b * 100;
    
    // save starting values
    int rstart = r;
    int gstart = g;
    int bstart = b;
    
    // divide by 255 to get increments
    int rinc = rstart / 128;
    int ginc = gstart / 128;
    int binc = bstart / 255;
    
    for (int i = 255; i > 0; i -= 2) {
        for (int j = 0; j < size; j++) {
            leds.setPixelColor(led[j], leds.Color(r/100, g/100, b/100));
        }  
        leds.show();
        delayMicroseconds(wait);
        r = r - rinc;
        g = g - ginc;
        b = b - binc;
    }
    for (int j = 0; j < size; j++) {
            leds.setPixelColor(led[j], leds.Color(0,0,0));
        }
}

void fade_in(int * led, int size, int r, int g, int b, int wait) {
    // multiply by 100 for finer resolution without floats
    r = r * 100;
    g = g * 100;
    b = b * 100;
    
    // save starting values
    int rstart = r;
    int gstart = g;
    int bstart = b;
    
    // divide by 255 to get increments
    int rinc = rstart / 128;
    int ginc = gstart / 128;
    int binc = bstart / 128;
    
    r = 0;
    g = 0;
    b = 0;
    
    for (int i = 255; i > 0; i -= 2) {
        for (int j = 0; j < size; j++) {
            leds.setPixelColor(led[j], leds.Color(r/100, g/100, b/100));
        }    
        leds.show();
        delayMicroseconds(wait);
        r = r + rinc;
        g = g + ginc;
        b = b + binc;
    }
}

void blank_ledcolors() {
    for (int i=0; i < LEDCOUNT; i++) {
        for (int j=0; j < 3; j++) {
            ledcolors[i][j] = 0;
        }
    }
}

void render_leds() {
    for (int i=0; i < LEDCOUNT; i++) {
        leds.setPixelColor(i, leds.Color(ledcolors[i][0], ledcolors[i][1], ledcolors[i][2]));
    }
    leds.show();
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

void ack(bool success, const char * msg) {
    ArduinoJson::Generator::JsonObject<8> outgoing;
    outgoing["msgtype"] = "ack";
    outgoing["result"] = success;
    outgoing["detail"] = msg;

    Serial.print(outgoing);
    Serial.print("\r\n");
}

void test_relay(int pin) {
    printf("testing pin %d\r\n",pin);
    digitalWrite(pin,HIGH);
    delay(500);
    digitalWrite(pin,LOW);
    delay(1000);
}
