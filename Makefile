CROSS_COMPILE = riscv64-unknown-elf-
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld

CFLAGS = -Wall -O2 -nostdlib -ffreestanding \
         -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -mno-relax \
         -fno-builtin -fno-common \
         -I./kernel                    # 让C文件能找到kernel/defs.h等头文件

LDFLAGS = -T kernel/kernel.ld -melf64lriscv

OBJS = kernel/entry.o kernel/start.o kernel/uart.o kernel/printf.o \
       kernel/console.o kernel/vm.o kernel/kalloc.o kernel/string.o kernel/test.o

all: kernel/kernel.elf

# 汇编文件编译
kernel/entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c $< -o $@

# C文件编译
kernel/%.o: kernel/%.c kernel/defs.h
	$(CC) $(CFLAGS) -c $< -o $@

# 链接
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