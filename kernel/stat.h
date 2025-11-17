#define T_DIR     1   // 目录类型
#define T_FILE    2   // 普通文件类型
#define T_DEVICE  3   // 设备文件类型
#include "types.h"

struct stat {
  int dev;     // 文件所在的磁盘设备编号
  uint ino;    // 文件的索引节点（inode）编号，唯一标识文件
  short type;  // 文件类型（目录、普通文件或设备文件）
  short nlink; // 指向该文件的硬链接数量
  uint64 size; // 文件的字节大小
};
