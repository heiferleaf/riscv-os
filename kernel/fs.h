// 磁盘上的文件系统格式。
// 内核和用户程序都使用此头文件。
#pragma once
#include "types.h"

#define ROOTINO  1   // 根目录的 i-number
#define BSIZE 1024  // 块大小（字节）

// 磁盘布局：
// [ 引导块 | 超级块 | 日志 | inode块 |
//                                 空闲位图 | 数据块 ]
//
// mkfs 计算超级块并构建初始文件系统。
// 超级块描述磁盘布局：
struct superblock {
  uint magic;        // 必须为 FSMAGIC，文件系统魔数
  uint size;         // 文件系统镜像大小（块数）
  uint nblocks;      // 数据块数量
  uint ninodes;      // inode 数量
  uint nlog;         // 日志块数量
  uint logstart;     // 第一个日志块的块号
  uint inodestart;   // 第一个 inode 块的块号
  uint bmapstart;    // 第一个空闲位图块的块号
};

#define FSMAGIC 0x10203040  // 文件系统魔数

#define NDIRECT 12  // inode 直接块数量
#define NINDIRECT (BSIZE / sizeof(uint)) // 间接块可存放的地址数
#define MAXFILE (NDIRECT + NINDIRECT)    // 文件最大块数

// 磁盘上的 inode 结构体
struct dinode {
  short type;           // 文件类型
  short major;          // 主设备号（仅 T_DEVICE 类型）
  short minor;          // 次设备号（仅 T_DEVICE 类型）
  short nlink;          // 文件系统中指向该 inode 的链接数
  uint size;            // 文件大小（字节）
  uint addrs[NDIRECT+1];   // 数据块地址（12个直接块+1个间接块）
};

// 每块可存放的 inode 数量
#define IPB           (BSIZE / sizeof(struct dinode))

// 包含第 i 个 inode 的块号
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// 每块可存放的位图位数
#define BPB           (BSIZE*8)

// 包含第 b 个块的空闲位图块号
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// 目录是一个包含一系列 dirent 结构体的文件
#define DIRSIZ 14  // 目录项名称最大长度

// name 字段最多可有 DIRSIZ 个字符，可能不以 NUL 结尾
struct dirent {
  ushort inum;                // inode 编号
  char name[DIRSIZ] __attribute__((nonstring)); // 文件名
};
