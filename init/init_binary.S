.section .rodata.tasks, "a"
.global _task_shell_start
.global _task_shell_size

_task_shell_start:
    .incbin "init/SHELL.elf"
_task_shell_end:

_task_shell_size:
    .quad _task_shell_end - _task_shell_start