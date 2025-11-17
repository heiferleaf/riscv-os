#pragma once

#include "types.h"
#include "riscv.h"
#include "memlayout.h"

extern pagetable_t kernel_pagetable;
extern struct proc *initproc;
struct context;
struct spinlock;
struct proc;
struct sleeplock;
struct stat;
struct superblock;
struct file;
struct inode;
struct pipe;


// bio.c
void            binit(void);
struct buf*     bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);
void            itrunc(struct inode*);
void            ireclaim(int);

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(void);
void            end_op(void);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);

// main.c
void            main(void);

// kalloc.c
void            kinit(void);
void*           kalloc(void);
void            kfree(void *);

// string.c
void*           memset(void *dst, int c, unsigned long n);
void*           memmove(void*, const void*, uint);
void*           memcpy(void *dst, const void *src, unsigned long n);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);


// uart.c
void            uart_putc(char c);

// console.c
void            console_putc(char c);
void            console_puts(const char *s);
void            clear_screen(void);

// spinlock.c
void            initlock(struct spinlock *lk, char *name);
void            acquire(struct spinlock *lk);
void            release(struct spinlock *lk);
int             holding(struct spinlock *lk);
void            push_off(void);
void            pop_off(void);

// vm.c
pte_t*          walk_create(pagetable_t pt, uint64 va);
pte_t*          walk_lookup(pagetable_t pt, uint64 va);
int             map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm);
int             unmap_page(pagetable_t pt, uint64 va);
int             map_page(pagetable_t pagetable, uint64 va, uint64 pa, int perm);
void            uvmfree(pagetable_t pagetable, uint64 sz);
void            uvmunmap(pagetable_t pagetable, uint64, uint64 npages, int do_free);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
uint64          walkaddr(pagetable_t pt, uint64 va);
pagetable_t     create_pagetable(void);
void            destroy_pagetable(pagetable_t pt);
void            dump_pagetable(pagetable_t pt, int level);
void            kvminit(void);
void            kvminithart(void);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
int             copyout(pagetable_t, uint64, char *, uint64);
void            uvminit(pagetable_t, uchar *, uint);
uint64          vmfault(pagetable_t pagetable, uint64 va, int read);
int             ismapped(pagetable_t, uint64);

// proc.c
int             cpuid(void);
struct cpu*     mycpu(void);
void            sched(void);
int             killed(struct proc *);
void            yield(void);
void            scheduler(void);
void            sleep(void *, struct spinlock *);
void            wakeup(void *);
struct proc*    myproc(void);
struct proc*    allocproc(void);
void            proc_mapstacks(pagetable_t);
void            procinit(void);
void            freeproc(struct proc *p);
void            proc_freepagetable(pagetable_t, uint64);
pagetable_t     proc_pagetable(struct proc *);
int             kfork(void);
int             kwait(uint64);
void            kexit(int status);
int             kkill(int);
int             killed(struct proc*);
void            setkilled(struct proc*);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void            procdump(void);

// syscall.c
void            syscall(void);
void            argint(int, int*);
int             argstr(int, char*, int);
void            argaddr(int, uint64 *);
int             fetchstr(uint64, char*, int);
int             fetchaddr(uint64, uint*);

// trap.c
extern uint     ticks;
extern struct spinlock tickslock;
void            trapinit(void);
void            trapinithart(void);
void            prepare_return(void);
void            register_interrupt(int irq, void (*handler)(void));
void            interrupt_dispatch(int irq);

// plic.c
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int irq);

// test.c
void            test_physical_memory(void);
void            test_pagetable(void);
void            test_virtual_memory(void);
void            test_printf(void);
void            test_kernel_timer_interrupt(void);
void            test_entry(void);

// fs.c
void            fsinit(int);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr(void);

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
