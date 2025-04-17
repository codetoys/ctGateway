//ConnectionBase.h 基础连接

#pragma once

class InterfaceDeviceConfig;
class IConnection
{
public:
	virtual ~IConnection() {}
	virtual bool IConnection_isConnected() = 0;//返回连接状态
	virtual bool EnsureConnected(InterfaceDeviceConfig* pInterface) = 0;//确认连接，如果未连接则进行连接，返回连接状态
	virtual bool Send(char const* data, int len) = 0;//发送
	virtual bool Recv(char* buf, int len, long* recvLen, int timeout_ms) = 0;//接收
};
