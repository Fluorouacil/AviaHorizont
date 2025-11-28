ifeq ($(OS),Windows_NT)
  RM = del
  FIXPATH = $(subst /,\,$1)
  MKDIR = -mkdir
  RMDIR = -rmdir /s /q
else
  RM = rm -f
  FIXPATH = $1
  MKDIR = mkdir -p
  RMDIR = rm -rf
endif

# Конфигурация
MCU = atmega328p
F_CPU = 16000000UL
PORT1 = COM4
PORT2 = COM5
BAUD = 115200
BAUD_OLD = 57600
OPT = -Os
COMMON_CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) $(OPT) -Wall -Wextra -std=gnu11 -I.

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

# -----------------------------
# Общие файлы (для обоих MCU)
# -----------------------------
COMMON_SOURCES = \
	lib/UART/uart.c

COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)

# -----------------------------
# MCU1: ведущий (с MPU6050, кнопкой, UART TX)
# -----------------------------
MCU1_SOURCES = \
	mcu1.c \
	lib/MPU6050/MPU6050.c \
	lib/I2C/I2C.c \
	lib/Button/Button.c \
	lib/ST7735S/ST7735S.c \
	lib/Screen/st7735s_screen.c

MCU1_OBJECTS = $(MCU1_SOURCES:.c=.o)
MCU1_CFLAGS = $(COMMON_CFLAGS) -DMCU1=1

# -----------------------------
# MCU2: ведомый (только UART RX + отрисовка)
# -----------------------------
MCU2_SOURCES = \
	mcu2.c \
	lib/ST7735S/ST7735S.c \
	lib/Screen/st7735s_screen.c

MCU2_OBJECTS = $(MCU2_SOURCES:.c=.o)
MCU2_CFLAGS = $(COMMON_CFLAGS) -DMCU2=1

# -----------------------------
# Цели
# -----------------------------
.PHONY: all mcu1 mcu2 flash-mcu1 flash-mcu2 clean size

all: mcu1 mcu2

# Сборка MCU1
mcu1: mcu1.hex

mcu1.elf: $(MCU1_OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking MCU1..."
	$(CC) $(MCU1_CFLAGS) -o $@ $^ -lm

mcu1.hex: mcu1.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Сборка MCU2
mcu2: mcu2.hex

mcu2.elf: $(MCU2_OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking MCU2..."
	$(CC) $(MCU2_CFLAGS) -o $@ $^ -lm

mcu2.hex: mcu2.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# === Правила компиляции ===
# Для mcu1.c (корневой файл → объект в корне)
mcu1.o: mcu1.c
	@echo "Compiling mcu1.c"
	$(CC) $(MCU1_CFLAGS) -c $< -o $@

mcu2.o: mcu2.c
	@echo "Compiling mcu2.c"
	$(CC) $(MCU2_CFLAGS) -c $< -o $@

# Общие библиотеки (UART и т.п.) → используют COMMON_CFLAGS
$(COMMON_OBJECTS): %.o: %.c
	@$(MKDIR) "$(dir $@)"
	@echo "Compiling $<"
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

# Файлы, специфичные для MCU1 (MPU6050, Button...) → MCU1_CFLAGS
$(filter-out mcu1.o,$(MCU1_OBJECTS)): %.o: %.c
	@$(MKDIR) "$(dir $@)"
	@echo "Compiling $< (MCU1)"
	$(CC) $(MCU1_CFLAGS) -c $< -o $@

# Файлы, специфичные для MCU2 (ST7735S, screen...) → MCU2_CFLAGS
$(filter-out mcu2.o,$(MCU2_OBJECTS)): %.o: %.c
	@$(MKDIR) "$(dir $@)"
	@echo "Compiling $< (MCU2)"
	$(CC) $(MCU2_CFLAGS) -c $< -o $@

# === Прошивка ===
flash-mcu1: mcu1.hex
	@echo "Flashing MCU1 to $(PORT1)..."
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT1) -b $(BAUD) -U flash:w:mcu1.hex:i

flash-mcu2: mcu2.hex
	@echo "Flashing MCU2 to $(PORT2)..."
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT2) -b $(BAUD_OLD) -F -U flash:w:mcu2.hex:i

# === Размеры ===
size: mcu1.elf mcu2.elf
	@echo "=== MCU1 size ==="
	$(SIZE) -C --mcu=$(MCU) mcu1.elf
	@echo "=== MCU2 size ==="
	$(SIZE) -C --mcu=$(MCU) mcu2.elf

# === Очистка ТОЛЬКО временных файлов: .o, .elf, .hex ===
clean:
	@echo "Cleaning..."
	-$(RM) $(call FIXPATH,$(COMMON_OBJECTS) $(MCU1_OBJECTS) $(MCU2_OBJECTS) mcu1.elf mcu1.hex mcu2.elf mcu2.hex) 2>nul || exit 0