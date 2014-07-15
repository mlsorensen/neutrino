#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <mysql.h>
#include <libconfig.h++>
#include <RF24.h>
#include <skipjack.h>

#define SKIPJACK_KEY_SIZE 10

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

//nRF24L01+, raspberry pi vs beaglebone black macros
#if defined __ARM_ARCH_6__
    RF24 radio("/dev/spidev0.0",8000000 , 25);
#elif defined __ARM_ARCH_7A__
    RF24 radio(115, 117);
#endif
uint64_t pipes[6];

// this struct should always be a multiple of 64 bits so we can easily encrypt it (skipjack 64bit blocks)
struct __attribute__((packed))
sensordata {
    int8_t  addr;
    int8_t  placeholder1;
    int16_t tempc;
    int16_t humidity;
    int16_t placeholder2;
    int32_t pressurep;
    int32_t millivolts;
};

// struct message = 28 bytes. Want to keep this <= 32 bytes so that it will fit in one radio message
struct message {
    int8_t addr;
    bool encrypted;
    char key[SKIPJACK_KEY_SIZE];
    sensordata s;
};

//prototypes
void radio_init();
void get_args(int argc, char *argv[]);
void usage();
bool zabbix_send(const char * key, const char * value);
bool publish_zabbix(message *m);
int insert_neutrino_data(message *m);
float ctof(int16_t c);
bool decrypt_sensordata(message *m);
bool encryption_key_is_empty(char * key);
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
            message m;
            bzero(m.key, SKIPJACK_KEY_SIZE);
            bool nread = radio.read(&m, sizeof m);
            if(nread) {
                ostringstream key;
                ostringstream value;

                if(m.encrypted  == true && decrypt_sensordata(&m) == false) {
                    if (decrypt_sensordata(&m) == false || m.key == 0x00) {
                        printf("message from sensor address %d was encrypted and unable to decrypt\n", m.addr);
                        continue;
                    }
                }

                printf("radio at address %d :\n",m.addr);
                printf("encryption key is %s\n",m.key);
                printf("encryption is %s\n", m.encrypted ? "true" : "false");
                printf("Temperature is %.2f F\n", ctof(m.s.tempc));
                printf("Temperature is %.2f C\n", (float)m.s.tempc / 100);
                printf("pressure is %d Pa\n", m.s.pressurep);
                printf("humidity is %.2f%\n", (float)m.s.humidity / 100);
                printf("voltage is %.3f \n", (float)m.s.millivolts / 1000);
                printf("size of w is %lu\n", sizeof m);

                if (strlen(zabbixserver) > 0 && strlen(zabbixclient) > 0) {
                    publish_zabbix(&m);
                }

                if (insert_neutrino_data(&m) == 0) {
                    fprintf(stderr, "inserted data for node %d, sensorhub %d into db\n", m.addr, sensorhubid);
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

bool decrypt_sensordata(message *m) {
    // look up encryption key in db
    char sql[256];
    MYSQL *conn;

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
        fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
        exit(1);
    } else {
        fprintf(stdout, "Connected to database\n");
    }

    sprintf(sql,"SELECT sensor_encryption_key from sensor where sensor_address=%d and sensor_hub_id=%d", m->addr, sensorhubid);
    if(mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on sensor encryption key lookup: %s\n", mysql_error(conn));
        return false;
    }
    bzero(sql, 256);

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "could not fetch sensor id from db: %s\n", mysql_error(conn));
        mysql_free_result(result);
        return false;
    }

    MYSQL_ROW row;
    row = mysql_fetch_row(result);
    mysql_free_result(result);

    if (row == NULL) {
        fprintf(stderr, "no result when looking up sensor encryption key\n");
        return false;
    }

    fprintf(stderr, "found key %s in database\n", row[0]);

    // decrypt memory blocks containing sensor data. skipjack is 64 bit (8 byte) blocks
    for (int i = 0; i < sizeof(m->s); i += 8) {
        int ptrshift = i * 8;
        skipjack_dec((&m->s + ptrshift), row[0]);
    }
    return true;
}

int insert_neutrino_data(message *m) {
    char sql[256];
    int resultcode  = 0;
    unsigned int sensorid = 0;
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
    sprintf(sql,"INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id,sensor_encryption_key) VALUES (%d, %d, '%s')", m->addr, sensorhubid, m->key);
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on sensor insert: %s\n", mysql_error(conn));
        return -1;
    }
    bzero(sql, 256);

    // get sensor's db id
    sensorid = get_sensor_id(conn, m->addr, sensorhubid);
    if (sensorid == 0) {
        return -1;
    }

    // add data to data table
    sprintf(sql,"insert into data (sensor_id, voltage, fahrenheit, celsius, pascals, humidity) values (%u,%.3f,%.2f,%.2f,%ld,%.2f);",
                sensorid, (float)m->s.millivolts/1000, ctof(m->s.tempc) ,(float)m->s.tempc/100, (long)m->s.pressurep, (float)m->s.humidity/100);
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
        mysql_free_result(result);
        return 0;
    }

    MYSQL_ROW row;

    row = mysql_fetch_row(result);
    unsigned int rowid = atoi(row[0]);
    mysql_free_result(result);
    return rowid;
}

void radio_init() {
    radio.begin();
    radio.setChannel(sensorhubid);
    radio.setPALevel(RF24_PA_MAX);
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

bool publish_zabbix(message *m) {
    ostringstream key;
    ostringstream value;

    key << "neutrino." << (int)m->addr << ".temperature.fahrenheit";
    value << std::fixed << std::setprecision(2) << ctof(m->s.tempc);
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)m->addr << ".temperature.celsius";
    value << std::fixed << std::setprecision(2) << (float)m->s.tempc/100;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)m->addr << ".pressure";
    value << (long)m->s.pressurep;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)m->addr << ".voltage";
    value << std::fixed << std::setprecision(3) << (float)m->s.millivolts/1000;
    zabbix_send(key.str().c_str(), value.str().c_str());

    key.str("");
    value.str("");
    key << "neutrino." << (int)m->addr << ".humidity";
    value << std::fixed << std::setprecision(2) << (float)m->s.humidity/100;
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

