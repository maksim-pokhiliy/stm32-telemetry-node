CC := arm-none-eabi-gcc
SIZE := arm-none-eabi-size
CFLAGS := -mcpu=cortex-m4 -mthumb -Og -g3 -ffreestanding -Wall -Wextra
DEPFLAGS := -MMD -MP
LDFLAGS := -nostdlib -T stm32f411re.ld -Wl,-Map=build/firmware.map
OBJCOPY := arm-none-eabi-objcopy

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
DEPFILES := $(patsubst %.o,%.d,$(OBJECTS))

.PHONY: all flash clean

all: build/firmware.bin

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

build/firmware.elf: $(OBJECTS) stm32f411re.ld | build
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ -lgcc
	$(SIZE) $@

build/firmware.bin: build/firmware.elf
	$(OBJCOPY) -O binary $< $@

flash: build/firmware.bin
	st-flash write $< 0x08000000
	
clean:
	rm -rf build

-include $(DEPFILES)
