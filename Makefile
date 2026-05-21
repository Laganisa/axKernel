# 검증 필요
# aarch64 OS 커널 빌드 설정

CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

# 리포지토리 경로 정의
LIB_DIR   = ../myLib
SHELL_DIR = ../myShell

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

all: user_modules $(KERNEL).img

# 1. 외부 유저 공간 빌드 및 바이너리 땡겨오기
user_modules:
	@mkdir -p init
	@$(MAKE) -C $(LIB_DIR) --no-print-directory
	@$(MAKE) -C $(SHELL_DIR) --no-print-directory
	@cp -f $(SHELL_DIR)/SHELL.BIN init/ 2>/dev/null || touch init/SHELL.BIN

$(BUILD_DIR):
	@mkdir -p $@

# 2. 컴파일 규칙 (패턴 매칭 하나로 통일)
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: boot/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: init/%.S init/SHELL.BIN | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 3. 링킹 및 이미지 추출
$(KERNEL).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(KERNEL).img: $(KERNEL).elf
	$(OBJCOPY) $< -O binary $@

clean:
	rm -rf $(BUILD_DIR) init/SHELL.BIN
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory 2>/dev/null || true
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory 2>/dev/null || true