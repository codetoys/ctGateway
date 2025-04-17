ctGateway 网关程序

sln和vcxproj适用于Visual Studio 2022，仅供修改文件。
编译环境：Linux（以下基于Ubuntu 18.04）

开发时home下的文件需要放在运行环境的/home下（运行时设备自带）
sudo cp * /home/

将源码的gateway、lib、third三个目录传至my目录下（如果目录不同，下面的脚本需要相应修改）
依赖库：
openssl-1.1.1k
zlib-1.2.11
mosquitto-2.0.18
libmodbus-rtu-over-tcp

ulimit -c unlimited
export PATH=.:$PATH
export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
cd
cd my/third/ctfc/platform
make -f makefile.mk linux debug
cd ../src
chmod 755 *.sh
makeall.sh
cd ../../../gateway
chmod 755 *.sh
makeall.sh

Ubuntu设置时区：
timedatectl set-timezone Asia/Shanghai

Ubuntu测试重启需要用sudo启动程序或脚本
使用串口需要权限
sudo chmod 777 /dev/ttyS0

MQTT本地安装
命令行下进入解压路径，启动 EMQX
cd c:\emqx
./bin/emqx start
