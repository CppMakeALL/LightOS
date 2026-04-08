; 告诉汇编器：这段内核代码，将来会被加载到内存地址 0x1000 处运行
; 所有变量、标签的地址都从 0x1000 开始计算
org  0x1000

; 告诉汇编器：生成 16位 的机器码
; 因为现在还在 BIOS 实模式下，CPU 是 16位 运行状态
bits 16

; ==============================
; 内核入口点（真正开始执行的地方）
; ==============================
start:
    ; 把 AX 寄存器清零（异或自己 = 0）
    xor ax, ax

    ; 将 DS（数据段寄存器）设置为 0
    ; 表示我们的数据段从内存 0x0000 开始
    mov ds, ax

    ; 将 ES（额外段寄存器）设置为 0
    mov es, ax

    mov si, msg_welcome
    call serial_print

; ==============================
; 命令行主循环（死循环，永不退出）
; 作用：显示提示符 -> 读取输入 -> 解析命令 -> 再次显示提示符
; ==============================
command_loop:
    ; 把提示符字符串 ">" 的地址传给 SI
    mov si, prompt

    call serial_print

    ; 调用串口读行函数（等待用户输入命令）
    call serial_read_line

    ; 调用命令解析函数（判断用户输入了什么命令）
    call parse_command

    ; 跳回循环开头，继续等待下一条命令
    jmp command_loop

; ==============================
; 函数名：serial_print
; 功能：通过串口 COM1（0x3F8）打印以 0 结尾的字符串
; 输入：SI = 字符串起始地址
; ==============================
serial_print:
    ; 从 SI 指向的内存读取 1 个字节到 AL
    ; 同时 SI 自动 +1，指向下一个字符
    lodsb

    ; 测试 AL 是否为 0（字符串结束符）
    test al, al

    ; 如果 AL = 0，跳转到 .end 结束函数
    jz .end

    ; 设置串口端口号 0x3F8（COM1 数据端口）
    mov dx, 0x3F8

    ; 将 AL 中的字符通过串口发送出去
    out dx, al

    ; 继续打印下一个字符（循环）
    jmp serial_print

.end:
    ; 函数返回，回到调用处
    ret

; ==============================
; 函数名：serial_read_line
; 功能：从串口读取一行用户输入，保存到缓冲区，自带回显
; 输出：input_buf = 用户输入的字符串（以 0 结尾）
; ==============================
serial_read_line:
    ; 保存 DI 寄存器（因为后面要使用，调用栈规则：用完要恢复）
    push di

    ; 将 DI 指向输入缓冲区 input_buf
    ; DI = 目的变址寄存器，用于写入数据
    mov di, input_buf

; 循环读取每一个字符
.next_char:
    ; 读取串口线路状态寄存器 0x3FD
    ; 用于判断是否有数据可读
    mov dx, 0x3FD

; 等待串口数据到来（死等，直到有按键输入）
.in_wait:
    ; 从 0x3FD 端口读取状态到 AL
    in al, dx

    ; 测试第 0 位是否为 1（= 收到数据）
    test al, 1

    ; 如果没有数据，跳回 .in_wait 继续等待
    jz .in_wait

    ; 有数据了，从 0x3F8 读取数据到 AL
    mov dx, 0x3F8
    in al, dx

    ; 判断是否是回车键（0x0D = 回车）
    cmp al, 0x0D

    ; 如果是回车，跳转到 .enter 处理
    je .enter

    ; 如果不是回车，就是普通字符
    ; 将 AL 写入 DI 指向的缓冲区
    stosb

    ; 回显：把输入的字符发回串口，让用户看到自己输入的内容
    mov dx, 0x3F8
    out dx, al

    ; 继续读取下一个字符
    jmp .next_char

