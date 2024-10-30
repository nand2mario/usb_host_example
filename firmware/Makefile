#
# The xPack GNU RISC-V Embedded GCC
# https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack
#
TARGET_ELF = firmware.elf
TARGET_BIN = firmware.bin
TARGET_HEX = firmware.hex
TARGET_LST = firmware.lst
RISCV = riscv-none-elf
CC = $(RISCV)-gcc
OBJCOPY = $(RISCV)-objcopy
OBJDUMP = $(RISCV)-objdump
CFLAGS  = -Wall -O2 -g -mabi=ilp32 -march=rv32i -ffreestanding -Iusb -I.
LFLAGS = -mabi=ilp32 -march=rv32i -Wl,--build-id=none,-Bstatic,-T,baremetal.ld -nostdlib
LIBS = -lgcc

SRCS := start.S firmware.c minlibc.c usb/usb_core.c usb/usb_hw.c
OBJS := $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)
HDRS := $(wildcard *.h)

ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

all: $(TARGET_HEX) $(TARGET_LST)

%.o: %.c $(HDRS)
ifneq ($(VERBOSE),1)
	@echo CC $@
endif
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S $(HDRS)
ifneq ($(VERBOSE),1)
	@echo CC $@
endif
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET_BIN) $(TARGET_ELF) $(OBJS)

$(TARGET_ELF): $(OBJS)
ifneq ($(VERBOSE),1)
	@echo CC $@
endif
	$(Q)$(CC) $(LFLAGS) $(OBJS) $(LIBS) -o $@

$(TARGET_BIN): $(TARGET_ELF)
ifneq ($(VERBOSE),1)
	@echo OBJCOPY $@
endif
	$(Q)$(OBJCOPY) $(TARGET_ELF) $(TARGET_BIN) -O binary

$(TARGET_HEX): $(TARGET_BIN)
ifneq ($(VERBOSE),1)
	@echo HEXDUMP $@
endif
	$(Q)hexdump -v -e '1/4 "%08x\n"' $(TARGET_BIN) > $(TARGET_HEX)

$(TARGET_LST): $(TARGET_ELF)
ifneq ($(VERBOSE),1)
	@echo OBJDUMP $@
endif
	$(Q)$(OBJDUMP) -S -d -M reg-names-raw $(TARGET_ELF) > $(TARGET_LST)

clean:
	$(Q)rm -f $(OBJS) $(TARGET_ELF) $(TARGET_BIN) $(TARGET_HEX) $(TARGET_LST)

.PHONY: default all clean
