CROSS_COMPILE ?= aarch64-linux-gnu-
CXX      := $(CROSS_COMPILE)g++
OBJCOPY  := $(CROSS_COMPILE)objcopy

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/kernel8.img
ELF       := $(BUILD_DIR)/gameplayOS.elf

CXXFLAGS := -std=c++20 -O2 -Wall -Wextra \
            -ffreestanding -fno-exceptions -fno-rtti \
            -fno-stack-protector -fno-pic -fno-pie \
            -fno-threadsafe-statics -nostdlib \
            -mcpu=cortex-a76 -mgeneral-regs-only \
            -Ikernel -Iinclude

ASFLAGS := -mcpu=cortex-a76
LDFLAGS := -T arch/aarch64/linker.ld -nostdlib -static \
           -Wl,--build-id=none -Wl,-Map,$(BUILD_DIR)/gameplayOS.map

SOURCES := kernel/main.cpp \
            drivers/display/framebuffer.cpp \
            drivers/display/dsi.cpp
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES)) \
           $(BUILD_DIR)/boot/start.o

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/boot/start.o: boot/start.S
	@mkdir -p $(dir $@)
	$(CROSS_COMPILE)gcc $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJECTS) arch/aarch64/linker.ld
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)
