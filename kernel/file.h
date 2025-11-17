/*
 * 文件: file.h
 * 说明: 定义了文件系统相关的结构体和宏，用于管理进程打开的文件、设备和内存中的inode等。
 *
 * struct file
 * ------------
 * 进程打开文件的结构体，描述文件的类型、引用计数、读写权限及相关资源。
 *   type      : 文件类型（FD_NONE:无效, FD_PIPE:管道, FD_INODE:普通文件, FD_DEVICE:设备文件）
 *   ref       : 文件结构体的引用计数，用于管理文件的生命周期
 *   readable  : 是否可读（1表示可读，0表示不可读）
 *   writable  : 是否可写（1表示可写，0表示不可写）
 *   pipe      : 指向管道结构体的指针，仅当type为FD_PIPE时有效
 *   ip        : 指向inode结构体的指针，仅当type为FD_INODE或FD_DEVICE时有效
 *   off       : 文件偏移量，仅用于普通文件（FD_INODE）
 *   major     : 设备主编号，仅用于设备文件（FD_DEVICE）
 *
 * major(dev), minor(dev), mkdev(m, n)
 * -----------------------------------
 * 设备号相关的宏定义：
 *   major(dev) : 获取设备主编号
 *   minor(dev) : 获取设备次编号
 *   mkdev(m, n): 由主编号和次编号生成设备号
 *
 * struct inode
 * -------------
 * 内存中的inode结构体，描述文件或设备的元数据及状态。
 *   dev      : 设备号，标识该inode属于哪个设备
 *   inum     : inode编号，唯一标识文件
 *   ref      : 引用计数，表示有多少结构体或进程引用该inode
 *   lock     : 睡眠锁，保护inode结构体的并发访问
 *   valid    : 标志位，表示该inode是否已从磁盘读取到内存
 *   type     : 文件类型（如普通文件、目录、设备等）
 *   major    : 设备主编号（仅设备文件有效）
 *   minor    : 设备次编号（仅设备文件有效）
 *   nlink    : 文件的硬链接数
 *   size     : 文件大小（字节数）
 *   addrs    : 数据块地址数组，保存文件内容在磁盘上的位置
 *
 * struct devsw
 * -------------
 * 设备驱动函数表，用于根据主设备号调用对应的读写函数。
 *   read  : 设备读函数指针
 *   write : 设备写函数指针
 *
 * extern struct devsw devsw[]
 * ---------------------------
 * 设备驱动函数表的全局数组，根据主设备号索引，调用对应设备的读写操作。
 *
 * #define CONSOLE 1
 * -----------------
 * 控制台设备的主编号定义
 */
#pragma once
  
#include "types.h"
#include "sleeplock.h"
#include "fs.h"
  
  // Per-process open file structure

struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct inode *ip;  // FD_INODE and FD_DEVICE
    uint off;          // FD_INODE
    short major;       // FD_DEVICE
  };
  
  #define major(dev)  ((dev) >> 16 & 0xFFFF)
  #define minor(dev)  ((dev) & 0xFFFF)
  #define	mkdev(m,n)  ((uint)((m)<<16| (n)))
  
  // in-memory copy of an inode
  struct inode {
    uint dev;           // Device number
    uint inum;          // Inode number
    int ref;            // Reference count
    struct sleeplock lock; // protects everything below here
    int valid;          // inode has been read from disk?
  
    short type;         // copy of disk inode
    short major;
    short minor;
    short nlink;
    uint size;
    uint addrs[NDIRECT+1];
  };
  
  // map major device number to device functions.
  struct devsw {
    int (*read)(int, uint64, int);
    int (*write)(int, uint64, int);
  };
  
  extern struct devsw devsw[];
  
  #define CONSOLE 1
  