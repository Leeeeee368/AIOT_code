#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <stdint.h>
#include <string.h>
#include "json.h"

typedef struct {
    int dev_id;          // 设备号（0x01-0x03）
    int temperature;       // 温度（°C）
    int humidity;          // 湿度（%RH）
    int co_ppm;            // CO浓度（ppm）
    int light_lux;         // 光照度（lux）
    pthread_mutex_t mutex;   // 数据访问互斥锁
} SensorData;

extern SensorData sensor_data;
extern int id;
extern char Json_str_send[256];
extern char Json_str_rev[256];
// const char sensor_send_cmd[3][8] = {
//     {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x08},   // 温湿度查询指令
//     {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38},   // CO指令
//     {0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8}    //光照度指令
// };

void *humiture_sensor_recv_thread(void *arg);
void send_data(char *data);
void sensor_data_init(void);
char* pack_sensor_to_json(void);
void handle_sensor_data(const uint8_t *data, uint16_t len);

#endif
