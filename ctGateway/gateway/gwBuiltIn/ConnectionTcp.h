//ConnectionTcp.h TCP连接

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "ConnectionBase.h" 
#include "mysocket.h" 

class CTcpConnection :public IConnection
{
private:
	CMySocket s;
	ProtocolParam_TCP_or_SerialPort* protocolParam;
public:
	CTcpConnection(ProtocolParam_TCP_or_SerialPort* p) :protocolParam(p) {}
public://IConnection
	virtual ~CTcpConnection() override {}
	virtual bool IConnection_isConnected()override { return s.IsConnected(); }
	virtual bool EnsureConnected(InterfaceDeviceConfig* pInterface)override
	{
		if (NULL == protocolParam)
		{
			thelog << "====================================================无法连接，未提供配置" << endi;
		}
		else if (!s.IsConnected())
		{
			thelog << "====================================================连接 " << protocolParam->ip << ":" << protocolParam->port << endi;
			if (!s.Connect(protocolParam->ip, protocolParam->port, 3))
			{
				thelog << "====================================================连接失败 " << protocolParam->ip << ":" << protocolParam->port << ende;
			}
			else
			{
				thelog << "====================================================连接成功 " << protocolParam->ip << ":" << protocolParam->port << endi;
			}
		}
		return s.IsConnected();
	}
	virtual bool Send(char const* data, int len)override
	{
		if (!s.Send(data, len))
		{
			s.Close();
			return false;
		}
		else
		{
			return true;
		}
	}
	virtual bool Recv(char* buf, int len, long* recvLen, int timeout_ms)override
	{
		*recvLen = 0;

		bool bReady;
		if (!s.IsSocketReadReady_ms(timeout_ms, bReady))
		{
			thelog << "IsSocketReadReady出错" << ende;
			return false;
		}
		if (!bReady)
		{
			DEBUG_LOG << "IsSocketReadReady 没有准备好" << ende;
			return true;
		}
		if (!s.Recv(buf, len, recvLen))
		{
			s.Close();
			return false;
		}
		else
		{
			if (0 == *recvLen)
			{
				s.Close();//对方关闭
			}
			return true;
		}
	}
};
