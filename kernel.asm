org 0x1000
bits 16

start:
    ; 内核信息 -> 串口输出
    mov si, msg_kernel
    call serial_print

    ; 死循环停住
    jmp $

; ==========================
; 内核也用串口打印
; ==========================
serial_print:
    lodsb
    test al, al
    jz .end
    mov dx, 0x3F8
    out dx, al
    jmp serial_print
.end:
    ret

msg_kernel db "LightOS Kernel Running!", 0x0D, 0x0A, 0