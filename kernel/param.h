#define NPROC        64  // 最大进程数
#define NCPU          8  // 最大CPU数
#define NOFILE       16  // 每个进程可打开的文件数
#define NFILE       100  // 系统可打开的文件数
#define NINODE       50  // 活动i节点最大数
#define NDEV         10  // 主设备号最大值
#define ROOTDEV       1  // 文件系统根磁盘的设备号
#define MAXARG       32  // exec参数最大数量
#define MAXOPBLOCKS  10  // 文件系统操作最多写入的块数
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // 磁盘日志中的最大数据块数
#define NBUF         (MAXOPBLOCKS*3)  // 磁盘块缓存大小
#define FSSIZE       2000  // 文件系统块数
#define MAXPATH      128   // 文件路径名最大长度
#define USERSTACK    1     // 用户栈页数

