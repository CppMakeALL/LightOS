// 串口端口号
#define SERIAL_PORT 0x3F8
#include <stddef.h> //编译器头文件
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "memory.h"
#include "disk.h"
#include "interrupt.h"
#include "process.h"
#include "privilege.h"
#include "syscall.h"

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "d"(port));
}

void putchar(char c) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0);
    outb(SERIAL_PORT, c);
}

void print(const char *str) {
    while (*str) {
        putchar(*str);
        str++;
    }
}


void print_dec(unsigned int num) {
    char buf[16];
    int i = 0;
    if (num == 0) {
        putchar('0');
        return;
    }
    while (num > 0 && i < 15) {
        buf[i++] = num % 10 + '0';
        num /= 10;
    }
    while (i > 0) {
        putchar(buf[--i]);
    }
}

void print_hex(unsigned int n) {
    char buf[11];
    int i = 0;
    buf[i++] = '0';
    buf[i++] = 'x';
    for (int j = 7; j >= 0; j--) {
        int d = (n >> (j * 4)) & 0xF;
        buf[i++] = d < 10 ? '0' + d : 'A' + d - 10;
    }
    buf[i] = 0;
    print(buf);
}

int strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int strlen(const char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

int strstartswith(const char *str, const char *pre) {
    while (*pre) {
        if (*str++ != *pre++) return 0;
    }
    return 1;
}

void read_command(char *buf, int max_len) {
    int len = 0;
    char c;
    while (1) {
        while ((inb(SERIAL_PORT + 5) & 1) == 0);
        c = inb(SERIAL_PORT);

        if (c == '\r' || c == '\n') {
            buf[len] = 0;
            print("\r\n");
            return;
        }

        if ((c == '\b' || c == 0x7F) && len > 0) {
            len--;
            print("\b \b");
            continue;
        }

        if (c >= 32 && c < 127 && len < max_len - 1) {
            buf[len++] = c;
            putchar(c);
        }
    }
}

// ==============================
// 新增命令实现
// ==============================
void cmd_clear() {
    print("\033[2J\033[H");
}

void cmd_echo(const char *arg) {
    if (!arg || !*arg) {
        print("Usage: echo [message]\r\n");
        return;
    }
    print(arg);
    print("\r\n");
}

void cmd_ver() {
    print("LightOS v1.0 | 32-bit Kernel | Serial Shell\r\n");
}

void cmd_mem() {
    print("Memory: Kernel loaded at 0x1000\r\n");
    print("Stack: 0x7000\r\n");
    print("Free memory: ~0x60000 bytes\r\n");
}

void cmd_ls() {
    print("Built-in Commands:\r\n");
    print(" help   hello   reboot   clear   echo   ver   mem   ls\r\n");
    print(" format   mount   create   delete   read   write   fsinfo\r\n");
    print(" fork   ps\r\n");
}

void execute_command(const char *cmd) {
    if (strcmp(cmd, "hello") == 0) {
        print("Hello from LightOS Kernel!\r\n");
    }
    else if (strcmp(cmd, "help") == 0) {
        print("Available commands:\r\n");
        print(" hello   help   reboot   clear   echo   ver   mem   ls\r\n");
        print(" format   mount   create   delete   read   write   fsinfo\r\n");
        print(" fork   ps\r\n");
    }
    else if (strcmp(cmd, "reboot") == 0) {
        print("Rebooting...\r\n");
        __asm__ volatile ("int $0x18");
        while(1);
    }
    else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    }
    else if (strcmp(cmd, "ver") == 0) {
        cmd_ver();
    }
    else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    }
    else if (strstartswith(cmd, "echo ")) {
        cmd_echo(cmd + 5);
    }
    else if (strcmp(cmd, "mem") == 0) {
        mem_info();
    }
    else if (strcmp(cmd, "malloc") == 0) {
        void* p = malloc(1024);
        if (p) {
            print("Allocated: ");
            print_hex((unsigned int)p);
            print("\r\n");
        } else {
            print("Alloc failed!\r\n");
        }
    }
    else if (strcmp(cmd, "free") == 0) {
        print("Use free <addr>\r\n");
    }
    else if (strcmp(cmd, "format") == 0) {
        if (fs_format() == 0) {
            print("Filesystem formatted successfully!\r\n");
        } else {
            print("Format failed!\r\n");
        }
    }
    else if (strcmp(cmd, "mount") == 0) {
        if (fs_mount() == 0) {
            print("Filesystem mounted successfully!\r\n");
        } else {
            print("Mount failed! Try 'format' first.\r\n");
        }
    }
    else if (strstartswith(cmd, "create ")) {
        if (fs_create(cmd + 7) == 0) {
            print("File created successfully!\r\n");
        } else {
            print("Create failed!\r\n");
        }
    }
    else if (strstartswith(cmd, "delete ")) {
        if (fs_delete(cmd + 7) == 0) {
            print("File deleted successfully!\r\n");
        } else {
            print("Delete failed!\r\n");
        }
    }
    else if (strstartswith(cmd, "read ")) {
        unsigned char buffer[256];
        const char* filename = cmd + 5;
        int result = fs_read(filename, 0, buffer, 256);
        if (result > 0) {
            print("Content: ");
            for (int i = 0; i < result; i++) {
                putchar(buffer[i]);
            }
            print("\r\n");
        } else {
            print("Read failed!\r\n");
        }
    }
    else if (strstartswith(cmd, "write ")) {
        char filename[64];
        char data[128];
        const char* ptr = cmd + 6;
        int i = 0;
        
        while (*ptr && *ptr != ' ' && i < 63) {
            filename[i++] = *ptr++;
        }
        filename[i] = 0;
        
        while (*ptr == ' ') ptr++;
        
        i = 0;
        while (*ptr && i < 127) {
            data[i++] = *ptr++;
        }
        data[i] = 0;
        
        if (filename[0] != 0 && data[0] != 0) {
            int result = fs_write(filename, 0, (const unsigned char*)data, strlen(data));
            if (result > 0) {
                print("Written ");
                print_dec(result);
                print(" bytes.\r\n");
            } else {
                print("Write failed!\r\n");
            }
        } else {
            print("Usage: write <filename> <data>\r\n");
        }
    }
    else if (strcmp(cmd, "fsinfo") == 0) {
        fs_info();
    }
    else if (strcmp(cmd, "fork") == 0) {
        int pid = fork();
        if (pid >= 0) {
            print("fork: ");
            print_dec(pid);
            print("\r\n");
        } else {
            print("fork failed!\r\n");
        }
    }
    else if (strcmp(cmd, "ps") == 0) {
        process_info();
    }
    else if (*cmd) {
        print("Unknown command: ");
        print(cmd);
        print("\r\n");
    }
}

