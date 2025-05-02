#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "MQTTClient.h"
#include "json.h"

#define MQTT_DEFAULT_PORT 1883
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_MAX_MESSAGE_SIZE 1024

// MQTT客户端配置结构体
typedef struct {
    char *server_uri;       // MQTT服务器URI（格式：tcp://ip:port）
    char *client_id;        // 客户端ID（可选，默认自动生成）
    char *username;         // 用户名（可选）
    char *password;         // 密码（可选）
    int port;               // 端口号（默认1883）
    int keepalive;          // 保持连接时间（默认60秒）
    MQTTClient client;      // Paho MQTT客户端句柄
} MQTTClientConfig;

// 消息回调函数类型
typedef void (*mqtt_message_callback)(const char *topic, json_object *json_data, void *user_data);

// 初始化MQTT客户端
int mqtt_client_init(MQTTClientConfig *config);

// 连接到MQTT服务器
int mqtt_client_connect(MQTTClientConfig *config);

// 断开MQTT连接
void mqtt_client_disconnect(MQTTClientConfig *config);

// 订阅主题
int mqtt_client_subscribe(MQTTClientConfig *config, const char *topic, int qos);

// 发布JSON数据到主题
int mqtt_client_publish_json(MQTTClientConfig *config, const char *topic, char *json_data, int qos);

// 设置消息回调函数
// void mqtt_client_set_message_callback(MQTTClientConfig *config, mqtt_message_callback callback, void *user_data);

#endif
