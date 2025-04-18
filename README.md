# ctGateway
IOT gateway C++ OpenSSL GmSSL

## ctGateway
### introduction
The gateway program is implemented in C++,Use the UTF-8 character set.
Support Modbus and transparent transmission, MQTT and MQTTS.
Log files and data files are limited in size to ensure that there is no disk full failure.
The data is saved instantly to ensure that the power is not lost.
The data sequence number and confirmation mechanism ensure that there is no data loss from the gateway to the platform.
The storage capacity of the network is different depending on the amount of data, and the gateway supports data compression.

### Source
subdirectory ctGateway
Gateway program, C++, Linux.
The sln file can be opened with Visual Studio 2022, but only used to edit file.
How to compile the project see [readme.txt](ctGateway/readme.txt) in subdirectory ctGateway.

### Copyright
Please see the file [LICENSE](./LICENSE) for copyright.

Copyright notices for third-party software used in this software:
See [third-party-LICENSE.txt](./third-party-LICENSE.txt)
