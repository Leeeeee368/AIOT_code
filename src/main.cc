#include "RS485.h"
#include "sensor.h"
#include <pthread.h>

int main() {
    if (init_serial() < 0) return -1;

    // 初始化传感器数据对象
    // sensor_data_init();

    pthread_t recv_tid;
    // pthread_create(&recv_tid, NULL, rs485_recv_thread, NULL);

    char send_cmd[8] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38}; // 温湿度查询指令

    while (1) {
        // send_data(send_cmd);
        write(fd, send_cmd, 8);
        sleep(1); // 每秒发送一次
    }

    close(fd);
    return 0;
}
