#include "syscall.h"
#include "process.h"

extern void print(const char* str);
extern void print_dec(unsigned int num);
extern void print_hex(unsigned int addr);

// 系统调用参数结构（由汇编 stub 压栈后传递）
typedef struct {
    unsigned int gs, fs, es, ds; //段寄存器
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; // 通用寄存器
    unsigned int eip, cs, eflags; // 下一条指令位置 + 权限 + CPU状态
    unsigned int user_esp, ss;  // 用户态栈
} syscall_frame_t;

// 系统调用处理函数声明
static int sys_print_impl(const char* str);
static int sys_getpid_impl(void);
static int sys_exit_impl(int code);
static int sys_getchar_impl(void);

// 系统调用分发表(函数指针数组)
static int (*syscall_table[])(unsigned int, unsigned int, unsigned int) = {
    (int (*)(unsigned int, unsigned int, unsigned int))sys_print_impl,   // 0
    (int (*)(unsigned int, unsigned int, unsigned int))sys_getpid_impl,  // 1
    (int (*)(unsigned int, unsigned int, unsigned int))sys_exit_impl,    // 2
    (int (*)(unsigned int, unsigned int, unsigned int))sys_getchar_impl, // 3
};

#define NR_SYSCALLS (sizeof(syscall_table) / sizeof(syscall_table[0]))

// ========== 系统调用具体实现 ==========

static int sys_print_impl(const char* str) {
    if (str) {
        print(str);
    }
    return 0;
}

static int sys_getpid_impl(void) {
    if (current) {
        return current->pid;
    }
    return -1;
}

static int sys_exit_impl(int code) {
    print("Process exited with code: ");
    print_dec(code);
    print("\r\n");
    if (current) {
        current->state = TASK_ZOMBIE;
    }
    while (1) {
        __asm__ volatile ("hlt");
    }
    return 0;
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static int sys_getchar_impl(void) {
    #define SERIAL_PORT 0x3F8
    while ((inb(SERIAL_PORT + 5) & 1) == 0);
    return inb(SERIAL_PORT);
}

// ========== C语言系统调用入口 ==========

void syscall_handler_c(syscall_frame_t* frame) {
    unsigned int syscall_num = frame->eax;
    unsigned int arg1 = frame->ebx;
    unsigned int arg2 = frame->ecx;
    unsigned int arg3 = frame->edx;

    if (syscall_num < NR_SYSCALLS) {
        int ret = syscall_table[syscall_num](arg1, arg2, arg3);
        frame->eax = ret;
    } else {
        print("Unknown syscall: ");
        print_dec(syscall_num);
        print("\r\n");
        frame->eax = -1;
    }
}

// ========== 初始化 ==========

void syscall_init(void) {
    print("System call initialized (int 0x80)\r\n");
}
