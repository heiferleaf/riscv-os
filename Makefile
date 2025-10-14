# RISC-V OS Makefile (updated for your kernel structure)

CROSS_COMPILE = riscv64-unknown-elf-
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld

CFLAGS = -Wall -O2 -nostdlib -ffreestanding -g \
         -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -mno-relax \
         -fno-builtin -fno-common \
         -I./kernel

LDFLAGS = -T kernel/kernel.ld -melf64lriscv

# 汇编文件列表（.S文件）
ASM_OBJS = kernel/entry.o kernel/proc_switch.o kernel/trampoline.o kernel/kernelvec.o

# C文件列表（按你的目录结构补全）
C_OBJS = kernel/start.o kernel/uart.o kernel/printf.o kernel/console.o \
         kernel/vm.o kernel/kalloc.o kernel/string.o kernel/test.o \
         kernel/main.o kernel/plic.o kernel/spinlock.o kernel/proc.o \
         kernel/trap.o

OBJS = $(ASM_OBJS) $(C_OBJS)

all: kernel/kernel.elf

# 汇编文件编译规则
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@

# C文件编译规则
kernel/%.o: kernel/%.c kernel/defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# 链接规则
kernel/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -f kernel/*.o kernel/*.elf

run: kernel/kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel/kernel.elf

nm: 
	$(CROSS_COMPILE)nm kernel/kernel.elf

objdump:
	$(CROSS_COMPILE)objdump -h kernel/kernel.elf