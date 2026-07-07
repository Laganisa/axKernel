CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

# 경로 정의
LIB_DIR   = usr/axLib
SHELL_DIR = usr/axShell

# 헤더 경로를 변수로 관리 (커널은 커널 헤더만, 필요시 유저 헤더도 추가)
KERNEL_INC = -Iinclude -I$(LIB_DIR)/include -I$(SHELL_DIR)/include

# CFLAGS에 KERNEL_INC 적용
CFLAGS  = -mcpu=cortex-a72 -ffreestanding -nostdlib -nostdinc -O0 -g $(KERNEL_INC)
LDFLAGS = -T linker.ld --gc-sections

BUILD_DIR = build
KERNEL    = $(BUILD_DIR)/kernel8

SRCS = $(shell find src boot init -name "*.c" -o -name "*.S")
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

.PHONY: all clean user_modules

all: user_modules $(KERNEL).img

user_modules:
	@echo "Building User Modules..."
	@$(MAKE) -C $(LIB_DIR) --no-print-directory
	@$(MAKE) -C $(SHELL_DIR) --no-print-directory
	@mkdir -p init
	@cp -f $(SHELL_DIR)/build/SHELL.elf init/

$(BUILD_DIR)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL).img: $(KERNEL).elf
	$(OBJCOPY) $< -O binary $@
	@echo "---------------------------------------"
	@echo "axKernel Build Success"
	@echo "---------------------------------------"

clean:
	rm -rf $(BUILD_DIR) init/SHELL.BIN
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory 2>/dev/null || true
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory 2>/dev/null || true