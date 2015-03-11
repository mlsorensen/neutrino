CPP          = avr-g++
CC           = avr-gcc
LIBINC       = $(shell for i in `ls libs`; do echo -I libs/$$i; done)
ARDUINO_CORE = arduino-core
INCLUDE      = $(LIBINC) -I $(ARDUINO_CORE)
MCU          = atmega328p
PROG         = neutrinohub-avr
DEFINE       = -DARDUINO=105 -D__AVR_ATmega328P__ -DF_CPU=16000000L -D__PROG_TYPES_COMPAT__
EXTRA        = -Os -mmcu=$(MCU) -fno-exceptions -ffunction-sections -fdata-sections -Wno-deprecated-declarations -MMD -lm
AVRDUDE      = bin/avrdude
AVRDUDE_CONF = $(shell if [ -e /dev/ttyAMA0 ]; then echo avrdude-rpi.conf; else echo avrdude.conf; fi)
BUILDDIR     = build
ARDUINO_CPP  = $(shell find arduino-core -maxdepth 1 -name "*.cpp" -exec basename {} \;| cut -d. -f1)
ARDUINO_C    = $(shell find arduino-core -maxdepth 1 -name "*.c" -exec basename {} \;| cut -d. -f1)

$(shell if [ ! -e $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi)

all: $(BUILDDIR)/$(PROG).o
	avr-ar rcs $(BUILDDIR)/core.a $(BUILDDIR)/*.o
	$(CC) -Wl,--gc-sections -o $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/core.a $(DEFINE) $(EXTRA) $(INCLUDE)
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/$(PROG).eep
	avr-objcopy -O ihex -R .eeprom $(BUILDDIR)/$(PROG).elf $(BUILDDIR)/$(PROG).hex
	avr-size --mcu=$(MCU) --format=avr -C $(BUILDDIR)/$(PROG).elf

$(BUILDDIR)/$(PROG).o: $(PROG).cpp $(patsubst %,$(BUILDDIR)/%.o, $(ARDUINO_CPP) $(ARDUINO_C) JsonGenerator JsonParser Adafruit_NeoPixel)
	$(CPP) -c $(PROG).cpp  -o $@ $(INCLUDE) $(DEFINE) $(EXTRA) -std=c++11 

$(BUILDDIR)/%.o: arduino-core/%.cpp
	$(CPP) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/%.o: arduino-core/%.c
	$(CC) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/JsonGenerator.o: libs/JsonGenerator/JsonGenerator.cpp
	$(CPP) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA) 

$(BUILDDIR)/JsonParser.o: libs/JsonParser/JsonParser.cpp
	$(CPP) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

$(BUILDDIR)/Adafruit_NeoPixel.o: libs/Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
	$(CPP) -c $< -o $@ $(INCLUDE) $(DEFINE) $(EXTRA)

clean:
	@if [ -e build ]; then echo cleaning build dir; rm -rf build; fi

install: all
	@echo !!! flashing AVR bootloader !!!
	$(AVRDUDE) -C$(AVRDUDE_CONF) -p$(MCU) -cgpio -Pgpio -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xDA:m -Ulfuse:w:0xFF:m
	@echo !!! flashing AVR code !!!
	$(AVRDUDE) -C$(AVRDUDE_CONF) -p$(MCU) -cgpio -Pgpio -Uflash:w:$(BUILDDIR)/$(PROG).hex:i
