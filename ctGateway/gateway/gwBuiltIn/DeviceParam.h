//DeviceParam.h 设备参数

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h> 
#include "../interface/IProtocol.h"

struct DLL_PUBLIC DeviceParam_std_all : public IDeviceParam
{
	//通用
	string transferRuleText;//文本的字节顺序AB BA
	string transferRule16;//16位数据的字节顺序AB BA
	string transferRule32;//32位数据的字节顺序ABCD CDAB BADC DCBA

	//DLT645协议
	string deviceAddr;//设备地址，注意，此值同样来自stationCode

	//modbus协议
	int stationCode = 0;//modbus协议的从站号

	//隧道协议
	string encodeType;//隧道协议的编码类型，hex：16进制字符，忽略空格； ascii：字符串直接输出
	string deviceModel;//设备型号

	virtual string _ToString()const override
	{
		stringstream ss;
		ss << "deviceAddr:" << deviceAddr << " stationCode:" << stationCode << " encodeType：" << encodeType << " deviceModel：" << deviceModel;
		return ss.str();
	}
	virtual bool LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_confTabs_item)override;
};
