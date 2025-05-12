#include "RS485.h"
#include "sensor.h"
#include "mqtt_client.h"
#include <pthread.h>

/*DEBUG
    0 open
    1 close
*/
#ifndef DEBUG
    #define DEBUG 1
#endif

#ifndef IS_HUAWEI
    #define IS_HUAWEI 0
#endif

int main(int argc, char **argv) {
    int opt;
    char * json_data;
    int port = MQTT_DEFAULT_PORT;

#if IS_HUAWEI == 0
    char *server_ip         = "127.0.0.1"; // 默认本地IP
    const char *topic       = "/RK3588S/SENSOR/";
    const char *rev_topic   = "/FARM/CTRL/";
#else
    char *server_ip         = "0aa59c9052.st1.iotda-device.cn-north-4.myhuaweicloud.com"; // 默认本地IP
    const char *topic       = "$oc/devices/{681c12959314d11851199ea9}/sys/properties/report";
#endif

    if (argc != 2)
    {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return -1;
    }
    server_ip = (char *)argv[1];    // 参数1，mqtt服务器ip

    // 构造MQTT服务器URI
    char server_uri[128];
    snprintf(server_uri, sizeof(server_uri), "tcp://%s:%d", server_ip, port);

#if IS_HUAWEI == 0
    // 初始化客户端配置
    MQTTClientConfig config = {
        .server_uri = server_uri,
        .client_id = "aiot_client",
        .username = NULL, // 如需认证请填写
        .password = NULL, // 如需认证请填写
        .port = port,
        .keepalive = MQTT_DEFAULT_KEEPALIVE
    };
#else
    MQTTClientConfig config = {
        .server_uri = server_uri,
        .client_id = "681c12959314d11851199ea9_1234_0_0_2025050803",
        .username = "681c12959314d11851199ea9_1234", // 如需认证请填写
        .password = "b7a6b7f6a5d9a2db28be4003db2a03442fcf6a9afa4f5f474289788d23b0cde2", // 如需认证请填写
        .port = port,
        .keepalive = MQTT_DEFAULT_KEEPALIVE
    };
#endif

    // 初始化MQTT客户端
    if (mqtt_client_init(&config) != MQTTCLIENT_SUCCESS) {
        return 1;
    }

    mqtt_client_set_callback(&config, print_received_message); // 设置回调

    printf("尝试连接的服务器: %s\n", server_uri);

    // 连接到服务器
    if (mqtt_client_connect(&config) != MQTTCLIENT_SUCCESS) {
        mqtt_client_disconnect(&config);
        return 1;
    }

#if IS_HUAWEI == 0
    // 订阅主题（检查返回值）
    if (mqtt_client_subscribe(&config, topic, 0) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "订阅主题 %s 失败\n", topic);
        mqtt_client_disconnect(&config);
        return 1;
    }
    if (mqtt_client_subscribe(&config, rev_topic, 0) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "订阅主题 %s 失败\n", rev_topic);
        mqtt_client_disconnect(&config);
        return 1;
    }
#endif
    // 保持程序运行以接收消息（阻塞循环）
    printf("MQTT client running. Press Ctrl+C to exit.\n");

    if (init_serial() < 0) return -1;

    // 初始化传感器数据对象
    sensor_data_init();

    pthread_t recv_tid, send_tid;
    pthread_create(&recv_tid, NULL, rs485_recv_thread, NULL);
    pthread_create(&send_tid, NULL, rs485_sned_thread, NULL);
    // char send_cmd[3][8] = {
    //     {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B},
    //     {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38},
    //     {0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8}
    // };
    while (1) {
        usleep(500000);

    #if IS_HUAWEI == 0
        json_data = pack_sensor_to_json();
    #else
        json_data = pack_sensor_to_huaweicloud();
    #endif

    #if DEBUG == 0
        printf("json_data%s\n",json_data);
    #endif

        if(mqtt_client_publish_json(&config, topic, json_data, 0) != 0)
            printf("mqtt_send_json is fail!!!\n");
    }

    close(fd);
    pthread_join(recv_tid, NULL);
    pthread_join(send_tid, NULL);
    // 断开连接（实际不会执行到这里，需通过信号处理退出）
    mqtt_client_disconnect(&config);
    return 0;
}
