#include "RS485.h"
#include "sensor.h"
#include <ctype.h>

int fd;
pthread_mutex_t rs485_mutex = PTHREAD_MUTEX_INITIALIZER;

/* RS485模式配置 */
void set_rs485_mode(int enable_tx) {
    struct serial_rs485 rs485_conf = {0};
    rs485_conf.flags |= SER_RS485_ENABLED;
    if (enable_tx) {
        rs485_conf.flags |= SER_RS485_RTS_ON_SEND; // 发送时拉高RTS
    }
    ioctl(fd, TIOCSRS485, &rs485_conf);
}

/* 串口初始化 */
int init_serial(void) {
    fd = open(RS485_DEV, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Open serial failed");
        return -1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetispeed(&tty, BAUDRATE);
    cfsetospeed(&tty, BAUDRATE);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8数据位
    tty.c_cflag &= ~PARENB;                        // 无校验
    tty.c_cflag &= ~CSTOPB;                        // 1停止位
    tty.c_cc[VMIN] = 1;                            // 至少读取1字节
    tcsetattr(fd, TCSANOW, &tty);

    set_rs485_mode(0); // 初始化为接收模式
    return 0;
}

/* CRC16校验函数（MODBUS RTU标准） */
static uint16_t crc16_modbus(const unsigned char *data, int len) {
    uint16_t crc = 0xFFFF;
    int i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* 16进制转10进制 */
static int hexToDecimal(const char *hex) {
    int decimal = 0;
    while (*hex) {
        char c = tolower(*hex++);
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else {
            printf("错误：非法16进制字符 '%c'\n", c);
            return -1;
        }
        decimal = decimal * 16 + digit;
    }
    return decimal;
}

/* 将buf里的16进制数据转字符串 */
static void hex_to_string(const char *buf, int len, char *hex_str) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_str + i * 2, "%02x", (unsigned char)buf[i]);
    }
    hex_str[len * 2] = '\0';
    printf("Received %d bytes: %s\n", len, hex_str);
}

/* 拼接字符串提取温湿度 */
static void concat_strings(const char *hex_str, char *str_sd, char *str_wd) {
    strncpy(str_sd, hex_str + 6, 4);  // 假设湿度数据从第7个字符开始（0-based索引6）
    str_sd[4] = '\0';
    strncpy(str_wd, hex_str + 10, 4); // 温度从第11个字符开始
    str_wd[4] = '\0';
    printf("sd: %s, wd:%s\n", str_sd, str_wd);
}

/* 字符串转浮点型 */
static void str_to_float(const char *str_sd, const char *str_wd, float *sd, float *wd) {
    *sd = (float)hexToDecimal(str_sd) / 10;
    *wd = (float)hexToDecimal(str_wd) / 10;
    printf("sd:%.1f, wd:%.1f\n", *sd, *wd);
}

// 在 rs485.c 或主程序中
void *rs485_recv_thread(void *arg) {
    RS485_Frame frame;
    char Json_str_rev[256] = {0};
    while (1) {
        memset(frame.raw_data, 0, sizeof(frame.raw_data));
        usleep(100000); // 调整睡眠周期
        // 读取完整数据帧（含CRC校验）
        int len = read(fd, frame.raw_data, sizeof(frame.raw_data));
        if (len < 5) continue;  // 至少需要设备号+功能码+数据+2字节CRC
        // hex_to_string(frame.raw_data, len, Json_str_rev); // 打印原始16进制数据
        // frame.frame_len = len;
        // CRC校验（假设frame.raw_data包含完整数据帧）
        uint16_t recv_crc = (frame.raw_data[len-2] << 8) | frame.raw_data[len-1];
        uint16_t calc_crc = crc16_modbus(frame.raw_data, len-2);

        if (recv_crc == calc_crc) {
            // 数据校验通过，分发设备处理
            handle_sensor_data(frame.raw_data, len);
        } else {
            printf("CRC校验失败，丢弃数据\n");
        }
    }
    return NULL;
}
