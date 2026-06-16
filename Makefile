CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

LIB_DIR   = usr/axLib
SHELL_DIR = usr/axShell

CFLAGS  = -mcpu=cortex-a72 -ffreestanding -nostdlib -nostdinc -O0 -g -Iinclude
LDFLAGS = -T linker.ld --gc-sections

BUILD_DIR = build
KERNEL    = $(BUILD_DIR)/kernel8

# 소스 및 오브젝트 자동 수집
SRCS = $(wildcard src/*.c) $(wildcard boot/*.S) $(wildcard init/*.S)
OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(filter src/%.c, $(SRCS))) \
       $(patsubst boot/%.S, $(BUILD_DIR)/%.o, $(filter boot/%.S, $(SRCS))) \
       $(patsubst init/%.S, $(BUILD_DIR)/%.o, $(filter init/%.S, $(SRCS)))

.PHONY: all clean user_modules

# 빌드 순서 보장
all: user_modules $(KERNEL).img

# 유저 공간 모듈 빌드
user_modules:
	@echo "Building User Modules..."
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory
	@$(MAKE) -C $(LIB_DIR) --no-print-directory
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory
	@$(MAKE) -C $(SHELL_DIR) --no-print-directory
	@mkdir -p init
	@cp -f $(SHELL_DIR)/SHELL.BIN init/

# 빌드 디렉토리 생성
$(BUILD_DIR):
	@mkdir -p $@

# 컴파일 규칙 (의존성 포함)
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: boot/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: init/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 링킹 및 이미지 추출
$(KERNEL).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL).img: $(KERNEL).elf
	$(OBJCOPY) $< -O binary $@
	@echo "---------------------------------------"
	@echo "axKernel Build Success"
	@echo "---------------------------------------"

clean:
	rm -rf $(BUILD_DIR) init/SHELL.BIN
	find . -name "*.o" -type f -delete
	find . -name "*.elf" -type f -delete
	find . -name "*.bin" -type f -delete
	find . -name "*.img" -type f -delete
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory 2>/dev/null || true
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory 2>/dev/null || true