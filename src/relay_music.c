#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>


//串口初始化
int serial_init(char* device, int baudrate)
{
	int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	struct termios oldtio,newtio;
	// 设置串口属性
	tcgetattr(fd,&oldtio);
	bzero(&newtio,sizeof(newtio));
	newtio.c_cflag = baudrate|CS8|CLOCAL|CREAD;
	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cflag &= ~PARENB;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	tcflush(fd,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	tcgetattr(fd,&oldtio);

	return fd;
}


int main(int argc, char *argv[]) {
	int rc, fd;
	//读地址
	// unsigned char temp[8] = {0xFF, 0x03, 0x01, 0x00, 0x00, 0x01, 0x90, 0x28};//温湿度指令
	//恢复出厂设置
	// unsigned char temp[8] = {0xFF, 0x06, 0x10, 0x00, 0x00, 0xAA, 0x18, 0xAB};//温湿度指令

	//温湿度数据修改
	// unsigned char temp[8] = {0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0xC8, 0x37};//温湿换id
	// unsigned char temp[8] = {0x03, 0x06, 0x01, 0x01, 0x00, 0x02, 0x59, 0xD5};//温湿换比特率
	// unsigned char temp[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x08};//温湿度指令

	//CO数据修改
	// unsigned char temp[8] = {0x01, 0x06, 0x01, 0x00, 0x00, 0x02, 0x08, 0xF6};//CO换id
	// unsigned char temp[8] = {0x02, 0x06, 0x01, 0x01, 0x00, 0x02, 0x58, 0x04};//CO换比特率
	// unsigned char temp[8] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38};//CO指令

	//光照数据修改
	// unsigned char temp[8] = {0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0xC8, 0x37};//光照换id
	// unsigned char temp[8] = {0x03, 0x06, 0x01, 0x01, 0x00, 0x02, 0x59, 0xD5};//光照换比特率
	// unsigned char temp[8] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38};//光照度指令
	// unsigned char temp[8] = {0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8};	//光照度指令

	//继电器
	unsigned char relay_1[2][8] = {
		{0x04, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8D, 0xAA},	//继电器1的1路开
		{0x04, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCC, 0x5A},	//继电器1关1路关
	};

	unsigned char relay_2[2][8] = {
		{0x04, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDC, 0x6A},	//继电器1开2路开
		{0x04, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9D, 0x9A}	//继电器1开2路关
	};

	unsigned char relay_3[2][8] = {
		{0x05, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8C, 0x7B},	//继电器2的1路开
		{0x05, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCD, 0x8B}	//继电器2的1路关
	};

	unsigned char relay_4[2][8] = {
		{0x05, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDD, 0xBB},	//继电器2开2路开
		{0x05, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9C, 0x4B}	//继电器2开2路关
	};

	unsigned char buf[256] = {0};
	unsigned char str[100] = {0};
	fd_set readfds, writefds;
	struct timeval timeout={0,50};

	//打开串口（RS485）
	fd = serial_init("/dev/ttyS1", B4800);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	while(1) {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(fd, &readfds);
		FD_SET(fd, &writefds);
		memset(buf, '\0', sizeof(buf));

		if (select(fd+1, &readfds, &writefds, NULL, &timeout) > 0) {
			if (FD_ISSET(fd, &writefds)) {
				// 定义每拍时长（500ms，可根据实际节奏调整）
				const int BEAT_TIME = 500000;  // 500ms = 500000us
				const int HALF_BEAT = BEAT_TIME / 2;  // 250ms

				// 演奏4个小节
				for (int measure = 0; measure < 4; measure++) {
					for (int beat = 1; beat <= 4; beat++) {
						// --------------------- 底鼓（继电器1） ---------------------
						if (beat == 1) {
							write(fd, relay_1[0], 8);  // 第一拍开
							usleep(HALF_BEAT);
						} else {
							write(fd, relay_1[1], 8);  // 其他拍关
							usleep(HALF_BEAT);
						}

						// --------------------- 军鼓（继电器2） ---------------------
						if (beat == 3) {
							write(fd, relay_2[0], 8);  // 第三拍开
							usleep(HALF_BEAT);
						} else {
							write(fd, relay_2[1], 8);  // 其他拍关
							usleep(HALF_BEAT);
						}

						// --------------------- 镲片（继电器3） ---------------------
						// 前半拍关，后半拍开
						write(fd, relay_3[1], 8);  // 关（前半拍）
						usleep(HALF_BEAT);
						write(fd, relay_3[0], 8);  // 开（后半拍）
						usleep(HALF_BEAT);

						// --------------------- 贝斯（继电器4） ---------------------
						if (beat == 1) {
							write(fd, relay_4[0], 8);  // 第一拍开，持续两拍（覆盖beat=1和beat=2）
							// 贝斯持续到下一拍结束，这里通过延迟处理（beat=1时多延迟一个节拍）
							if (beat == 1) {
								usleep(BEAT_TIME);  // 额外延迟一拍，保持开启
								continue;  // 跳过后续beat=1的普通延迟
							}
						} else {
							write(fd, relay_4[1], 8);  // 其他拍关
						}

						// 普通节拍延迟（贝斯在beat=1时已处理额外延迟，此处不重复）
						if (beat != 1) {
							usleep(BEAT_TIME);
						}
					}
				}

				printf("节奏段落演奏完成\n");
			}
		}
		FD_SET(fd, &readfds);
	}

	return 0;
}
