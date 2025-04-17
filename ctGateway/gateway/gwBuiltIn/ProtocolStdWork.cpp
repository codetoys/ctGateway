//协议处理基础功能

#include "ProtocolStdWork.h"
#include "../gwmain/gwconfig.h"

void CStdWork::StdWork(CStdWork::IStdWork* pIStdWork)
{
	bool isRetry = false;
	while (!_StdWork(isRetry, pIStdWork))
	{
		g_Global.pIPlatform->post_Report("_StdWork 失败", m_pInertfaceDevice->AllDeviceName().c_str());
		isRetry = true;
	}
}
bool CStdWork::_StdWork(bool isRetry, IStdWork* pIStdWork)
{
	//g_Global.pIPlatform->post_Report("开始采集", m_pInertfaceDevice->AllDeviceName().c_str());
	if (isRetry)g_Global.pIPlatform->post_Report("retry", m_pInertfaceDevice->AllDeviceName().c_str());

	//确认连接
	if (!pIStdWork->_EnsureInterfaceConnected())
	{
		g_Global.pIPlatform->post_Report("_EnsureInterfaceConnected 失败", m_pInertfaceDevice->AllDeviceName().c_str());
		SleepSeconds(30);
		return false;
	}
	if (isRetry)g_Global.pIPlatform->post_Report("retry成功", m_pInertfaceDevice->AllDeviceName().c_str());

	stringstream ss;
	bool bMaybeBroke = false;//或许断开，没有读到任何东西并且读失败不为零意味着连接可能断开
	bool bGetAnyData = false;//取到了任何数据，表明没有断开
	for (auto& v : m_pInertfaceDevice->devices)
	{
		Device& device = *v;
		pIStdWork->_SetDevice(&device);

		int countWriteOK = 0;
		int countWriteError = 0;
		int countReadOK = 0;
		int countReadError = 0;
		int countNotThreshold = 0;//因为不达标而不上报
		int countConfigError = 0;//配置错误

		bool bNotThreshold;

		CMyTime t3;//计算一轮操作所用的时间
		t3.SetCurrentTime();

		//初始化一轮采集
		for (Channel* channel : device.channels)
		{
			channel->_changed = false;//清变化标志
			channel->_new_report = false;//清上报标志
		}

		//写
		for (Channel* channel : device.channels)
		{
			if (!channel->CanWrite() || !channel->HasWriteValue())continue;
			if (!pIStdWork->_WriteChannel(&device, channel))
			{
				++countWriteError;
			}
			else
			{
				++countWriteOK;
				g_Global.pIPlatform->post_Report("写入成功", m_pInertfaceDevice->interfaceCode.c_str(), device.deviceCode.c_str(), channel->paramCode.c_str());
			}
		}

		//读
		bool bAll = (device._rate > 0 && (!device._allTime.hasTime() || device._allTime.GetTimeSpanMS() >= device._rate));//是否全读
		if (bAll)device._allTime.SetCurrentTime();

		set<string> events;

		//处理变化触发全部上传事件
		if (device._events.size() > 0)
		{
			bAll = true;
			events.insert(device._events);
			device._events = "";
		}

		DEBUG_LOG << (bAll ? "全部采集" : "部分采集") << endi;

		bool bFastOK = false;
		{
			bFastOK = pIStdWork->_FastReadAllChannel(&device, countReadOK, bAll);//非全采仅读一个保持连接
			if (!bFastOK)countReadOK = 0;//清除计数，后面还要重新采集
		}
		if (!bAll || !bFastOK)
		{
			for (Channel* channel : device.channels)
			{
				if (!channel->CanRead())continue;
				//读操作判断是否到达上报时间
				if (!bAll)
				{
					//非变化上报检查是否到达间隔时间，仅需要判断独立设置的collectorRate，统一的间隔已经通过bAll处理，依赖上报仅在依赖项上报时处理
					if (ReportStrategy_DEPEND == channel->reportStrategy.reportStrategy)
					{
						continue;
					}
					else if (!channel->reportStrategy.needCheckChanged())
					{
						if (channel->collectorRate <= 0)continue;//未单独定义
						if (channel->HasValue() && channel->GetValueTime().GetTimeSpanMS() < channel->collectorRate)
						{
							continue;
						}
						//thelog << channel.HasValue() << " " << channel.GetValueTime().GetTimeSpanMS() << " " << channel.collectorRate << ende;
					}
				}

				bNotThreshold = false;
				if (!pIStdWork->_ReadChannel(bAll, &device, channel, bNotThreshold))
				{
					++countReadError;
				}
				else
				{
					++countReadOK;
					if (channel->_changed)events.insert(channel->paramCode);

					if (bNotThreshold)++countNotThreshold;
				}
			}
			//关联上报
			for (Channel* channel : device.channels)
			{
				if (ReportStrategy_DEPEND == channel->reportStrategy.reportStrategy)
				{
					Channel* pDep = device.FindByChannelNo(channel->collectorRate);
					if (!pDep)
					{
						thelog << "依赖的通道不存在 " << channel->channelNo << " " << channel->collectorRate << ende;
						++countConfigError;
					}
					else if (pDep->_new_report)
					{
						events.insert(pDep->paramCode);
						//thelog << "依赖的通道值：" << pDep->_value.ChannelValue_ToStringUTF8(pDep->dataType, pDep->charsetCode) << endi;
						if (!pIStdWork->_ReadChannel(true, &device, channel, bNotThreshold))
						{
							++countReadError;
						}
						else
						{
							++countReadOK;
						}
					}
					else
					{
						//thelog << "依赖的通道值未变化：" << pDep->_value.ChannelValue_ToStringUTF8(pDep->dataType, pDep->charsetCode) << endi;
					}
				}
			}
		}

		//上报数据
		for (Channel* channel : device.channels)
		{
			if (!channel->_new_report)continue;
			bool isNumber = ("string" != channel->dataType);
			g_Global.pIPlatform->AddPushData((long)m_pInertfaceDevice, device.deviceCode.c_str(), channel->channelNo, channel->paramCode.c_str(), channel->paramName.c_str()
				, channel->GetValueTime().GetTS_ms(),
				isNumber, (isNumber ? channel->GetValue().ChannelValue_ToDouble(channel->dataType) : 0)
				, (!isNumber ? channel->GetValue().ChannelValue_ToStringUTF8(channel->dataType, channel->charsetCode).c_str() : "")
				, 0, "");
		}
		if (bAll || g_Global.pIPlatform->GetDataCount((long)m_pInertfaceDevice) > 0)
		{
			string events_str;
			for (auto& e : events)
			{
				if (events_str.size() != 0)events_str += " ";
				events_str += e;
			}
			g_Global.pIPlatform->post_DataMessage((long)m_pInertfaceDevice, device.deviceCode.c_str(), bAll, events_str, t3.GetTimeSpanMS());
		}

		//设备状态检查
		if (0 == countReadOK && countReadError > 0)
		{
			g_Global.pIPlatform->post_Report("设备似乎无响应", m_pInertfaceDevice->interfaceCode.c_str(), device.deviceCode.c_str());
			device._DeviceState = DeviceState::CONNECT_ERROR;
			bMaybeBroke = true;
		}
		if (countReadOK > 0)
		{
			device._DeviceState = DeviceState::CONNECT_OK;
			bGetAnyData = true;
		}
		ss << device.deviceCode << " 写成功 " << countWriteOK << " 写失败 " << countWriteError << " 读成功 " << countReadOK << " 读失败 " << countReadError << " 未达标 " << countNotThreshold
			<< " 配置错误 " << countConfigError << " 耗时 " << t3.GetTimeSpanUS() << " 微秒 " << (bAll ? "全部采集" : "部分采集") << (bFastOK ? "快速采集" : "逐个采集") << endl;
	}

	//g_Global.pIPlatform->post_Report(ss.str().c_str(), m_pInertfaceDevice->interfaceCode.c_str());
	DEBUG_LOG << ss.str() << endi;
	if (!bGetAnyData && bMaybeBroke)
	{
		g_Global.pIPlatform->post_Report("连接状态似乎异常，将断开重新连接", m_pInertfaceDevice->AllDeviceName().c_str());
		pIStdWork->_DisconnectInterface();
		for (auto& v : m_pInertfaceDevice->devices)
		{
			v->_allTime.clear();
		}
		return false;
	}
	return true;
}
