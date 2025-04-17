//ProtocolParam.h 协议参数

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h> 
#include "../interface/IProtocol.h"

struct DLL_PUBLIC ProtocolParam_TCP_or_SerialPort : IProtocolParam
{
	//TCP
	string ip;
	int port = 0;

	//串口
	string serial_port;
	int baud = 0;
	char parity = 'N';//N 无奇偶校验 E 偶数校验 O 奇数校验
	int data_bit = 8;
	int stop_bit = 1;
	int timeout_s = 1;//响应时间，秒，默认为1秒

	virtual string _ToString()const override
	{
		stringstream ss;
		if (ip.size() > 0)	ss << " ip:" << ip << " port:" << port;
		if (serial_port.size() > 0)	ss << " serial_port:" << serial_port << " baud:" << baud << " parity:" << _ParityStr()
			<< " data_bit:" << data_bit << " stop_bit:" << stop_bit << " timeout_s:" << timeout_s;
		ss << endl;

		return ss.str();
	}
	char const* _ParityStr()const
	{
		if ('N' == parity)return "N 无校验";
		if ('E' == parity)return "E 偶校验";
		if ('O' == parity)return "O 奇校验";
		else return "错误";
	}

	virtual bool LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_resolvePropertyMap)override;
};
