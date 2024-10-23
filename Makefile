# Microcontroller and clock frequency definition
MCU = atmega2560
F_CPU = 16000000UL

# Compilation flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -O2 -Wall -Wextra -std=c11 -Iinclude

# Linker flags
LDFLAGS = -Wl,--gc-sections -fdata-sections -ffunction-sections -fno-exceptions -flto

# List of object files to be generated
OBJS = main.o

# Main target: compiles the objects and generates the ELF file
all: firmware.elf

# Rule to create the ELF file from the object files
firmware.elf: $(OBJS)
	avr-gcc $(CFLAGS) $(LDFLAGS) -o firmware.elf $(OBJS)

# Rule to compile .c files into .o object files
%.o: %.c
	avr-gcc $(CFLAGS) -c $< -o $@

# Flash the firmware to the ATmega2560 using avrdude (Arduino Mega 2560)
flash: firmware.elf
	avrdude -c wiring -p m2560 -P COM8 -b 115200 -D -U flash:w:firmware.elf

# Erase the flash memory of the ATmega2560 using avrdude
erase:
	avrdude -c wiring -p m2560 -P COM8 -b 115200 -e

# Clean the generated files
clean:
	del /Q *.o firmware.elf
