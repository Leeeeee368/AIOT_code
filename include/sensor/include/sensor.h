/**
 * @file sensor.h
 * @brief 传感器相关功能的头文件，定义了传感器数据结构体、函数原型等。
 */

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <stdint.h>
#include <string.h>
#include "json.h"

/**
 * @struct SensorData
 * @brief 存储传感器数据的结构体。
 */
typedef struct {
    int dev_id;                 /**< 设备号（范围 0x01 - 0x03） */
    double temperature;            /**< 温度（单位：°C） */
    double humidity;               /**< 湿度（单位：%RH） */
    double co_ppm;                 /**< CO 浓度（单位：ppm） */
    double co_per;                 /**< CO 浓度（单位：ppm） */
    double light_lux;              /**< 光照度（单位：lux） */
    pthread_mutex_t mutex;      /**< 数据访问互斥锁，用于保证线程安全 */
} SensorData;

extern SensorData sensor_data;      /**< 外部可访问的传感器数据结构体实例 */
extern int id;                      /**< 外部可访问的设备 ID 变量 */
extern char Json_str_send[256];     /**< 外部可访问的用于发送的 JSON 字符串数组 */
extern char Json_str_rev[256];      /**< 外部可访问的用于接收的 JSON 字符串数组 */

/**
 * @brief 温湿度传感器接收线程函数。
 * @param arg 传递给线程的参数。
 * @return 线程返回值，通常为 NULL。
 */
void *humiture_sensor_recv_thread(void *arg);

/**
 * @brief 发送数据到 RS485 总线。
 * @param data 要发送的数据指针。
 */
void send_data(char *data);

/**
 * @brief 初始化传感器数据结构体及互斥锁。
 */
void sensor_data_init(void);

/**
 * @brief 将传感器数据打包成 JSON 字符串。
 * @return 包含传感器数据的 JSON 字符串指针。
 */
char* pack_sensor_to_json(void);

/**
 * @brief 处理接收到的传感器数据。
 * @param data 接收到的传感器数据指针。
 * @param len 接收到的数据长度。
 */
void handle_sensor_data(const uint8_t *data, uint16_t len);

#endif
