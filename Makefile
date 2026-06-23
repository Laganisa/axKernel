CC      = aarch64-linux-gnu-gcc
LD      = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

LIB_DIR   = usr/axLib
SHELL_DIR = usr/axShell

# 컴파일 플래그
CFLAGS  = -mcpu=cortex-a72 -ffreestanding -nostdlib -nostdinc -O0 -g -Iinclude
LDFLAGS = -T linker.ld --gc-sections

# 빌드 디렉토리
BUILD_DIR = build
KERNEL    = $(BUILD_DIR)/kernel8

# 1. 소스 자동 탐색: src, boot, init 폴더 내 모든 .c, .S 파일
SRCS = $(shell find src boot init -name "*.c" -o -name "*.S")

# 2. 오브젝트 경로 매핑: .c/.S 파일을 build/ 아래의 .o 파일로 변환
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

.PHONY: all clean user_modules

# 전체 빌드 순서
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

# 오브젝트 빌드 규칙 (하위 디렉토리 자동 생성 포함)
$(BUILD_DIR)/%.o: %
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 링킹
$(KERNEL).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# 바이너리 이미지 생성
$(KERNEL).img: $(KERNEL).elf
	$(OBJCOPY) $< -O binary $@
	@echo "---------------------------------------"
	@echo "axKernel Build Success"
	@echo "---------------------------------------"

# 청소
clean:
	rm -rf $(BUILD_DIR) init/SHELL.BIN
	@$(MAKE) -C $(LIB_DIR) clean --no-print-directory 2>/dev/null || true
	@$(MAKE) -C $(SHELL_DIR) clean --no-print-directory 2>/dev/null || true