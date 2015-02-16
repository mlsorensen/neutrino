CPP        = g++
CC         = gcc
CFLAGS     = -O3
LIBINC     = $(shell for i in `ls libs | grep -v spi | grep -v RF24`; do echo -I libs/$$i; done)
MYSQLFLAGS = $(shell mysql_config --cflags)
MYSQLLIBS  = $(shell mysql_config --libs)
PROG       = sensor-listener
BUILDDIR   = build

# build rf24 for beaglebone or rpi, depending on arch
PLATFORM   = $(shell gcc -dM -E - < /dev/null | grep -q ARM_ARCH_6 && echo rpi || echo bbb)
$(info "SELECTED PLATFORM $(PLATFORM)")

LIBINC += -I libs/RF24-$(PLATFORM) -I libs/spi-$(PLATFORM)

$(shell if [ ! -e $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi)

all: $(PROG)

sensor-listener: sensor-listener.cpp $(patsubst %,$(BUILDDIR)/%.o, skipjack hmac-md5 RF24 gpio spi compatibility b64encode)
	$(CPP) $(CFLAGS) -o $(PROG) $(PROG).cpp $(BUILDDIR)/*.o $(LIBINC)  $(MYSQLFLAGS) $(MYSQLLIBS) -lconfig++

$(BUILDDIR)/skipjack.o: libs/skipjack/skipjack.c
	$(CC) $(CFLAGS) -c libs/skipjack/skipjack.c -o $@ $(LIBINC)

$(BUILDDIR)/hmac-md5.o: libs/hmac-md5/hmac-md5.c $(BUILDDIR)/md5.o
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBINC)

$(BUILDDIR)/md5.o: libs/md5/md5.c $(BUILDDIR)/md5_sbox.o
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBINC)

$(BUILDDIR)/md5_sbox.o: libs/md5/md5_sbox.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBINC)

$(BUILDDIR)/b64encode.o: libs/base64/b64encode.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBINC)

ifeq ($(PLATFORM), rpi)
$(BUILDDIR)/RF24.o: $(BUILDDIR)/RF24-bcm.o $(BUILDDIR)/RF24-part.o
	$(LD) -r $(BUILDDIR)/RF24-bcm.o $(BUILDDIR)/RF24-part.o -o $@ $(LIBINC)
	rm -f $(BUILDDIR)/RF24-bcm.o
	rm -f $(BUILDDIR)/RF24-part.o

$(BUILDDIR)/RF24-bcm.o: libs/RF24-$(PLATFORM)/bcm2835.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBINC)

$(BUILDDIR)/RF24-part.o: libs/RF24-$(PLATFORM)/RF24.cpp
	$(CPP) $(CFLAGS) -c $< -o $@ $(LIBINC)

else
$(BUILDDIR)/RF24.o: libs/RF24-$(PLATFORM)/RF24.cpp
	$(CPP) $(CFLAGS) -c libs/RF24-$(PLATFORM)/RF24.cpp -o $@ $(LIBINC)
endif

$(BUILDDIR)/gpio.o: libs/gpio/gpio.cpp
	$(CPP) $(CFLAGS) -c libs/gpio/gpio.cpp -o $@ $(LIBINC)

$(BUILDDIR)/spi.o: libs/spi-$(PLATFORM)/spi.cpp
	$(CPP) $(CFLAGS) -c libs/spi-$(PLATFORM)/spi.cpp -o $@ $(LIBINC)

$(BUILDDIR)/compatibility.o: libs/compatibility/compatibility.c
	$(CC) $(CFLAGS) -c libs/compatibility/compatibility.c -o $@ $(LIBINC)

clean:
	@if [ -e $(PROG) ]; then echo cleaning $(PROG); rm -f $(PROG); fi
	@if [ -e $(BUILDDIR) ]; then echo cleaning $(BUILDDIR); rm -rf $(BUILDDIR); fi
