//LocalConfig.cpp 本地配置模块

#include "LocalConfig.h" 
#include "gwconfig.h" 
#include "ProtocolMgr.h" 
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "myhttpd.h"

class CWebCommand_GWConfig :public CWebCommand
{
public:
	CWebCommand_GWConfig()
	{
		SetWebCommand("GWConfig", "GWConfig", "网关配置");
	}

	virtual bool InitWebFunction()override { return true; }
	virtual bool UnInitWebFunction() override { return true; }
	virtual bool doWebFunction(CHttpRequest const* pRequest, CMySocket& s, CHttpRespond* pRespond)override
	{
		pRespond->AppendBody(pRequest->RequestHtmlReport());

		CHtmlDoc::CHtmlTable2 table;

		{
			BaseInfo* pBaseInfo = &g_Global.pCGwConfig->GetBaseInfo();
			pBaseInfo->Update();
			table.AddLine();
			table.AddData("型号");
			table.AddData(pBaseInfo->model);
			//table.AddLine();
			table.AddData("版本");
			table.AddData(pBaseInfo->sw_version);
			table.AddLine();
			table.AddData("序列号");
			table.AddData(pBaseInfo->sn);
			//table.AddLine();
			table.AddData("ICCID");
			table.AddData(pBaseInfo->iccid);
		}

		pRespond->AppendBody(table.MakeHtmlTable());
		return true;
	}
};

class CWebCommand_DeviceManager :public CWebCommand
{
public:
	CWebCommand_DeviceManager()
	{
		SetWebCommand("DeviceManager", "DeviceManager", "设备管理");
	}

