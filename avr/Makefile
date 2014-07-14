CPP          = avr-g++
CC           = avr-gcc
LIBINC       = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
ARDUINO_CORE = arduino-core
INCLUDE      = $(LIBINC) -I $(ARDUINO_CORE)
MCU          = atmega328p
PROG         = neutrinosensor
DEFINE       = -DARDUINO=105 -D__AVR_ATmega328P__ -DF_CPU=8000000L -D__PROG_TYPES_COMPAT__
EXTRA        = -Os -mmcu=$(MCU) -fno-exceptions -ffunction-sections -fdata-sections -MMD
AVRDUDE      = /usr/share/arduino/hardware/tools/avrdude
AVRDUDE_CONF = /usr/share/arduino/hardware/tools/avrdude.conf

all: $(PROG)
	avr-ar rcs core.a *.o
	$(CC) -Wl,--gc-sections -o $(PROG).elf core.a $(DEFINE) $(EXTRA)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(PROG).elf $(PROG).eep
	avr-objcopy -O ihex -R .eeprom $(PROG).elf $(PROG).hex
	avr-size --mcu=$(MCU) --format=avr -C $(PROG).elf

$(PROG): BMP180.o SI7021.o RF24.o LowPower.o arduino skipjack.o
	$(CPP) -c $(PROG).cpp $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11

RF24.o: SPI.o
	$(CPP) -c libs/RF24/RF24.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

SI7021.o: Wire.o
	$(CPP) -c libs/SI7021/SI7021.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

BMP180.o: Wire.o
	$(CPP) -c libs/BMP180/BMP180.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

Wire.o: twi.o
	$(CPP) -c libs/Wire/Wire.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

twi.o:
	$(CC) -c libs/twi/twi.c $(INCLUDE) $(DEFINE) $(EXTRA)

SPI.o:
	$(CPP) -c libs/SPI/SPI.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

LowPower.o:
	$(CPP) -c libs/LowPower/LowPower.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

arduino:
	$(CC) -c arduino-core/*.c $(INCLUDE) $(DEFINE) $(EXTRA)
	$(CPP) -c arduino-core/*.cpp $(INCLUDE) $(DEFINE) $(EXTRA)

skipjack.o:
	$(CC) -c libs/skipjack/skipjack.c $(INCLUDE) $(DEFINE) $(EXTRA)

clean:
	@if [ -e $(PROG).elf ]; then echo cleaning $(PROG).elf; rm -f $(PROG).elf; fi
	@if [ -e $(PROG).hex ]; then echo cleaning $(PROG).hex; rm -f $(PROG).hex; fi
	@if [ -e $(PROG).eep ]; then echo cleaning $(PROG).eep; rm -f $(PROG).eep; fi
	@if [ -e core.a ]; then echo cleaning core.a; rm -f core.a; fi
	@find . -name "*.o" -exec echo cleaning {} \; -exec rm -f {} \;
	@find . -name "*.d" -exec echo cleaning {} \; -exec rm -f {} \;

install: all
	@echo !!! flashing AVR !!!
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -Uflash:w:$(PROG).hex:i
