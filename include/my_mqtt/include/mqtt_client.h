#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "MQTTClient.h"
#include "json.h"

#define MQTT_DEFAULT_PORT 1883
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_MAX_MESSAGE_SIZE 1024

// MQTT客户端配置结构体
typedef struct {
    char *server_uri;    // MQTT服务器URI（格式：tcp://ip:port）
    char *client_id;     // 客户端ID（可选，默认自动生成）
    char *username;      // 用户名（可选）
    char *password;      // 密码（可选）
    int port;            // 端口号（默认1883）
    int keepalive;       // 保持连接时间（默认60秒）
    MQTTClient client;   // Paho MQTT客户端句柄
} MQTTClientConfig;

/**
 * @brief 消息到达回调函数类型
 * @param topic 接收到的消息主题
 * @param json_msg 已解析的JSON对象
 */
typedef void (*MQTTMessageCallback)(const char* topic, json_object* json_msg);

/**
 * @brief 设置消息回调函数
 * @param config MQTT客户端配置
 * @param callback 用户自定义的消息处理回调函数
 */
void mqtt_client_set_callback(MQTTClientConfig *config, MQTTMessageCallback callback);

/**
 * @brief 初始化MQTT客户端
 * @param config MQTT客户端配置
 * @return 成功返回MQTTCLIENT_SUCCESS，失败返回错误码
 */
int mqtt_client_init(MQTTClientConfig *config);

/**
 * @brief 连接到MQTT服务器
 * @param config MQTT客户端配置
 * @return 成功返回MQTTCLIENT_SUCCESS，失败返回错误码
 */
int mqtt_client_connect(MQTTClientConfig *config);

/**
 * @brief 断开MQTT连接
 * @param config MQTT客户端配置
 */
void mqtt_client_disconnect(MQTTClientConfig *config);

/**
 * @brief 订阅主题
 * @param config MQTT客户端配置
 * @param topic 要订阅的主题
 * @param qos 服务质量等级（0/1/2）
 * @return 成功返回MQTTCLIENT_SUCCESS，失败返回错误码
 */
int mqtt_client_subscribe(MQTTClientConfig *config, const char *topic, int qos);

/**
 * @brief 发布JSON数据到主题
 * @param config MQTT客户端配置
 * @param topic 目标主题
 * @param json_data JSON格式字符串
 * @param qos 服务质量等级（0/1/2）
 * @return 成功返回MQTTCLIENT_SUCCESS，失败返回错误码
 */
int mqtt_client_publish_json(MQTTClientConfig *config, const char *topic, char *json_data, int qos);

/**
 * @brief 打印接收到的MQTT消息（JSON格式）
 * @param topic 消息主题
 * @param json_msg 已解析的JSON对象
 */
void print_received_message(const char* topic, json_object* json_msg);

#endif
