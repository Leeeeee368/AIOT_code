#include "mqtt_client.h"
#include "RS485.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 全局回调函数指针
static MQTTMessageCallback user_callback = NULL;

// 初始化MQTT客户端
int mqtt_client_init(MQTTClientConfig *config) {
    if (!config->server_uri) {
        fprintf(stderr, "MQTT server URI is required\n");
        return -1;
    }

    // 生成默认客户端ID
    if (!config->client_id) {
        config->client_id = "mqtt_client_12345";
    }

    // 创建客户端句柄
    int rc = MQTTClient_create(&config->client, config->server_uri,
                               config->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to create MQTT client: %d\n", rc);
        return rc;
    }

    return MQTTCLIENT_SUCCESS;
}

// 连接到MQTT服务器
int mqtt_client_connect(MQTTClientConfig *config) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = config->keepalive;
    conn_opts.cleansession = 1;
    conn_opts.username = config->username;
    conn_opts.password = config->password;

    // 设置消息回调
    // MQTTClient_setCallbacks(config->client, config, NULL, message_arrived, NULL);

    int rc = MQTTClient_connect(config->client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect to MQTT server: %d\n", rc);
    }
    return rc;
}

// 断开连接
void mqtt_client_disconnect(MQTTClientConfig *config) {
    MQTTClient_disconnect(config->client, 1000);
    MQTTClient_destroy(&config->client);
}

// 订阅主题
int mqtt_client_subscribe(MQTTClientConfig *config, const char *topic, int qos) {
    int rc = MQTTClient_subscribe(config->client, topic, qos);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to subscribe to topic: %d\n", rc);
    }
    return rc;
}

// Paho库要求的消息到达回调
static int message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    MQTTClientConfig *config = (MQTTClientConfig *)context;  // 从上下文中获取配置
    json_object *json_msg = NULL;
    const char *payload = (char *)message->payload;

    if (user_callback && payload) {
        json_msg = json_tokener_parse(payload);
        if (json_msg) {
            // 传递 topicName 和 json_msg 到用户回调
            user_callback(topicName, json_msg);
            json_object_put(json_msg);
        } else {
            fprintf(stderr, "JSON解析失败: %.*s\n", (int)message->payloadlen, payload);
            return 1;  // 提前返回，避免调用空回调
        }
    }
    MQTTClient_free(topicName);
    MQTTClient_freeMessage(&message);
    return 1;
}

// 设置用户自定义回调
void mqtt_client_set_callback(MQTTClientConfig *config, MQTTMessageCallback callback) {
    user_callback = callback;
    MQTTClient_setCallbacks(config->client, config, NULL, message_arrived, NULL);
}

// 发布JSON数据
int mqtt_client_publish_json(MQTTClientConfig *config, const char *topic, char *json_data, int qos) {
    MQTTClient_message msg = MQTTClient_message_initializer;
    msg.payload = json_data;
    msg.payloadlen = strlen(json_data);
    msg.qos = qos;
    msg.retained = 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(config->client, topic, &msg, &token);
    return rc;
}

void print_received_message(const char* topic, json_object* json_msg) {
    printf("\n=== 收到MQTT消息 ===\n");
    printf("主题: %s\n", topic);
    printf("原始JSON: %s\n", json_object_to_json_string(json_msg));

    // 解析JSON字段并执行对应操作
    json_object *value;

    /* 灯光控制 */
    if (json_object_object_get_ex(json_msg, "Light", &value)) {
        const char* state = json_object_get_string(value);
        if (strcmp(state, "open") == 0) {
            light_control(ON);  // 开灯
        } else if (strcmp(state, "close") == 0) {
            light_control(OFF); // 关灯
        }
    }

    /* 窗户控制 */
    if (json_object_object_get_ex(json_msg, "Window", &value)) {
        const char* state = json_object_get_string(value);
        if (strcmp(state, "open") == 0) {
            window_control(ON);  // 开窗
        } else if (strcmp(state, "close") == 0) {
            window_control(OFF); // 关窗
        }
    }
    printf("==================\n");
}
