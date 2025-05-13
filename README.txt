socat -d -d pty,raw,echo=0 pty,raw,echo=0

安装json-c

安装EMQX

1.配置 EMQX Apt 源
curl -s https://assets.emqx.com/scripts/install-emqx-deb.sh | sudo bash

2.安装 EMQX
sudo apt-get install emqx

3.启动 EMQX
sudo systemctl start emqx

安装node-red

bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)

Any errors will be logged to   /var/log/nodered-install.log

安装paho.mqtt

git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
mkdir build
cd build
cmake .. -DPAHO_WITH_SSL=ON -DPAHO_BUILD_DOCUMENTATION=OFF -DPAHO_BUILD_SAMPLES=ON
make
sudo make install
sudo ldconfig

使用方法

进入src/main.cc文件，在重定义处选择是否开启华为云，日志打印
进入./build-linux_RK3588.sh文件

上node-red就留着
cd ./install/AIOT_demo_Linux && ./AIOT_demo 192.168.1.196

上华为云就留着
cd ./install/AIOT_demo_Linux && ./AIOT_demo "0aa59c9052.st1.iotda-device.cn-north-4.myhuaweicloud.com"

然后退出，到./build-linux_RK3588.sh同级目录
终端输入命令：./build-linux_RK3588.sh 程序将自行构建并运行