	virtual bool InitWebFunction()override { return true; }
	virtual bool UnInitWebFunction() override { return true; }
	virtual bool doWebFunction(CHttpRequest const* pRequest, CMySocket& s, CHttpRespond* pRespond)override
	{
		pRespond->AppendBody(pRequest->RequestHtmlReport());

		CHtmlDoc::CHtmlTable2 table;

		table.SetColFormInput(table.AddCol("设备代码"), 16);
		table.SetColFormInput(table.AddCol("连接"), 16);
		table.SetColFormInput2(table.AddCol("协议"), 16, CProtocolMgr::getAllProtocol());
		table.SetColFormInput(table.AddCol("设备参数", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 16);
		table.AddCol("通道", CHtmlDoc::CHtmlDoc_DATACLASS_HTML);
		table.EnableForm(true, true, true, "");
		for (auto& interface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
		{
			for (auto& device : interface->devices)
			{
				table.AddLine();
				table.AddData(device->deviceCode);
				table.AddData(interface->interfaceCode);
				table.AddData(interface->protocolCode);
				table.AddData(device->deviceParam->ToString());
				char buf[1024];
				sprintf(buf, "<A href=\"/bin/ChannelManager.asp?deviceCode=%s\">打开通道配置</A>", device->deviceCode.c_str());
				table.AddData(buf);
			}
		}

		pRespond->AppendBody(table.MakeHtmlTable());
		return true;
	}
};
class CWebCommand_ChannelManager :public CWebCommand
{
public:
	CWebCommand_ChannelManager()
	{
		SetWebCommand("ChannelManager", "ChannelManager", "通道管理");
		this->AddWebCommandParam("deviceCode", "设备代码", "", "");
	}

	virtual bool InitWebFunction()override { return true; }
	virtual bool UnInitWebFunction() override { return true; }
	virtual bool doWebFunction(CHttpRequest const* pRequest, CMySocket& s, CHttpRespond* pRespond)override
	{
		pRespond->AppendBody(pRequest->RequestHtmlReport());

		string deviceCode = pRequest->GetParam("deviceCode");
		for (auto& interface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
		{
			for (auto& device : interface->devices)
			{
				if (deviceCode != device->deviceCode)continue;

				CHtmlDoc::CHtmlTable2 table;
				table.EnableForm(true, true, true, "");

				device->ToTable(interface->protocolCode, table, true);
				pRespond->AppendBody(table.MakeHtmlTable());
			}
		}

		return true;
	}
};

class CWebCommand_Show :public CWebCommand
{
public:
	CWebCommand_Show() 
	{
		SetWebCommand("show", "show", "动态监视");
		this->SetNotStdPage();
	}

	virtual bool InitWebFunction()override { return true; }
	virtual bool UnInitWebFunction() override { return true; }
	virtual bool doWebFunction(CHttpRequest const* pRequest, CMySocket& s, CHttpRespond* pRespond)override
	{
		CHttpRespond& m_respond = *pRespond;//应答
		CMySocket& m_s = s;//连接

		m_respond.Init();
		m_respond.AddHeaderNoCache();
		m_respond.AddHeaderContentTypeByFilename("*.html");
		m_respond.AppendBodyHtmlStart("show");
		pRespond->AppendBody(pRequest->RequestHtmlReport());

		string str;

		map<string, CHtmlDoc::CHtmlTable2 >  oldtables;

		//第一次输出表格
		for (auto& interface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
		{
			for (auto& device : interface->devices)
			{
				CHtmlDoc::CHtmlTable2& table = oldtables[device->deviceCode];

				device->ToTable(interface->protocolCode, table, false);
				m_respond.AppendBody(table.MakeHtmlTable());
			}
		}
		m_respond.AppendBody("<HR />");

		long count;
		for (count = 0; (count) < 60; ++count)
		{
			for (auto& interface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
			{
				for (auto& device : interface->devices)
				{
					//输出更新脚本
					CHtmlDoc::CHtmlTable2 table;
					device->ToTable(interface->protocolCode, table, false);
					m_respond.AppendBody(table.MakeUpdateScript(&oldtables[device->deviceCode]));
					oldtables[device->deviceCode] = table;
				}
			}

			if (!m_respond.Flush(m_s))
			{
				m_s.Close();
				break;
			}

			SleepSeconds(1);
		}
		m_respond.AppendBody("<script language=\"JavaScript\">window.location.reload();</script>");
		m_respond.AppendBodyHtmlEnd();
		m_respond.Flush(m_s);
		m_s.Close();
		return true;
	}
};
CLocalConfig::CLocalConfig()
{
}

CLocalConfig::~CLocalConfig()
{
	if(m_httpd_thread)pthread_cancel(m_httpd_thread->native_handle());
}

void CLocalConfig::_start_httpd()
{
	if (!CUserManager::getInstPtr()->InitUserManager())
	{
		thelog << "初始化用户管理失败" << ende;
	}

	vector<CWebCommand* >  ws;
	ws.push_back(new CWebCommand());
	ws.push_back(new CWebCommand_Show());
	ws.push_back(new CWebCommand_GWConfig());
	ws.push_back(new CWebCommand_DeviceManager());
	ws.push_back(new CWebCommand_ChannelManager());

	thelog << "start server ..." << endi;
	CServer server;
	server.run(ws, "GateWayConfig", 10000, "./wwwroot/", false);
	thelog << "服务已结束" << endi;
}

void CLocalConfig::start_httpd()
{
	if (!m_httpd_thread)m_httpd_thread = new thread(_start_httpd);
}

void CLocalConfig::worker_job()
{
	int sockfd;
	struct sockaddr_in saddr;
	int r;
	char recvline[1025];
	struct sockaddr_in presaddr;
	socklen_t len;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(9999);
	bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
	while (1)
	{
		r = recvfrom(sockfd, recvline, sizeof(recvline), 0, (struct sockaddr*)&presaddr, &len);
		if (r <= 0)
		{
			thelog<<"recvfrom 失败"<<ende;
			break;
		}
		recvline[r] = 0;
		DEBUG_LOG << "recvfrom " << inet_ntoa(presaddr.sin_addr) << " " << recvline << endi;
	}
	close(sockfd);
}

void CLocalConfig::timer_job()
{
	int sockfd;
	struct sockaddr_in des_addr;
	int r;
	const int on = 1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); //设置套接字选项
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	string to = "255.255.255.255";
	des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址，这个可能只发送给一个网卡（没有收到，不确定原因）
	des_addr.sin_port = htons(9999);
	string msg = "物理广播 " + to;
	r = sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&des_addr, sizeof(des_addr));
	if (r <= 0)
	{
		perror("");
		close(sockfd);
		return;
	}

	struct ifaddrs* ifc, * ifc1;
	char ip[64] = {};
	char nm[64] = {};

	if (0 != getifaddrs(&ifc))
	{
		thelog << "getifaddrs error" << ende;
		return;
	}
	ifc1 = ifc;
	
	//printf("iface\tIP address\tNetmask\n");
	for (; NULL != ifc; ifc = (*ifc).ifa_next)
	{
		//printf("%s", (*ifc).ifa_name);
		if (NULL != (*ifc).ifa_addr)
		{
			inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifc).ifa_addr))->sin_addr), ip, 64);
			//printf("\t%s", ip);
		}
		else
		{
			//printf("\t\t");
		}
		if (NULL != (*ifc).ifa_netmask)
		{
			inet_ntop(AF_INET, &(((struct sockaddr_in*)((*ifc).ifa_netmask))->sin_addr), nm, 64);
			//printf("\t%s", nm);
		}
		else
		{
			//printf("\t\t");
		}
		//printf("\n");

		if (NULL != (*ifc).ifa_netmask)
		{
			//向特定IP段发送
			unsigned int a = *(unsigned int*)&(((struct sockaddr_in*)((*ifc).ifa_addr))->sin_addr);
			unsigned int b = *(unsigned int*)&(((struct sockaddr_in*)((*ifc).ifa_netmask))->sin_addr);
			b = ~b;
			a = a | b;
			des_addr.sin_addr = *(in_addr*)&a;
			inet_ntop(AF_INET, &des_addr.sin_addr, nm, 64);
			DEBUG_LOG << "发送给 " << nm << endi;
			msg = "网段广播 ";msg +=nm;
			r = sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&des_addr, sizeof(des_addr));
			if (r <= 0)
			{
				thelog<<"sendto 失败"<<ende;
			}
		}

	}

	close(sockfd);
	//freeifaddrs(ifc);
	freeifaddrs(ifc1);

}
