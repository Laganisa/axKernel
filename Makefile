CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

LIB_DIR   = usr/axLib
SHELL_DIR = usr/axShell

KERNEL_BUILD = build/kernel
DEVO_BUILD   = build/devo

KERNEL_INC = -Iinclude \
             -I$(LIB_DIR)/include \
             -I$(SHELL_DIR)/include

BASE_CFLAGS = \
	-mcpu=cortex-a72 \
	-ffreestanding \
	-nostdlib \
	-nostdinc \
	-O0 \
	-g \
	$(KERNEL_INC)

LDFLAGS = -T linker.ld --gc-sections

SRCS := $(shell find src boot init -name "*.c" -o -name "*.S")

KERNEL_OBJS := $(SRCS:%=$(KERNEL_BUILD)/%.o)
DEVO_OBJS   := $(SRCS:%=$(DEVO_BUILD)/%.o)

.PHONY: all kernel devo_test clean user_modules

all: kernel

kernel: user_modules $(KERNEL_BUILD)/kernel8.img

devo: user_modules $(DEVO_BUILD)/net8.img

user_modules:
	@echo "Building User Modules..."
	@$(MAKE) -C $(LIB_DIR) --no-print-directory
	@$(MAKE) -C $(SHELL_DIR) --no-print-directory
	@mkdir -p init
	@cp -f $(SHELL_DIR)/build/SHELL.elf init/


$(KERNEL_BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(BASE_CFLAGS) -c $< -o $@

$(DEVO_BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(BASE_CFLAGS) -DDEVO_TEST -c $< -o $@


$(KERNEL_BUILD)/kernel8.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(DEVO_BUILD)/net8.elf: $(DEVO_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(KERNEL_BUILD)/kernel8.img: $(KERNEL_BUILD)/kernel8.elf
	$(OBJCOPY) $< -O binary $@
	@echo "---------------------------------------"
	@echo " axKernel Build Success"
	@echo "---------------------------------------"

$(DEVO_BUILD)/net8.img: $(DEVO_BUILD)/net8.elf
	$(OBJCOPY) $< -O binary $@
	@echo "---------------------------------------"
	@echo " axDEVO Test Build Success"
	@echo "---------------------------------------"

clean:
	rm -rf build
	rm -f init/SHELL.elf
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory 2>/dev/null || true
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory 2>/dev/null || true