; 处理回车键
.enter:
    ; 发送回车符 0x0D
    mov al, 0x0D
    mov dx, 0x3F8
    out dx, al

    ; 发送换行符 0x0A
    mov al, 0x0A
    out dx, al

    ; 在字符串末尾添加 0（表示字符串结束）
    mov byte [di], 0

    ; 恢复之前保存的 DI 寄存器
    pop di

    ; 函数返回
    ret

; ==============================
; 函数名：parse_command
; 功能：解析用户输入的命令，执行对应操作
; ==============================
parse_command:
    ; SI 指向输入缓冲区（用户输入的命令）
    mov si, input_buf

    ; ==============================
    ; 命令 1：判断是否输入 "help"
    ; ==============================
    ; DI 指向命令字符串 "help"
    mov di, cmd_help

    ; 调用字符串比较函数
    call strcmp

    ; 如果相等（CF=1），跳转到 .help 执行
    jc .help

    ; ==============================
    ; 命令 2：判断是否输入 "hello"
    ; ==============================
    mov di, cmd_hello
    call strcmp
    jc .hello

    ; ==============================
    ; 命令 3：判断是否输入 "clear"
    ; ==============================
    mov di, cmd_clear
    call strcmp
    jc .clear

    ; ==============================
    ; 命令 4：判断是否输入 "quit"
    ; ==============================
    mov di, cmd_quit
    call strcmp
    jc .quit

    ; ==============================
    ; 没有匹配到任何命令：未知命令
    ; ==============================
    mov si, msg_unknown
    call serial_print
    ret

; 执行 help 命令
.help:
    mov si, msg_help_list
    call serial_print
    ret

; 执行 hello 命令
.hello:
    mov si, msg_hello
    call serial_print
    ret

; 执行 clear 清屏命令
.clear:
    mov si, msg_clear   ; ANSI 清屏序列
    call serial_print
    ret

; 执行 quit 退出命令
.quit:
    mov si, msg_halt
    call serial_print
    jmp $               ; 死循环，系统停机

; ==============================
; 函数名：strcmp
; 功能：比较两个字符串是否完全相等
; 输入：SI = 字符串1，DI = 字符串2
; 输出：CF=1 相等，CF=0 不相等
; ==============================
strcmp:
    ; 保存寄存器
    push si
    push di

.loop:
    ; 读取 SI 的一个字符到 AL
    lodsb

    ; 读取 DI 的一个字符到 BL
    mov bl, [di]

    ; DI 指向下一个字符
    inc di

    ; 比较两个字符
    cmp al, bl

    ; 不相等，跳转到 .no
    jne .no

    ; 如果 AL = 0，说明两个字符串都结束了，且完全相等
    test al, al
    je .yes

    ; 继续比较下一个字符
    jmp .loop

; 字符串相等
.yes:
    stc         ; 设置 CF 标志位 = 1
    pop di
    pop si
    ret

; 字符串不相等
.no:
    clc         ; 清除 CF 标志位 = 0
    pop di
    pop si
    ret

; ==============================
; 数据区（所有字符串、缓冲区定义）
; ==============================
; 欢迎信息，0x0D=回车，0x0A=换行，0=结束
msg_welcome  db  "LightOS Serial Shell Ready", 0x0D,0x0A,0

; 命令提示符
prompt       db  "> ",0

; 命令字符串
cmd_help     db  "help",0
cmd_hello    db  "hello",0
cmd_clear    db  "clear",0
cmd_quit     db  "quit",0

; 命令输出信息
msg_hello    db  "Hello from LightOS kernel!", 0x0D,0x0A,0
msg_help_list db "Commands: help | hello | clear | quit", 0x0D,0x0A,0
msg_unknown  db  "Unknown command", 0x0D,0x0A,0
msg_halt     db  "System halted.", 0x0D,0x0A,0

; ANSI 终端清屏序列（\033[2J 清屏 + \033[H 光标归位）
msg_clear    db  0x1B,"[2J",0x1B,"[H",0

; 输入缓冲区：分配 128 字节空间，用于存储用户输入的命令
input_buf times 128 db 0