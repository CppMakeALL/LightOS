# 实现本操作系统的基础知识
# 常见段寄存器
CS  代码段
DS  数据段
ES  附加数据段
FS  扩展段
GS  扩展段
SS  栈段

# 源地址寄存器和目标地址寄存器
esi  源地址寄存器
edi  目标地址寄存器
ecx  拷贝次数寄存器
使用案例
mov esi, 0x10000   ; 源地址给 ESI
mov edi, 0x20000   ; 目标地址给 EDI
mov ecx, 100       ; 拷贝100字节
rep movsb          ; 自动循环拷贝

# 常用寄存器
eip  指令寄存器 CPU 下一条要执行的指令的内存地址
esp  栈指针寄存器
ebp  基础指针寄存器
eflags  状态标志寄存器,里面存的是 CPU 刚执行完指令后的各种 “状态、结果、开关”
eax,ebx,ecx,edx  通用寄存器
ax,bx,cx,dx  通用寄存器exx的低16位
al,bl,cl,dl  ax,bx,cx,dx的低8位
ah,bh,ch,dh  ax,bx,cx,dx的高8位

# 汇编语法
int $0x80      正确：触发 0x80 号中断(AT&T 汇编语法)
int 0x80       错误：去访问内存地址 0x80 的值，然后当中断号(AT&T 汇编语法)

GCC内联汇编格式
__asm__ __volatile__(
    "指令"
    : 输出部分 (写结果)
    : 输入部分 (给数据)
    : 破坏部分
);