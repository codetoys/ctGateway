//gwconfig.h 配置文件

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h> 
#include "htmldoc.h" 
#include "gwdefine.h"
#include "CharSetConv.h"
#include "../interface/gwUtil.h"
#include "../interface/gwValue.h"
#include "gwAppConfig.h"
#include "../interface/gwChannel.h"
#include "ProtocolSvr.h"
#include "../interface/IProtocol.h"

//请使用全局变量g_Global.pCGwConfig->

constexpr char const* defaultNorthConfigFile = "defaultNorthConfig.json";
constexpr char const* NorthConfigFile = "northConfig.json";

constexpr char const* defaultDeviceConfigFile = "defaultDeviceConfig.json";
constexpr char const* deviceConfigFile = "deviceConfig.json";

constexpr char const* prepareShellFile = "prepare.sh";//这个脚本由程序写入，在程序启动前被执行，用于放网关的IP等配置

//基本信息
struct BaseInfo
{
	string sn;//序列号
	string model;//型号
	string sw_version;//软件版本
	string latitude;
	string longitude;
	string serial_port_name;//默认串口名，同型号是固定的，不同型号不一样，如果有多个一般是顺序编号

	string GetSerialPortName(char const* serial_port)
	{//对4G网关可能需要针对性处理
		if (NULL != serial_port && strlen(serial_port) > 0)return serial_port;
		else return serial_port_name;
	}

	//动态信息
	string iccid;//SIM卡号
	string csq;//信号强度
	bool Update()
	{
		char const* sn_file_name = "/home/sysInfo.txt";
		CEasyFile file;
		string str;
		if (!file.ReadFile(sn_file_name, str))
		{
			thelog << "未能打开文件 " << sn_file_name << ende;
			return false;
		}
		StringTokenizer st(str, ",");

		for (size_t i = 0; i < st.size(); ++i)
		{
			StringTokenizer st2(st[i], ":");
			if (st2.size() < 2)continue;
			if (st2[0] == "iccid")iccid = st2[1];
			if (st2[0] == "csq")csq = st2[1];
		}

		Trim(iccid, "\"");
		Trim(csq, "\"");

		thelog << "ICCID [" <<iccid << "] CSQ[" << csq << "]" << endi;
		return true;
	}
};
//北向配置
struct NorthConfig
{
	string type;
	string platform;
	string hostname;
	int port;
	string userid;
	string upwd;
	string ntp;
};
class CGwConfig
{
public:
	class DeviceConfig
	{
	public:
		vector<InterfaceDeviceConfig* > interfaces;
		string m_config_error;//配置错误信息

		Device* FindDevice(char const* deviceCode, InterfaceDeviceConfig** pInterface = NULL)
		{
			for (auto &i : interfaces)
			{
				if(pInterface)*pInterface = i;
				for (auto &d : i->devices)
				{
					if (deviceCode == d->deviceCode)return d;
				}
			}
			return NULL;
		}
		string ToString(bool withDetail = true)const
		{
			stringstream ss;
			ss << "共有接口 " << interfaces.size() << " 个：" << endl;
			for (auto& v : interfaces)
			{
				ss << v->ToString(withDetail) << endl;
			}
			ss << "配置错误：" << endl << m_config_error;

			return ss.str();
		}
	};
private:
	BaseInfo m_BaseInfo;

	CGwAppConfig m_AppConfig;
	NorthConfig m_NorthConfig;
	DeviceConfig m_DeviceConfig;

