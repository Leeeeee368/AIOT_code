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
	// unsigned char temp[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};//温湿度指令

	//CO数据修改
	// unsigned char temp[8] = {0x01, 0x06, 0x01, 0x00, 0x00, 0x02, 0x08, 0xF6};//CO换id
	// unsigned char temp[8] = {0x02, 0x06, 0x01, 0x01, 0x00, 0x02, 0x58, 0x04};//CO换比特率
	// unsigned char temp[8] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38};//CO指令

	//光照数据修改
	// unsigned char temp[8] = {0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0xC8, 0x37};//光照换id
	// unsigned char temp[8] = {0x03, 0x06, 0x01, 0x01, 0x00, 0x02, 0x59, 0xD5};//光照换比特率
	unsigned char temp[8] = {0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8};	//光照度指令

	//继电器
	unsigned char relay[4][8] = {
		{0x04, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8D, 0xAA},
		{0x04, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCC, 0x5A},
		{0x04, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDC, 0x6A},
		{0x04, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9D, 0x9A}
	};
	// unsigned char temp[8] = {0x04, 0x05, 0x00, 0x10, 0xFF, 0x00, 0x8D, 0xAA};//继电器1开
	// unsigned char temp[8] = {0x04, 0x05, 0x00, 0x10, 0x00, 0x00, 0xCC, 0x5A};//继电器1关
	// unsigned char temp[8] = {0x04, 0x05, 0x00, 0x11, 0xFF, 0x00, 0xDC, 0x6A};//继电器1开
	// unsigned char temp[8] = {0x04, 0x05, 0x00, 0x11, 0x00, 0x00, 0x9D, 0x9A};//继电器1关

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

	while(1)
	{
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(fd, &readfds);
		FD_SET(fd, &writefds);
		memset(buf,'\0',sizeof(buf));//清空缓冲区
		if (select(fd+1, &readfds, &writefds, NULL, &timeout) > 0){
			if (FD_ISSET(fd, &writefds)) {
				write(fd, temp, 8);
				printf("success\n");
			}
			usleep(1000000);
			if (FD_ISSET(fd, &readfds)) {
				int ret = read(fd, buf, 9);//主机要获得传感器的数据
				for(int i = 0; i < 9; i++){
					sprintf(str + i*2, "%02x", buf[i]);//i*2”是因为每个字符需要2个字符的空间来存储16进制数（格式化）。
				}
				printf("%s\n", str);
				usleep(1000000);
			}
		}

		// if (FD_ISSET(fd, &writefds)) {
		// 	for(int i=0;i<4;i++){
		// 		write(fd, relay[i], 8);
		// 		usleep(1000000);
		// 	}
		// 	printf("success\n");
		// }


		FD_SET(fd, &readfds);
	}

	return 0;
}
