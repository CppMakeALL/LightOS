#ifndef MEMORY_H
#define MEMORY_H

void memory_init(void);
void* malloc(unsigned int size);
void free(void* ptr);
void mem_info(void);

#endif