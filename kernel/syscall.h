#ifndef SYSCALL_H
#define SYSCALL_H

// 系统调用号定义
#define SYSCALL_PRINT       0   // 打印字符串
#define SYSCALL_GETPID      1   // 获取进程ID
#define SYSCALL_EXIT        2   // 进程退出
#define SYSCALL_GETCHAR     3   // 从串口读取一个字符
#define SYSCALL_ALLOC_PAGE  4   // 分配虚拟页面（用于用户态缺页处理）
#define SYSCALL_FREE_PAGE   5   // 释放虚拟页面

// 页面分配标志
#define ALLOC_FLAG_WRITE    0x01  // 可写
#define ALLOC_FLAG_USER     0x02  // 用户态可访问

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

// 用户态请求分配虚拟页面
// vaddr: 请求的虚拟地址（按页对齐）
// flags: ALLOC_FLAG_WRITE | ALLOC_FLAG_USER
// 返回值：0成功，-1失败
static inline int sys_alloc_page(unsigned int vaddr, unsigned int flags) {
    return syscall2(SYSCALL_ALLOC_PAGE, (int)vaddr, (int)flags);
}

// 用户态请求释放虚拟页面
// vaddr: 虚拟地址
// 返回值：0成功，-1失败
static inline int sys_free_page(unsigned int vaddr) {
    return syscall1(SYSCALL_FREE_PAGE, (int)vaddr);
}

// 内核态系统调用处理入口
void syscall_init(void);
void syscall_handler_asm(void);

#endif
