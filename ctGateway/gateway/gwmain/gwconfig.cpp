//gwconfig.cpp 配置文件

#include "gwconfig.h" 
#include "ProtocolMgr.h" 

bool CGwConfig::_LoadChannelList(InterfaceDeviceConfig* pInterface, Device* pDevice, cJSON* jsonChannelList)
{
	int resolveParamConfigVOList_count = cJSON_GetArraySize(jsonChannelList);
	pDevice->channels.reserve(resolveParamConfigVOList_count);
	pDevice->channels.resize(resolveParamConfigVOList_count);

	for (int i = 0; i < (int)pDevice->channels.size(); ++i)
	{
		pDevice->channels[i] = new Channel(pInterface->_ProtocolSvr->getIProtocol()->getIChannelParam(pDevice->deviceCode.c_str(), i));
	}

	bool ret = true;
	for (int k = 0; k < resolveParamConfigVOList_count; ++k)
	{
		Channel* pChannel = pDevice->channels[k];
		cJSON* cjson_resolveParamConfigVOList_item = cJSON_GetArrayItem(jsonChannelList, k);

		pChannel->channelNo = k;
		
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "paramCode", pChannel->paramCode))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "paramName", pChannel->paramName))ret = false;
	
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "dataType", pChannel->dataType))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "dataLength", pChannel->dataLength))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "processType", pChannel->processType))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "reportStrategy", pChannel->reportStrategy.reportStrategy))ret = false;

		CJsonUtil::_TryGetJsonParam(cjson_resolveParamConfigVOList_item, "transferRuleStr", pChannel->transferRuleStr);
		CJsonUtil::_TryGetJsonParam(cjson_resolveParamConfigVOList_item, "associationParamCode", pChannel->dependParamCode);

		//加载自定义参数
		if (!pChannel->pChannelParam->LoadConfig(pInterface->protocolCode, cjson_resolveParamConfigVOList_item))ret = false;
		
		//if (PROTOCOL_CODE_DLT645_1997 == pInterface->protocolCode || PROTOCOL_CODE_DLT645_2007 == pInterface->protocolCode)
		//{
		//	pChannel->dataType = "string";//数据类型统一为string
		//}
	}

	//处理依赖关系
	for (Channel* c : pDevice->channels)
	{
		if (ReportStrategy_DEPEND == c->reportStrategy.reportStrategy)
		{
			bool bOK = false;
			for (Channel* tmp : pDevice->channels)
			{
				if (tmp->paramCode == c->dependParamCode)
				{
					c->collectorRate = tmp->channelNo;
					bOK = true;
					break;
				}
			}
			if (!bOK)
			{
				ret = false;
				thelog << c->paramCode << " 依赖的通道未找到 [" << c->dependParamCode << "]" << ende;
			}
		}
	}

	thelog << "_LoadChannelList 返回 " << ret << endi;
	return ret;
}
bool CGwConfig::_LoadDeviceConfig()
{
	string filename = deviceConfigFile;
	CEasyFile file;
	string str;
	if (!file.ReadFile(filename.c_str(), str))
	{
		thelog << "未能打开设备配置文件 " << filename << " 使用默认配置" << ende;
		filename = defaultDeviceConfigFile;
		if (!file.ReadFile(filename.c_str(), str))
		{
			thelog << "未能打开默认设备配置文件 " << filename << ende;
			return false;
		}
	}

	cJSON* cjson = cJSON_Parse(str.c_str());

	if (NULL == cjson)
	{
		thelog << "解析设备配置出错 " << filename << endl << str << ende;
		return false;
	}
	bool ret = true;
	cJSON* cjson_data = cJSON_GetObjectItem(cjson, "data");
	if (NULL == cjson_data)
	{
		thelog << "解析设备配置出错，没有data项 " << ende;
		ret = false;
	}
	else
	{
		int data_count = cJSON_GetArraySize(cjson_data);
		m_DeviceConfig.interfaces.reserve(data_count);
		for (int i = 0; i < data_count; ++i)
		{
			cJSON* cjson_data_item = cJSON_GetArrayItem(cjson_data, i);

			int ignore = 0;
			if (CJsonUtil::_TryGetJsonParam(cjson_data_item, "ignore", ignore) && 1 == ignore)continue;

			string protocolCode;
			if (!CJsonUtil::_GetJsonParam(cjson_data_item, "protocolCode", protocolCode))ret = false;
			ProtocolSvr * pProtocolSvr = CProtocolMgr::CreateProtocolSvr(protocolCode.c_str());
			if (NULL == pProtocolSvr)
			{
				thelog << "解析设备配置出错，未知的protocolCode " << protocolCode << ende;
				ret = false;
				continue;
			}
			else
			{
				thelog << "protocolCode " << protocolCode << endi;
			}
			
			InterfaceDeviceConfig* pInterface = new InterfaceDeviceConfig();
			m_DeviceConfig.interfaces.push_back(pInterface);
			pInterface->_ProtocolSvr = pProtocolSvr;
			pInterface->protocolCode = protocolCode;

			CJsonUtil::_TryGetJsonParam(cjson_data_item, "interfaceCode", pInterface->interfaceCode);//不一定存在，可能被串口协议用作串口名

			cJSON* cjson_resolvePropertyMap = cJSON_GetObjectItem(cjson_data_item, "resolvePropertyMap");
			if (NULL == cjson_resolvePropertyMap)
			{
				thelog << "解析设备配置出错，没有resolvePropertyMap项 " << ende;
				ret = false;
				continue;
			}
			else
			{
				IProtocolParam* protocolParam = pInterface->_ProtocolSvr->getIProtocol()->getIProtocolParam();
				if (!CJsonUtil::_GetJsonParam(cjson_resolvePropertyMap, "rate", protocolParam->rate))ret = false;
				ret = protocolParam->LoadConfig(pInterface, cjson_resolvePropertyMap);//加载自定义协议参数
			}

			cJSON* cjson_confTabs = cJSON_GetObjectItem(cjson_data_item, "confTabs");
			if (NULL == cjson_confTabs)
			{
				thelog << "解析设备配置出错，没有confTabs项 " << ende;
				ret = false;
				continue;
			}
			else
			{
				int confTabs_count = cJSON_GetArraySize(cjson_confTabs);
				pInterface->devices.reserve(confTabs_count);
				for (int j = 0; j < confTabs_count; ++j)
				{

					cJSON* cjson_confTabs_item = cJSON_GetArrayItem(cjson_confTabs, j);

					string deviceCode;
					if (!CJsonUtil::_GetJsonParam(cjson_confTabs_item, "deviceCode", deviceCode))
					{
						thelog << "解析设备配置出错，deviceCode未定义" << ende;
						ret = false;
						continue;
					}
					if (m_DeviceConfig.FindDevice(deviceCode.c_str()))
					{
						thelog << "解析设备配置出错，设备已经存在 " << deviceCode << ende;
						ret = false;
						continue;
					}

					Device* pDevice = new Device(pInterface->_ProtocolSvr->getIProtocol()->getIDeviceParam(deviceCode.c_str()));
					pInterface->devices.push_back(pDevice);

					pDevice->deviceCode = deviceCode;

					//加载设备参数
					if (!(ret = pInterface->_ProtocolSvr->getIProtocol()->getIDeviceParam(pDevice->deviceCode.c_str())->LoadConfig(pInterface, cjson_confTabs_item)))
					{
						continue;
					}

					//通道列表
					cJSON* cjson_resolveParamConfigVOList = cJSON_GetObjectItem(cjson_confTabs_item, "resolveParamConfigVOList");
					if (NULL == cjson_resolveParamConfigVOList)
					{
						thelog << "解析设备配置出错，没有resolveParamConfigVOList项 " << ende;
						ret = false;
						continue;
					}
					else
					{
						ret = _LoadChannelList(pInterface, pDevice, cjson_resolveParamConfigVOList);
						//thelog << pDevice->ToString() << endi;
					}
				}
			}
		}
	}

	cJSON_free(cjson);
	return ret;
}

