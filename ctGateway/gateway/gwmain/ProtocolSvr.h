//协议处理基础功能

#pragma once

#include "WorkThread.h"
#include <interface/IProtocol.h>

class ProtocolSvr : private IWorkThread
{
private://IWorkThread
	virtual void worker_job()override { pIProtocol->protocol_process_job(); }
	virtual void timer_job()override { pIProtocol->protocol_process_timer_job(); }
	CWorkerThread m_WorkThread;
private:
	IProtocol* pIProtocol;
public:
	ProtocolSvr(IProtocol * p):pIProtocol(p){}
	IProtocol* getIProtocol()
	{
		return pIProtocol;
	}
	string getProtocolCode()const { return pIProtocol->m_ProtocolCode; }
	bool isSerialPort()const { return pIProtocol->m_isSerialPort; }
	//启动
	bool StartProtocolSvr();
	//激活
	void ProtoSvrActive()
	{
		m_WorkThread.ActiveWorkerThread();
	}

};