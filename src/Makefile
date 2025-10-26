# Целевой микроконтроллер
MCU = atmega328p
F_CPU = 16000000UL
PORT = COM7
BAUD = 115200
OPT = -Os
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) $(OPT) -Wall -Wextra -std=gnu11 -I.
LDFLAGS = -mmcu=$(MCU) $(OPT)
LIBS = -lm

# Sources relative to current dir (since make is run from src/)
SOURCES = main.c lib/MPU6050/MPU6050.c lib/ST7789/ST7789.c lib/I2C/I2C.c
OBJECTS = $(SOURCES:.c=.o)

TARGET = main

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

.PHONY: all build flash clean size

all: build

build: $(TARGET).hex

# Generic rule to compile .c files in subdirs
$(OBJECTS): %.o: %.c
	if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(TARGET).elf
	$(SIZE) -C --mcu=$(MCU) $<

flash: $(TARGET).hex
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

clean:
	-$(RM) $(OBJECTS) $(TARGET).elf $(TARGET).hex