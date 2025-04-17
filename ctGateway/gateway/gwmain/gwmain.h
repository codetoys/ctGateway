//gwmain.h 网关主控对象

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "gwconfig.h"
#include "PlatformCT.h"
#include "MqttSvr.h"
#include "gwdefine.h"
#include "../interface/IProtocol.h"
#include "DataManager.h"
#include "LedControl.h"
#include "LocalConfig.h"
#include <list>

class CGwMain
{
private:
	CGwConfig m_GwConfig;
	CCTPlatform m_CTPlatform;//CT平台
	CDataManager m_CDataManager;
	CLedControl m_LedControl;
	CLocalConfig m_LocalConfig;

public:
	bool GwMainInit(char const* sw_version)
	{
		//设置全局对象
		g_Global.pCGwConfig = &m_GwConfig;
		g_Global.pIPlatform = &m_CTPlatform;
		g_Global.pCDataManager = &m_CDataManager;

		g_Global.satrt_time = time(NULL);
		g_Global.last_platform_connection_recv = time(NULL);
		g_Global.last_platform_connection_send = time(NULL);

		if (!g_Global.pCGwConfig->LoadConfig(sw_version))
		{
			thelog << "加载配置失败" << ende;
			return false;
		}
		g_Global.pCGwConfig->PrepareInterfaceDevice();
		//thelog << "配置测试，加载配置完成，停止程序" << endi;
		//exit(0);
	
		m_CDataManager.CDataManagerInit();

		//启动平台处理
		m_CTPlatform.InitPlatform();

		//启动数采处理
		for (auto& inertface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
		{
			if (inertface->_ProtocolSvr)
			{
				inertface->_ProtocolSvr->getIProtocol()->ProtoSvrInit(inertface);
				inertface->_ProtocolSvr->StartProtocolSvr();
			}
		}

		//启动Led
		m_LedControl.m_WorkerThread.StartWorkerThread(&m_LedControl,"LedControl", 1000);
		//启动本地配置模块
		m_LocalConfig.m_WorkerThread.StartWorkerThread(&m_LocalConfig,"LocalConfig", 1000 * 10);
	
		//启动附带的web服务
		SleepSeconds(5);//等配置解析完成，不然后面的日志会混入配置报错信息
		m_LocalConfig.start_httpd();

		return true;
	}
	void GwMainUnInit()
	{
		m_LedControl.m_WorkerThread.StopWorkerThread();

		for (auto& inertface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
		{
			inertface->_ProtocolSvr->getIProtocol()->ProtoSvrUnInit();
			delete inertface->_ProtocolSvr;
			inertface->_ProtocolSvr = nullptr;
		}
		m_CTPlatform.UninitPlatform();
	}
};
