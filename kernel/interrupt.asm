bits 32

; 这两个函数是在C语言里写的,需要声明extern
extern isr_handler
extern irq_handler
extern page_fault_handler

%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    cli
    push 0
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
  global isr%1
  isr%1:
    cli
    push %1
    jmp isr_common_stub
%endmacro

%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push %2
    push %1
    jmp irq_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
; ISR 14 (Page Fault) 在下面单独定义，需要特殊处理CR2寄存器
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern isr_common_handler
global isr_common_stub
isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp ;当前栈指针
    call isr_handler
    pop esp
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

extern irq_common_handler
global irq_common_stub
irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_handler
    pop esp
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

; ========== 缺页异常处理 ISR 14 ==========
; 缺页异常（Page Fault）是ISR 14，它会压入错误码
; 需要特殊处理：读取CR2寄存器获取缺页地址
extern page_fault_handler
global isr14

isr14:
    cli
    push 14
    jmp page_fault_stub

page_fault_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 读取CR2寄存器（CPU自动将缺页虚拟地址存入CR2）
    mov eax, cr2
    push eax

    ; 获取错误码（在栈上的位置）
    ; 当前栈结构：gs,fs,es,ds,edi,esi,ebp,esp,ebx,edx,ecx,eax,error_code,irq_num,eip,cs,eflags
    ; 错误码在 [esp + 52] 的位置（push eax后）
    mov ebx, [esp + 52]
    push ebx

    ; 调用C语言处理函数：page_fault_handler(error_code, cr2)
    call page_fault_handler
    add esp, 8  ; 清理压入的cr2和error_code参数

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8  ; 清理错误码和中断号
    iret

; ========== 系统调用中断 int 0x80 ==========
extern syscall_handler_c
global syscall_handler_asm
global isr128

isr128:
    cli
    push 0
    push 128
    jmp syscall_common_stub

syscall_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10 ; 0x10 是内核数据段的段选择子
    mov ds, ax ; 把 ds/es/fs/gs 全部切到内核段
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp ; 把栈指针作为参数传给 C 函数
    call syscall_handler_c ; 调用 C 函数
    pop esp ; 调用完恢复栈指针
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8 ; 清理栈上的：中断号 + 错误码
    iret ; 中断返回,从内核态 → 用户态