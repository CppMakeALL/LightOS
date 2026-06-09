#include "memory.h"

// 仅声明外部函数，不定义！
void print(const char* str);
void print_dec(unsigned int num);
void print_hex(unsigned int addr);

// ==================== 物理内存管理 ====================

#define MEM_START   0x10000
#define MEM_END     0x80000
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

// ==================== 分页内存管理 ====================

// 当前使用的页目录物理地址
static unsigned int current_page_dir = 0;

// 内核页目录（静态分配，确保4KB对齐）
// __attribute__((aligned(4096))) 确保页目录按页边界对齐，这是x86硬件要求
static unsigned int kernel_page_dir[PAGE_DIR_SIZE] __attribute__((aligned(4096)));

// 内核页表（静态分配，用于映射低端内存）
static unsigned int kernel_page_table0[PAGE_TABLE_SIZE] __attribute__((aligned(4096)));

// 分配一个物理页，返回物理地址
// 通过物理页位图分配，返回0表示失败
static unsigned int alloc_physical_page(void) {
    int page = alloc_page();
    if (page < 0) {
        return 0;
    }
    // 返回物理地址
    return MEM_START + page * PAGE_SIZE;
}

// 释放一个物理页
// paddr: 物理地址
static void free_physical_page(unsigned int paddr) {
    if (paddr < MEM_START || paddr >= MEM_END) return;
    int page = (paddr - MEM_START) / PAGE_SIZE;
    free_page(page);
}

// 获取页目录项
// page_dir: 页目录物理地址
// vaddr: 虚拟地址
static unsigned int* get_pde_ptr(unsigned int page_dir_phys, unsigned int vaddr) {
    // 将页目录物理地址转换为内核可以直接访问的虚拟地址
    // 由于我们在内核态，且页目录映射了低端内存，物理地址可以直接访问
    unsigned int *page_dir = (unsigned int*)page_dir_phys;
    return &page_dir[PD_INDEX(vaddr)];
}

// 获取页表项指针
// page_table_phys: 页表物理地址
// vaddr: 虚拟地址
static unsigned int* get_pte_ptr(unsigned int page_table_phys, unsigned int vaddr) {
    unsigned int *page_table = (unsigned int*)page_table_phys;
    return &page_table[PT_INDEX(vaddr)];
}

// 刷新整个TLB（Translation Lookaside Buffer）
// 通过重新加载CR3寄存器实现
void flush_tlb(void) {
    __asm__ volatile (
        "movl %%cr3, %%eax\n"
        "movl %%eax, %%cr3\n"
        ::: "eax"
    );
}

// 刷新指定虚拟地址的TLB条目
// 使用invlpg指令，只刷新单个页的TLB缓存
void flush_tlb_entry(unsigned int vaddr) {
    __asm__ volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
}

// 切换页目录
// 修改CR3寄存器为新的页目录物理地址
void switch_page_dir(unsigned int page_dir_phys) {
    current_page_dir = page_dir_phys;
    __asm__ volatile (
        "movl %0, %%cr3\n"
        :: "r"(page_dir_phys)
    );
}

// 获取当前页目录地址
unsigned int get_current_page_dir(void) {
    return current_page_dir;
}

