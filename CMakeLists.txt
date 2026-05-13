cmake_minimum_required(VERSION 3.16)
project(LightOS ASM_NASM C)

# --------------------------
# 1. 编译 boot.asm → boot.bin
# --------------------------
add_custom_target(boot
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/boot.asm -o boot.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS boot.bin
)

# --------------------------
# 2. 编译 kernel_entry.asm → entry.o
# --------------------------
add_custom_target(kernel_entry
    COMMAND nasm -f elf32 ${CMAKE_SOURCE_DIR}/kernel_entry.asm -o entry.o
    COMMAND nasm -f elf32 ${CMAKE_SOURCE_DIR}/kernel/interrupt.asm -o interrupt.o
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS entry.o interrupt.o
)

set(src_files
    kernel/kernel.c
    kernel/memory.c
    kernel/disk.c
    kernel/interrupt.c
    kernel/process.c
    kernel/privilege.c
    kernel/syscall.c
)
add_compile_options(-O2 -m32 -ffreestanding -fno-pie) 
#-m32 生成32位代码 -ffreestanding 独立环境模式,裸机内核无任何依赖 -fno-pie 不生成位置无关代码,内核代码必须运行在固定位置
add_library(kernel_o OBJECT ${src_files})

# --------------------------
# 3. 编译 kernel.c → kernel.o
# --------------------------
add_custom_target(kernel_c
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/kernel.c -o kernel.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/memory.c -o memory.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/disk.c -o disk.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/interrupt.c -o interrupt_c.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/process.c -o process.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/privilege.c -o privilege.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/syscall.c -o syscall.o -fno-pie
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS kernel.o memory.o disk.o interrupt_c.o process.o privilege.o syscall.o
)

# --------------------------
# 4. 链接 → kernel.bin
# --------------------------
add_custom_target(kernel_bin
    COMMAND ld -m elf_i386 -N -Ttext 0x1000 --entry _start entry.o kernel.o memory.o disk.o interrupt.o interrupt_c.o process.o privilege.o syscall.o -o kernel.bin --oformat binary
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS kernel_entry kernel_c
    BYPRODUCTS kernel.bin
)

# --------------------------
# 运行
# --------------------------
add_custom_target(run
    COMMAND rm -f floppy.img
    COMMAND cat boot.bin kernel.bin > floppy.img
    COMMAND /usr/libexec/qemu-kvm -drive format=raw,file=floppy.img -serial stdio -display none
    DEPENDS boot kernel_bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)