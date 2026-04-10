#include "memory.h"

// 仅声明外部函数，不定义！
void print(const char* str);
void print_dec(unsigned int num);
void print_hex(unsigned int addr);

#define MEM_START   0x10000
#define MEM_END     0x80000
#define PAGE_SIZE   4096
#define PAGE_COUNT  ((MEM_END - MEM_START) / PAGE_SIZE)

static unsigned char memory_bitmap[PAGE_COUNT / 8];

void memory_init() {
    for (int i = 0; i < PAGE_COUNT / 8; i++) {
        memory_bitmap[i] = 0;
    }
}

static int alloc_page() {
    for (int i = 0; i < PAGE_COUNT; i++) {
        int byte = i / 8;
        int bit = i % 8;
        if (!(memory_bitmap[byte] & (1 << bit))) {
            memory_bitmap[byte] |= (1 << bit);
            return i;
        }
    }
    return -1;
}

static void free_page(int page) {
    if (page < 0 || page >= PAGE_COUNT) return;
    int byte = page / 8;
    int bit = page % 8;
    memory_bitmap[byte] &= ~(1 << bit);
}

void* malloc(unsigned int size) {
    if (size == 0 || size > PAGE_SIZE) return 0;
    int page = alloc_page();
    if (page < 0) return 0;
    return (void*)(MEM_START + page * PAGE_SIZE);
}

void free(void* ptr) {
    if (!ptr) return;
    unsigned int addr = (unsigned int)ptr;
    if (addr < MEM_START || addr >= MEM_END) return;
    int page = (addr - MEM_START) / PAGE_SIZE;
    free_page(page);
}

void mem_info() {
    int used = 0;
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (memory_bitmap[i / 8] & (1 << (i % 8))) used++;
    }

    print("Memory Status:\r\n");
    print(" Total: ");
    print_dec(PAGE_COUNT);
    print(" pages\r\n");

    print(" Used:  ");
    print_dec(used);
    print(" pages\r\n");

    print(" Free:  ");
    print_dec(PAGE_COUNT - used);
    print(" pages\r\n");
}