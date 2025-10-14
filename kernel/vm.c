#include "types.h"
#include "defs.h"
#include "riscv.h"
#include "printf.h"
#include "memlayout.h"
#include <stdint.h>

pagetable_t kernel_pagetable;

extern char _stext[], _etext[];
extern char _srodata[], _erodata[];
extern char _sdata[], _edata[];
extern char _sbss[], _ebss[];
extern char stack_bottom[], stack_top[];
extern char _end[];

pagetable_t
kvmmake(void)
{
  pagetable_t kpgtbl = (pagetable_t)kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // 1. 设备区 UART
  map_region(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  map_region(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // 映射 CLINT 区域（定时器、软件中断）
  map_region(kpgtbl, 0x02000000, 0x02000000, 0x10000, PTE_R | PTE_W);
   // PLIC
  map_region(kpgtbl, PLIC, PLIC, 0x4000000, PTE_R | PTE_W);

  // 2. .text 只读可执行
  map_region(kpgtbl, (uint64)_stext, (uint64)_stext, (uint64)_etext - (uint64)_stext, PTE_R | PTE_X);

  // 3. .rodata 只读
  map_region(kpgtbl, (uint64)_srodata, (uint64)_srodata, (uint64)_erodata - (uint64)_srodata, PTE_R);

  // 4. .data 可读写
  map_region(kpgtbl, (uint64)_sdata, (uint64)_sdata, (uint64)_edata - (uint64)_sdata, PTE_R | PTE_W);

  // 5. .bss 可读写
  map_region(kpgtbl, (uint64)_sbss, (uint64)_sbss, (uint64)_ebss - (uint64)_sbss, PTE_R | PTE_W);

  // 6. 栈区 可读写
  map_region(kpgtbl, (uint64)stack_bottom, (uint64)stack_bottom, (uint64)stack_top - (uint64)stack_bottom, PTE_R | PTE_W);

  return kpgtbl;
}

// 创建内核页表
void kvminit(void) {
    kernel_pagetable = kvmmake();
}

// 激活内核页表
void kvminithart(void) {
    // 设置SATP寄存器，开启分页
    uint64_t pa = (uint64_t)kernel_pagetable;
    // MODE=Sv39, PPN=pa>>12,设置内核采用39位虚拟地址，三级页表机制，同时告诉硬件关于关于内核跟页表的物理页号
    uint64_t satp = SATP_SV39 | (pa >> 12);
    // 内联汇编，意思是“把satp的值写入SATP寄存器”
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma zero, zero");
}

pagetable_t
create_pagetable(void)
{
    pagetable_t pagetable;
    pagetable = (pagetable_t) kalloc(); // 分配一页内存给页表
    if(pagetable == 0) {
        printf("create_pagetable: kalloc failed\n");
        return 0;
    }
    memset(pagetable, 0, PGSIZE); // 初始化页表内容为0
    return pagetable;
}

// walk_create: 返回va的叶子PTE指针，必要时自动创建中间页表
// 类似xv6的walk
pte_t*
walk_create(pagetable_t pt, uint64 va)
{
    for(int level = 2; level > 0; level--) {
        pte_t *pte = &pt[PX(level, va)];
        if(*pte & PTE_V) {
            pt = (pagetable_t) PTE2PA(*pte);    // 下一级页表
        } else {
            pt = (pagetable_t) kalloc();
            if(pt == 0) {
                return 0;
            }
            memset(pt, 0, PGSIZE);
            *pte = PA2PTE(pt) | PTE_V;
        }
    }
    return &pt[PX(0, va)];
}

// walk_lookup: 返回va的叶子PTE指针，找不到返回0
pte_t*
walk_lookup(pagetable_t pt, uint64 va)
{
    for(int level = 2; level > 0; level--) {
        pte_t *pte = &pt[PX(level, va)];
        if(*pte & PTE_V) {
            pt = (pagetable_t) PTE2PA(*pte);    // 下一级页表
        } else {
            return 0;
        }
    }
    
    return &pt[PX(0, va)];
}

// map_page: 建立va->pa的单页映射
// perm: 权限标志,如PTE_R|PTE_W|PTE_X
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm) {
    if((va % PGSIZE) || (pa % PGSIZE))
        return -1; // 必须页对齐
    pte_t *pte = walk_create(pt, va);
    // 没有分配成功
    if(!pte)
        return -1; // 内存不足
    // 叶子页表项已经存在
    if(*pte & PTE_V)
        return -2; // 已映射
    *pte = PA2PTE(pa) | perm | PTE_V;
    return 0;
}

// 释放整个页表
// (包括中间页表的页表项和叶子节点的实际物理页)
void uvmfree(pagetable_t pagetable, uint64 sz)
{
    if(sz > 0)
        uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
    destroy_pagetable(pagetable);
}

// 销毁整个页表递归释放所有页表页
// （不释放映射的叶子节点中物理页）
void destroy_pagetable(pagetable_t pt) {
    // 遍历页表的512个条目（每个页表有512项）
    for(int i=0; i<512; ++i) {
        // 取出第i项页表项（PTE）
        pte_t pte = pt[i];
        // 检查该PTE是否有效（PTE_V），且不是叶子节点（没有R/W/X权限）
        if((pte & PTE_V) && !(pte & (PTE_R|PTE_W|PTE_X))) {
            // 递归销毁子页表
            pagetable_t child = (pagetable_t)PTE2PA(pte);
            destroy_pagetable(child);
            // 释放子页表占用的内存
            pt[i] = 0;
        }
    }
    // 释放当前页表占用的内存
    kfree(pt);
}

// 提供一个把进程中的所有物理页释放掉的接口
// (也就是释放页表中的叶子节点的页表项)
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    // 查找叶子页表项（即实际映射的物理页）
    if((pte = walk_lookup(pagetable, a) == 0))
      continue;   // 没有分配页表项，跳过
    if((*pte & PTE_V) == 0)
      continue;   // 没有分配物理页，跳过
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa); // 释放物理页
    }
    *pte = 0; // 清空页表项
  }
}

