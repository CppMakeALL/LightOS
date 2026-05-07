#include <stddef.h>
#include "process.h"
#include "memory.h"

// 外部函数声明
void print(const char* str);
void print_dec(unsigned int num);
void print_hex(unsigned int addr);

// 全局变量定义
task_struct_t *current = NULL;           // 当前运行进程
task_struct_t *ready_queue = NULL;       // 就绪队列头
static unsigned int next_pid = 1;         // 下一个可用PID

// 定义TSS（任务状态段）
tss_t tss;

// 声明汇编函数
extern void switch_to_task(task_struct_t *prev, task_struct_t *next);

// ==================== 辅助函数 ====================

// 初始化PCB
static void init_task(task_struct_t *task) {
    task->pid = 0;
    task->state = TASK_READY;
    task->priority = 1;
    task->time_slice = TIME_SLICE;
    task->esp = 0;
    task->ebp = 0;
    task->eip = 0;
    task->cr3 = 0;
    task->stack = NULL;
    task->next = NULL;
    task->prev = NULL;
}

// 添加进程到就绪队列
static void add_to_ready_queue(task_struct_t *task) {
    if (!ready_queue) {
        // 队列为空
        ready_queue = task;
        task->next = task;
        task->prev = task;
    } else {
        // 插入到队列头部
        task->next = ready_queue;
        task->prev = ready_queue->prev;
        ready_queue->prev->next = task;
        ready_queue->prev = task;
    }
}

// 从就绪队列移除进程
static void remove_from_ready_queue(task_struct_t *task) {
    if (task->next == task) {
        // 队列中只有一个进程
        ready_queue = NULL;
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
        if (ready_queue == task) {
            ready_queue = task->next;
        }
    }
}

// ==================== TSS相关 ====================

// 初始化TSS
static void tss_init(void) {
    // 设置TSS段描述符
    unsigned int tss_addr = (unsigned int)&tss;
    
    // 填充GDT中TSS描述符（需要在boot.asm中添加）
    // 这里简化处理，直接设置必要字段
    tss.ss0 = 0x10;  // 内核数据段选择子
    tss.iomap = 0xFFFF;
}

// 设置内核栈（用于中断返回）
void set_kernel_stack(unsigned int esp) {
    tss.esp0 = esp;
}

// ==================== 进程初始化 ====================

// 创建空闲进程（idle进程）
static task_struct_t* create_idle_process(void) {
    task_struct_t *task = (task_struct_t*)malloc(sizeof(task_struct_t));
    if (!task) return NULL;
    
    init_task(task);
    task->pid = 0;
    task->state = TASK_RUNNING;
    task->priority = 0;
    task->time_slice = TIME_SLICE;
    
    // 分配进程栈
    task->stack = (unsigned int*)malloc(STACK_SIZE);
    if (!task->stack) {
        free(task);
        return NULL;
    }
    
    // 设置栈顶
    task->esp = (unsigned int)(task->stack + STACK_SIZE / sizeof(unsigned int));
    task->ebp = task->esp;
    
    print("Created idle process (PID: 0)\r\n");
    return task;
}

// 进程系统初始化
void process_init(void) {
    print("Initializing process management...\r\n");
    
    // 初始化TSS
    tss_init();
    
    // 创建idle进程
    current = create_idle_process();
    if (!current) {
        print("Failed to create idle process!\r\n");
        return;
    }
    
    // 将idle进程加入就绪队列
    add_to_ready_queue(current);
    
    // 设置当前进程的内核栈
    set_kernel_stack(current->esp);
    
    print("Process management initialized\r\n");
}

// ==================== 进程创建（fork） ====================

int fork(void) {
    task_struct_t *child = (task_struct_t*)malloc(sizeof(task_struct_t));
    if (!child) {
        print("fork: failed to allocate PCB\r\n");
        return -1;
    }
    
    init_task(child);
    
    // 分配新的PID
    child->pid = next_pid++;
    
    // 复制父进程属性
    child->priority = current->priority;
    child->time_slice = TIME_SLICE;
    
    // 分配进程栈
    child->stack = (unsigned int*)malloc(STACK_SIZE);
    if (!child->stack) {
        free(child);
        print("fork: failed to allocate stack\r\n");
        return -1;
    }
    
    // 复制父进程栈内容
    unsigned int *parent_stack = (unsigned int*)current->esp;
    unsigned int *child_stack = (unsigned int*)((unsigned int)child->stack + STACK_SIZE);
    unsigned int stack_size = (unsigned int)(current->stack + STACK_SIZE) - current->esp;
    
    // 从高地址向低地址复制
    for (unsigned int i = 0; i < stack_size / sizeof(unsigned int); i++) {
        child_stack[-i] = parent_stack[-i];
    }
    
    // 计算子进程栈指针
    unsigned int stack_offset = current->esp - (unsigned int)current->stack;
    child->esp = (unsigned int)child->stack + stack_offset;
    child->ebp = child->esp;
    
    // 设置子进程返回值为0（fork在子进程中返回0）
    // 需要修改栈中的返回地址对应的返回值
    // 这里简化处理，直接设置
    unsigned int *ret_addr = (unsigned int*)child->esp;
    *ret_addr = 0;  // 子进程中fork返回0
    
    // 设置子进程状态为就绪
    child->state = TASK_READY;
    
    // 添加到就绪队列
    add_to_ready_queue(child);
    
    print("fork: created process ");
    print_dec(child->pid);
    print("\r\n");
    
    return child->pid;
}

