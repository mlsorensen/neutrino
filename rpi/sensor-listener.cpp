#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <mysql.h>
#include <libconfig.h++>
#include "RF24.h"

using namespace libconfig;

const char version[]           = "0.1";
const char progname[]          = "sensor-listener";

// config
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
#endif
uint64_t pipes[6];
int payloadsize = 16;

struct __attribute__((packed))
weather {
    int8_t  addr;
    int16_t tempc;
    int16_t humidity;
    int32_t pressurep;
    int32_t millivolts;
};

//prototypes
void radio_init();
void get_args(int argc, char *argv[]);
void usage();
bool zabbix_send(const char * key, const char * value);
bool publish_zabbix(weather *w);
int insert_neutrino_data(weather *w);
float ctof(int16_t c);
unsigned int get_sensor_id(MYSQL *conn, int sensorid, int sensorhubid);

int main(int argc, char** argv) {
    //don't buffer what we print
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    //get arguments, set things up
    get_args(argc,argv);

    // set up radio
    radio_init();

    while(1) {
        if (radio.available()) {
            weather w;
            bool nread = radio.read(&w, sizeof w);
            if(nread) {
                ostringstream key;
                ostringstream value;

                printf("radio at address %d :\n",w.addr);
                printf("Temperature is %.2f F\n", ctof(w.tempc));
                printf("Temperature is %.2f C\n", (float)w.tempc / 100);
                printf("pressure is %d Pa\n", w.pressurep);
                printf("humidity is %.2f%\n", (float)w.humidity / 100);
                printf("voltage is %.3f \n", (float)w.millivolts / 1000);
                printf("size of w is %lu\n", sizeof w);

                if (strlen(zabbixserver) > 0 && strlen(zabbixclient) > 0) {
                    publish_zabbix(&w);
                }

                if (insert_neutrino_data(&w) == 0) {
                    fprintf(stderr, "inserted data for node %d, sensorhub %d into db\n", w.addr, sensorhubid);
                }
            } else {
                printf("!!!!!!\n failed to read from radio\n !!!!!!\n");
                radio.stopListening();
                radio.startListening();
            }
        } else {
            usleep(50000);
        }
    }

    return 0;
}

float ctof (int16_t c) {
    return c * .018 + 32;
}

int insert_neutrino_data(weather *w) {
    char sql[256];
    int resultcode  = 0;
    unsigned int sensorid = 0;
    MYSQL_RES *res_set;
    MYSQL_ROW row;
    MYSQL *conn;

    // open database, do this every time so that we reconnect in case of intermittent db outage
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
        fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
        exit(1);
    } else {
        fprintf(stdout, "Connected to database\n");
    }

    // ensure sensor is in sensor table
    sprintf(sql,"INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id) VALUES (%d, %d)", w->addr, sensorhubid);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on sensor insert: %s\n", mysql_error(conn));
        return -1;
    }
    bzero(sql, 256);

    // get sensor's db id
    sensorid = get_sensor_id(conn, w->addr, sensorhubid);
    if (sensorid == 0) {
        return -1;
    }

    // add data to data table
    sprintf(sql,"insert into data (sensor_id, voltage, fahrenheit, celsius, pascals, humidity) values (%u,%.3f,%.2f,%.2f,%ld,%.2f);",
                sensorid, (float)w->millivolts/1000, ctof(w->tempc) ,(float)w->tempc/100, (long)w->pressurep, (float)w->humidity/100);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on data insert: %s\n", mysql_error(conn));
        return -1;
    }

    mysql_close(conn);
    return 0;
}

