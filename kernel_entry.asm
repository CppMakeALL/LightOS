bits 32 ;是32位保护模式，生成32位指令
global _start   ; 必须是下划线 _start 操作系统内核默认地址
extern kernel_main

_start:          ; 必须是 _start
    call kernel_main
    jmp $