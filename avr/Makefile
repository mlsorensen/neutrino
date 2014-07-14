CPP          = avr-g++
CC           = avr-gcc
LIBINC       = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
ARDUINO_CORE = arduino-core
INCLUDE      = $(LIBINC) -I $(ARDUINO_CORE)
DEFINE       = -DARDUINO=105 -D__AVR_ATmega328P__ -DF_CPU=8000000L -D__PROG_TYPES_COMPAT__
EXTRA        = -Os -mmcu=atmega328p -fno-exceptions -ffunction-sections -fdata-sections -MMD

all: neutrinosensor
	avr-ar rcs core.a *.o
	$(CC) -Wl,--gc-sections -o neutrinosensor.elf core.a $(DEFINE) $(EXTRA)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 neutrinosensor.elf neutrinosensor.eep
	avr-objcopy -O ihex -R .eeprom neutrinosensor.elf neutrinosensor.hex
	avr-size --mcu=atmega328p --format=avr -C neutrinosensor.elf

neutrinosensor: BMP180.o SI7021.o RF24.o LowPower.o arduino
	$(CPP) -c neutrinosensor.cpp $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11

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

clean:
	@if [ -e neutrinosensor.elf ]; then echo cleaning neutrinosensor.elf; rm -f neutrinosensor.elf; fi
	@if [ -e neutrinosensor.hex ]; then echo cleaning neutrinosensor.hex; rm -f neutrinosensor.hex; fi
	@if [ -e neutrinosensor.d ]; then echo cleaning neutrinosensor.d; rm -f neutrinosensor.d; fi
	@if [ -e neutrinosensor.eep ]; then echo cleaning neutrinosensor.eep; rm -f neutrinosensor.eep; fi
	@if [ -e core.a ]; then echo cleaning core.a; rm -f core.a; fi
	@find . -name "*.o" -exec echo cleaning {} \; -exec rm -f {} \;
	@find . -name "*.d" -exec echo cleaning {} \; -exec rm -f {} \;
