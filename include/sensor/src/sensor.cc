#include "sensor.h"
#include "RS485.h"
#include <ctype.h>

#define DEVICE_ID_01 0x01
#define DEVICE_ID_02 0x02
#define DEVICE_ID_03 0x03

SensorData sensor_data;

int id = 1;
char Json_str_send[256] = {0};
char Json_str_rev[256] = {0};
char sensor_send_cmd[3][8] = {
    {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x08},   // 温湿度查询指令
    {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38},   // CO指令
    {0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8}    //光照度指令
};

// 初始化传感器数据及互斥锁
void sensor_data_init(void) {
    memset(&sensor_data, 0, sizeof(SensorData));
    pthread_mutex_init(&sensor_data.mutex, NULL);
}

// 线程安全的JSON打包函数
char* pack_sensor_to_json(void) {
    static char json_buf[256];  // 线程安全的局部缓冲区
    json_object *root = json_object_new_object();

    pthread_mutex_lock(&sensor_data.mutex);
    json_object_object_add(root, "DeviceID", json_object_new_int(sensor_data.dev_id));

    json_object *sensor_obj = json_object_new_object();
    json_object_object_add(sensor_obj, "Temperature", json_object_new_double(sensor_data.temperature));
    json_object_object_add(sensor_obj, "Humidity", json_object_new_double(sensor_data.humidity));
    json_object_object_add(sensor_obj, "CO_PPM", json_object_new_double(sensor_data.co_ppm));
    json_object_object_add(sensor_obj, "Light_Lux", json_object_new_double(sensor_data.light_lux));

    json_object_object_add(root, "SensorData", sensor_obj);
    const char *json_str = json_object_to_json_string(root);
    snprintf(json_buf, sizeof(json_buf), "%s", json_str);
    pthread_mutex_unlock(&sensor_data.mutex);

    json_object_put(root);
    return json_buf;
}

// 设备01处理函数（温湿度解析）
static void handle_device_01(const uint8_t *data, uint16_t len) {
    if (len < 4) return;

    // 提取数据（大端模式：高字节在前）
    uint16_t humidity_raw = (data[0] << 8) | data[1];
    uint16_t temp_raw = (data[2] << 8) | data[3];

    // 单位转换（假设湿度/温度单位为0.1%RH/0.1°C）
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.humidity = (float)humidity_raw / 10.0;
    sensor_data.temperature = (float)temp_raw / 10.0;
    pthread_mutex_unlock(&sensor_data.mutex);
}

// 设备02处理函数（CO浓度解析）
static void handle_device_02(const uint8_t *data, uint16_t len) {
    if (len < 2) return;
    uint16_t co_raw = (data[0] << 8) | data[1];
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.co_ppm = (float)co_raw * 0.1;
    pthread_mutex_unlock(&sensor_data.mutex);
}

// 设备03处理函数（光照度解析）
static void handle_device_03(const uint8_t *data, uint16_t len) {
    if (len < 2) return;
    uint16_t light_raw = (data[0] << 8) | data[1];
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.light_lux = (float)light_raw * 0.1;
    pthread_mutex_unlock(&sensor_data.mutex);
}

// 通用设备处理分发
void handle_sensor_data(const uint8_t *data, uint16_t len) {
    uint8_t dev_id = data[0];  // 设备号在数据帧第一个字节
    const uint8_t *payload = data + 3;  // 有效数据从第二个字节开始（跳过设备号）
    uint16_t payload_len = len - 3;     // 减去设备号字节

    switch (dev_id) {
        case 0x01: handle_device_01(payload, payload_len); break;
        case 0x02: handle_device_02(payload, payload_len); break;
        case 0x03: handle_device_03(payload, payload_len); break;
        default: printf("未知设备号: 0x%02X\n", dev_id);
    }
}

// /* 通用接收线程（处理不同设备） */
// void *general_sensor_recv_thread(void *arg) {
//     char buf[256] = {0}; // 足够大的缓冲区
//     uint16_t recv_crc, calc_crc;
//     int dev_id, data_len, n;

//     while (1) {
//         memset(buf, 0, sizeof(buf));
//         usleep(100000); // 调整睡眠周期

//         n = read(fd, buf, sizeof(buf));
//         if (n >= 5) { // 至少需要设备号+功能码+数据+2字节CRC
//             dev_id = buf[0];
//             data_len = n - 2; // 数据长度（不含CRC）
//             recv_crc = (buf[n-2] << 8) | buf[n-1]; // 接收的CRC（高字节在前）
//             calc_crc = crc16_modbus(buf, data_len); // 计算数据部分CRC

//             if (recv_crc == calc_crc) { // CRC校验通过
//                 hex_to_string(buf, n, Json_str_rev); // 打印原始16进制数据

//                 switch (dev_id) {
//                     case DEVICE_ID_01:
//                         printf("处理设备01数据\n");
//                         handle_device_01(buf + 1, data_len - 1); // 跳过设备号，传入数据部分
//                         break;
//                     case DEVICE_ID_02:
//                         printf("处理设备02数据\n");
//                         handle_device_02(buf + 1, data_len - 1);
//                         break;
//                     case DEVICE_ID_03:
//                         printf("处理设备03数据\n");
//                         handle_device_03(buf + 1, data_len - 1);
//                         break;
//                     default:
//                         printf("未知设备号: 0x%02X\n", dev_id);
//                         break;
//                 }
//             } else {
//                 printf("CRC校验失败！接收: 0x%04X 计算: 0x%04X\n", recv_crc, calc_crc);
//             }
//         }
//     }
//     return NULL;
// }


/* 发送数据 */
void send_data(char *data) {
    pthread_mutex_lock(&rs485_mutex);
    set_rs485_mode(1);
    write(fd, data, 8);
    usleep(10000);
    set_rs485_mode(0);
    pthread_mutex_unlock(&rs485_mutex);
}
