#include <cstdlib>
#include <ctime>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <skipjack.h>
#include <hmac-md5.h>
#include "sensor-listener.h"
using namespace std;

int main(int argc, char** argv) {
    //don't buffer what we print
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    //get arguments, set things up
    get_args(argc,argv);

    if ( generator )
        launch_random_generator();

    // set up radio
    radio_init();

    while(1) {
        if (radio.available()) {
            fprintf(stderr, "got message on radio\n");
            message message;
            bool nread = radio.read(&message, sizeof message);

            // parse message header
            int messagetype = 0x3 & message.header;
            int sensoraddr  = (0x1c & message.header) >> 2;

            if(nread) {
                if(messagetype == 0) {
                    printf("Got message type 0\n");
                    handle_sensor_message(&message, sensoraddr);
                } else if (messagetype == 1) {
                    printf("Got message type 1\n");
                    printf("Need to pair sensor %d\n", sensoraddr);
                    handle_pairing_message(&message, sensoraddr);
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

template<typename T> T generate_random(std::vector<T>& elems, int pos, int add, int sub) {
    elems[pos] = elems[pos] + (rand() % add) - sub;

    if ( elems[pos] > 60000 )
        elems[pos] = rand() % 15;

    return elems[pos];
}

void launch_random_generator() {
    srand(time(0));

    std::vector<int16_t>  temps;
    std::vector<int16_t>  humds;
    std::vector<uint16_t> press;
    std::vector<uint16_t> volts;

    for ( int jj = 0; jj < generator_count; jj++ ) {
        insert_fake_sensor((uint8_t) jj);
        temps.push_back((rand() % 30) + 10);
        humds.push_back((rand() % 30) + 10);
        press.push_back((rand() % 10) + 5);
        volts.push_back((rand() %  5) + 5);
    }

    uint8_t sig[SIGNATURE_KEY_SIZE] = {};

    while ( 1 ) {
        for ( int ii = 0; ii < generator_count; ii++ ) {
            struct sensordata data;
            data.addr = ii;
            data.proximity  = rand() % 2;
            data.tempc      = generate_random(temps, ii, 5, 3);
            data.humidity   = generate_random(humds, ii, 2, 2);
            data.pressuredp = generate_random(press, ii, 2, 2);
            data.millivolts = generate_random(volts, ii, 1, 1);
            data.signature[0] = 0;

            std::cout << "addr=" << (int) data.addr << ", tempc=" << data.tempc << ", humidity=" << data.humidity << ", pressuredp=" << data.pressuredp << ", millivolts=" << data.millivolts;

            if (insert_neutrino_data(&data) != 0) {
                std::cout << "Failed to insert" << std::endl;
                exit(0);
            }
        }
        std::cout << "" << std::endl;

        sleep(generator_interval);
    }
}

float ctof (int16_t c) {
    return c * .018 + 32;
}

void handle_sensor_message(message * message, int addr) {
    sensordata data;
    memmove(&data, &message->data, sizeof message->data);
    // decrypt sensor data before continuing
    fprintf(stderr, "decrypting message\n");
    if (decrypt_sensordata(&data, addr) == false) {
        printf("message from sensor address %d was encrypted and unable to decrypt\n", addr);
        return;
    }
    // check signature before continuing
    fprintf(stderr, "checking signature for sensor data\n");
    if (check_sensordata_signature(&data) == false) {
        printf("unable to verify signature on decrypted data from sensor %d, ignoring\n", addr);
        //return;
    }

    printf("radio at address %d :\n", addr);
    //printf("encryption key is %s\n",enckey);
    //printf("hmac key is %x%x%x%x\n", m.sigkey[0], m.sigkey[1], m.sigkey[2], m.sigkey[3]);
    //printf("encryption is %s\n", m.encrypted ? "true" : "false");
    printf("reed switch is %s\n", data.proximity ? "closed" : "open");
    printf("Temperature is %.2f F\n", ctof(data.tempc));
    printf("Temperature is %.2f C\n", (float)data.tempc / 100);
    printf("pressure is %d Pa\n", data.pressuredp * 10);
    printf("humidity is %.2f%\n", (float)data.humidity / 100);
    printf("voltage is %.3f \n", (float)data.millivolts / 1000);

    if (strlen(zabbixserver) > 0 && strlen(zabbixclient) > 0) {
        publish_zabbix(&data);
    }

    if (insert_neutrino_data(&data) == 0) {
        fprintf(stderr, "inserted data for node %d, sensorhub %d into db\n", addr, sensorhubid);
    } else {
        fprintf(stderr, "failed to insert data into db\n");
    }
}

void handle_pairing_message(message * message, int addr) {
    pairingdata pdata;
    memmove(&pdata, &message->data, sizeof message->data);

    // encryption key human readable
    char enckey[SKIPJACK_KEY_SIZE];
    bzero(enckey, SKIPJACK_KEY_SIZE);
    for (int i=0; i<sizeof(pdata.enckey); i++) {
        char * byte = (char*)malloc(2);
        sprintf(byte,"%x",pdata.enckey[i]);
        strncat(enckey, byte, 2);
        free(byte);
    }

    fprintf(stderr, "pairing message received\n");
    printf("encryption key is %s\n",enckey);
    printf("hmac key is %x%x%x%x\n", pdata.sigkey[0], pdata.sigkey[1], pdata.sigkey[2], pdata.sigkey[3]);


    // if we are in discovery mode, save the keys, otherwise, ignore
    if (listener_should_discover()) {
        MYSQL *conn;
        conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
            fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
            exit(1);
        } else {
            fprintf(stdout, "Connected to database\n");
        }
        fprintf(stdout, "adding sensor %d into database for hub %d\n", addr, sensorhubid);

        ostringstream sql;
        sql << "INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id,sensor_encryption_key,sensor_signature_key) VALUES (" 
            << (int) addr << "," << (int) sensorhubid << ", '" << std::string(pdata.enckey, pdata.enckey + SKIPJACK_KEY_SIZE) 
            << "','" << std::string(pdata.sigkey, pdata.sigkey + SIGNATURE_KEY_SIZE) <<"')";
        fprintf(stderr, "sql is '%s'\n", sql.str().c_str());

        if (mysql_query(conn, sql.str().c_str())) {
            fprintf(stderr, "SQL error on sensor insert: %s\n", mysql_error(conn));
            return;
        }
        fprintf(stdout,"ran sql\n");
        mysql_close(conn);
    } else {
        fprintf(stderr, "sensor hub not in discovery mode, skipping\n");
    }
}

void insert_fake_sensor(uint8_t addr) {
    MYSQL *conn;
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, mysqlserver, mysqluser, mysqlpass, mysqldb, 0, NULL, 0)) {
        fprintf(stderr, "Cant connect to database: %s\n", mysql_error(conn));
        exit(1);
    } else {
        fprintf(stdout, "Connected to database\n");
    }
    fprintf(stdout, "adding sensor %d into database for hub %d\n", addr, sensorhubid);

    ostringstream sql;
    sql << "INSERT IGNORE INTO sensor (sensor_address,sensor_hub_id,sensor_encryption_key,sensor_signature_key) VALUES ("
        << (int) addr << "," << (int) sensorhubid << ", 'FAKE', 'MORE_FAKE')";
    fprintf(stderr, "sql is '%s'\n", sql.str().c_str());

    if (mysql_query(conn, sql.str().c_str())) {
        fprintf(stderr, "SQL error on sensor insert: %s\n", mysql_error(conn));
        return;
    }
    fprintf(stdout,"ran sql\n");
    mysql_close(conn);
}

bool sensor_is_known(int sensoraddr) {
    ostringstream sql;
    sql << "SELECT id from sensor where sensor_address=" << (int) sensoraddr << " and sensor_hub_id=" << (int) sensorhubid;
    MYSQL_ROW row;

    row = sql_select_row(sql.str().c_str());

    if (row == NULL) {
        return false;
    }

    if (row[0] > 0) {
        fprintf(stderr, "found sensor id %s\n", row[0]);
        return true;
    }

    return false;
}

MYSQL_ROW sql_select_row(const char * sql) {
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
    ostringstream sql;
    sql << "SELECT value from configuration where name='discovery'";

    MYSQL_ROW row;

    row = sql_select_row(sql.str().c_str());

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

bool decrypt_sensordata(sensordata *data, int addr) {
    // look up encryption key in db
    ostringstream sql;
    sql << "SELECT sensor_encryption_key from sensor where sensor_address=" << (int) addr << " and sensor_hub_id=" << (int) sensorhubid;
    MYSQL_ROW row;

    row = sql_select_row(sql.str().c_str());

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
    for (int i = 0; i < sizeof(*data); i += 8) {
        int ptrshift = i;
        skipjack_dec((data + ptrshift), row[0]);
    }
    return true;
}

bool check_sensordata_signature(sensordata *data) {
    // look up hmac key in db
    ostringstream sql;
    sql << "SELECT sensor_signature_key from sensor where sensor_address=" << (int) data->addr << " and sensor_hub_id=" << (int) sensorhubid;
    MYSQL_ROW row;

    row = sql_select_row(sql.str().c_str());

    if (row == NULL) {
        fprintf(stderr, "no result when looking up sensor encryption key\n");
        return false;
    }

    fprintf(stderr, "found hmac key '%s' '%x%x%x%x' in database\n", row[0], row[0][0], row[0][1], row[0][2], row[0][3]);

    uint8_t hmac[SIGNATURE_SIZE];
    hmac_md5(&hmac, row[0], SIGNATURE_KEY_SIZE*8, data, 80);

    if(memcmp(hmac, data->signature, SIGNATURE_SIZE) != 0) {
        fprintf(stderr, "signature verification failure, '%x%x%x%x%x%x' != '%x%x%x%x%x%x'\n", hmac[0], hmac[1], hmac[2], hmac[3], hmac[4], hmac[5], data->signature[0], data->signature[1], data->signature[2], data->signature[3], data->signature[4], data->signature[5] );
        return false;
    }

    fprintf(stderr, "Signature verification successful!\n");
    return true;
}

int insert_neutrino_data(sensordata *data) {
    ostringstream sql;
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

    // get sensor's db id
    sensorid = get_sensor_id(conn, data->addr, sensorhubid);
    if (sensorid == 0) {
        return -1;
    }

    // add data to data table
    sql << "insert into data (sensor_id, voltage, fahrenheit, celsius, pascals, humidity) values (" << (int) sensorid 
        << "," << std::fixed << std::setprecision(3) << (float)data->millivolts/1000
        << "," << std::fixed << std::setprecision(2) << ctof(data->tempc)
        << "," << std::fixed << std::setprecision(2) << (float)data->tempc/100
        << "," << (long)data->pressuredp * 10
        << "," << std::fixed << std::setprecision(2) << (float)data->humidity/100
        <<")";

    if (mysql_query(conn, sql.str().c_str())) {
        fprintf(stderr, "SQL error on data insert: %s\n", mysql_error(conn));
        return -1;
    }

    mysql_close(conn);
    return 0;
}

unsigned int get_sensor_id(MYSQL *conn, int sensor_addr, int sensor_hub_id) {
    ostringstream sql;
    sql << "SELECT id FROM sensor where sensor_address=" << (int) sensor_addr << " and sensor_hub_id=" << (int) sensor_hub_id;

    if (mysql_query(conn, sql.str().c_str())) {
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
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.setRetries(6,15);

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
    ostringstream buffer;

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
    buffer << "{\"request\":\"sender data\",\"data\":[{\"value\":\"" << zvalue << "\",\"key\":\"" << zkey << "\",\"host\":\"" << zabbixclient << "\"}]}\n";
    int n = write(sockfd, buffer.str().c_str(), strlen(buffer.str().c_str()));
    if (n < 0) {
        perror("unable to write to zabbix socket\n");
    } else {
        printf("wrote to zabbix: ");
        printf("%s",buffer.str().c_str());
    }
    close(sockfd);
}

bool publish_zabbix(sensordata *data) {
    ostringstream zkey;
    ostringstream zvalue;

    zkey << "neutrino." << (int)data->addr << ".temperature.fahrenheit";
    zvalue << std::fixed << std::setprecision(2) << ctof(data->tempc);
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)data->addr << ".temperature.celsius";
    zvalue << std::fixed << std::setprecision(2) << (float)data->tempc/100;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)data->addr << ".pressure";
    zvalue << (long)data->pressuredp * 10;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)data->addr << ".voltage";
    zvalue << std::fixed << std::setprecision(3) << (float)data->millivolts/1000;
    zabbix_send(zkey.str().c_str(), zvalue.str().c_str());

    zkey.str("");
    zvalue.str("");
    zkey << "neutrino." << (int)data->addr << ".humidity";
    zvalue << std::fixed << std::setprecision(2) << (float)data->humidity/100;
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
    bool gotconfig = false;
    while ((c = getopt (argc, argv, "hc:")) != -1){
        switch(c) {
            case 'h':
                usage();
                exit(1);
            case 'c':
                configfile = optarg;
                gotconfig = true;
            break;
        }
    }

    if (! gotconfig) {
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
        generator    = cfg.exists("generator");
        if ( generator ) {
            generator_count    = atoi(cfg.lookup("generator_count").c_str());
            generator_interval = atoi(cfg.lookup("generator_interval").c_str());
        }
    } catch (const SettingNotFoundException &settingexception) {
        fprintf(stderr, "A setting was not found: %s -- set any unneeded options to ''\n", settingexception.getPath());
        exit(1);
    }

    printf("mysql server is '%s','%s'\n",cfg.lookup("mysqlserver").c_str(),mysqlserver);
}

