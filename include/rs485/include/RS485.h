#ifndef _RS485_H_
#define _RS485_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <pthread.h>

#define RS485_DEV "/dev/ttyS3"
#define BAUDRATE B4800

// 假设接收到的完整数据帧为：[设备号][功能码][数据长度][数据字段][CRC低][CRC高]
// 示例索引： 0    1      2        3-6        7     8
// 数据帧：   01   03     04       02 A6 00 EA  27    9A
#define MAX_FRAME_LEN 256  // 最大数据帧长度

// 接收数据帧结构体（包含原始数据）
typedef struct {
    uint8_t raw_data[MAX_FRAME_LEN];  // 原始16进制数据帧
    uint16_t frame_len;               // 实际接收数据长度
} RS485_Frame;

extern int fd;
extern pthread_mutex_t rs485_mutex;

int init_serial(void);
void set_rs485_mode(int enable_tx);
void *rs485_recv_thread(void *arg);

#endif
