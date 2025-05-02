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
