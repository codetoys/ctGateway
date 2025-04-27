# ctGateway (Engish-Chinese 英文中文对照)
IOT gateway C++ OpenSSL GmSSL Linux Arm/Arm64
Open Source , Modular and Scalable

物联网网关 C++ OpenSSL GmSSL Arm/Arm64
开源 模块化 可扩展

## ctGateway
### introduction 介绍
The gateway program is implemented in C++,Use the UTF-8 character set.<br>
Support Modbus and transparent transmission, MQTT and MQTTS.<br>
Log files and data files are limited in size to ensure that there is no disk full failure.<br>
The data is saved instantly to ensure not lost data when power lost.<br>
The data sequence number and confirmation mechanism ensure that there is no data loss from the gateway to the platform.<br>
The storage capacity of the network is different depending on the amount of data, and the gateway supports data compression.<br>

网关程序以C++实现，使用utf-8字符集。<br>
支持Modbus和透传，支持MQTT和MQTTS。<br>
日志文件和数据文件大小受限，确保不出现磁盘满故障。<br>
数据即时存盘，确保断电不丢失。<br>
数据序号和确认机制保证网关到平台不存在数据丢失。<br>
断网存储能力取决于数据量，网关支持数据压缩。<br>

### Source 源码
subdirectory ctGateway<br>
Gateway program, C++, Linux.<br>
The sln file can be opened with Visual Studio 2022, but only used to edit file.<br>
How to compile the project see [readme.txt](ctGateway/readme.txt) in subdirectory ctGateway.<br>

子目录 ctGateway<br>
网关程序, C++, Linux.<br>
sln 文件可以用 Visual Studio 2022 打开, 但只用来编辑。<br>
如何编译项目请看子目录ctGateway下的 [readme.txt](ctGateway/readme.txt)。<br>

### Copyright 许可证
Please see the file [LICENSE](./LICENSE) for copyright.<br>

Copyright notices for third-party software used in this software:<br>
See [third-party-LICENSE.txt](./third-party-LICENSE.txt)<br>

请看文件 [LICENSE](./LICENSE) 了解许可证信息。<br>

本软件用到的第三方软件的许可证信息:<br>
请看 [third-party-LICENSE.txt](./third-party-LICENSE.txt)<br>

## ManageTools
### introduction 介绍
Supporting management tools.(a mqtt client)<br>
C# .Net 8.0<br>
The basic functionality depends MQTT only.<br>

The program is for testing purposes only.<br>

配套的管理工具。(一个 mqtt 客户端工具)<br>
C# .Net 8.0<br>
基本功能仅仅依赖 MQTT 。<br>

这个程序仅用作测试目的<br>

## Document 文档
Please see the file [index.html](https://codetoys.github.io/ctGateway/index.html) , English and Chinese.<br>
请看文件 [index.html](https://codetoys.github.io/ctGateway/index.html) ，英文和中文。<br>

### Quick Guide 快速指南
1. Quick Guide 编译和运行<br>
Linux or Linux virtual machine, MQTT broker(server), [Developer Guide](https://codetoys.github.io/ctGateway/doc/en/ctGateway_DeveloperGuide.htm)<br>
Linux 或 Linux 虚拟机, MQTT 代理(服务器), [开发指南](https://codetoys.github.io/ctGateway/doc/zh/ctGateway_DeveloperGuide.htm)<br>
2. North Config MQTT config 北向配置 MQTT配置<br>
defaultNorthConfig.json:
```json
{
	"func": "connt",
	"data": {
		"comment": "本机测试",
		"type": "MQTTS",
		"platform": "localtest",
		"hostname": "mqtt.test.com",
		"port": "8883",
		"userid": "user",
		"upwd": "user1234",
		"ntp": "time.windows.com"
	}
}
```
3. Using Modbus 使用Modbus
```json
{
  "func": "conf",
  "data": [
    {
      "protocolCode": "MODBUS_TCP",
      "protocolName": "ModbusTcp协议",
      "resolvePropertyMap": {
        "port": "10503",
        "rate": "30000",
        "ip": "127.0.0.1"
      },
      "confTabs": [
        {
          "deviceCode": "device503",
          "stationCode": "1",
          "resolveParamConfigVOList": [
            {
              "paramCode": "code1",
              "paramName": "工单规格",
              "dataType": "ushort",
              "paramAddr": "0001",
              "functionCode": "03",
              "transferRuleStr": "",
              "dataLength": 1,
              "processType": "3",
              "reportStrategy": 1
            },
            {
              "paramCode": "code2",
              "paramName": "负值",
              "dataType": "short",
              "paramAddr": "0002",
              "functionCode": "03",
              "transferRuleStr": "",
              "dataLength": 1,
              "processType": "3",
              "reportStrategy": 1
            },
            {
              "paramCode": "codef1",
              "paramName": "浮点",
              "dataType": "float",
              "paramAddr": "0004",
              "functionCode": "03",
              "transferRuleStr": "ABCD",
              "dataLength": 1,
              "processType": "3",
              "reportStrategy": 1
            }
          ]
        }
      ]
    }
  ]
}
```
4. Using Tunnel Protocol(Transparent transmission) 使用隧道协议（透传）
```json
{
  "func": "conf",
  "data": [
    {
      "protocolCode": "TCP_TUNNEL",
      "protocolName": "TCP隧道协议",
      "resolvePropertyMap": {
        "ip": "192.168.136.1",
        "port": "503",
        "rate": "30000"
      },
      "confTabs": [
        {
          "deviceCode": "deviceTcpTunnel",
          "encodeType": "hex",
          "resolveParamConfigVOList": [
            {
              "paramCode": "read",
              "paramName": "从0开始读取10个保持寄存器",
              "paramCommand": "00 06 00 00 00 06 01 03 00 00 00 0A"
            }
          ]
        }
      ]
    },
    {
      "protocolCode": "SERIAL_PORT_TUNNEL",
      "protocolName": "串口隧道协议",
      "resolvePropertyMap": {
        "rate": "30000",
        "serial_port": "/dev/ttyS0",
        "baudRate": 115200,
        "parity": "N",
        "dataBit": 8,
        "stopBit": 1,
        "timeout": 1
      },
      "confTabs": [
        {
          "deviceCode": "deviceSerialPortTunnel",
          "encodeType": "hex",
          "resolveParamConfigVOList": [
            {
              "paramCode": "read",
              "paramName": "从0开始读取10个保持寄存器",
              "paramCommand": "01 03 00 00 00 0A C5 CD "
            }
          ]
        }
      ]
    }
  ]
}
```
5. Using Custom Protocol 使用定制协议
gateway/gwprotocol/ProtocolDemo.h .cpp
QJ57B_1A is an instrument for measuring the resistance of a cable.<br>
QJ57B_1A 是一款测量电缆电阻的仪器.<br>
```json
{
  "func": "conf",
  "data": [
    {
      "protocolCode": "TCP_DEMO",
      "protocolName": "TCP demo协议",
      "resolvePropertyMap": {
        "ip": "192.168.213.1",
        "port": "2200",
        "rate": "30000"
      },
      "confTabs": [
        {
          "deviceCode": "QJ57B_1A",
          "encodeType": "hex",
          "deviceModel": "QJ57B_1A"
        }
      ]
    }
  ]
}
```
6. More protocols 更多协议<br>
Some of the adapted protocols will be available in an extended manner<br>
部分适配过的协议将以扩展的方式提供<br>
DLT645,Siemens S7,OPC-UA<br>
