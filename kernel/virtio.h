//
// virtio 设备定义。
// 包括 mmio 接口和 virtio 描述符。
// 仅在 qemu 下测试过。
//
// virtio 规范：
// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf
//

// virtio mmio 控制寄存器，从 0x10001000 映射开始。
// 来源于 qemu virtio_mmio.h

#include "types.h"

#define VIRTIO_MMIO_MAGIC_VALUE		0x000 // 魔数寄存器，固定为 0x74726976，用于识别 virtio 设备
#define VIRTIO_MMIO_VERSION		0x004 // 版本号寄存器，应该为 2
#define VIRTIO_MMIO_DEVICE_ID		0x008 // 设备类型寄存器；1 表示网络设备，2 表示磁盘设备
#define VIRTIO_MMIO_VENDOR_ID		0x00c // 厂商 ID 寄存器，固定为 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010 // 设备支持的特性位，驱动可读取
#define VIRTIO_MMIO_DRIVER_FEATURES	0x020 // 驱动支持的特性位，驱动可写入
#define VIRTIO_MMIO_QUEUE_SEL		0x030 // 选择队列，写入队列编号
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034 // 当前队列最大长度，只读
#define VIRTIO_MMIO_QUEUE_NUM		0x038 // 当前队列长度，驱动写入
#define VIRTIO_MMIO_QUEUE_READY		0x044 // 队列就绪位，驱动设置为 1 表示队列可用
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050 // 通知设备队列有新请求，写入队列编号
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060 // 中断状态寄存器，只读
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064 // 中断确认寄存器，驱动写入以清除中断
#define VIRTIO_MMIO_STATUS		0x070 // 设备状态寄存器，读写
#define VIRTIO_MMIO_QUEUE_DESC_LOW	0x080 // 描述符表物理地址低 32 位，写入
#define VIRTIO_MMIO_QUEUE_DESC_HIGH	0x084 // 描述符表物理地址高 32 位，写入
#define VIRTIO_MMIO_DRIVER_DESC_LOW	0x090 // available ring 物理地址低 32 位，写入
#define VIRTIO_MMIO_DRIVER_DESC_HIGH	0x094 // available ring 物理地址高 32 位，写入
#define VIRTIO_MMIO_DEVICE_DESC_LOW	0x0a0 // used ring 物理地址低 32 位，写入
#define VIRTIO_MMIO_DEVICE_DESC_HIGH	0x0a4 // used ring 物理地址高 32 位，写入

// 状态寄存器的各个标志位，来源于 qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1 // 驱动已识别设备
#define VIRTIO_CONFIG_S_DRIVER		2 // 驱动已初始化
#define VIRTIO_CONFIG_S_DRIVER_OK	4 // 驱动已准备好
#define VIRTIO_CONFIG_S_FEATURES_OK	8 // 驱动与设备协商特性完成

// 设备特性位
#define VIRTIO_BLK_F_RO              5	/* 磁盘为只读 */
#define VIRTIO_BLK_F_SCSI            7	/* 支持 SCSI 命令透传 */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* 配置中支持写回模式 */
#define VIRTIO_BLK_F_MQ             12	/* 支持多个队列 */
#define VIRTIO_F_ANY_LAYOUT         27	/* 支持任意描述符布局 */
#define VIRTIO_RING_F_INDIRECT_DESC 28	/* 支持间接描述符 */
#define VIRTIO_RING_F_EVENT_IDX     29	/* 支持事件索引 */

// virtio 描述符数量，必须为 2 的幂
#define NUM 8

// 单个描述符结构体，参见规范
struct virtq_desc {
  uint64 addr;   // 数据缓冲区物理地址
  uint32 len;    // 缓冲区长度（字节数）
  uint16 flags;  // 标志位，指示描述符属性
  uint16 next;   // 下一个描述符索引（用于链式描述符）
};
// 描述符标志位
#define VRING_DESC_F_NEXT  1 // 与下一个描述符链式连接
#define VRING_DESC_F_WRITE 2 // 设备写入（否则为设备读取）

// avail ring（可用环）结构体，参见规范
struct virtq_avail {
  uint16 flags; // 标志位，通常为 0
  uint16 idx;   // 驱动写入 ring[idx]，表示下一个可用描述符
  uint16 ring[NUM]; // 描述符链头的编号数组
  uint16 unused;    // 保留字段
};

// used ring（已用环）中的单个条目，设备通知驱动已完成的请求
struct virtq_used_elem {
  uint32 id;   // 已完成的描述符链起始索引
  uint32 len;  // 处理的数据长度
};

// used ring（已用环）结构体
struct virtq_used {
  uint16 flags; // 标志位，通常为 0
  uint16 idx;   // 设备每添加一个 ring[] 条目就递增
  struct virtq_used_elem ring[NUM]; // 已完成请求的条目数组
};

// 以下为 virtio 块设备（磁盘）专用结构体，详见规范第 5.2 节

#define VIRTIO_BLK_T_IN  0 // 读取磁盘
#define VIRTIO_BLK_T_OUT 1 // 写入磁盘

// 磁盘请求的第一个描述符的格式
// 后面还需跟两个描述符：一个用于数据块，一个用于 1 字节状态
struct virtio_blk_req {
  uint32 type;     // 请求类型：VIRTIO_BLK_T_IN 或 VIRTIO_BLK_T_OUT
  uint32 reserved; // 保留字段，未使用
  uint64 sector;   // 扇区号
};
