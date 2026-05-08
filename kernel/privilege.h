#ifndef PRIVILEGE_H
#define PRIVILEGE_H

// 段选择子定义
#define KERNEL_CODE_SEL  0x08  // 内核代码段
#define KERNEL_DATA_SEL  0x10  // 内核数据段
#define USER_CODE_SEL    0x18  // 用户代码段
#define USER_DATA_SEL    0x20  // 用户数据段
#define TSS_SEL          0x28  // TSS段

// 特权级定义
#define PRIVILEGE_KERNEL 0  // 内核态
#define PRIVILEGE_USER   3  // 用户态

// 函数声明
void privilege_init(void);
void switch_to_user_mode(void);
void syscall_handler(void);

#endif