//ProtocolParam.cpp 协议参数

#include "ProtocolParam.h"
#include "../gwmain/gwconfig.h"

bool ProtocolParam_TCP_or_SerialPort::LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_resolvePropertyMap)
{
	bool ret = true;
	if (pInterface->_ProtocolSvr->isSerialPort())
	{
		CJsonUtil::_TryGetJsonParam(cjson_resolvePropertyMap, "serial_port", serial_port);
		//修正串口号，如果没有指定，使用默认值
		serial_port = g_Global.pCGwConfig->GetBaseInfo().GetSerialPortName(serial_port.c_str());

		if (0 == pInterface->interfaceCode.size())
		{
			pInterface->interfaceCode = serial_port;
		}
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "baudRate", baud))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "parity", parity))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "dataBit", data_bit))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "stopBit", stop_bit))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "timeout", timeout_s))ret = false;

		if ('0' == parity)parity = 'N';
		else if ('1' == parity)parity = 'O';
		else if ('2' == parity)parity = 'E';
	}
	else
	{
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "ip", ip))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "port", port))ret = false;
		if (0 == pInterface->interfaceCode.size())
		{
			char buf[256];
			sprintf(buf, "%s:%d", ip.c_str(), port);
			pInterface->interfaceCode = buf;
		}
	}

	return ret;
}