// ==================== 上下文切换 ====================

void switch_to(int next_pid) {
    task_struct_t *next = ready_queue;
    
    // 在就绪队列中查找目标进程
    if (!next) return;
    
    do {
        if (next->pid == next_pid && next->state == TASK_READY) {
            break;
        }
        next = next->next;
    } while (next != ready_queue);
    
    if (next->pid != next_pid || next->state != TASK_READY) {
        print("switch_to: process not found or not ready\r\n");
        return;
    }
    
    // 执行上下文切换
    task_struct_t *prev = current;
    
    // 更新状态
    prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    
    // 保存当前进程上下文
    __asm__ __volatile__ (
        "movl %%esp, %0\n"
        "movl %%ebp, %1\n"
        : "=m"(prev->esp), "=m"(prev->ebp)
    );
    
    // 切换到下一个进程
    current = next;
    set_kernel_stack(next->esp);
    
    // 恢复下一个进程上下文
    __asm__ __volatile__ (
        "movl %0, %%esp\n"
        "movl %1, %%ebp\n"
        : : "m"(next->esp), "m"(next->ebp)
    );
}

// ==================== 调度器 ====================

void scheduler(void) {
    if (!ready_queue || !current) {
        return;
    }
    
    // 如果当前进程是唯一进程，不切换
    if (ready_queue->next == ready_queue) {
        return;
    }
    
    // 找到下一个就绪进程
    task_struct_t *next = current->next;
    
    // 跳过非就绪状态的进程
    while (next != current && next->state != TASK_READY) {
        next = next->next;
    }
    
    // 如果只有当前进程就绪，不切换
    if (next == current) {
        return;
    }
    
    // 执行上下文切换
    task_struct_t *prev = current;
    
    // 更新状态
    prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    
    // 保存当前进程上下文
    __asm__ __volatile__ (
        "pusha\n"
        "movl %%esp, %0\n"
        "movl %%ebp, %1\n"
        : "=m"(prev->esp), "=m"(prev->ebp)
    );
    
    // 切换到下一个进程
    current = next;
    set_kernel_stack(next->esp);
    
    // 恢复下一个进程上下文
    __asm__ __volatile__ (
        "movl %0, %%esp\n"
        "movl %1, %%ebp\n"
        "popa\n"
        : : "m"(next->esp), "m"(next->ebp)
    );
}

// 触发调度（时钟中断调用）
void schedule(void) {
    if (!current || !ready_queue) {
        return;
    }
    
    // 减少时间片
    current->time_slice--;
    
    // 如果时间片用完，触发调度
    if (current->time_slice <= 0) {
        // 重置时间片
        current->time_slice = TIME_SLICE;
        
        // 如果有其他就绪进程，进行调度
        task_struct_t *next = current->next;
        while (next != current) {
            if (next->state == TASK_READY) {
                scheduler();
                return;
            }
            next = next->next;
        }
    }
}

// ==================== 进程信息打印 ====================

void process_info(void) {
    print("\r\nProcess List:\r\n");
    print("=============\r\n");
    
    if (!ready_queue) {
        print("No processes\r\n");
        return;
    }
    
    task_struct_t *task = ready_queue;
    do {
        print(" PID: ");
        print_dec(task->pid);
        print(" State: ");
        switch (task->state) {
            case TASK_RUNNING: print("RUNNING"); break;
            case TASK_READY: print("READY"); break;
            case TASK_BLOCKED: print("BLOCKED"); break;
            case TASK_ZOMBIE: print("ZOMBIE"); break;
            default: print("UNKNOWN");
        }
        print(" Time: ");
        print_dec(task->time_slice);
        print("\r\n");
        
        task = task->next;
    } while (task != ready_queue);
}