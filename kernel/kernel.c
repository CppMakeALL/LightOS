// 串口端口号
#define SERIAL_PORT 0x3F8

// 向串口打印一个字符
void putchar(char c) {
    __asm__ volatile ("outb %0, %1" : : "a"(c), "d"(SERIAL_PORT));
}

// 打印字符串
void print(const char *str) {
    while (*str) {
        putchar(*str);
        str++;
    }
}

// 内核主函数（C 语言入口）
void kernel_main() {
    print("Hello from 32-bit C Kernel!\r\n");
    print("This is my first OS kernel written in C!\r\n");

    // 死循环
    //while (1);
}