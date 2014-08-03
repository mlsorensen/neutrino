#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <skipjack.h>
#include <hmac-md5.h>
#include "sensor-listener.h"

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
            fprintf(stderr, "got message on radio\n");
            message m;
            bzero(m.enckey, SKIPJACK_KEY_SIZE);
            bool nread = radio.read(&m, sizeof m);
            if(nread) {
                if(m.encrypted) {
                    // decrypt sensor data before continuing
                    fprintf(stderr, "decrypting message\n");
                    if (decrypt_sensordata(&m) == false || m.enckey == 0x00) {
                        printf("message from sensor address %d was encrypted and unable to decrypt\n", m.addr);
                        continue;
                    }
                    // check signature before continuing
                    fprintf(stderr, "checking signature for sensor data\n");
                    if (check_sensordata_signature(&m) == false || m.sigkey == 0x00) {
                        printf("unable to verify signature on decrypted data from sensor %d, ignoring\n", m.addr);
                        continue;
                    }
                } else {
                    // if unencrypted and sensor is not known about, add it if we are in discovery mode. If we are not, simply warn and loop.
                    if(sensor_is_known(m.addr) == false && listener_should_discover() == false) {
                        fprintf(stderr, "Received unencrypted communication from sensor addr %d, and we are not in discovery mode. Dropping broadcast\n", m.addr);
                        continue;
                    } 
                }

                // encryption key human readable
                char enckey[8];
                bzero(enckey, 8);
                for (int i=0; i<sizeof(m.enckey); i++) {
                    char * byte = (char*)malloc(2);
                    sprintf(byte,"%x",m.enckey[i]);
                    strncat(enckey, byte, 2);
                    free(byte);
                }                

                printf("radio at address %d :\n",m.addr);
                printf("encryption key is %s\n",enckey);
                printf("hmac key is %x%x%x%x\n", m.sigkey[0], m.sigkey[1], m.sigkey[2], m.sigkey[3]);
                printf("encryption is %s\n", m.encrypted ? "true" : "false");
                printf("Temperature is %.2f F\n", ctof(m.s.tempc));
                printf("Temperature is %.2f C\n", (float)m.s.tempc / 100);
                printf("pressure is %d Pa\n", m.s.pressuredp * 10);
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

bool sensor_is_known(int sensoraddr) {
    char sql[256];
    MYSQL_ROW row;

    sprintf(sql,"SELECT id from sensor where sensor_address=%d and sensor_hub_id=%d", sensoraddr, sensorhubid);
    row = sql_select_row(sql);

    if (row == NULL) {
        return false;
    }

    if (row[0] > 0) {
        fprintf(stderr, "found sensor id %s\n", row[0]);
        return true;
    }

    return false;
}

MYSQL_ROW sql_select_row(char * sql) {
    MYSQL *conn;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
        fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
    } else {
        fprintf(stdout, "Connected to database\n");
    }

    if(mysql_query(conn, sql)) {
        fprintf(stderr, "SQL error on fetch row: %s\n", mysql_error(conn));
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "could not fetch row from db: %s\n", mysql_error(conn));
        mysql_free_result(result);
    }

    row = mysql_fetch_row(result);
    mysql_free_result(result);

    if (row == NULL) {
        fprintf(stderr, "no result from mysql query\n");
    }

    mysql_close(conn);
    return row;
}

bool listener_should_discover() {
    char sql[256];
    MYSQL_ROW row;

    sprintf(sql,"SELECT value from configuration where name='discovery'");
    row = sql_select_row(sql);

    if (row == NULL) {
        fprintf(stderr, "no result when looking up discovery mode\n");
        return false;
    }

    if(strcmp(row[0],"1") == 0) {
        fprintf(stderr, "looks like we are in discovery mode\n");
        return true;
    }

    fprintf(stderr, "we are not in discovery mode\n");
    return false;
}

bool decrypt_sensordata(message *m) {
    // look up encryption key in db
    char sql[256];
    MYSQL_ROW row;

    sprintf(sql,"SELECT sensor_encryption_key from sensor where sensor_address=%d and sensor_hub_id=%d", m->addr, sensorhubid);
    row = sql_select_row(sql);

    if (row == NULL) {
        fprintf(stderr, "no result when looking up sensor encryption key\n");
        return false;
    }

    //fprintf(stderr, "found encryption key %s in database\n", row[0]);
    char enckey[20];
    bzero(enckey, 20);
    for (int i = 0; i < SKIPJACK_KEY_SIZE; i++) {
        char * byte = (char*)malloc(2);
        sprintf(byte,"%x",row[0][i]);
        strncat(enckey, byte, 2);
        free(byte);
    }
    fprintf(stderr, "found encryption key %s in database\n", enckey);

    // decrypt memory blocks containing sensor data. skipjack is 64 bit (8 byte) blocks
    for (int i = 0; i < sizeof(m->s); i += 8) {
        int ptrshift = i;
        skipjack_dec((&m->s + ptrshift), row[0]);
    }
    return true;
}

bool check_sensordata_signature(message *m) {
    // look up hmac key in db
    char sql[256];
    MYSQL_ROW row;

    sprintf(sql,"SELECT sensor_signature_key from sensor where sensor_address=%d and sensor_hub_id=%d", m->addr, sensorhubid);
    row = sql_select_row(sql);

    if (row == NULL) {
        fprintf(stderr, "no result when looking up sensor encryption key\n");
        return false;
    }

    fprintf(stderr, "found hmac key '%s' '%x%x%x%x' in database\n", row[0], row[0][0], row[0][1], row[0][2], row[0][3]);

    uint8_t hmac[SIGNATURE_SIZE];
    hmac_md5(&hmac, row[0], SIGNATURE_KEY_SIZE*8, &m->s, 88);

    if(memcmp(hmac, m->s.signature, SIGNATURE_SIZE) != 0) {
        fprintf(stderr, "signature verification failure, '%x%x%x%x%x' != '%x%x%x%x%x'\n", hmac[0], hmac[1], hmac[2], hmac[3], hmac[4], m->s.signature[0], m->s.signature[1], m->s.signature[2], m->s.signature[3], m->s.signature[4] );
        return false;
    }

    fprintf(stderr, "Signature verification successful!\n");
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
    sprintf(sql,"INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id,sensor_encryption_key,sensor_signature_key) VALUES (%d, %d, '%s','%s')", m->addr, sensorhubid, m->enckey, m->sigkey);
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
                sensorid, (float)m->s.millivolts/1000, ctof(m->s.tempc) ,(float)m->s.tempc/100, (long)m->s.pressuredp * 10, (float)m->s.humidity/100);
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

bool zabbix_send(const char * zkey, const char * zvalue) {
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
    sprintf(buffer, "{\"request\":\"sender data\",\"data\":[{\"value\":\"%s\",\"key\":\"%s\",\"host\":\"%s\"}]}\n", zvalue, zkey, zabbixclient);
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
    ostringstream zkey;
    ostringstream zvalue;

    zkey << "neutrino." << (int)m->addr << ".temperature.fahrenheit";
    zvalue << std::fixed << std::setprecision(2) << ctof(m->s.tempc);
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)m->addr << ".temperature.celsius";
    zvalue << std::fixed << std::setprecision(2) << (float)m->s.tempc/100;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)m->addr << ".pressure";
    zvalue << (long)m->s.pressuredp * 10;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)m->addr << ".voltage";
    zvalue << std::fixed << std::setprecision(3) << (float)m->s.millivolts/1000;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)m->addr << ".humidity";
    zvalue << std::fixed << std::setprecision(2) << (float)m->s.humidity/100;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());
    
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

