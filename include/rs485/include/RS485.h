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

#define ON 0
#define OFF 1

// #define RS485_DEV "/dev/ttyS3"
#define RS485_DEV "/dev/pts/5"
#define BAUDRATE B4800

// 假设接收到的完整数据帧为：[设备号][功能码][数据长度][数据字段][CRC低][CRC高]
// 示例索引：                   0    1      2        3-6        7     8
// 数据帧：                     01   03     04       02 A6 00 EA  27    9A
#define MAX_FRAME_LEN 256  // 最大数据帧长度

// 接收数据帧结构体（包含原始数据）
typedef struct {
    uint8_t raw_data[MAX_FRAME_LEN];  // 原始16进制数据帧
    uint16_t frame_len;               // 实际接收数据长度
} RS485_Frame;

extern int fd;
extern pthread_mutex_t rs485_mutex;

/**
 * @brief 串口初始化函数（RS485 通信）
 * @details 打开指定的 RS485 设备文件，配置串口参数（波特率、数据位、停止位、校验位等），并初始化 RS485 模式为接收状态。
 * @return 0 表示初始化成功，-1 表示初始化失败（如设备打开失败或参数配置错误）
 */
int init_serial(void);

/**
 * @brief 设置 RS485 工作模式（发送/接收）
 * @details 通过 ioctl 配置串口的 RS485 模式，控制 RTS 引脚状态以切换收发模式。
 * @param enable_tx 模式控制参数：1 表示启用发送模式（RTS 置高），0 表示禁用发送模式（接收模式，RTS 置低）
 */
void set_rs485_mode(int enable_tx);

/**
 * @brief RS485 数据接收线程函数
 * @details 创建独立线程持续读取串口数据，进行 CRC16 校验（MODBUS RTU 标准），校验通过后调用传感器数据处理函数。
 * @param arg 线程参数（未使用，置为 NULL）
 * @return 线程返回值（未使用，置为 NULL）
 */
void *rs485_recv_thread(void *arg);

/**
 * @brief 灯光设备控制函数（通过继电器）
 * @details 根据输入状态发送对应的继电器控制指令（16 进制数据帧），实现灯光的开启或关闭。
 * @param state 设备状态：0 表示开启（ON），1 表示关闭（OFF）
 */
void light_control(int state);

/**
 * @brief 窗户设备控制函数（通过继电器）
 * @details 根据输入状态发送对应的继电器控制指令（16 进制数据帧），实现窗户的开启或关闭。
 * @param state 设备状态：0 表示开启（ON），1 表示关闭（OFF）
 */
void window_control(int state);

/**
 * @brief MODBUS RTU 协议 CRC16 校验算法
 * @details 计算指定数据缓冲区的 CRC16 值（MODBUS RTU 标准），用于验证数据帧完整性。
 * @param data 待校验的数据缓冲区（16 进制字节数组）
 * @param len 数据长度（字节数）
 * @return 16 位 CRC 校验值（高位在后，低位在前）
 */
static uint16_t crc16_modbus(const unsigned char *data, int len);

#endif
