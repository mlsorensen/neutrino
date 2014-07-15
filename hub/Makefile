CPP        = g++
CC         = gcc
CFLAGS     = -O3
LIBINC     = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
MYSQLFLAGS = $(shell mysql_config --cflags)
MYSQLLIBS  = $(shell mysql_config --libs)
PROG       = sensor-listener
BUILDDIR   = build
$(shell if [ ! -e $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi)

all: $(PROG)

sensor-listener: sensor-listener.cpp $(patsubst %,$(BUILDDIR)/%.o, skipjack RF24 gpio spi compatibility)
	$(CPP) $(CFLAGS) -o $(PROG) $(PROG).cpp $(BUILDDIR)/*.o $(LIBINC)  $(MYSQLFLAGS) $(MYSQLLIBS) -lconfig++

$(BUILDDIR)/skipjack.o: libs/skipjack/skipjack.c
	$(CC) $(CFLAGS) -c libs/skipjack/skipjack.c -o $@ $(LIBINC)

$(BUILDDIR)/RF24.o: libs/RF24/RF24.cpp
	$(CPP) $(CFLAGS) -c libs/RF24/RF24.cpp -o $@ $(LIBINC)

$(BUILDDIR)/gpio.o: libs/gpio/gpio.cpp
	$(CPP) $(CFLAGS) -c libs/gpio/gpio.cpp -o $@ $(LIBINC)

$(BUILDDIR)/spi.o: libs/spi/spi.cpp
	$(CPP) $(CFLAGS) -c libs/spi/spi.cpp -o $@ $(LIBINC)

$(BUILDDIR)/compatibility.o: libs/compatibility/compatibility.c
	$(CC) $(CFLAGS) -c libs/compatibility/compatibility.c -o $@ $(LIBINC)

clean:
	@if [ -e $(PROG) ]; then echo cleaning $(PROG); rm -f $(PROG); fi
	@if [ -e $(BUILDDIR) ]; then echo cleaning $(BUILDDIR); rm -rf $(BUILDDIR); fi