// 映射虚拟地址到物理地址
// page_dir_phys: 页目录物理地址
// vaddr: 虚拟地址（会自动按页对齐）
// paddr: 物理地址（会自动按页对齐）
// flags: 页属性标志位（如 PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER）
// 返回值：0成功，-1失败
int map_page(unsigned int page_dir_phys, unsigned int vaddr, unsigned int paddr, unsigned int flags) {
    // 确保地址按页对齐
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);
    paddr = ALIGN_DOWN(paddr, PAGE_SIZE);

    // 获取页目录项指针
    unsigned int *pde = get_pde_ptr(page_dir_phys, vaddr);
    unsigned int pde_val = *pde;

    // 检查页表是否存在
    // 如果页目录项的PRESENT位未设置，需要分配一个新的页表
    if (!(pde_val & PAGE_PRESENT)) {
        // 分配一个新的页表物理页
        unsigned int new_page_table = alloc_physical_page();
        if (!new_page_table) {
            print("map_page: failed to allocate page table\r\n");
            return -1;
        }

        // 清空新页表
        unsigned int *pt = (unsigned int*)new_page_table;
        for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
            pt[i] = 0;
        }

        // 设置页目录项：指向新页表，并设置标志位
        // 页目录项始终设置为内核可读写，用户态访问由页表项控制
        *pde = new_page_table | PAGE_PRESENT | PAGE_WRITABLE | flags;
        pde_val = *pde;
    }

    // 获取页表物理地址（去掉标志位）
    unsigned int page_table_phys = pde_val & 0xFFFFF000;

    // 获取页表项指针
    unsigned int *pte = get_pte_ptr(page_table_phys, vaddr);

    // 设置页表项：映射到物理地址
    *pte = paddr | PAGE_PRESENT | flags;

    // 刷新该虚拟地址的TLB条目
    flush_tlb_entry(vaddr);

    return 0;
}

// 取消虚拟地址映射
// page_dir_phys: 页目录物理地址
// vaddr: 虚拟地址
void unmap_page(unsigned int page_dir_phys, unsigned int vaddr) {
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

    unsigned int *pde = get_pde_ptr(page_dir_phys, vaddr);
    unsigned int pde_val = *pde;

    // 如果页表不存在，直接返回
    if (!(pde_val & PAGE_PRESENT)) {
        return;
    }

    unsigned int page_table_phys = pde_val & 0xFFFFF000;
    unsigned int *pte = get_pte_ptr(page_table_phys, vaddr);

    // 清除页表项
    *pte = 0;

    // 刷新TLB
    flush_tlb_entry(vaddr);
}

// 获取虚拟地址对应的物理地址
// 返回值：物理地址，如果未映射则返回0
unsigned int get_physical_addr(unsigned int page_dir_phys, unsigned int vaddr) {
    unsigned int *pde = get_pde_ptr(page_dir_phys, vaddr);
    unsigned int pde_val = *pde;

    if (!(pde_val & PAGE_PRESENT)) {
        return 0;
    }

    unsigned int page_table_phys = pde_val & 0xFFFFF000;
    unsigned int *pte = get_pte_ptr(page_table_phys, vaddr);
    unsigned int pte_val = *pte;

    if (!(pte_val & PAGE_PRESENT)) {
        return 0;
    }

    // 物理页基址 + 页内偏移
    return (pte_val & 0xFFFFF000) | PAGE_OFFSET(vaddr);
}

// 分配一个物理页并映射到指定虚拟地址
// page_dir_phys: 页目录物理地址
// vaddr: 虚拟地址
// flags: 页属性标志位
// 返回值：虚拟地址（与输入vaddr相同），失败返回0
void* alloc_virtual_page(unsigned int page_dir_phys, unsigned int vaddr, unsigned int flags) {
    // 分配物理页
    unsigned int paddr = alloc_physical_page();
    if (!paddr) {
        print("alloc_virtual_page: no free physical page\r\n");
        return 0;
    }

    // 建立映射
    if (map_page(page_dir_phys, vaddr, paddr, flags) != 0) {
        // 映射失败，释放物理页
        free_physical_page(paddr);
        return 0;
    }

    return (void*)vaddr;
}

// 释放虚拟地址对应的物理页
// 先取消映射，再释放物理页
void free_virtual_page(unsigned int page_dir_phys, unsigned int vaddr) {
    // 获取物理地址
    unsigned int paddr = get_physical_addr(page_dir_phys, vaddr);
    if (paddr) {
        free_physical_page(paddr);
    }
    // 取消映射
    unmap_page(page_dir_phys, vaddr);
}

