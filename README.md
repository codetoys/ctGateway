# ctGateway
IOT gateway C++ OpenSSL GmSSL

## ctGateway
### introduction
The gateway program is implemented in C++,Use the UTF-8 character set.<br>
Support Modbus and transparent transmission, MQTT and MQTTS.<br>
Log files and data files are limited in size to ensure that there is no disk full failure.<br>
The data is saved instantly to ensure that the power is not lost.<br>
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
