//DeviceParam.cpp 设备参数

#include "DeviceParam.h"
#include "../gwmain/gwconfig.h"

bool DeviceParam_std_all::LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_confTabs_item)
{
	bool ret = true;

	//不同协议的设备参数
	if (PROTOCOL_CODE_TCP_TUNNEL == pInterface->protocolCode || PROTOCOL_CODE_SERIAL_PORT_TUNNEL == pInterface->protocolCode
		|| PROTOCOL_CODE_TCP_DEMO == pInterface->protocolCode || PROTOCOL_CODE_SERIAL_PORT_DEMO == pInterface->protocolCode)
	{
		if (!CJsonUtil::_GetJsonParam(cjson_confTabs_item, "encodeType", encodeType))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_confTabs_item, "deviceModel", deviceModel))ret = false;
		if (deviceModel.size() > 0)
		{
			if (!AllDeviceModel::isInDeviceModels(deviceModel.c_str()))
			{
				thelog << "解析设备配置出错，未识别的设备型号 " << deviceModel << ende;
				ret = false;
			}
		}
	}
	//else if (PROTOCOL_CODE_DLT645_1997 == pInterface->protocolCode || PROTOCOL_CODE_DLT645_2007 == pInterface->protocolCode)
	//{
	//	if (!CJsonUtil::_GetJsonParam(cjson_confTabs_item, "stationCode", deviceAddr))ret = false;//注意，deviceAddr来自stationCode
	//}
	else
	{
		if (!CJsonUtil::_GetJsonParam(cjson_confTabs_item, "stationCode", stationCode))ret = false;
	}

	return ret;
}
