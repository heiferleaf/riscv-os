#include <stdint.h>
#include "printf.h"
#include "defs.h"

extern void uart_puts(const char *s);
extern void main();
void timerinit();

void start() {

  // 设置 M 前一特权模式为管理模式（Supervisor），用于 mret 指令返回后进入管理模式。
  unsigned long x = r_mstatus();  // 读取mstatus寄存器的值
  x &= ~MSTATUS_MPP_MASK;         // 清除 MPP 位，原来是机器模式
  x |= MSTATUS_MPP_S;             // 设置 MPP 位为 S（Supervisor）
  w_mstatus(x);                   // 写回 mstatus 寄存器

  // 设置 M 异常程序计数器为 main 的地址，mret 返回后会跳转到 main。硬件保护的一部分
  // 需要使用 gcc -mcmodel=medany 以支持地址的获取。
  w_mepc((uint64)main);

  // 暂时关闭分页机制。
  w_satp(0);

  // 将所有中断和异常委托给管理模式（Supervisor mode）。
  w_medeleg(0xffff);  // 委托所有异常（如系统调用等）给 S 模式处理
  w_mideleg(0xffff);  // 委托所有中断给 S 模式处理

  // 打开管理模式下的外部中断和定时器中断。
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // 配置物理内存保护（PMP），允许管理模式访问所有物理内存。
  w_pmpaddr0(0x3fffffffffffffull); // 设置 PMP 地址范围
  w_pmpcfg0(0xf);                  // 设置 PMP 配置为全部可访问

  // 请求定时器中断。
  timerinit();

  // 将当前 CPU 的 hartid 保存在 tp 寄存器，方便后续 cpuid() 获取。
  int id = r_mhartid();
  w_tp(id);

  // 切换到管理模式并跳转到 main()，通过 mret 指令实现。
  asm volatile("mret");
}

void timerinit()
{
    // 通过使能 M 模式下的定时器中断，和start中的委托设置，最终使得定时器中断在 S 模式下被处理。
  w_mie(r_mie() | MIE_STIE);
  
  // enable the sstc extension (i.e. stimecmp).
  // 启用定时器拓展
  w_menvcfg(r_menvcfg() | (1L << 63)); 
  
  // allow supervisor to use stimecmp and time.
  // 允许管理模式访问时间寄存器和定时器比较寄存器
  w_mcounteren(r_mcounteren() | 2);
}