// 用户态辅助函数
static int user_strlen(const char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

static int user_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int user_strstartswith(const char *str, const char *pre) {
    while (*pre) {
        if (*str++ != *pre++) return 0;
    }
    return 1;
}

// 用户态 putchar（通过 sys_print 输出单个字符）
static void user_putchar(char c) {
    char buf[2] = {c, 0};
    sys_print(buf);
}

// 用户态读取命令
static void user_read_command(char *buf, int max_len) {
    int len = 0;
    while (1) {
        char c = (char)sys_getchar();

        if (c == '\r' || c == '\n') {
            buf[len] = 0;
            sys_print("\r\n");
            return;
        }

        if ((c == '\b' || c == 0x7F) && len > 0) {
            len--;
            sys_print("\b \b");
            continue;
        }

        if (c >= 32 && c < 127 && len < max_len - 1) {
            buf[len++] = c;
            user_putchar(c);
        }
    }
}

// 用户态命令执行
static void user_execute_command(const char *cmd) {
    if (user_strcmp(cmd, "hello") == 0) {
        sys_print("Hello from User Mode Shell!\r\n");
    }
    else if (user_strcmp(cmd, "help") == 0) {
        sys_print("User Mode Commands:\r\n");
        sys_print("  hello   - Say hello\r\n");
        sys_print("  help    - Show this help\r\n");
        sys_print("  pid     - Show process ID\r\n");
        sys_print("  ver     - Show version\r\n");
        sys_print("  clear   - Clear screen\r\n");
        sys_print("  reboot  - Reboot system\r\n");
        sys_print("  exit    - Exit user shell\r\n");
    }
    else if (user_strcmp(cmd, "pid") == 0) {
        int pid = sys_getpid();
        sys_print("PID: ");
        if (pid == 1) {
            sys_print("1\r\n");
        } else if (pid == 2) {
            sys_print("2\r\n");
        } else {
            sys_print("(unknown)\r\n");
        }
    }
    else if (user_strcmp(cmd, "ver") == 0) {
        sys_print("LightOS User Shell v1.0\r\n");
    }
    else if (user_strcmp(cmd, "clear") == 0) {
        sys_print("\033[2J\033[H");
    }
    else if (user_strcmp(cmd, "reboot") == 0) {
        sys_print("Rebooting...\r\n");
        __asm__ volatile ("int $0x18");
        while(1);
    }
    else if (user_strcmp(cmd, "exit") == 0) {
        sys_print("Exiting user shell...\r\n");
        sys_exit(0);
    }
    else if (*cmd) {
        sys_print("Unknown command: ");
        sys_print(cmd);
        sys_print("\r\n");
    }
}

// 用户进程入口函数（运行在用户态）
void user_process_entry(void) {
    char cmd[128];

    sys_print("\033[2J\033[H");
    sys_print("=====================================\r\n");
    sys_print("     LightOS User Mode Shell\r\n");
    sys_print("     (Running in Ring 3)\r\n");
    sys_print("=====================================\r\n");
    sys_print("Type 'help' for commands.\r\n\r\n");

    while (1) {
        sys_print("user> ");
        user_read_command(cmd, 128);
        user_execute_command(cmd);
    }
}

// 创建用户进程并切换到用户态
static void create_user_process(void) {
    // 分配PCB
    task_struct_t *task = (task_struct_t *)malloc(sizeof(task_struct_t));
    if (!task) {
        print("Failed to allocate user process PCB\r\n");
        return;
    }

    // 初始化PCB
    task->pid = 1;
    task->state = TASK_READY;
    task->priority = 1;
    task->time_slice = TIME_SLICE;
    task->cr3 = 0;
    task->next = NULL;
    task->prev = NULL;

    // 分配进程栈
    task->stack = (unsigned int *)malloc(STACK_SIZE);
    if (!task->stack) {
        free(task);
        print("Failed to allocate user process stack\r\n");
        return;
    }

    // 计算栈顶（从高地址向低地址增长）
    unsigned int *stack_top = (unsigned int *)((unsigned int)task->stack + STACK_SIZE);

    // 构建中断返回栈帧（iret 需要的数据）
    // iret 会依次弹出：eip, cs, eflags, esp, ss

    // ss: 用户数据段选择子 | RPL=3
    *(--stack_top) = USER_DATA_SEL | 0x03;
    // user_esp: 用户栈顶（使用进程栈顶部）
    *(--stack_top) = (unsigned int)task->stack + STACK_SIZE;
    // eflags: 开启中断 (IF=1)
    *(--stack_top) = 0x202;
    // cs: 用户代码段选择子 | RPL=3
    *(--stack_top) = USER_CODE_SEL | 0x03;
    // eip: 用户进程入口
    *(--stack_top) = (unsigned int)user_process_entry;

    // 设置进程栈指针
    task->esp = (unsigned int)stack_top;
    task->ebp = task->esp;

    // 添加到就绪队列
    if (!ready_queue) {
        ready_queue = task;
        task->next = task;
        task->prev = task;
    } else {
        task->next = ready_queue;
        task->prev = ready_queue->prev;
        ready_queue->prev->next = task;
        ready_queue->prev = task;
    }

    print("Created user process (PID: 1)\r\n");

    // 切换到用户进程
    current = task;
    task->state = TASK_RUNNING;

    // 设置TSS中的内核栈指针（用于中断时切换回内核栈）
    // 使用当前内核栈（kernel_main的栈）作为中断返回时的内核栈
    extern void set_kernel_stack(unsigned int esp);
    set_kernel_stack(0x7000);

    print("Switching to user mode...\r\n");

    // 使用 iret 切换到用户态
    __asm__ volatile (
        "cli\n"
        // 设置用户态数据段
        "movl %0, %%eax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        // 压栈构造 iret 帧
        "pushl %0\n"       // ss
        "pushl %1\n"       // esp
        "pushfl\n"         // eflags
        "popl %%eax\n"
        "orl $0x200, %%eax\n"  // 确保 IF=1
        "pushl %%eax\n"
        "pushl %2\n"       // cs
        "pushl %3\n"       // eip
        "iret\n"
        :
        : "r"(USER_DATA_SEL | 0x03),
          "r"((unsigned int)task->stack + STACK_SIZE),
          "r"(USER_CODE_SEL | 0x03),
          "r"((unsigned int)user_process_entry)
        : "eax", "memory"
    );
}

void kernel_main() {
    memory_init();
    interrupt_init();
    process_init();
    privilege_init();
    syscall_init();

    disk_init();
    fs_format();
    fs_sync();
    fs_mount();

    cmd_clear();
    print("=====================================\r\n");
    print("        LightOS 32-bit Kernel\r\n");
    print("        Serial Command Shell\r\n");
    print("=====================================\r\n");
    print("Type 'help' for commands.\r\n\r\n");

    // 创建用户进程并进入用户态
    print("Creating user process and entering user mode...\r\n");
    create_user_process();

    // 如果用户进程返回（理论上不会），继续运行shell
    char cmd[128];

    while (1) {
        print("> ");
        read_command(cmd, 128);
        execute_command(cmd);
    }
}