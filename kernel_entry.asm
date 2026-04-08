bits 32
global _start   ; 必须是下划线 _start
extern kernel_main

_start:          ; 必须是 _start
    call kernel_main
    jmp $