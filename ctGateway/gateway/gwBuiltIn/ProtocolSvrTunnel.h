//ProtocolSvrTunnel.h 隧道协议

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "LockedList.h" 
#include "uuid.h" 
#include <thread>
#include <mutex>
#include <condition_variable>
#include "../gwmain/gwconfig.h" 
#include "../interface/IProtocol.h"
#include "mysocket.h" 
#include "ConnectionBase.h"
#include "ConnectionSerialPort.h" 
#include "ConnectionTcp.h" 
#include "DeviceParam.h"
#include "ProtocolParam.h"
#include "ChannelParam.h"

class CTunnelBase : public IProtocol
{
private:
	//连接对象
	IConnection* pConnection = nullptr;

	bool m_bReceivedData = false;//接收到了数据，表明设备有反应

	//接收，不超过ms限制
	void recv_ms(string const& encodeType, int ms, bool once);
	//发送
	void send_recv(string const& encodeType, string const& str);
	//处理下发
	void process_send_command();
protected:
	ProtocolParam_TCP_or_SerialPort m_ProtocolParam;
	DeviceParam_std_all m_DeviceParam;//本协议仅支持一个设备（无法区分设备）
	map<int, TunnelChannelParam*> m_channelparams;
public://IProtocol
	CTunnelBase(char const* code, bool isSerialPort, IConnection* conn) : IProtocol(code, isSerialPort), pConnection(conn) {}
	virtual IProtocolParam* getIProtocolParam()
	{
		return &m_ProtocolParam;
	}
	virtual IDeviceParam* getIDeviceParam(char const* deviceCode)override
	{
		return &m_DeviceParam;
	}
	virtual IChannelParam* getIChannelParam(char const* deviceCode, int channelNo)override
	{
		TunnelChannelParam* ret = new TunnelChannelParam();
		m_channelparams[channelNo] = ret;
		return ret;
	}
	virtual void protocol_process_job()override;
	virtual void protocol_process_timer_job()override {}
	virtual ~CTunnelBase()override
	{}
	virtual bool ProtoSvrInit(InterfaceDeviceConfig* p)override
	{
		m_pInertfaceDevice = p;

		return true;
	}
	virtual bool OnAfterLoadDeviceConfig(Device* pDevice)override { return true; }
	virtual bool ProtoSvrUnInit()override
	{
		//需要首先停止子线程，不过一般设备都是直接断电重启的，所以无所谓

		return true;
	}
};

class CSerialPortTunnelSvr : public CTunnelBase
{
private:
	CSerialPortConnection m_conn;
public:
	static char const* ProtocolCode() { return PROTOCOL_CODE_SERIAL_PORT_TUNNEL; }
	CSerialPortTunnelSvr() :CTunnelBase(ProtocolCode(), true, &m_conn), m_conn(&m_ProtocolParam){}
};

class CTcpTunnelSvr : public CTunnelBase
{
private:
	CTcpConnection m_conn;
public:
	static char const* ProtocolCode() { return PROTOCOL_CODE_TCP_TUNNEL; }
	CTcpTunnelSvr() :CTunnelBase(ProtocolCode(), false, &m_conn), m_conn(&m_ProtocolParam) {}
};
