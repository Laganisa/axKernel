.section .rodata.tasks, "a"

// 쉘 쪽
.global _task_shell_start
.global _task_shell_size

_task_shell_start:
    .incbin "init/SHELL.elf"
_task_shell_end:
_task_shell_size:
    .quad _task_shell_end - _task_shell_start

// 컴파일러 쪽
.global _task_compiler_start
.global _task_compiler_size

_task_compiler_start:
    .incbin "init/COMPILER.elf"
_task_compiler_end:
_task_compiler_size:
    .quad _task_compiler_end - _task_compiler_start

// 컴파일러 쪽
.global _task_bridge_start
.global _task_bridge_size

_task_bridge_start:
    .incbin "init/BRIDGE.elf"
_task_bridge_end:
_task_bridge_size:
    .quad _task_bridge_end - _task_bridge_start