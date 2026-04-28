#ifndef DISK_H
#define DISK_H

#define SECTOR_SIZE 512
#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096
#define DATA_BLOCK_SIZE 512

typedef struct {
    char name[MAX_FILENAME];
    unsigned int size;
    unsigned int start_block;
    unsigned int block_count;
    unsigned int is_used;
} file_entry_t;

typedef struct {
    unsigned int magic;
    unsigned int total_blocks;
    unsigned int free_blocks;
    file_entry_t files[MAX_FILES];
    unsigned char data_blocks[MAX_FILES][DATA_BLOCK_SIZE];
} filesystem_t;

void disk_init(void);
int disk_read(unsigned int sector, unsigned char* buffer);
int disk_write(unsigned int sector, const unsigned char* buffer);
int fs_format(void);
int fs_mount(void);
int fs_create(const char* filename);
int fs_delete(const char* filename);
int fs_read(const char* filename, unsigned int offset, unsigned char* buffer, unsigned int size);
int fs_write(const char* filename, unsigned int offset, const unsigned char* buffer, unsigned int size);
int fs_list(void);
int fs_info(void);

int strlen(const char* str);

#endif