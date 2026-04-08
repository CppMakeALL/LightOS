org  0x7c00
bits 16

KERNEL_ADDR equ 0x1000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; 串口打印
    mov si, msg_boot
    call serial_print

    ; 读磁盘
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, KERNEL_ADDR
    int 0x13

    jc disk_error

    mov si, msg_ok
    call serial_print

    ; 跳内核
    jmp KERNEL_ADDR

halt:
    jmp $

disk_error:
    mov si, msg_err
    call serial_print
    jmp halt

; ==========================
; 串口打印函数（0x3F8）
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

msg_boot db "Booting LightOS...", 0x0D, 0x0A, 0
msg_ok   db "Read kernel ok!", 0x0D, 0x0A, 0
msg_err  db "Disk read error!", 0x0D, 0x0A, 0

times 510 - ($ - $$) db 0
dw 0xaa55