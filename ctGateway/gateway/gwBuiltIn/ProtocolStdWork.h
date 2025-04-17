//协议处理基础功能

#pragma once

#include "WorkThread.h"

class InterfaceDeviceConfig;
class CMqttSvr;
class Device;
class Channel;
class IProtocolParam;
class IDeviceParam;

class CStdWork
{
private:
	InterfaceDeviceConfig* m_pInertfaceDevice;
public:
	CStdWork(InterfaceDeviceConfig* p) :m_pInertfaceDevice(p) {}

	//标准工作处理过程的接口
	class IStdWork
	{
	public:
		//确认Interface已经连接，未连接则连接
		virtual bool _EnsureInterfaceConnected() = 0;
		virtual void _DisconnectInterface() = 0;//m_mobus.UnInitModbus();
		virtual bool _SetDevice(Device* pDevice) = 0;//m_mobus.SetSlave(device.deviceParam.stationCode);
		//写入
		virtual bool _WriteChannel(Device* pDevice, Channel* pChannel) = 0;
		//采集单个通道并处理变化
		virtual bool _ReadChannel(bool must, Device* pDevice, Channel* pChannel, bool& bNotThreshold) = 0;
		virtual bool _FastReadAllChannel(Device* pDevice, int& countReadOK, bool bReally) = 0;
	};
	//标准工作处理过程
	void StdWork(IStdWork* pIStdWork);
private:
	//先处理写，后处理读，保证写指令最快速被处理，如果处理失败返回false
	bool _StdWork(bool isRetry, IStdWork* pIStdWork);
};