#ifndef SYSCALL_H
#define SYSCALL_H

// 系统调用号定义
#define SYSCALL_PRINT       0   // 打印字符串
#define SYSCALL_GETPID      1   // 获取进程ID
#define SYSCALL_EXIT        2   // 进程退出
#define SYSCALL_GETCHAR     3   // 从串口读取一个字符

// 用户态系统调用接口（内联汇编）
static inline int syscall0(int num) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num)
        : "memory" //表示这段汇编可能修改了内存里的数据，编译器你要重新读取内存，不要用缓存的值
    );
    return ret;
}

static inline int syscall1(int num, int arg1) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1)
        : "memory"
    );
    return ret;
}

static inline int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2)
        : "memory"
    );
    return ret;
}

// 用户态可用的系统调用包装函数
static inline int sys_print(const char* str) {
    return syscall1(SYSCALL_PRINT, (int)str);
}

static inline int sys_getpid(void) {
    return syscall0(SYSCALL_GETPID);
}

static inline int sys_exit(int code) {
    return syscall1(SYSCALL_EXIT, code);
}

static inline int sys_getchar(void) {
    return syscall0(SYSCALL_GETCHAR);
}

// 内核态系统调用处理入口
void syscall_init(void);
void syscall_handler_asm(void);

#endif