unsigned int get_sensor_id(MYSQL *conn, int sensor_addr, int sensor_hub_id) {
    char sql[256];

    sprintf(sql, "SELECT id FROM sensor where sensor_address=%d and sensor_hub_id=%d",sensor_addr, sensor_hub_id);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "could not fetch sensor id from db: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL) {
        fprintf(stderr, "could not fetch sensor id from db: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row;

    row = mysql_fetch_row(result);
    unsigned int rowid = atoi(row[0]);
    mysql_free_result(result);
    return rowid;
}

void radio_init() {
    uint16_t channel = 0x4c;
    channel += (sensorhubid * 4); // separate the channels a bit
    radio.begin();
    radio.setChannel(channel);
    radio.setPALevel(RF24_PA_MAX);
    radio.setPayloadSize(payloadsize);
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(30,30);

    for(int i = 0; i < 6; i++) {
        uint64_t pipe = 0XFCFCFCFC00LL + (sensorhubid << 8) + i;
        printf("pipe %d is address %#012llx\n", i, pipe);
        pipes[i] = pipe;
        radio.openReadingPipe(i, pipes[i]);
    }

    radio.startListening();
    radio.printDetails();
}

bool zabbix_send(const char * key, const char * value) {
    // prepare struct
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    server = gethostbyname(zabbixserver);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(zabbixport);

    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening inet socket to zabbix\n");
    }
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        perror("ERROR connecting zabbix");
    }
    bzero(buffer, 256);
    sprintf(buffer, "{\"request\":\"sender data\",\"data\":[{\"value\":\"%s\",\"key\":\"%s\",\"host\":\"%s\"}]}\n", value, key, zabbixclient);
    int n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
        perror("unable to write to zabbix socket\n");
    } else {
        printf("wrote to zabbix: ");
        printf("%s",buffer);
    }
    close(sockfd);
}

bool publish_zabbix(weather *w) {
    ostringstream key;
    ostringstream value;

    key << "neutrino." << (int)w->addr << ".temperature.fahrenheit";
    value << std::fixed << std::setprecision(2) << ctof(w->tempc);
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)w->addr << ".temperature.celsius";
    value << std::fixed << std::setprecision(2) << (float)w->tempc/100;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)w->addr << ".pressure";
    value << (long)w->pressurep;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)w->addr << ".voltage";
    value << std::fixed << std::setprecision(3) << (float)w->millivolts/1000;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)w->addr << ".humidity";
    value << std::fixed << std::setprecision(2) << (float)w->humidity/100;
    zabbix_send(key.str().c_str(), value.str().c_str());
    
}

void usage() {
    fprintf(stderr,"\n\n%s version %s\n\n", progname, version);
    fprintf(stderr,"Usage:%s [options]...\n\n", progname);
    fprintf(stderr,"-c configfile\n");
}

void get_args(int argc, char *argv[]) {
    Config cfg;
    int c;
    const char * configfile;
    while ((c = getopt (argc, argv, "hc:")) != -1){
        switch(c) {
            case 'h':
                usage();
                exit(1);
            case 'c':
                configfile = optarg;
            break;
        }
    }

    if (strlen(configfile) == 0) {
        fprintf(stderr, "No config file supplied!\n");
        usage();
        exit(1);
    }

    try {
        cfg.readFile(configfile);
    } catch (const FileIOException &fioex) {
        fprintf(stderr, "Unable to read config file\n");
        exit(1);
    } catch (const ParseException &pex) {
        fprintf(stderr, "Parse error at %s : %d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
        exit(1);
    }

    try {
        mysqlserver = cfg.lookup("mysqlserver").c_str();
        mysqldb     = cfg.lookup("mysqldb").c_str();
        mysqluser   = cfg.lookup("mysqluser").c_str();
        mysqlpass   = cfg.lookup("mysqlpass").c_str();
        sensorhubid = atoi(cfg.lookup("sensorhubid").c_str());
        zabbixserver = cfg.lookup("zabbixserver").c_str();
        zabbixport   = atoi(cfg.lookup("zabbixport").c_str());
        zabbixclient = cfg.lookup("zabbixclient").c_str();
    } catch (const SettingNotFoundException &settingexception) {
        fprintf(stderr, "A setting was not found: %s -- set any unneeded options to ''\n", settingexception.getPath());
        exit(1);
    }

    printf("mysql server is '%s','%s'\n",cfg.lookup("mysqlserver").c_str(),mysqlserver);
}

