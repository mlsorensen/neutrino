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
Config cfg;
const char * zabbixserver;
const char * zabbixclient;
int zabbixport;

const char * mysqlserver;
const char * mysqldb;
const char * mysqluser;
const char * mysqlpass;

int sensorhubid;

//nRF24L01+
RF24 radio("/dev/spidev0.0",8000000 , 25);  //spi device, speed and CSN,only CSN is NEEDED in RPI
uint64_t pipes[6];
int payloadsize = 16;

struct __attribute__((packed))
weather {
    int8_t  addr;
    int16_t tempf;
    int32_t pressurep;
    int32_t altitude;
    int32_t millivolts;
};

//prototypes
void radio_init();
void get_args(int argc, char *argv[]);
void usage();
bool zabbix_send(const char * key, const char * value);
bool publish_zabbix(weather *w);
int insert_neutrino_data(MYSQL *conn, weather *w);

int main(int argc, char** argv) {
    //don't buffer what we print
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    //get arguments, set things up
    get_args(argc,argv);

    // set up radio
    radio_init();

    // open database
    MYSQL *conn;
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
        fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
        exit(1);
    } else {
        fprintf(stdout, "Connected to database\n");
    }

    while(1) {
        if (radio.available()) {
            weather w;
            bool nread = radio.read(&w, sizeof w);
            if(nread) {
                ostringstream key;
                ostringstream value;

                printf("radio at address %d :\n",w.addr);
                printf("Temperature is %.2f Fht\n", (float)w.tempf/100);
                printf("pressure is %d Pa\n", w.pressurep);
                printf("altitude is %.2f ft\n", (float)w.altitude/100);
                printf("voltage is %.3f \n", (float)w.millivolts/1000);
                printf("size of w is %lu\n", sizeof w);

                if (strlen(zabbixserver) > 0 && strlen(zabbixclient) > 0) {
                    publish_zabbix(&w);
                }

                if (insert_neutrino_data(conn, &w) == 0) {
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

int insert_neutrino_data(MYSQL *conn, weather *w) {
    char sql[256];
    int resultcode  = 0;

    // ensure sensor is in sensor table
    sprintf(sql,"INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id) VALUES (%d, %d)", w->addr, sensorhubid);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on sensor insert: %s\n", mysql_error(conn));
        return -1;
    }

    // add data to data table
    sprintf(sql,"insert into data (sensor_address, voltage, temperature, pressure) values (%d,%.3f,%.2f,%ld);",
                w->addr, (float)w->millivolts/1000, (float)w->tempf/100, (long)w->pressurep);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on data insert: %s\n", mysql_error(conn));
        return -1;
    }
    return 0;
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

    key << "neutrino." << (int)w->addr << ".temperature";
    value << std::fixed << std::setprecision(2) << (float)w->tempf/100;
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
    
}

void usage() {
    fprintf(stderr,"\n\n%s version %s\n\n", progname, version);
    fprintf(stderr,"Usage:%s [options]...\n\n", progname);
    fprintf(stderr,"-c configfile\n");
}

void get_args(int argc, char *argv[]) {
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

    mysqlserver = cfg.lookup("mysqlserver").c_str();
    mysqldb     = cfg.lookup("mysqldb").c_str();
    mysqluser   = cfg.lookup("mysqluser").c_str();
    mysqlpass   = cfg.lookup("mysqlpass").c_str();

    sensorhubid = atoi(cfg.lookup("sensorhubid").c_str());

    zabbixserver = cfg.lookup("zabbixserver").c_str();
    zabbixport   = atoi(cfg.lookup("zabbixport").c_str());
    zabbixclient = cfg.lookup("zabbixclient").c_str();

    printf("mysql server is '%s','%s'\n",cfg.lookup("mysqlserver").c_str(),mysqlserver);
}

