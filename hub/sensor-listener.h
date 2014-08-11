#ifndef SENSOR_LISTENER_H
#define SENSOR_LISTENER_H

#include <mysql.h>
#include <libconfig.h++>
#include <RF24.h>

#define SKIPJACK_KEY_SIZE 10
#define SIGNATURE_KEY_SIZE 4
#define SIGNATURE_SIZE 6
#define PAYLOAD_SIZE 17

using namespace libconfig;

const char version[]           = "0.1";
const char progname[]          = "sensor-listener";

Config cfg;
const char * zabbixserver;
const char * zabbixclient;
int zabbixport;

const char * mysqlserver;
const char * mysqldb;
const char * mysqluser;
const char * mysqlpass;

int sensorhubid;

//nRF24L01+, raspberry pi vs beaglebone black macros
#if defined __ARM_ARCH_6__
    RF24 radio("/dev/spidev0.0",8000000 , 25);
#elif defined __ARM_ARCH_7A__
    RF24 radio(115, 117);
#else // build on non-arm with bogus initializer
    RF24 radio(115, 117);
#endif
uint64_t pipes[6];

// this struct should always be a multiple of 64 bits so we can easily encrypt it (skipjack 64bit blocks)
struct __attribute__((packed))
sensordata {
    uint8_t   addr;
    bool     proximity; // is reed switch open?
    int16_t  tempc; // temp in centicelsius
    int16_t  humidity; // humidity in basis points (percent of percent)
    uint16_t pressuredp; // pressure in decapascals
    uint16_t millivolts; // battery voltage millivolts
    uint8_t  signature[SIGNATURE_SIZE]; //truncated md5 of sensor data
};

struct __attribute__((packed))
pairingdata {
    unsigned char enckey[SKIPJACK_KEY_SIZE]; // 10 bytes
    unsigned char sigkey[SIGNATURE_KEY_SIZE]; // 4 bytes
    int16_t padding; // 2 bytes
};

/* keep messages small, increases radio range. Currently 17 bytes (PAYLOAD_SIZE).

header is one byte that represents the following:

MSB  00000000  LSB
     RRRAAAMM

MM - message type
     00 - contains sensor data message
     01 - pairing, contains keys message
     10 - reserved
     11 - reserved

AAA - sensor address (if applicable)

RRR - reserved
*/

struct message {
    int8_t header;
    char data[PAYLOAD_SIZE - 1];
};

struct sensordatamessage {
    int8_t header; // 1 byte
    sensordata data; // 16 bytes, will encrypt
};

struct sensorpairingmessage {
    int8_t header; // 1 byte
    pairingdata data; // 16 bytes unencrypted
};

//prototypes
void radio_init();
void get_args(int argc, char * argv[]);
void usage();
bool zabbix_send(const char * key, const char * value);
bool publish_zabbix(sensordata * data);
int insert_neutrino_data(sensordata * data);
float ctof(int16_t c);
bool decrypt_sensordata(sensordata * data, int addr);
bool check_sensordata_signature(sensordata * data);
bool encryption_key_is_empty(char * key);
bool sensor_is_known(int sensorid);
bool listener_should_discover();
unsigned int get_sensor_id(MYSQL * conn, int sensorid, int sensorhubid);
MYSQL_ROW sql_select_row(const char * sql);
void handle_sensor_message(message * message, int addr);
void handle_pairing_message(message * message, int addr);


#endif
