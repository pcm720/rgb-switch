# AVR Makefile
# Originally written by Michael Cousins (http://github.com/mcous)
# Modified by pcm720 (https://github.com/pcm720)

# Makefile
#
# targets:
#   all:    compiles the source code
#   test:   tests the isp connection to the mcu
#   flash:  writes compiled hex file to the mcu's flash memory
#   fuse:   writes the fuse bytes to the MCU
#   disasm: disassembles the code for debugging
#   clean:  removes all .hex, .elf, and .o files in the source code and library directories

PRJ = main

MCU = atmega328p
CLK = 16000000

PRG = apu2 -P /dev/ttyUSB0
LFU = 0xFF
HFU = 0xDE
EFU = 0x05

SRC = $(PRJ).c
CFLAGS    = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE)
CFILES    = $(PRJ).c

AVRDUDE = avrdude -c $(PRG) -p $(MCU) -C ~/.avrduderc -C /etc/avrdude.conf
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

all: $(PRJ).hex

test:
	$(AVRDUDE) -v

flash: all
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i

fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PRJ).elf

clean:
	rm -f *.hex *.elf *.o
	$(foreach dir, $(EXT), rm -f $(dir)/*.o;)
	
$(PRJ).elf:
	$(CC) $(CFLAGS) -c $(PRJ).c -o $(PRJ).elf

$(PRJ).hex: $(PRJ).elf
	rm -f $(PRJ).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PRJ).elf $(PRJ).hex
	$(SIZE) $(PRJ).elf