// axOS_kernel/init/init_binary.S
.section .rodata
.global _shell_binary_start
.global _shell_binary_size

_shell_binary_start:
    .incbin "init/SHELL.BIN"
_shell_binary_end:

_shell_binary_size:
    .quad _shell_binary_end - _shell_binary_start