	bool _LoadSN(int retryCount = 0)
	{
		char const* sn_file_name = "/home/sn.txt";
		CEasyFile file;
		string str;

		if (!file.ReadFile(sn_file_name, str))
		{//4G版
			m_BaseInfo.model = "GW-4G";
			m_BaseInfo.sn = "UnknownSN";
		}
		else
		{//5G版和虚拟机，格式为“type:型号,imei:序列号”
			StringTokenizer st(str, ",");

			for (size_t i = 0; i < st.size(); ++i)
			{
				StringTokenizer st2(st[i], ":");
				if (st2.size() < 2)continue;
				if (st2[0] == "type")m_BaseInfo.model = st2[1];
				if (st2[0] == "imei")m_BaseInfo.sn = st2[1];
			}
		}

		Trim(m_BaseInfo.model, "\" \t\r\n");
		Trim(m_BaseInfo.sn, "\" \t\r\n");
		
		if (m_BaseInfo.model == "VM")m_BaseInfo.serial_port_name = "/dev/ttyS0";
		else if (m_BaseInfo.model == "GW-5G") m_BaseInfo.serial_port_name = "/dev/ttySE1";
		else if (m_BaseInfo.model == "GW-4G") m_BaseInfo.serial_port_name = "/dev/ttyS1";
		else m_BaseInfo.serial_port_name = "需要指定";

		thelog << str << " 型号 [" << m_BaseInfo.model << "] SN[" << m_BaseInfo.sn << "] 默认串口[" << m_BaseInfo.serial_port_name << "]" << endi;
		if (0 == m_BaseInfo.sn.size())
		{
			if (retryCount > 100)
			{
				thelog << "硬件启动尚未完成，超过最大重试次数，删除日志文件重启" << ende;
				return false;
			}
			else
			{
			thelog << "硬件启动尚未完成" << endi;
				SleepSeconds(5);
				return _LoadSN(retryCount + 1);
			}
		}

		m_BaseInfo.Update();
		return true;
	}
	bool _LoadNorthConfig()
	{
		string filename = NorthConfigFile;
		CEasyFile file;
		string str;
	
		if (!g_Global.bUseDefaultNorthConfig)
		{
			if (!file.ReadFile(filename.c_str(), str))
			{
				thelog << "未能打开平台配置文件 " << filename << " 使用默认配置" << endi;
				str = "";
				g_Global.bUseDefaultNorthConfig = true;
			}
		}
		else
		{
			thelog << "忽略平台配置，使用默认配置" << endi;
		}

		if (0 == str.size())
		{
			filename = defaultNorthConfigFile;
			if (!file.ReadFile(filename.c_str(), str))
			{
				thelog << "未能打开默认平台配置文件 " << filename << ende;
				return false;
			}
		}

		cJSON* cjson = cJSON_Parse(str.c_str());

		if (NULL == cjson)
		{
			thelog << "解析平台配置出错 " << filename << endl << str << ende;
			return false;
		}
		bool ret = true;
		cJSON* cjson_data = cJSON_GetObjectItem(cjson, "data");
		if (NULL == cjson_data)
		{
			thelog << "解析平台配置出错，没有data项 " << ende;
			ret = false;
		}
		else
		{
			if (!CJsonUtil::_GetJsonParam(cjson_data, "type", m_NorthConfig.type))ret = false;
			if (!CJsonUtil::_GetJsonParam(cjson_data, "hostname", m_NorthConfig.hostname))ret = false;
			if (!CJsonUtil::_GetJsonParam(cjson_data, "port", m_NorthConfig.port))ret = false;
			if (!CJsonUtil::_GetJsonParam(cjson_data, "userid", m_NorthConfig.userid))ret = false;
			if (!CJsonUtil::_GetJsonParam(cjson_data, "upwd", m_NorthConfig.upwd))ret = false;
			CJsonUtil::_GetJsonParam(cjson_data, "ntp", m_NorthConfig.ntp);
			CJsonUtil::_GetJsonParam(cjson_data, "platform", m_NorthConfig.platform);
		}

		cJSON_free(cjson);
		return ret;
	}
	//加载通道列表,jsonChannelList是数组
	bool _LoadChannelList(InterfaceDeviceConfig* pInterface, Device* pDevice, cJSON* jsonChannelList);
	/*
	设备配置文件utf-8编码
	变化的部分：
	confTabs[]增加deviceCode，设备名称，该名称也用于加载MCGS导出文件
	*/
	bool _LoadDeviceConfig();

	bool _LoadCSV();

public:
	bool LoadConfig(string sw_version)
	{
		m_BaseInfo.sw_version = sw_version;

		if (m_AppConfig.LoadAppConfig() && _LoadSN() && _LoadNorthConfig())
		{
			theLog.setCache(1024 * 1024);
			theLog.setOutput(false);
			
			_LoadDeviceConfig();//设备配置错误仍然需要运行，以便通过平台修复
			bool ret = _LoadCSV();
			
			theLog.GetCachedLog(this->m_DeviceConfig.m_config_error);
			theLog.ClearCache();
			thelog.setOutput(true);
			thelog << "=========================================================" << endi;
			thelog << endl << m_DeviceConfig.ToString(false) << endi;
			
			return ret;
		}
		else
		{
			return false;
		}
	}
	bool PrepareInterfaceDevice();

	string const& GetSn()const { return m_BaseInfo.sn; }
	BaseInfo & GetBaseInfo() { return m_BaseInfo; }
	AppConfig& GetAppConfig() { return m_AppConfig.m_AppConfig; }
	NorthConfig const& GetNorthConfig()const { return m_NorthConfig; }
	DeviceConfig& GetDeviceConfig() { return m_DeviceConfig; }

	long GetMaxDataFile() { return m_AppConfig.m_AppConfig.max_data_file_size; }
	
	T_DATA_SEQ GetConfirmedSeq() { return m_AppConfig.m_RuntimeData.confirmed_data_sequence; }
	void SetConfirmedSeq(T_DATA_SEQ value) { m_AppConfig.m_RuntimeData.confirmed_data_sequence = value; m_AppConfig.m_RuntimeData.Save(); }
	
	T_DATA_SEQ GetLastDataSeq() { return m_AppConfig.m_RuntimeData.last_data_sequence; }
	void SetLastDataSeq(T_DATA_SEQ value) { m_AppConfig.m_RuntimeData.last_data_sequence = value; m_AppConfig.m_RuntimeData.Save();}
	T_DATA_SEQ Add_last_data_sequence() { m_AppConfig.m_RuntimeData.Add_last_data_sequence(); m_AppConfig.m_RuntimeData.Save(); return m_AppConfig.m_RuntimeData.last_data_sequence; }

	void SetLastSendedSeq(T_DATA_SEQ value) { m_AppConfig.m_RuntimeData.last_sended_seq = value; m_AppConfig.m_RuntimeData.Save(); }
	T_DATA_SEQ GetLastSendedSeq() { return m_AppConfig.m_RuntimeData.last_sended_seq; }

	T_DATA_SEQ NextSeq(T_DATA_SEQ seq){return RuntimeData::NextSeq(seq);}
	T_DATA_SEQ PrevSeq(T_DATA_SEQ seq) { return RuntimeData::PrevSeq(seq); }

	//重新采集一遍
	void ReAcquire();

};
