# Целевой микроконтроллер
MCU = atmega328p

# Тактовая частота (в Гц)
F_CPU = 16000000UL

# Последовательный порт (можно переопределить: make flash PORT=/dev/ttyUSB0)
PORT = /dev/ttyACM0

# Скорость загрузчика Arduino Uno
BAUD = 115200

# Оптимизация
OPT = -Os

# Флаги компилятора
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) $(OPT) -Wall -Wextra -std=gnu11 -I.

# Флаги линковщика
LDFLAGS = -mmcu=$(MCU) $(OPT)

# Библиотеки
LIBS = -lm

# Имена файлов
TARGET = main
SOURCES = main.c MPU6050.c ST7789.c I2C.c
OBJECTS = $(SOURCES:.c=.o)

# Программы
CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

# Цели
.PHONY: all build flash clean size

all: build

build: $(TARGET).hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(TARGET).elf
	$(SIZE) -C --mcu=$(MCU) $<

# Прошивка через Arduino bootloader (USB)
flash: $(TARGET).hex
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$(TARGET).hex:i && \
	$(MAKE) clean

clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).hex