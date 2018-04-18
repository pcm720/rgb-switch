PRJ = screen_test
MCU = atmega328p
CLK = 16000000UL

# External libraries
U8G2LIB = ../../libs/u8g2/csrc
HALLIB = ../../libs/u8g2-hal
LIBSRC = $(wildcard *.c $(U8G2LIB)/*.c $(HALLIB)/*.c)
LIBOBJ = $(LIBSRC:.c=.o)
LIBHDR = $(LIBSRC:.c=.h)

# Compiler and linker settings
CPPFLAGS = -mmcu=$(MCU) -DF_CPU=$(CLK) -I. -I$(LIBDIR) -Wall
CFLAGS = -Os -g -std=gnu11 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections 
LDFLAGS = -Wl,-Map,$(PRJ).map -mmcu=$(MCU) 
LDFLAGS += -Wl,--gc-sections

OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

# Programmer settings
PRG = apu2 -P ft0
AVRDUDE = taskset -c 1 avrdude -c $(PRG) -p $(MCU) -C ~/.avrduderc -C /etc/avrdude.conf
LFU = 0xFF
HFU = 0xDF
EFU = 0xFD

all: $(PRJ).hex

clean:
	rm -f *.hex *.elf *.o *.map $(LIBOBJ)

test:
	$(AVRDUDE) -v

flash: all
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i

fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U -B 1200 hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PRJ).elf

%.o: %.c $(LIBHDR) Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<;

$(PRJ).elf: $(LIBOBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PRJ).hex: $(PRJ).elf
	rm -f $(PRJ).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PRJ).elf $(PRJ).hex
	$(SIZE) $(PRJ).elf