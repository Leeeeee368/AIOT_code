#include "RS485.h"
#include "sensor.h"
#include <ctype.h>

int fd;
pthread_mutex_t rs485_mutex = PTHREAD_MUTEX_INITIALIZER;

//继电器
char relay_1[2][8] = {
	{0x04, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8D, 0xAA},	//继电器1的1路开
	{0x04, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCC, 0x5A},	//继电器1关1路关
};

char relay_2[2][8] = {
	{0x04, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDC, 0x6A},	//继电器1开2路开
	{0x04, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9D, 0x9A}	//继电器1开2路关
};

char relay_3[2][8] = {
	{0x05, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8C, 0x7B},	//继电器2的1路开
	{0x05, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCD, 0x8B}	//继电器2的1路关
};

char relay_4[2][8] = {
	{0x05, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDD, 0xBB},	//继电器2开2路开
	{0x05, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9C, 0x4B}	//继电器2开2路关
};

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

    // 关闭回显功能
    tty.c_lflag &= ~(ECHO | ECHOE);

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
    // char rev_data[9] = {0x01, 0x03, 0x04, 0x02, 0xA6, 0x00, 0xEA, 0x27, 0x9A};
    char str[100] = {0};
    memset(frame.raw_data, 0, sizeof(MAX_FRAME_LEN));
    char Json_str_rev[256] = {0};
    while (1) {
        memset(frame.raw_data, 0, MAX_FRAME_LEN);
        // 读取完整数据帧（含CRC校验）
        usleep(1000000);
        int len = read(fd, frame.raw_data, MAX_FRAME_LEN);
        printf("working\n");
        if (len < 5) {
            printf("no rev--%d\n",len);
            for(int i = 0; i < 9; i++){
					sprintf(str + i*2, "%02x", frame.raw_data[i]);//i*2”是因为每个字符需要2个字符的空间来存储16进制数（格式化）。
				}
			printf("%s\n", str);
            // usleep(100000); // 调整睡眠周期
            continue;  // 至少需要设备号+功能码+数据+2字节CRC
        }
        for(int i = 0; i < len; i++){
				sprintf(str + i*2, "%02x", frame.raw_data[i]);//i*2”是因为每个字符需要2个字符的空间来存储16进制数（格式化）。
			}
		printf("%s\n", str);
        printf("********************\n");
        // 在接收线程中添加调试打印（确认数据接收）
        hex_to_string((char *)frame.raw_data, len, Json_str_rev); // 恢复被注释的打印

        // CRC校验（假设frame.raw_data包含完整数据帧）
        uint16_t recv_crc = (frame.raw_data[len-1] << 8) | frame.raw_data[len-2];
        uint16_t calc_crc = crc16_modbus((uint8_t *)frame.raw_data, len-2);

        printf("接收数据长度: %d\n", len);
        printf("CRC接收: 0x%04X 计算: 0x%04X\n", recv_crc, calc_crc);

        if (recv_crc == calc_crc) {
            // 数据校验通过，分发设备处理
            handle_sensor_data((uint8_t *)frame.raw_data, len);
        } else {
            printf("CRC校验失败，丢弃数据\n");
        }
    }
    return NULL;
}

void light_control(int state) {
    printf("[执行] 灯光状态设置为: %d\n", state);
    switch (state)
    {
        case 0:
            send_data(relay_1[0]);
            break;

        case 1:
            send_data(relay_1[1]);
            break;

        default:
            break;
    }
    // 这里添加实际硬件控制代码
}

void window_control(int state) {
    printf("[执行] 窗户状态设置为: %d\n", state);
    switch (state)
    {
        case 0:
            send_data(relay_2[0]);
            break;

        case 1:
            send_data(relay_2[1]);
            break;

        default:
            break;
    }
    // 这里添加实际硬件控制代码
}
