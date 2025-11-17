#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// 简单的日志系统，允许文件系统（FS）系统调用并发执行。
//
// 一个日志事务包含多个文件系统系统调用的更新。
// 日志系统只会在没有活动的文件系统系统调用时提交。
// 因此，无需考虑提交时是否会将未提交的系统调用的更新写入磁盘。
//
// 每个系统调用应在开始和结束时分别调用 begin_op()/end_op()。
// 通常 begin_op() 只是增加正在进行的文件系统系统调用计数并返回。
// 但如果日志空间快用完了，它会等待直到最后一个 end_op() 提交事务。
//
// 日志是物理重做日志，包含磁盘块。
// 日志的磁盘格式：
//   头块，包含块号（block #）列表：A、B、C...
//   块A
//   块B
//   块C
//   ...
// 日志追加是同步的。

// 日志头块的内容，既用于磁盘上的头块，也用于内存中跟踪已记录的块号（在提交前）。
struct logheader {
    int n;                  // 已记录的块数量
    int block[LOGBLOCKS];   // 已记录的块号列表
};

// 日志结构体，管理日志状态
struct log {
    struct spinlock lock;   // 日志锁，保证并发安全
    int start;              // 日志起始块号
    int outstanding;        // 正在执行的文件系统系统调用数量
    int committing;         // 是否正在提交事务
    int dev;                // 设备号
    struct logheader lh;    // 日志头
};
struct log log;

static void recover_from_log(void); // 从日志恢复
static void commit();               // 提交事务

// 初始化日志系统
void
initlog(int dev, struct superblock *sb)
{
    printf("initlog: dev=%d, logstart=%d\n", dev, sb->logstart);
    if (sizeof(struct logheader) >= BSIZE)
        panic("initlog: too big logheader"); // 日志头过大

    initlock(&log.lock, "log");        // 初始化日志锁
    log.start = sb->logstart;          // 设置日志起始块号
    log.dev = dev;                     // 设置设备号
    recover_from_log();                // 从日志恢复
}

// 将已提交的块从日志复制到其实际位置
static void
install_trans(int recovering)
{
    int tail;

    for (tail = 0; tail < log.lh.n; tail++) {
        if(recovering) {
            printf("recovering tail %d dst %d\n", tail, log.lh.block[tail]);
        }
        struct buf *lbuf = bread(log.dev, log.start+tail+1); // 读取日志块
        struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // 读取目标块
        memmove(dbuf->data, lbuf->data, BSIZE);  // 将日志块内容复制到目标块
        bwrite(dbuf);  // 写回目标块到磁盘
        if(recovering == 0)
            bunpin(dbuf); // 非恢复时解除pin
        brelse(lbuf);   // 释放日志块缓冲区
        brelse(dbuf);   // 释放目标块缓冲区
    }
}

// 从磁盘读取日志头到内存
static void
read_head(void)
{
    struct buf *buf = bread(log.dev, log.start); // 读取日志头块
    struct logheader *lh = (struct logheader *) (buf->data);
    int i;
    log.lh.n = lh->n; // 读取已记录块数量
    for (i = 0; i < log.lh.n; i++) {
        log.lh.block[i] = lh->block[i]; // 读取已记录块号
    }
    brelse(buf); // 释放缓冲区
}

// 将内存中的日志头写回磁盘
// 这是事务真正提交的时刻
static void
write_head(void)
{
    struct buf *buf = bread(log.dev, log.start); // 读取日志头块
    struct logheader *hb = (struct logheader *) (buf->data);
    int i;
    hb->n = log.lh.n; // 写入已记录块数量
    for (i = 0; i < log.lh.n; i++) {
        hb->block[i] = log.lh.block[i]; // 写入已记录块号
    }
    bwrite(buf); // 写回磁盘
    brelse(buf); // 释放缓冲区
}

// 从日志恢复
static void
recover_from_log(void)
{
    read_head();         // 读取日志头
    install_trans(1);    // 如果已提交，则从日志复制到磁盘
    log.lh.n = 0;        // 清空日志头
    write_head();        // 清空日志
}

// 每个文件系统系统调用开始时调用
void
begin_op(void)
{
    acquire(&log.lock); // 获取日志锁
    while(1){
        if(log.committing){
            sleep(&log, &log.lock); // 如果正在提交，等待
        } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGBLOCKS){
            // 本次操作可能耗尽日志空间，等待提交
            sleep(&log, &log.lock);
        } else {
            log.outstanding += 1; // 增加正在进行的系统调用计数
            release(&log.lock);   // 释放日志锁
            break;
        }
    }
}

// 每个文件系统系统调用结束时调用
// 如果这是最后一个操作，则提交事务
void
end_op(void)
{
    int do_commit = 0;

    acquire(&log.lock); // 获取日志锁
    log.outstanding -= 1; // 减少正在进行的系统调用计数
    if(log.committing)
        panic("log.committing"); // 不应在提交时调用
    if(log.outstanding == 0){
        do_commit = 1;      // 需要提交
        log.committing = 1; // 标记正在提交
    } else {
        // begin_op() 可能在等待日志空间，
        // 减少 outstanding 后可能释放空间
        wakeup(&log);       // 唤醒等待的进程
    }
    release(&log.lock);   // 释放日志锁

    if(do_commit){
        // 提交事务时不能持有锁，因为提交过程中可能会睡眠
        commit();
        acquire(&log.lock); // 重新获取日志锁
        log.committing = 0; // 清除提交标记
        wakeup(&log);       // 唤醒等待的进程
        release(&log.lock); // 释放日志锁
    }
}

// 将修改过的块从缓存写入日志
static void
write_log(void)
{
    int tail;

    for (tail = 0; tail < log.lh.n; tail++) {
        struct buf *to = bread(log.dev, log.start+tail+1); // 日志块
        struct buf *from = bread(log.dev, log.lh.block[tail]); // 缓存块
        memmove(to->data, from->data, BSIZE); // 复制数据到日志块
        bwrite(to);  // 写日志块到磁盘
        brelse(from); // 释放缓存块
        brelse(to);   // 释放日志块
    }
}

// 提交事务
static void
commit()
{
    if (log.lh.n > 0) {
        write_log();     // 将修改过的块从缓存写入日志
        write_head();    // 写日志头到磁盘，真正提交
        install_trans(0);// 将日志内容安装到实际位置
        log.lh.n = 0;    // 清空日志头
        write_head();    // 清空日志
    }
}

// 调用者已修改 b->data 并完成缓冲区操作
// 记录块号并通过增加引用计数将其pin在缓存中
// commit()/write_log() 会负责写磁盘
//
// log_write() 替代 bwrite()；典型用法：
//   bp = bread(...)
//   修改 bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
    int i;

    acquire(&log.lock); // 获取日志锁
    if (log.lh.n >= LOGBLOCKS)
        panic("too big a transaction"); // 事务过大
    if (log.outstanding < 1)
        panic("log_write outside of trans"); // 未在事务中调用

    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno)   // 日志吸收，避免重复记录
            break;
    }
    log.lh.block[i] = b->blockno; // 记录块号
    if (i == log.lh.n) {  // 新块，添加到日志
        bpin(b);            // pin住缓冲区，防止被回收
        log.lh.n++;         // 增加已记录块数量
    }
    release(&log.lock);   // 释放日志锁
}