// 创建新页目录（用于新进程）
// 分配一个新的页目录页，并复制内核空间的映射
// 返回值：新页目录的物理地址，失败返回0
unsigned int create_page_dir(void) {
    // 分配页目录物理页
    unsigned int new_page_dir = alloc_physical_page();
    if (!new_page_dir) {
        print("create_page_dir: failed to allocate page directory\r\n");
        return 0;
    }

    // 清空页目录
    unsigned int *pd = (unsigned int*)new_page_dir;
    for (int i = 0; i < PAGE_DIR_SIZE; i++) {
        pd[i] = 0;
    }

    // 复制内核空间映射（页目录的高半部分，即索引512~1023）
    // 这样所有进程共享内核空间的页表
    unsigned int *kernel_pd = (unsigned int*)current_page_dir;
    for (int i = PAGE_DIR_SIZE / 2; i < PAGE_DIR_SIZE; i++) {
        pd[i] = kernel_pd[i];
    }

    // 同时复制内核的恒等映射（页目录第0项，映射0~4MB）
    // 这是必需的，因为用户进程需要能够执行内核代码（通过iret进入用户态）
    // 以及访问内核中的函数和数据（如系统调用处理代码）
    // 注意：这里只复制页目录项，共享同一个页表，所以不额外占用内存
    pd[0] = kernel_pd[0];

    return new_page_dir;
}

// 销毁页目录
// 释放页目录本身以及用户空间的所有页表和物理页
void destroy_page_dir(unsigned int page_dir_phys) {
    if (!page_dir_phys) return;

    unsigned int *pd = (unsigned int*)page_dir_phys;

    // 遍历用户空间页目录项（0 ~ 511）
    for (int i = 0; i < PAGE_DIR_SIZE / 2; i++) {
        unsigned int pde = pd[i];
        if (pde & PAGE_PRESENT) {
            unsigned int page_table_phys = pde & 0xFFFFF000;
            unsigned int *pt = (unsigned int*)page_table_phys;

            // 释放该页表中所有已映射的物理页
            for (int j = 0; j < PAGE_TABLE_SIZE; j++) {
                unsigned int pte = pt[j];
                if (pte & PAGE_PRESENT) {
                    unsigned int paddr = pte & 0xFFFFF000;
                    free_physical_page(paddr);
                }
            }

            // 释放页表本身
            free_physical_page(page_table_phys);
        }
    }

    // 释放页目录本身
    free_physical_page(page_dir_phys);
}

// 分页系统初始化
// 建立恒等映射（identity mapping），将物理地址0~4MB直接映射到虚拟地址0~4MB
// 这样开启分页后，内核代码仍然可以正常执行
void paging_init(void) {
    print("Initializing paging...\r\n");

    // 清空页目录
    for (int i = 0; i < PAGE_DIR_SIZE; i++) {
        kernel_page_dir[i] = 0;
    }

    // 清空页表0
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        kernel_page_table0[i] = 0;
    }

    // 建立恒等映射：虚拟地址0~4MB -> 物理地址0~4MB
    // 这是必须的，因为开启分页后CPU会用虚拟地址查页表
    // 如果内核代码运行在0x1000处，必须保证0x1000虚拟地址能正确映射
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        unsigned int paddr = i * PAGE_SIZE;
        // 页表项：存在、可写、用户态可访问、全局页
        // PAGE_USER 是必需的，因为用户进程也需要访问这些页面
        // （用户进程通过iret进入，需要执行内核代码中的user_process_entry函数）
        kernel_page_table0[i] = paddr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_GLOBAL;
    }

    // 设置页目录的第0项指向页表0
    // 这样虚拟地址0x00000000~0x003FFFFF映射到相同的物理地址
    kernel_page_dir[0] = (unsigned int)kernel_page_table0 | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_GLOBAL;

    // 设置当前页目录
    current_page_dir = (unsigned int)kernel_page_dir;

    // 加载页目录到CR3寄存器
    switch_page_dir(current_page_dir);

    // 启用分页：设置CR0寄存器的PG位（第31位）和PE位（第0位）
    __asm__ volatile (
        "movl %%cr0, %%eax\n"
        "orl $0x80000000, %%eax\n"  // 设置PG位（分页使能）
        "movl %%eax, %%cr0\n"
        ::: "eax"
    );

    print("Paging enabled (identity mapping 0~4MB)\r\n");
}

