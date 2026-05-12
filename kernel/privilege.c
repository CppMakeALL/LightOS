#include "privilege.h"
#include "process.h"
#include <stddef.h>

extern task_struct_t *current;

void print(const char* str);

typedef struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_mid;
    unsigned char type_attr;
    unsigned char limit_high;
    unsigned char base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t extended_gdt[6];
static gdt_ptr_t extended_gdt_desc;

extern tss_t tss;

static void gdt_set_entry(int index, unsigned int base, unsigned int limit, unsigned char type_attr) {
    gdt_entry_t *entry = &extended_gdt[index];

    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    entry->limit_low = limit & 0xFFFF;
    // limit_high: 低4位是limit高4位，高4位是标志位(G=1, D=1, L=0, AVL=0) = 0xC0
    entry->limit_high = ((limit >> 16) & 0x0F) | 0xC0;

    entry->type_attr = type_attr;
}

void privilege_init(void) {
    print("Initializing privilege levels...\r\n");
    
    gdt_set_entry(0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A);
    gdt_set_entry(2, 0, 0xFFFFF, 0x92);
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA);
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2);
    gdt_set_entry(5, (unsigned int)&tss, sizeof(tss) - 1, 0x89);
    
    extended_gdt_desc.limit = sizeof(extended_gdt) - 1;
    extended_gdt_desc.base = (unsigned int)extended_gdt;
    
    __asm__ volatile ("lgdt %0" : : "m"(extended_gdt_desc));
    
    unsigned int kernel_data_sel = KERNEL_DATA_SEL;
    __asm__ volatile (
        "movl %0, %%eax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        : : "r"(kernel_data_sel)
    );
    
    __asm__ volatile ("ltr %w0" : : "r"(TSS_SEL));
    
    print("Privilege levels initialized\r\n");
}

void switch_to_user_mode(void) {
    unsigned int user_stack = (unsigned int)current->stack + STACK_SIZE;
    unsigned int user_data_sel = USER_DATA_SEL;
    unsigned int user_code_sel = USER_CODE_SEL;
    
    __asm__ __volatile__ (
        "movl %0, %%eax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        
        "pushl %0\n"
        "pushl %1\n"
        "pushfl\n"
        "popl %%eax\n"
        "orl $0x200, %%eax\n"
        "pushl %%eax\n"
        "pushl %2\n"
        "pushl $1f\n"
        "iret\n"
        
        "1:\n"
        : : "r"(user_data_sel), "r"(user_stack), "r"(user_code_sel)
        : "eax"
    );
}

void syscall_handler(void) {
    print("System call received\r\n");
}