#include "interrupt.h"

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

void print(const char* str);
void print_hex(unsigned int num);

static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "d"(port));
}

//num 中断号 base 中断地址 sel 选择子 flags 标志位(自动关中断和陷阱门标志)
void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

//初始化PIC控制器把硬件中断重新映射到 32 ~ 47,防止与CPU异常中断冲突
//PIC控制器默认管键盘,时钟，硬盘等中断
void pic_init() {
    // 1. 给主、从PIC发命令：要开始初始化了
    outb(0x20, 0x11);  // 主PIC开始初始化
    outb(0xA0, 0x11);  // 从PIC开始初始化

    // 2. 设置中断号偏移量（最重要！重映射！）
    outb(0x21, 0x20);  // 主PIC中断号从 32(0x20) 开始
    outb(0xA1, 0x28);  // 从PIC中断号从 40(0x28) 开始

    // 3. 设置主从连接关系（硬件固定接线）
    outb(0x21, 0x04);  // 主PIC连接着从PIC
    outb(0xA1, 0x02);  // 从PIC级联到主PIC

    // 4. 让PIC工作在 8086 模式（x86标准模式）
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // 5. 打开所有中断屏蔽位（允许硬件发中断）
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

void pic_eoi(unsigned char irq) {
    // 如果中断号 >= 8，说明是从 PIC 发来的（IRQ8~15）
    //主PIC 0-7 从PIC 8-15
    if (irq >= 8) {
        outb(0xA0, 0x20);  // 给 从PIC 发 EOI 结束信号
    }

    // 不管主还是从，都必须给 主PIC 发结束信号
    outb(0x20, 0x20);      // 给 主PIC 发 EOI 结束信号
}

static const char* exception_messages[] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Into detected overflow",
    "Out of bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "SIMD floating-point",
    "Virtualization",
    "Control protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(interrupt_frame_t* frame) {
    print("\r\nException: ");
    if (frame->error_code < 32) {
        print(exception_messages[frame->error_code]);
    } else {
        print("Unknown");
    }
    print("\r\n");
    print("Error code: ");
    print_hex(frame->error_code);
    print("\r\n");
    print("EIP: ");
    print_hex(frame->eip);
    print("\r\n");
}

void irq_handler(interrupt_frame_t* frame) {
    unsigned char irq = frame->error_code;
    
    if (irq == 0) {
        print("\r\nTimer interrupt\r\n");
    } else if (irq == 1) {
        print("\r\nKeyboard interrupt\r\n");
    }
    
    pic_eoi(irq);
}

//整个中断系统的初始化
void interrupt_init() {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base = (unsigned int)&idt;

    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (unsigned long)isr_common_stub, 0x08, 0x8E);
    }

    for (int i = 32; i < 48; i++) {
        idt_set_gate(i, (unsigned long)irq_common_stub, 0x08, 0x8E);
    }

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    
    pic_init();
}