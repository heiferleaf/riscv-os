CROSS_COMPILE = riscv64-unknown-elf-
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld

CFLAGS = -Wall -O2 -nostdlib -ffreestanding \
         -march=rv64imac -mabi=lp64 -mcmodel=medany

LDFLAGS = -T kernel/kernel.ld

# 目标文件
OBJS = kernel/entry.o kernel/start.o kernel/uart.o

# 最终内核
all: kernel/kernel.elf

# 编译汇编
kernel/entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c $< -o $@

# 编译C文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 链接
kernel/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# 清理
clean:
	rm -f kernel/*.o kernel/*.elf

# 运行QEMU
run: kernel/kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel/kernel.elf