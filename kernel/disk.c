#include "disk.h"

void print(const char* str);
void print_dec(unsigned int num);
int strlen(const char* str);

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "d"(port));
}

static filesystem_t* fs = (filesystem_t*)0x20000; //0x20000 及往后大片纯净空闲物理内存;0x00000 ~ 0x1FFFF栈、内核代码、全局变量、BIOS 占用、显存、IVT 中断向量区,不能乱用
static int fs_mounted = 0;

void disk_init() {
    fs_mounted = 0;
}

int disk_read(unsigned int sector, unsigned char* buffer) {
    unsigned int lba = sector;
    
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, lba & 0xFF);
    outb(0x1F4, (lba >> 8) & 0xFF);
    outb(0x1F5, (lba >> 16) & 0xFF);
    outb(0x1F7, 0x20);

    while ((inb(0x1F7) & 0x08) == 0);

    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        unsigned short data = *((unsigned short*)(0x1F0));
        ((unsigned short*)buffer)[i] = data;
    }

    return 0;
}

int disk_write(unsigned int sector, const unsigned char* buffer) {
    unsigned int lba = sector;
    
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, lba & 0xFF);
    outb(0x1F4, (lba >> 8) & 0xFF);
    outb(0x1F5, (lba >> 16) & 0xFF);
    outb(0x1F7, 0x30);

    while ((inb(0x1F7) & 0x08) == 0);

    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        *((unsigned short*)(0x1F0)) = ((unsigned short*)buffer)[i];
    }

    while ((inb(0x1F7) & 0x40) == 0);

    return 0;
}

int fs_format() {
    fs->magic = 0x4C49464F;
    fs->total_blocks = MAX_FILES;
    fs->free_blocks = MAX_FILES;

    for (int i = 0; i < MAX_FILES; i++) {
        fs->files[i].is_used = 0;
        fs->files[i].size = 0;
        fs->files[i].start_block = 0;
        fs->files[i].block_count = 0;
        for (int j = 0; j < MAX_FILENAME; j++) {
            fs->files[i].name[j] = 0;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        for (int j = 0; j < DATA_BLOCK_SIZE; j++) {
            fs->data_blocks[i][j] = 0;
        }
    }

    fs_mounted = 1;
    return 0;
}

int fs_mount() {
    disk_read(100, (unsigned char*)fs);

    if (fs->magic != 0x4C49464F) {
        return -1;
    }

    fs_mounted = 1;
    return 0;
}

int fs_sync() {
    if (!fs_mounted) return -1;
    disk_write(100, (unsigned char*)fs);
    return 0;
}

int fs_create(const char* filename) {
    if (!fs_mounted) return -1;
    if (fs->free_blocks == 0) return -2;

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 0) {
            fs->files[i].is_used = 1;
            fs->files[i].size = 0;
            fs->files[i].start_block = i;
            fs->files[i].block_count = 1;
            
            for (int j = 0; j < MAX_FILENAME && filename[j] != 0; j++) {
                fs->files[i].name[j] = filename[j];
            }
            fs->files[i].name[MAX_FILENAME - 1] = 0;

            fs->free_blocks--;
            fs_sync();
            return 0;
        }
    }

    return -3;
}

int fs_delete(const char* filename) {
    if (!fs_mounted) return -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 1) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (fs->files[i].name[j] != filename[j]) {
                    match = 0;
                    break;
                }
                if (fs->files[i].name[j] == 0 && filename[j] == 0) break;
            }

            if (match) {
                fs->files[i].is_used = 0;
                fs->files[i].size = 0;
                fs->files[i].start_block = 0;
                fs->files[i].block_count = 0;
                for (int j = 0; j < MAX_FILENAME; j++) {
                    fs->files[i].name[j] = 0;
                }

                for (int j = 0; j < DATA_BLOCK_SIZE; j++) {
                    fs->data_blocks[i][j] = 0;
                }

                fs->free_blocks++;
                fs_sync();
                return 0;
            }
        }
    }

    return -2;
}

int fs_read(const char* filename, unsigned int offset, unsigned char* buffer, unsigned int size) {
    if (!fs_mounted) return -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 1) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (fs->files[i].name[j] != filename[j]) {
                    match = 0;
                    break;
                }
                if (fs->files[i].name[j] == 0 && filename[j] == 0) break;
            }

            if (match) {
                if (offset >= fs->files[i].size) return -2;
                if (offset + size > fs->files[i].size) {
                    size = fs->files[i].size - offset;
                }

                for (unsigned int j = 0; j < size; j++) {
                    buffer[j] = fs->data_blocks[i][offset + j];
                }
                return size;
            }
        }
    }

    return -3;
}

int fs_write(const char* filename, unsigned int offset, const unsigned char* buffer, unsigned int size) {
    if (!fs_mounted) return -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 1) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (fs->files[i].name[j] != filename[j]) {
                    match = 0;
                    break;
                }
                if (fs->files[i].name[j] == 0 && filename[j] == 0) break;
            }

            if (match) {
                if (offset + size > MAX_FILE_SIZE) return -2;

                for (unsigned int j = 0; j < size; j++) {
                    fs->data_blocks[i][offset + j] = buffer[j];
                }

                if (offset + size > fs->files[i].size) {
                    fs->files[i].size = offset + size;
                }

                fs_sync();
                return size;
            }
        }
    }

    return -3;
}

int fs_list() {
    if (!fs_mounted) return -1;

    print("Files:\r\n");
    print("-------\r\n");

    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 1) {
            print(" ");
            print(fs->files[i].name);
            print("  (");
            print_dec(fs->files[i].size);
            print(" bytes)\r\n");
            count++;
        }
    }

    if (count == 0) {
        print(" (empty)\r\n");
    }

    return 0;
}

int fs_info() {
    if (!fs_mounted) return -1;

    print("Filesystem Info:\r\n");
    print("---------------\r\n");
    print(" Total blocks: ");
    print_dec(fs->total_blocks);
    print("\r\n");

    print(" Free blocks:  ");
    print_dec(fs->free_blocks);
    print("\r\n");

    print(" Used blocks:  ");
    print_dec(fs->total_blocks - fs->free_blocks);
    print("\r\n");

    int file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].is_used == 1) {
            file_count++;
        }
    }

    print(" Total files:  ");
    print_dec(file_count);
    print("\r\n");

    return 0;
}