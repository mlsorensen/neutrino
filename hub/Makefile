CPP        = g++
CC         = gcc
CFLAGS     = -O3
LIBINC     = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
MYSQLFLAGS = $(shell mysql_config --cflags)
MYSQLLIBS  = $(shell mysql_config --libs)
PROG       = sensor-listener

all: $(PROG)

sensor-listener: skipjack.o RF24.o gpio.o spi.o compatibility.o
	$(CPP) $(CFLAGS) -o $(PROG) $(PROG).cpp *.o $(MYSQLFLAGS) $(MYSQLLIBS) $(LIBINC) -lconfig++

skipjack.o:
	$(CC) $(CFLAGS) -c libs/skipjack/skipjack.c $(LIBINC)

RF24.o: 
	$(CPP) $(CFLAGS) -c libs/RF24/RF24.cpp $(LIBINC)

gpio.o:
	$(CPP) $(CFLAGS) -c libs/gpio/gpio.cpp $(LIBINC)

spi.o:
	$(CPP) $(CFLAGS) -c libs/spi/spi.cpp $(LIBINC)

compatibility.o:
	$(CC) $(CFLAGS) -c libs/compatibility/compatibility.c $(LIBINC)

clean:
	@if [ -e $(PROG) ]; then echo cleaning $(PROG); rm -f $(PROG); fi
	@find . -name "*.o" -exec echo cleaning {} \; -exec rm -f {} \;
