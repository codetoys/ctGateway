# ctGateway
IOT gateway C++ OpenSSL GmSSL

## ctGateway
### introduction
The gateway program is implemented in C++,Use the UTF-8 character set.<br>
Support Modbus and transparent transmission, MQTT and MQTTS.<br>
Log files and data files are limited in size to ensure that there is no disk full failure.<br>
The data is saved instantly to ensure not lost data when power lost.<br>
The data sequence number and confirmation mechanism ensure that there is no data loss from the gateway to the platform.<br>
The storage capacity of the network is different depending on the amount of data, and the gateway supports data compression.<br>

### Source
subdirectory ctGateway<br>
Gateway program, C++, Linux.<br>
The sln file can be opened with Visual Studio 2022, but only used to edit file.<br>
How to compile the project see [readme.txt](ctGateway/readme.txt) in subdirectory ctGateway.<br>

### Copyright
Please see the file [LICENSE](./LICENSE) for copyright.<br>

Copyright notices for third-party software used in this software:<br>
See [third-party-LICENSE.txt](./third-party-LICENSE.txt)<br>

## ManageTools
### introduction
Supporting management tools.(a mqtt client)<br>
C# .Net 8.0<br>
The basic functionality depends MQTT only.<br>

The program is for testing purposes only.<br>

## Document
Please see the file [index.html](./index.html) , english and chinese.<br>

# ctGateway 简体中文
物联网网关 C++ OpenSSL GmSSL

## ctGateway
### 介绍
网关程序以C++实现，使用utf-8字符集。<br>
支持Modbus和透传，支持MQTT和MQTTS。<br>
日志文件和数据文件大小受限，确保不出现磁盘满故障。<br>
数据即时存盘，确保断电不丢失。<br>
数据序号和确认机制保证网关到平台不存在数据丢失。<br>
断网存储能力取决于数据量，网关支持数据压缩。<br>

### 源码
子目录 ctGateway<br>
网关程序, C++, Linux.<br>
sln 文件可以用 Visual Studio 2022 打开, 但只用来编辑。<br>
如何编译项目请看子目录ctGateway下的 [readme.txt](ctGateway/readme.txt)。<br>

### 许可证
请看文件 [LICENSE](./LICENSE) 了解许可证信息。<br>

本软件用到的第三方软件的许可证信息:<br>
请看 [third-party-LICENSE.txt](./third-party-LICENSE.txt)<br>

## ManageTools
### 介绍
配套的管理工具。(一个 mqtt 客户端工具)<br>
C# .Net 8.0<br>
基本功能仅仅依赖 MQTT 。<br>

这个程序仅用作测试目的<br>

## 文档
请看文件 [index.html](./index.html) ，英文和中文。<br>
