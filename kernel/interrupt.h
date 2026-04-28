#ifndef INTERRUPT_H
#define INTERRUPT_H

#define IDT_ENTRIES 256

typedef struct {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_mid;//IDT硬件规定32位中断地址必须分为高16位和低16位
} __attribute__((packed)) idt_entry_t;

typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_ptr_t;

typedef struct {
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
    //程序用到的全部通用寄存器
    unsigned int gs;
    unsigned int fs;
    unsigned int es;
    unsigned int ds;
    //数据段寄存器
    unsigned int error_code;
    //错误码
    unsigned int eip;//下一条指令地址
    unsigned int cs;//代码段选择子
    unsigned int eflags;//标志寄存器
    //CPU自动硬件压栈寄存器
    unsigned int user_esp;
    unsigned int ss;
    //用户态栈（只有权限切换时才需要）
} __attribute__((packed)) interrupt_frame_t;

void interrupt_init(void);
void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags);
void pic_init(void);
void pic_eoi(unsigned char irq);

extern void isr_common_stub(void);
extern void irq_common_stub(void);

void isr_handler(interrupt_frame_t* frame);
void irq_handler(interrupt_frame_t* frame);

#endif