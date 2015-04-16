CPP          = avr-g++
CC           = avr-gcc
LIBINC       = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
ARDUINO_CORE = arduino-core
INCLUDE      = $(LIBINC) -I $(ARDUINO_CORE)
MCU          = atmega328p
PROG         = neutrinosensor
TESTSEND     = test-send
TESTRECV     = test-recv
DEFINE       = -DARDUINO=105 -D__AVR_ATmega328P__ -DF_CPU=8000000L -D__PROG_TYPES_COMPAT__
EXTRA        = -Os -mmcu=$(MCU) -fno-exceptions -ffunction-sections -fdata-sections -Wno-deprecated-declarations -MMD
AVRDUDE      = avrdude
AVRDUDE_CONF = /etc/avrdude.conf
BUILDDIR     = build
ARDUINO_CPP  = $(shell find arduino-core -maxdepth 1 -name "*.cpp" -exec basename {} \;| cut -d. -f1)
ARDUINO_C    = $(shell find arduino-core -maxdepth 1 -name "*.c" -exec basename {} \;| cut -d. -f1)

$(shell if [ ! -e $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi)

prog: $(BUILDDIR)/$(PROG).o
	avr-ar rcs $(BUILDDIR)/core.a $(BUILDDIR)/*.o
	$(CC) -Wl,--gc-sections -o $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/core.a $(DEFINE) $(EXTRA) $(INCLUDE)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/$(PROG).eep
	avr-objcopy -O ihex -R .eeprom $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/$(PROG).hex
	avr-size --mcu=$(MCU) --format=avr -C $(BUILDDIR)/$(PROG).elf
	
test-recv: $(BUILDDIR)/$(TESTRECV).o
	avr-ar rcs $(BUILDDIR)/$(TESTRECV).a $(BUILDDIR)/*.o
	$(CC) -Wl,--gc-sections -o $(BUILDDIR)/$(TESTRECV).elf $(BUILDDIR)/$(TESTRECV).a $(DEFINE) $(EXTRA) $(INCLUDE)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(BUILDDIR)/$(TESTRECV).elf $(BUILDDIR)/$(TESTRECV).eep
	avr-objcopy -O ihex -R .eeprom $(BUILDDIR)/$(TESTRECV).elf $(BUILDDIR)/$(TESTRECV).hex
	avr-size --mcu=$(MCU) --format=avr -C $(BUILDDIR)/$(TESTRECV).elf

test-send: $(BUILDDIR)/$(TESTSEND).o
	avr-ar rcs $(BUILDDIR)/$(TESTSEND).a $(BUILDDIR)/*.o
	$(CC) -Wl,--gc-sections -o $(BUILDDIR)/$(TESTSEND).elf $(BUILDDIR)/$(TESTSEND).a $(DEFINE) $(EXTRA) $(INCLUDE)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(BUILDDIR)/$(TESTSEND).elf $(BUILDDIR)/$(TESTSEND).eep
	avr-objcopy -O ihex -R .eeprom $(BUILDDIR)/$(TESTSEND).elf $(BUILDDIR)/$(TESTSEND).hex
	avr-size --mcu=$(MCU) --format=avr -C $(BUILDDIR)/$(TESTSEND).elf

$(BUILDDIR)/$(PROG).o: $(PROG).cpp $(patsubst %,$(BUILDDIR)/%.o, $(ARDUINO_CPP) $(ARDUINO_C) sha1 skipjack BMP180 SI7021 RF24 LowPower hmac-md5)
	$(CPP) -c $(PROG).cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11

$(BUILDDIR)/$(TESTRECV).o: $(TESTRECV).cpp $(patsubst %,$(BUILDDIR)/%.o, $(ARDUINO_CPP) $(ARDUINO_C) sha1 skipjack BMP180 SI7021 RF24 LowPower hmac-md5)
	$(CPP) -c $(TESTRECV).cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11

$(BUILDDIR)/$(TESTSEND).o: $(TESTSEND).cpp $(patsubst %,$(BUILDDIR)/%.o, $(ARDUINO_CPP) $(ARDUINO_C) sha1 skipjack BMP180 SI7021 RF24 LowPower hmac-md5)
	$(CPP) -c $(TESTSEND).cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11

$(BUILDDIR)/RF24.o: $(BUILDDIR)/SPI.o libs/RF24/RF24.cpp
	$(CPP) -c libs/RF24/RF24.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/SI7021.o: $(BUILDDIR)/Wire.o libs/SI7021/SI7021.cpp
	$(CPP) -c libs/SI7021/SI7021.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/BMP180.o: $(BUILDDIR)/Wire.o libs/BMP180/BMP180.cpp
	$(CPP) -c libs/BMP180/BMP180.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/Wire.o: $(BUILDDIR)/twi.o libs/Wire/Wire.cpp
	$(CPP) -c libs/Wire/Wire.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/twi.o: libs/twi/twi.c
	$(CC) -c libs/twi/twi.c -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/SPI.o: libs/SPI/SPI.cpp
	$(CPP) -c libs/SPI/SPI.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/LowPower.o: libs/LowPower/LowPower.cpp
	$(CPP) -c libs/LowPower/LowPower.cpp -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/md5.o: libs/md5/md5.c $(BUILDDIR)/md5_sbox.o
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/md5_sbox.o: libs/md5/md5_sbox.c
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/hmac-md5.o: libs/hmac-md5/hmac-md5.c $(BUILDDIR)/md5.o
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/%.o: arduino-core/%.cpp
	$(CPP) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/%.o: arduino-core/%.c
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/skipjack.o: libs/skipjack/skipjack.c
	$(CC) -c libs/skipjack/skipjack.c -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/sha1.o: libs/sha1/sha1.c
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

clean:
	@if [ -e build ]; then echo cleaning build dir; rm -rf build; fi

install: prog
	@echo !!! flashing AVR !!!
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -Uflash:w:$(BUILDDIR)/$(PROG).hex:i

installdev: prog
	@echo !!! flashing AVR, programming default EEPROM values for development !!!
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -Uflash:w:$(BUILDDIR)/$(PROG).hex:i -U eeprom:w:0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0x50,0x51,0x52,0x53:m

installbootloader:
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xDA:m -Ulfuse:w:0xFF:m 

installtest-send:
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -Uflash:w:$(BUILDDIR)/$(TESTSEND).hex:i

installtest-recv:
	$(AVRDUDE) -C$(AVRDUDE_CONF) -v -v -v -v -p$(MCU) -cusbasp -Pusb -Uflash:w:$(BUILDDIR)/$(TESTRECV).hex:i
