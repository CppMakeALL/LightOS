org  0x7c00
bits 16

KERNEL_ADDR equ 0x1000

start:
    xor ax, ax    ; 将 ax 寄存器置为 0 ,ax只是一个搬运工，将0值搬给其他寄存器，CPU 不允许直接把一个立即数（比如数字 0）赋值给段寄存器
    mov ds, ax    ; 数据段寄存器 DS = 0
    mov es, ax    ; 附加段寄存器 ES = 0
    mov ss, ax    ; 堆栈段寄存器 SS = 0
    mov sp, 0x7c00 ; 设置栈顶指针 SP

    ; 串口打印
    mov si, msg_boot
    call serial_print

    ; 读磁盘
    mov ah, 0x02 ;告诉 BIOS 我们要执行什么操作。0x02 代表读取扇区
    mov al, 1 ;告诉 BIOS 我们要读取 1 个扇区
    mov ch, 0 ;磁道（柱面）：设置为 0，表示从第 0 号磁道开始读。
    mov cl, 2 ;表示从第 2 号扇区开始读,通常第 1 个扇区（MBR）存放的是引导代码，而第 2 个扇区存放的是我们要加载的内核或下一阶段引导程序
    mov dh, 0; 磁头：设置为 0，表示使用 0 号磁头
    mov dl, 0x80 ;磁头：设置为 0，表示使用 0 号磁头
    mov bx, KERNEL_ADDR 
    int 0x13 ;中断,从磁盘读取数据，加载内核到内存

    jc disk_error

    mov si, msg_ok
    call serial_print

    ; 跳内核
    jmp KERNEL_ADDR ;这里是执行内核

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