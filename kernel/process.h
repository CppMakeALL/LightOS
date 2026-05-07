#ifndef PROCESS_H
#define PROCESS_H

// 进程状态定义
#define TASK_RUNNING  0  // 运行态
#define TASK_READY    1  // 就绪态
#define TASK_BLOCKED  2  // 阻塞态
#define TASK_ZOMBIE   3  // 僵尸态

// 最大进程数
#define NR_TASKS 64

// 进程栈大小
#define STACK_SIZE 4096

// 时间片大小（时钟中断次数）
#define TIME_SLICE 10

// 进程控制块（PCB）结构
typedef struct task_struct {
    unsigned int pid;                    // 进程ID
    unsigned int state;                  // 进程状态
    unsigned int priority;               // 优先级
    unsigned int time_slice;             // 剩余时间片
    
    unsigned int esp;                    // 栈指针
    unsigned int ebp;                    // 基址指针
    unsigned int eip;                    // 指令指针
    
    unsigned int cr3;                    // 页表基址（用于分页）
    
    unsigned int *stack;                 // 进程栈地址
    
    struct task_struct *next;            // 链表下一个节点
    struct task_struct *prev;            // 链表前一个节点
} task_struct_t;

// 任务状态段（TSS）结构
typedef struct {
    unsigned int back_link;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;
    unsigned int ldt;
    unsigned int trap;
    unsigned int iomap;
} __attribute__((packed)) tss_t;

// 全局变量声明
extern task_struct_t *current;           // 当前运行进程
extern task_struct_t *ready_queue;       // 就绪队列头

// 对外接口
void process_init(void);                 // 初始化进程系统
int fork(void);                          // 创建进程
void switch_to(int next_pid);            // 切换到指定进程
void scheduler(void);                    // 调度器
void schedule(void);                     // 触发调度
void process_info(void);                 // 打印进程信息

#endif