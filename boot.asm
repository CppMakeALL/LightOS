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
    mov ah, 0x02 ; 0x02表示去读磁盘
    mov al, 8 ; 表示读3个磁盘 ,(具体读几个需要根据kernel.c内核代码大小确定)
    mov ch, 0 ; 柱面号
    mov cl, 2 ; 起始扇区号，1是boot 2是kernel
    mov dh, 0 ; 磁头号
    mov dl, 0x80 ;0x80表示第一块磁盘
    mov bx, KERNEL_ADDR ;把磁盘地址读到这个内存
    int 0x13 ;调用bios磁盘中断

    jc disk_err ;读磁盘失败则报错

    ; 进入32位保护模式
    cli
    lgdt [gdt_desc]

    mov eax, cr0 ;cr0是控制寄存器，不能直接修改需要借助eax和al
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

gdt_start: ;0x00
    dq 0 ; 8字节全0 = 空描述符
gdt_code: ;0x08 8字节
    dw 0xFFFF   ; Limit 0~15
    dw 0        ; Base 0~15
    db 0        ; Base 16~23
    db 0b10011010 ; 代码段，特权0，可读可执行
    db 0b11001111 ; 4K粒度，32位，Limit 16~19
    db 0        ; Base 24~31
gdt_data: ;0x10
    dw 0xFFFF
    dw 0
    db 0
    db 0b10010010
    db 0b11001111
    db 0
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1 ; 大小
    dd gdt_start ; 地址

bits 32
start_32bit:
    mov ax, 0x10     ; 0x10 = GDT 数据段选择子
    mov ds, ax       ; 把所有段寄存器 → 指向数据段
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    jmp KERNEL_ADDR

msg_boot db "Booting 32-bit C Kernel...",0x0D,0x0A,0
msg_err  db "Disk read error",0
times 510 - ($-$$) db 0
dw 0xAA55