int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
    pte_t *pte;
    uint64 pa, i;
    uint flags;
    char *mem;
    
    for(i = 0; i < sz; i += PGSIZE){
        if((pte = walk_lookup(old, i)) == 0)
            continue;   // old中没有映射，跳过 
        if((*pte & PTE_V) == 0)
            continue;   // old中没有映射，跳过
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        if((mem = kalloc()) == 0)
            goto err;
        memmove(mem, (char*)pa, PGSIZE);
        if(map_page(new, i, pa, flags) != 0){
            kfree(mem);
            goto err;
        }
    }
    return 0;
    
     err:
    uvmunmap(new, 0, i / PGSIZE, 1);
    return -1;
}

// 调试用：递归打印页表内容
void dump_pagetable(pagetable_t pt, int level) {
    for(int i = 0; i < 512; i++) {
        pte_t pte = pt[i];
        if(pte & PTE_V) {
            uint64 pa = PTE2PA(pte);
            
            // 打印缩进
            for(int j = 0; j < level * 2; j++) {
                printf(" ");
            }
            
            // 打印基本信息，使用你支持的格式
            printf("[");
            printf("%d", i);
            printf("] PTE: 0x");
            printf("%x", (uint32)(pte >> 32));  // 高32位
            printf("%x", (uint32)(pte & 0xFFFFFFFF));  // 低32位
            printf("  PA: 0x");
            printf("%x", (uint32)(pa >> 32));   // 高32位  
            printf("%x", (uint32)(pa & 0xFFFFFFFF));   // 低32位
            printf("  ");
            
            if(pte & PTE_R) printf("R");
            if(pte & PTE_W) printf("W");
            if(pte & PTE_X) printf("X");
            if(pte & PTE_U) printf("U");
            printf("\n");
            
            if(!(pte & (PTE_R|PTE_W|PTE_X)))
                dump_pagetable((pagetable_t)pa, level + 1);
        }
    }
}
// 连续映射一段区间
int map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm) {
    if((va % PGSIZE) || (pa % PGSIZE) || (sz % PGSIZE)) // 必须页对齐
        return -1;
    uint64 end = va + sz;
    for(; va < end; va += PGSIZE, pa += PGSIZE) {
        if(map_page(pt, va, pa, perm) != 0)
            return -1; // 遇到错误提前返回
    }
    return 0;
}

// 解除一个虚拟地址在叶子页表中对物理页的映射
// (不会删除物理页)
int unmap_page(pagetable_t pt, uint64 va) {
    pte_t* pte = walk_lookup(pt, va);
    if(!pte || !(*pte & PTE_V)) return -1;
    *pte = 0;
    return 0;
}

// 虚拟地址查物理地址
uint64 walkaddr(pagetable_t pt, uint64 va) {
    pte_t* pte = walk_lookup(pt, va);
    if(!pte || !(*pte & PTE_V)) return 0;
    return PTE2PA(*pte) | (va & 0xFFF);
}