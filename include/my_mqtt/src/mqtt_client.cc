#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 消息接收回调（Paho库回调函数）
static void message_arrived(MessageData *md) {
    MQTTClientConfig *config = (MQTTClientConfig *)md->userContext;
    mqtt_message_callback callback = config->client.callback;
    void *user_data = config->client.userContext;

    // 解析JSON数据
    json_object *json_data = json_tokener_parse((char *)md->message->payload);
    if (!json_data) {
        printf("Error parsing JSON message\n");
        return;
    }

    // 调用用户自定义回调
    if (callback) {
        callback(md->topicName, json_data, user_data);
    }

    json_object_put(json_data);
}

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
    MQTTClient_setCallbacks(config->client, config, NULL, message_arrived, NULL);

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

// 发布JSON数据
int mqtt_client_publish_json(MQTTClientConfig *config, const char *topic, json_object *json_data, int qos) {
    char *json_str = json_object_to_json_string(json_data);
    MQTTClient_message msg = MQTTClient_message_initializer;
    msg.payload = json_str;
    msg.payloadlen = strlen(json_str);
    msg.qos = qos;
    msg.retained = 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(config->client, topic, &msg, &token);
    free(json_str); // 释放JSON字符串内存
    return rc;
}

// 设置消息回调函数
void mqtt_client_set_message_callback(MQTTClientConfig *config, mqtt_message_callback callback, void *user_data) {
    config->client.callback = callback;
    config->client.userContext = user_data;
}
