// 串口端口号
#define SERIAL_PORT 0x3F8
#include "memory.h"
#include "disk.h"

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
}

void execute_command(const char *cmd) {
    if (strcmp(cmd, "hello") == 0) {
        print("Hello from LightOS Kernel!\r\n");
    }
    else if (strcmp(cmd, "help") == 0) {
        print("Available commands:\r\n");
        print(" hello   help   reboot   clear   echo   ver   mem   ls\r\n");
        print(" format   mount   create   delete   read   write   fsinfo\r\n");
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
    else if (*cmd) {
        print("Unknown command: ");
        print(cmd);
        print("\r\n");
    }
}

void kernel_main() {
    memory_init();
    
    disk_init();
    fs_format();
    fs_sync();
    fs_mount();

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