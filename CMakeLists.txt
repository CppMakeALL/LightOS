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
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS entry.o
)

# --------------------------
# 3. 编译 kernel.c → kernel.o
# --------------------------
add_custom_target(kernel_c
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/kernel.c -o kernel.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/memory.c -o memory.o -fno-pie
    COMMAND gcc -m32 -ffreestanding -c ${CMAKE_SOURCE_DIR}/kernel/disk.c -o disk.o -fno-pie
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS kernel.o memory.o disk.o
)

# --------------------------
# 4. 链接 → kernel.bin
# --------------------------
add_custom_target(kernel_bin
    COMMAND ld -m elf_i386 -N -Ttext 0x1000 --entry _start entry.o kernel.o memory.o disk.o -o kernel.bin --oformat binary
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