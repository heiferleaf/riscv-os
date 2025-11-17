# RISC-V OS Makefile (updated with user code support + FS log + virtio disk)

CROSS_COMPILE = riscv64-unknown-elf-
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -O2 -nostdlib -ffreestanding -g \
         -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -mno-relax \
         -fno-builtin -fno-common \
         -I./kernel

LDFLAGS = -T kernel/kernel.ld -melf64lriscv

# Disk image settings (used by QEMU virtio-blk)
DISK_IMG := fs.img
DISK_SIZE_MB := 64

# 汇编文件列表（.S文件）
ASM_OBJS = kernel/entry.o kernel/proc_switch.o kernel/trampoline.o kernel/kernelvec.o

# C文件列表（修复续行符、补全依赖：sleeplock、bio、fs、inode、log、virtio_disk）
C_OBJS = \
  kernel/start.o kernel/uart.o kernel/printf.o kernel/console.o \
  kernel/vm.o kernel/kalloc.o kernel/string.o kernel/test.o \
  kernel/main.o kernel/plic.o kernel/spinlock.o kernel/sleeplock.o kernel/proc.o \
  kernel/trap.o kernel/syscall.o kernel/sysproc.o \
  kernel/bio.o kernel/fs.o kernel/inode.o kernel/log.o kernel/virtio_disk.o \
  kernel/file.o kernel/pipe.o
# 用户初始代码
INITCODE_OBJ = initcode.o

OBJS = $(ASM_OBJS) $(C_OBJS) $(INITCODE_OBJ)

.PHONY: all clean run user fsimg cleanfs nm objdump

all: kernel/kernel.elf

# 创建用户目录
user:
	mkdir -p user

# 编译用户初始代码为ELF格式
user/initcode: user/initcode.S user
	$(CC) $(CFLAGS) -nostdinc -I. -Ikernel -c user/initcode.S -o user/initcode.o
	$(LD) -N -e start -Ttext 0 -o user/initcode.out user/initcode.o
	$(OBJCOPY) -S -O binary user/initcode.out user/initcode
	$(OBJDUMP) -S user/initcode.o > user/initcode.asm

# 将initcode转换为目标文件格式，以便链接到内核中
initcode.o: user/initcode user
	$(LD) -r -b binary -o initcode.o user/initcode

# 汇编文件编译规则
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@

# C文件编译规则
kernel/%.o: kernel/%.c kernel/defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# 显式编译syscall.c和sysproc.c，确保它们被包含（可选；通用规则已覆盖）
kernel/syscall.o: kernel/syscall.c kernel/defs.h
	$(CC) $(CFLAGS) -c kernel/syscall.c -o kernel/syscall.o

kernel/sysproc.o: kernel/sysproc.c kernel/defs.h
	$(CC) $(CFLAGS) -c kernel/sysproc.c -o kernel/sysproc.o

# 链接规则 - 所有目标文件一起链接
kernel/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# 生成/准备磁盘镜像（用于virtio-blk）
fsimg: $(DISK_IMG)

$(DISK_IMG):
	@echo "Creating $(DISK_IMG) ($(DISK_SIZE_MB) MB)..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=$(DISK_SIZE_MB) status=none
	@echo "Done."

clean:
	rm -f kernel/*.o kernel/*.elf user/*.o user/initcode user/initcode.out user/initcode.asm initcode.o

cleanfs:
	rm -f $(DISK_IMG)

run: kernel/kernel.elf fsimg
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel/kernel.elf \
	  -drive file=$(DISK_IMG),if=none,format=raw,id=fs \
	  -device virtio-blk-device,drive=fs,bus=virtio-mmio-bus.0 \
	  -global virtio-mmio.force-legacy=false

nm:
	$(CROSS_COMPILE)nm kernel/kernel.elf

objdump:
	$(CROSS_COMPILE)objdump -h kernel/kernel.elf