org  0x7c00
bits 16

KERNEL_ADDR equ 0x1000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov si, msg_boot
    call serial_print

    ; 读取内核
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, KERNEL_ADDR
    int 0x13

    jc disk_err

    ; 进入32位保护模式
    cli
    lgdt [gdt_desc]

    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 0x08:start_32bit

serial_print:
    lodsb
    test al, al
    jz .end
    mov dx, 0x3F8
    out dx, al
    jmp serial_print
.end:
    ret

disk_err:
    mov si, msg_err
    call serial_print
    jmp $

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 0b10011010
    db 0b11001111
    db 0
gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 0b10010010
    db 0b11001111
    db 0
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

bits 32
start_32bit:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    jmp KERNEL_ADDR

msg_boot db "Booting 32-bit C Kernel...",0x0D,0x0A,0
msg_err  db "Disk read error",0
times 510 - ($-$$) db 0
dw 0xAA55