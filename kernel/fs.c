#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

struct superblock sb;
extern char disk[][BSIZE];
#define DISK_BLOCKS 1024


/**
 * @brief 从磁盘读取超级块
 * @param dev 设备号
 * @param sb 超级块指针
 */
static void readsb(int dev, struct superblock *sb)
{
    struct buf *bp;
    bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    brelse(bp);
}

/**
 * @brief 文件系统初始化
 * @param dev 设备号
 * 读取超级块并初始化日志，回收孤立inode
 */
void fsinit(int dev)
{
    readsb(dev, &sb);
    if (sb.magic != FSMAGIC)
        panic("invalid file system");
    initlog(dev, &sb);
    ireclaim(dev);
}

/**
 * @brief 清零指定磁盘块
 * @param dev 设备号
 * @param bno 块号
 */
static void bzero(int dev, int bno)
{
    struct buf *bp;
    bp = bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    log_write(bp);
    brelse(bp);
}

/**
 * @brief 分配一个空闲磁盘块并清零
 * @param dev 设备号
 * @return 块号，失败返回0
 */
static uint balloc(uint dev)
{
    int b, bi, m;
    struct buf *bp;
    bp = 0;
    for (b = 0; b < sb.size; b += BPB) {
        bp = bread(dev, BBLOCK(b, sb));
        for (bi = 0; bi < BPB && b + bi < sb.size; bi++) {
            m = 1 << (bi % 8);
            if ((bp->data[bi / 8] & m) == 0) {
                bp->data[bi / 8] |= m;
                log_write(bp);
                brelse(bp);
                bzero(dev, b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    printf("balloc: out of blocks\n");
    return 0;
}

/**
 * @brief 释放磁盘块
 * @param dev 设备号
 * @param b 块号
 */
static void bfree(int dev, uint b)
{
    struct buf *bp;
    int bi, m;
    bp = bread(dev, BBLOCK(b, sb));
    bi = b % BPB;
    m = 1 << (bi % 8);
    if ((bp->data[bi / 8] & m) == 0)
        panic("freeing free block");
    bp->data[bi / 8] &= ~m;
    log_write(bp);
    brelse(bp);
}

// inode表结构体
struct {
    struct spinlock lock;
    struct inode inode[NINODE];
} itable;

/**
 * @brief 初始化inode表
 */
void iinit()
{
    int i = 0;
    initlock(&itable.lock, "itable");
    for (i = 0; i < NINODE; i++) {
        initsleeplock(&itable.inode[i].lock, "inode");
    }
}

static struct inode* iget(uint dev, uint inum);

/**
 * @brief 分配一个新的inode
 * @param dev 设备号
 * @param type inode类型
 * @return 新分配的inode指针，失败返回NULL
 */
struct inode* ialloc(uint dev, short type)
{
    int inum;
    struct buf *bp;
    struct dinode *dip;
    for (inum = 1; inum < sb.ninodes; inum++) {
        bp = bread(dev, IBLOCK(inum, sb));
        dip = (struct dinode*)bp->data + inum % IPB;
        if (dip->type == 0) {
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            log_write(bp);
            brelse(bp);
            return iget(dev, inum);
        }
        brelse(bp);
    }
    printf("ialloc: no inodes\n");
    return 0;
}

/**
 * @brief 将内存中的inode写回磁盘
 * @param ip inode指针
 * 调用者必须持有ip->lock
 */
void iupdate(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode*)bp->data + ip->inum % IPB;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    log_write(bp);
    brelse(bp);
}

/**
 * @brief 查找或分配inode表项
 * @param dev 设备号
 * @param inum inode编号
 * @return inode指针
 */
static struct inode* iget(uint dev, uint inum)
{
    struct inode *ip, *empty;
    acquire(&itable.lock);
    empty = 0;
    for (ip = &itable.inode[0]; ip < &itable.inode[NINODE]; ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            release(&itable.lock);
            return ip;
        }
        if (empty == 0 && ip->ref == 0)
            empty = ip;
    }
    if (empty == 0)
        panic("iget: no inodes");
    ip = empty;
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    ip->valid = 0;
    release(&itable.lock);
    return ip;
}

/**
 * @brief 增加inode引用计数
 * @param ip inode指针
 * @return inode指针
 */
struct inode* idup(struct inode *ip)
{
    acquire(&itable.lock);
    ip->ref++;
    release(&itable.lock);
    return ip;
}

/**
 * @brief 锁定inode并必要时从磁盘读取
 * @param ip inode指针
 */
void ilock(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;
    if (ip == 0 || ip->ref < 1)
        panic("ilock");
    acquiresleep(&ip->lock);
    if (ip->valid == 0) {
        bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        dip = (struct dinode*)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
        if (ip->type == 0)
            panic("ilock: no type");
    }
}

/**
 * @brief 解锁inode
 * @param ip inode指针
 */
void iunlock(struct inode *ip)
{
    if (ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
        panic("iunlock");
    releasesleep(&ip->lock);
}

/**
 * @brief 释放inode引用，必要时回收磁盘空间
 * @param ip inode指针
 * 所有调用必须在事务内
 */
void iput(struct inode *ip)
{
    acquire(&itable.lock);
    if (ip->ref == 1 && ip->valid && ip->nlink == 0) {
        acquiresleep(&ip->lock);
        release(&itable.lock);
        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;
        releasesleep(&ip->lock);
        acquire(&itable.lock);
    }
    ip->ref--;
    release(&itable.lock);
}

/**
 * @brief 解锁并释放inode
 * @param ip inode指针
 */
void iunlockput(struct inode *ip)
{
    iunlock(ip);
    iput(ip);
}

/**
 * @brief 回收孤立inode（nlink为0但type非0）
 * @param dev 设备号
 */
void ireclaim(int dev)
{
    for (int inum = 1; inum < sb.ninodes; inum++) {
        struct inode *ip = 0;
        struct buf *bp = bread(dev, IBLOCK(inum, sb));
        struct dinode *dip = (struct dinode *)bp->data + inum % IPB;
        if (dip->type != 0 && dip->nlink == 0) {
            printf("ireclaim: orphaned inode %d\n", inum);
            ip = iget(dev, inum);
        }
        brelse(bp);
        if (ip) {
            begin_op();
            ilock(ip);
            iunlock(ip);
            iput(ip);
            end_op();
        }
    }
}

/**
 * @brief 获取inode的第bn个数据块地址，如无则分配
 * @param ip inode指针
 * @param bn 块序号
 * @return 块号，失败返回0
 */
static uint bmap(struct inode *ip, uint bn)
{
    uint addr, *a;
    struct buf *bp;
    if (bn < NDIRECT) {
        if ((addr = ip->addrs[bn]) == 0) {
            addr = balloc(ip->dev);
            if (addr == 0)
                return 0;
            ip->addrs[bn] = addr;
        }
        return addr;
    }
    bn -= NDIRECT;
    if (bn < NINDIRECT) {
        if ((addr = ip->addrs[NDIRECT]) == 0) {
            addr = balloc(ip->dev);
            if (addr == 0)
                return 0;
            ip->addrs[NDIRECT] = addr;
        }
        bp = bread(ip->dev, addr);
        a = (uint*)bp->data;
        if ((addr = a[bn]) == 0) {
            addr = balloc(ip->dev);
            if (addr) {
                a[bn] = addr;
                log_write(bp);
            }
        }
        brelse(bp);
        return addr;
    }
    panic("bmap: out of range");
}

/**
 * @brief 截断inode内容（释放所有数据块）
 * @param ip inode指针
 * 调用者必须持有ip->lock
 */
void itrunc(struct inode *ip)
{
    int i, j;
    struct buf *bp;
    uint *a;
    for (i = 0; i < NDIRECT; i++) {
        if (ip->addrs[i]) {
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }
    if (ip->addrs[NDIRECT]) {
        bp = bread(ip->dev, ip->addrs[NDIRECT]);
        a = (uint*)bp->data;
        for (j = 0; j < NINDIRECT; j++) {
            if (a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }
    ip->size = 0;
    iupdate(ip);
}

/**
 * @brief 拷贝inode信息到stat结构体
 * @param ip inode指针
 * @param st stat结构体指针
 * 调用者必须持有ip->lock
 */
void stati(struct inode *ip, struct stat *st)
{
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->nlink;
    st->size = ip->size;
}

/**
 * @brief 从inode读取数据
 * @param ip inode指针
 * @param user_dst 是否用户空间
 * @param dst 目标地址
 * @param off 偏移
 * @param n 读取字节数
 * @return 实际读取字节数
 * 调用者必须持有ip->lock
 */
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n)
{
    uint tot, m;
    struct buf *bp;
    if (off > ip->size || off + n < off)
        return 0;
    if (off + n > ip->size)
        n = ip->size - off;
    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        uint addr = bmap(ip, off / BSIZE);
        if (addr == 0)
            break;
        bp = bread(ip->dev, addr);
        m = min(n - tot, BSIZE - off % BSIZE);
        if (either_copyout(user_dst, dst, bp->data + (off % BSIZE), m) == -1) {
            brelse(bp);
            tot = -1;
            break;
        }
        brelse(bp);
    }
    return tot;
}

/**
 * @brief 写数据到inode
 * @param ip inode指针
 * @param user_src 是否用户空间
 * @param src 源地址
 * @param off 偏移
 * @param n 写入字节数
 * @return 实际写入字节数，失败返回-1
 * 调用者必须持有ip->lock
 */
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n)
{
    uint tot, m;
    struct buf *bp;
    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;
    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        uint addr = bmap(ip, off / BSIZE);
        if (addr == 0)
            break;
        bp = bread(ip->dev, addr);
        m = min(n - tot, BSIZE - off % BSIZE);
        if (either_copyin(bp->data + (off % BSIZE), user_src, src, m) == -1) {
            brelse(bp);
            break;
        }
        log_write(bp);
        brelse(bp);
    }
    if (off > ip->size)
        ip->size = off;
    iupdate(ip);
    return tot;
}

/**
 * @brief 比较目录项名称
 * @param s 字符串1
 * @param t 字符串2
 * @return 是否相等
 */
int namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/**
 * @brief 在目录中查找指定名称的目录项
 * @param dp 目录inode指针
 * @param name 名称
 * @param poff 找到时返回偏移
 * @return 找到的inode指针，失败返回NULL
 */
struct inode* dirlookup(struct inode *dp, char *name, uint *poff)
{
    uint off, inum;
    struct dirent de;
    if (dp->type != T_DIR)
        panic("dirlookup not DIR");
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        if (namecmp(name, de.name) == 0) {
            if (poff)
                *poff = off;
            inum = de.inum;
            return iget(dp->dev, inum);
        }
    }
    return 0;
}

/**
 * @brief 在目录中写入新目录项
 * @param dp 目录inode指针
 * @param name 名称
 * @param inum inode编号
 * @return 成功返回0，失败返回-1
 */
int dirlink(struct inode *dp, char *name, uint inum)
{
    int off;
    struct dirent de;
    struct inode *ip;
    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if (de.inum == 0)
            break;
    }
    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        return -1;
    return 0;
}

/**
 * @brief 跳过路径中的一个元素，拷贝到name
 * @param path 路径字符串
 * @param name 输出元素名
 * @return 跳过后的路径指针
 */
static char* skipelem(char *path, char *name)
{
    char *s;
    int len;
    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

/**
 * @brief 路径查找inode
 * @param path 路径
 * @param nameiparent 是否返回父目录
 * @param name 最后元素名
 * @return inode指针
 * 必须在事务内调用
 */
static struct inode* namex(char *path, int nameiparent, char *name)
{
    struct inode *ip, *next;
    if (*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else
        ip = idup(myproc()->cwd);
    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            iunlock(ip);
            return ip;
        }
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

/**
 * @brief 路径查找inode
 * @param path 路径
 * @return inode指针
 */
struct inode* namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, 0, name);
}

/**
 * @brief 路径查找父目录inode
 * @param path 路径
 * @param name 输出最后元素名
 * @return 父目录inode指针
 */
struct inode* nameiparent(char *path, char *name)
{
    return namex(path, 1, name);
}
