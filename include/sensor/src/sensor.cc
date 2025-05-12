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

/* 发送数据 */
void send_data(char *data) {
    pthread_mutex_lock(&rs485_mutex);
    set_rs485_mode(1);
    write(fd, data, 8);
    usleep(10000);
    set_rs485_mode(0);
    pthread_mutex_unlock(&rs485_mutex);
}

// 线程安全的JSON打包函数
char* pack_sensor_to_json(void) {
    static char json_buf[256];  // 线程安全的局部缓冲区
    // 创建根对象
    json_object *root = json_object_new_object();

    pthread_mutex_lock(&sensor_data.mutex);
    // 添加 DeviceID 字段（整数类型）
    json_object_object_add(root, "DeviceID", json_object_new_int(sensor_data.dev_id));

    json_object *sensor_obj = json_object_new_object();
    json_object_object_add(sensor_obj, "CO", json_object_new_double(sensor_data.co_ppm));
    json_object_object_add(sensor_obj, "Light", json_object_new_double(sensor_data.light_lux));
    json_object_object_add(root, "SENSOR", sensor_obj);

    json_object *temp_obj = json_object_new_object();
    json_object_object_add(temp_obj, "TP", json_object_new_double(sensor_data.humidity));           // 浮点数
    json_object_object_add(temp_obj, "WD", json_object_new_double(sensor_data.temperature));           // 浮点数
    json_object_object_add(root, "TEMP", temp_obj);                           // 添加到根对象

    // 生成格式化的 JSON 字符串（带缩进）
    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);

    snprintf(json_buf, sizeof(json_buf), "%s", json_str);
    pthread_mutex_unlock(&sensor_data.mutex);

    json_object_put(root);
    return json_buf;
}

// 线程安全的JSON打包函数（华为云格式）
char* pack_sensor_to_huaweicloud(void) {
    static char json_buf[256];  // 静态缓冲区保证线程安全
    pthread_mutex_lock(&sensor_data.mutex);

    // 创建根对象
    json_object *root = json_object_new_object();

    // 创建services数组（华为云要求格式）
    json_object *services_array = json_object_new_array();

    // 创建service对象
    json_object *service_obj = json_object_new_object();
    json_object_object_add(service_obj, "service_id", json_object_new_string("SensorData"));

    // 创建properties对象
    json_object *properties = json_object_new_object();
    json_object_object_add(properties, "DeviceID", json_object_new_int(sensor_data.dev_id));
    json_object_object_add(properties, "CO", json_object_new_double(sensor_data.co_ppm));
    json_object_object_add(properties, "Light", json_object_new_double(sensor_data.light_lux));
    json_object_object_add(properties, "TP", json_object_new_double(sensor_data.humidity));
    json_object_object_add(properties, "WD", json_object_new_double(sensor_data.temperature));

    // 组装层级结构
    json_object_object_add(service_obj, "properties", properties);
    json_object_array_add(services_array, service_obj);
    json_object_object_add(root, "services", services_array);

    // 生成紧凑型JSON（华为云推荐格式）
    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    snprintf(json_buf, sizeof(json_buf), "%s", json_str);

    // 清理资源
    json_object_put(root);
    pthread_mutex_unlock(&sensor_data.mutex);

    return json_buf;
}

/**
 * @brief 设备 01 处理函数（温湿度解析）。
 * @details 解析设备 01 发送的温湿度数据，并更新传感器数据结构体。
 * @param data 接收到的设备 01 数据指针。
 * @param len 接收到的数据长度。
 */
// eg. 01 03 04 02 A6 00 EA 9A 27
static void handle_device_01(const uint8_t *data, uint16_t len) {
    if (len < 4) return;

    // 提取数据（大端模式：高字节在前）
    uint16_t humidity_raw   = (data[0] << 8) | data[1];
    uint16_t temp_raw       = (data[2] << 8) | data[3];

    // 单位转换（假设湿度/温度单位为0.1%RH/0.1°C）
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.humidity    = (double)humidity_raw / 10.0;
    sensor_data.temperature = (double)temp_raw / 10.0;
    pthread_mutex_unlock(&sensor_data.mutex);
}

/**
 * @brief 设备 02 处理函数（CO 浓度解析）。
 * @details 解析设备 02 发送的 CO 浓度数据，并更新传感器数据结构体。
 * @param data 接收到的设备 02 数据指针。
 * @param len 接收到的数据长度。
 */
// eg. 02 03 04 02 A6 00 EA A9 27
static void handle_device_02(const uint8_t *data, uint16_t len) {
    if (len < 2) return;
    uint16_t co_raw         = (data[0] << 8) | data[1];
    uint16_t co_per_raw     = (data[2] << 8) | data[3];
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.co_ppm      = (double)co_raw;
    sensor_data.co_per      = (double)co_per_raw * 0.1;
    pthread_mutex_unlock(&sensor_data.mutex);
}

/**
 * @brief 设备 03 处理函数（光照度解析）。
 * @details 解析设备 03 发送的光照度数据，并更新传感器数据结构体。
 * @param data 接收到的设备 03 数据指针。
 * @param len 接收到的数据长度。
 */
// eg. 03 03 02 04 DD 03 1D
static void handle_device_03(const uint8_t *data, uint16_t len) {
    if (len < 2) return;
    uint16_t light_raw      = (data[0] << 8) | data[1];
    pthread_mutex_lock(&sensor_data.mutex);
    sensor_data.light_lux   = (double)light_raw;
    pthread_mutex_unlock(&sensor_data.mutex);
}

// 通用设备处理分发
void handle_sensor_data(const uint8_t *data, uint16_t len) {
    uint8_t dev_id          = data[0];                  // 设备号（索引0）
    uint8_t func_code       = data[1];                  // 功能码（索引1）
    uint8_t data_len_field  = data[2];                  // 数据长度字段（索引2，如0x04表示4字节有效数据）
    const uint8_t *payload  = data + 3;                 // 有效数据从索引3开始
    uint16_t payload_len    = data_len_field;           // 直接使用数据长度字段

    switch (dev_id) {
        case 0x01:
            handle_device_01(payload, payload_len);
            printf("dev_id 0x%02X\n",dev_id);
            break;
        case 0x02:
            handle_device_02(payload, payload_len);
            printf("dev_id 0x%02X\n",dev_id);
            break;
        case 0x03:
            handle_device_03(payload, payload_len);
            printf("dev_id 0x%02X\n",dev_id);
            break;
        default:
            printf("未知设备号: 0x%02X\n", dev_id);
    }
}
