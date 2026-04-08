cmake_minimum_required(VERSION 3.16)
project(LightOS ASM_NASM)  # 直接启用汇编

#关键：设置 NASM 输出为纯二进制 bin 格式（引导程序必备）
set(CMAKE_ASM_NASM_FLAGS "-f bin")

# 编译 boot.asm 生成 boot.bin
add_custom_target(boot
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/boot.asm -o boot.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS boot.bin
)

add_custom_target(kernel
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/kernel.asm -o kernel.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS kernel.bin
)



# 运行 QEMU（你要的命令）
add_custom_target(run
    # 第一步：合并镜像
    COMMAND cat boot.bin kernel.bin > floppy.img
    # 第二步：启动 QEMU（你要的那条完整命令）
    COMMAND /usr/libexec/qemu-kvm -drive format=raw,file=floppy.img -serial stdio -display none
    DEPENDS boot kernel  # 先编译 boot 和 kernel
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)