// ==================== 缺页异常处理 ====================

// 缺页异常错误码解析
// bit 0 (P): 0=页不存在，1=页级保护违规
// bit 1 (W/R): 0=读操作，1=写操作
// bit 2 (U/S): 0=内核态，1=用户态
// bit 3 (RSV): 0=不是保留位违规，1=保留位违规
// bit 4 (I/D): 0=不是取指令，1=取指令

// 缺页异常处理函数
// error_code: CPU压入的错误码
// cr2: 触发缺页的虚拟地址（CPU自动存入CR2寄存器）
//
// 重要说明：用户态缺页处理
// 当用户态程序访问未映射的虚拟地址时，会触发缺页异常。
// 此时CPU自动切换到内核态，并调用此处理函数。
// 处理用户态缺页有两种方式：
//   1. 直接在内核态分配页面（当前实现方式，类似Linux的do_page_fault）
//   2. 通过系统调用让用户态程序主动请求分配页面
//
// 这里采用方式1：在内核态直接为用户分配页面。
// 这是因为缺页异常已经是内核态入口，不需要再通过int 0x80系统调用。
// 系统调用方式（sys_alloc_page）适用于用户态程序主动预分配内存的场景。
void page_fault_handler(unsigned int error_code, unsigned int cr2) {
    // 将缺页地址按页对齐
    unsigned int vaddr = ALIGN_DOWN(cr2, PAGE_SIZE);

    // 检查是否是用户态缺页（错误码bit 2 = 1表示用户态）
    if (error_code & 0x04) {
        // ========== 用户态缺页处理 ==========
        // 获取当前进程的页目录
        unsigned int page_dir = get_current_page_dir();

        // 检查该虚拟地址是否已经映射（可能是TLB不一致导致的）
        unsigned int existing_paddr = get_physical_addr(page_dir, vaddr);
        if (existing_paddr) {
            // 地址已映射，刷新TLB即可
            flush_tlb_entry(vaddr);
            return;
        }

        // 为用户态程序分配新页面
        // 标志位：存在、可写、用户态可访问
        void* result = alloc_virtual_page(page_dir, vaddr,
                                          PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        if (result) {
            // 页面分配成功，直接返回
            // 缺页异常返回后，用户态程序会重新执行触发缺页的指令
            return;
        }

        // 页面分配失败，打印错误信息
        print("\r\n=== User Page Fault ===\r\n");
        print("Faulting address: ");
        print_hex(cr2);
        print("\r\n");
        print("Error: Failed to allocate page for user process\r\n");
        print("=====================\r\n");

        // 挂起CPU（实际系统中应该终止进程）
        while (1) {
            __asm__ volatile ("hlt");
        }
    }

    // ========== 内核态缺页处理 ==========
    // 内核态缺页通常是内核bug，不应该发生
    // 打印详细的错误信息并挂起系统
    print("\r\n=== Kernel Page Fault ===\r\n");
    print("Faulting address: ");
    print_hex(cr2);
    print("\r\n");

    // 解析错误码
    print("Error code: ");
    print_hex(error_code);
    print(" (");

    if (error_code & 0x01) {
        print("protection violation");
    } else {
        print("page not present");
    }

    if (error_code & 0x02) {
        print(", write");
    } else {
        print(", read");
    }

    print(", kernel mode");

    print(")\r\n");
    print("Kernel halting due to page fault\r\n");
    print("=========================\r\n");

    // 挂起CPU
    while (1) {
        __asm__ volatile ("hlt");
    }
}
