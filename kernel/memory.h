#ifndef MEMORY_H
#define MEMORY_H

// ==================== 物理内存管理接口 ====================
void memory_init(void);
void* malloc(unsigned int size);
void free(void* ptr);
void mem_info(void);

// ==================== 分页内存管理接口 ====================

// 页大小 4KB
#define PAGE_SIZE   4096

// 页目录和页表相关常量
#define PAGE_DIR_SIZE     1024  // 页目录包含1024个页目录项
#define PAGE_TABLE_SIZE   1024  // 页表包含1024个页表项

// 页目录项/页表项标志位
#define PAGE_PRESENT      0x001  // 存在位：页在内存中
#define PAGE_WRITABLE     0x002  // 可写位：允许写入
#define PAGE_USER         0x004  // 用户位：用户态可访问
#define PAGE_WRITETHROUGH 0x008  // 写通位
#define PAGE_CACHE_DISABLE 0x010 // 禁用缓存
#define PAGE_ACCESSED     0x020  // 访问位：CPU访问过该页
#define PAGE_DIRTY        0x040  // 脏位：页被写过
#define PAGE_GLOBAL       0x100  // 全局位：TLB中不刷新

// 地址对齐宏
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))

// 从虚拟地址中提取页目录索引、页表索引和页内偏移
#define PD_INDEX(vaddr) (((unsigned int)(vaddr) >> 22) & 0x3FF)  // 高10位
#define PT_INDEX(vaddr) (((unsigned int)(vaddr) >> 12) & 0x3FF)  // 中10位
#define PAGE_OFFSET(vaddr) ((unsigned int)(vaddr) & 0xFFF)        // 低12位

// 内核虚拟地址映射基址（3GB以上）
#define KERNEL_VIRT_BASE  0xC0000000

// 页表初始化
void paging_init(void);

// 获取当前页目录地址
unsigned int get_current_page_dir(void);

// 创建新页目录（用于新进程）
unsigned int create_page_dir(void);

// 销毁页目录
void destroy_page_dir(unsigned int page_dir_phys);

// 映射虚拟地址到物理地址
// page_dir_phys: 页目录物理地址
// vaddr: 虚拟地址
// paddr: 物理地址
// flags: 页属性标志位
int map_page(unsigned int page_dir_phys, unsigned int vaddr, unsigned int paddr, unsigned int flags);

// 取消虚拟地址映射
void unmap_page(unsigned int page_dir_phys, unsigned int vaddr);

// 获取虚拟地址对应的物理地址
unsigned int get_physical_addr(unsigned int page_dir_phys, unsigned int vaddr);

// 分配一个物理页并映射到指定虚拟地址
void* alloc_virtual_page(unsigned int page_dir_phys, unsigned int vaddr, unsigned int flags);

// 释放虚拟地址对应的物理页
void free_virtual_page(unsigned int page_dir_phys, unsigned int vaddr);

// 切换页目录（修改CR3寄存器）
void switch_page_dir(unsigned int page_dir_phys);

// 刷新TLB（页表缓存）
void flush_tlb(void);

// 刷新指定虚拟地址的TLB条目
void flush_tlb_entry(unsigned int vaddr);

// 缺页异常处理函数
void page_fault_handler(unsigned int error_code, unsigned int cr2);

#endif
