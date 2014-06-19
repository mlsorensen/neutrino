CC=g++
CFLAGS=-I.
MYSQLFLAGS=$(shell mysql_config --cflags)
MYSQLLIBS=$(shell mysql_config --libs)

sensor-listener: sensor-listener.cpp
	$(CC) -o sensor-listener sensor-listener.cpp $(MYSQLFLAGS) $(MYSQLLIBS) -lrf24 -lconfig++
