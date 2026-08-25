# axKernel

laganisa가 kernel을 공부하기 위해 만드는

실험용 및 공부용 toyKERNEL

## 개요

종류 : Microkernel

목적 기기 : QEMU Emulator (v8.2.2)

목적 아키텍처 : ARMv8-A AArch64 (Cortex-A72)

해상도 : 640 * 360 (nHD)

프래임레이트 : 30fps

## 폴더 구조

```txt
AxKernel
asssets/

boot/
    boot.S
    proc.S
    IRV/

include/
    global/
    handler/
    manage/
    tools/
    _defs.h
    _macro.h
    _sect.h
    _types.h

init/
    init_binary.S

src/
    main.c
    frm/
    global/
    handler/
    manage/
    tools/

usr/
    axLib/
    axShell/
    axCompil/
    axBridge/
    
linker.ld
Makefile
README.md
reference.md
```

## 실행법

## 현재 구현되어 있는 기능

- 메모리 할당 및 메모리 관리자
- 프로세스 및 프로세스 관리자
- 파일 읽고 쓰기 및 파일 관리자
- 커널과 커널 프로세스 간 시스템 콜

## 자세한 정보

패치 노트

[patch note](./assets/PatchNote.txt)

더 많은 정보

[DETAIL](./assets/DETAIL.md)

## 추후에 구현하거나 수정할 점

구현 및 수정할 점

[TODO](./assets/next.md)
