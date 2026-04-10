// 串口端口号
#define SERIAL_PORT 0x3F8
#include "memory.h"

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
}

void execute_command(const char *cmd) {
    if (strcmp(cmd, "hello") == 0) {
        print("Hello from LightOS Kernel!\r\n");
    }
    else if (strcmp(cmd, "help") == 0) {
        print("Available commands:\r\n");
        print(" hello   help   reboot   clear   echo   ver   mem   ls\r\n");
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
        // 简单演示：你可以改成传入地址
        print("Use free <addr>\r\n");
    }
    else if (*cmd) {
        print("Unknown command: ");
        print(cmd);
        print("\r\n");
    }
}

void kernel_main() {
    memory_init();
    char cmd[128];

    cmd_clear();
    print("=====================================\r\n");
    print("        LightOS 32-bit Kernel\r\n");
    print("        Serial Command Shell\r\n");
    print("=====================================\r\n");
    print("Type 'help' for commands.\r\n\r\n");

    while (1) {
        print("> ");
        read_command(cmd, 128);
        execute_command(cmd);
    }
}