bool CGwConfig::_LoadCSV()
{
	//遍历加载每个设备
	for (auto& i : m_DeviceConfig.interfaces)
	{
		if (PROTOCOL_CODE_TCP_TUNNEL == i->protocolCode || PROTOCOL_CODE_SERIAL_PORT_TUNNEL == i->protocolCode)continue;

		for (auto& d : i->devices)
		{
			if (!i->_ProtocolSvr->getIProtocol()->OnAfterLoadDeviceConfig(d))
			{
				return false;
			}
		}
	}

	return true;
}

bool CGwConfig::PrepareInterfaceDevice()
{
	for (auto& i : m_DeviceConfig.interfaces)
	{
		InterfaceDeviceConfig* m_pInertfaceDevice = i;
		IProtocolParam* protocolParam = m_pInertfaceDevice->_ProtocolSvr->getIProtocol()->getIProtocolParam();

		//计算整个接口的最低采集间隔
		protocolParam->_rate = protocolParam->rate;
		for (auto& v : m_pInertfaceDevice->devices)
		{
			auto& d = *v;
			if (protocolParam->_rate <= 0)protocolParam->_rate = d.rate;
			else if (d.rate > 0 && d.rate < protocolParam->_rate)protocolParam->_rate = d.rate;
			else;

			//如果设备没有设置采集间隔，使用整个接口的。设备不一定需要定时采集
			if (d.rate <= 0)d._rate = protocolParam->rate;
			else d._rate = d.rate;

			for (auto const& c : d.channels)
			{
				if (ReportStrategy_CYCLE == c->reportStrategy.reportStrategy)
				{
					if (protocolParam->_rate <= 0)protocolParam->_rate = c->collectorRate;
					else if (c->collectorRate > 0 && c->collectorRate < protocolParam->_rate)protocolParam->_rate = c->collectorRate;
					else;
				}
				if (c->reportStrategy.needCheckChanged())
				{
					//存在实时采集
					if (protocolParam->_rate <= 0)
					{
						protocolParam->_rate = REALTIME_RATE;
						//d._rate = REALTIME_RATE;
					}
					else if (REALTIME_RATE < protocolParam->_rate)
					{
						protocolParam->_rate = REALTIME_RATE;
						//d._rate = REALTIME_RATE;
					}
					else;
				}
			}

			if (protocolParam->_rate > 10 * 1000)protocolParam->_rate = 10 * 1000;
		}
		if (protocolParam->_rate <= 0)protocolParam->_rate = DEFAULT_RATE;
		thelog << "最低采集间隔 " << protocolParam->_rate << " 毫秒" << endi;

	}
	return true;
}
void CGwConfig::ReAcquire()
{
	for (auto& i : m_DeviceConfig.interfaces)
	{
		for (auto& d : i->devices)
		{
			d->_allTime.clear();
		}
		if (i->_ProtocolSvr)i->_ProtocolSvr->ProtoSvrActive();
	}
}

string InterfaceDeviceConfig::ToString(bool withDetail)const
{
	stringstream ss;
	IProtocol* pIProtocol = _ProtocolSvr->getIProtocol();
	ss << "protocolCode:" << protocolCode << " interfaceCode:" << interfaceCode << " _InterfaceState:" << (int)_InterfaceState << endl;
	ss << "协议驱动 " << pIProtocol->m_DriverFile << " (" << pIProtocol->m_ProtocolCode << ") " << (pIProtocol->m_isSerialPort ? "串口" : "网口") << endl;
	ss << "协议参数 " << pIProtocol->getIProtocolParam()->ToString() << endl;
	ss << "共有设备 " << devices.size() << " 个：" << endl;
	for (auto& v : devices)
	{
		ss << v->ToString(protocolCode, withDetail) << endl;
	}

	return ss.str();
}
