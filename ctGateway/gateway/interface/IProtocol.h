//协议处理基础功能

#pragma once

#include "config.h"
#include "LockedList.h"
#include <string>
#include <sstream>
#include <cJSON/cJSON.h>
#include "gwChannel.h"

class InterfaceDeviceConfig;
class IProtocolParam;
class IDeviceParam;
class ProtocolSvr;
class Device;

struct IProtocolParam
{
	//自定义参数的输出
	virtual std::string _ToString()const { return ""; }
	//从配置json中加载
	virtual bool LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_resolvePropertyMap) { return true; }

	//通用
	int rate = 0;//每多少毫秒采集一次，此值不能高于所有通道设置的值

	int _rate = 0;//实际使用的值，由数采服务设置
	std::string ToString()const
	{
		std::stringstream ss;
		ss << " rate:" << rate << " _rate:" << _rate << std::endl;
		ss << _ToString() << std::endl;

		return ss.str();
	}
};

class IDeviceParam
{
public:
	//自定义参数的输出
	virtual std::string _ToString()const { return ""; }
	//从配置json中加载
	virtual bool LoadConfig(InterfaceDeviceConfig* pInterface, cJSON* cjson_confTabs_item) { return true; }

	std::string ToString()const
	{
		std::stringstream ss;
		ss << _ToString() << std::endl;

		return ss.str();
	}
};

//协议公共功能
class DLL_PUBLIC CProtocolPublic
{
public:
	static std::string GetSN();
	static bool isPlatformConnected();
	static bool post_Message(char const* func, char const* message);
	static void AddPushData(long conn, char const* deviceCode, int channelNo, char const* channelCode, char const* channelName, int64_t ts, bool isNumber, double numberValue, char const* strValue, int errorCode, char const* errorMessage);
	static bool post_DataMessage(long conn, char const* deviceCode, bool isAll, std::string events, long ms);
	static bool post_ReportWithFunc(char const* func, char const* message, char const* _interface = nullptr, char const* _device = nullptr, char const* _channel = nullptr);
};

//协议接口
class IProtocol
{
public:
	std::string m_ProtocolCode;//协议代码
	std::string m_DriverFile;//驱动文件名
	bool m_isSerialPort;
	InterfaceDeviceConfig* m_pInertfaceDevice = nullptr;
public:
	IProtocol(char const* code, bool isSerialPort) :m_ProtocolCode(code), m_isSerialPort(isSerialPort) {}
	//返回协议参数接口指针供框架调用
	virtual IProtocolParam* getIProtocolParam() = 0;
	//返回设备参数接口指针供框架调用，每个协议服务对象实例可能包含多个设备
	virtual IDeviceParam* getIDeviceParam(char const* deviceCode) = 0;
	//返回通道参数接口指针供框架调用，每个设备包含多个通道
	virtual IChannelParam* getIChannelParam(char const* deviceCode, int channelNo) = 0;
	//初始化
	virtual bool ProtoSvrInit(InterfaceDeviceConfig* p) = 0;
	//在设备配置加载之后执行
	virtual bool OnAfterLoadDeviceConfig(Device* pDevice) = 0;
	//卸载
	virtual bool ProtoSvrUnInit() = 0;
	//析构
	virtual ~IProtocol() {}
	//工作函数，由定时器激活，如果函数内部自己做循环不会退出，激活动作没有实际影响
	virtual void protocol_process_job() = 0;
	//定时器函数，在激活工作线程之前执行
	virtual void protocol_process_timer_job() = 0;
private:
	map<string, map<int, IChannelParam*> > m_device_channelparams;
};

extern "C"
{
	//协议驱动提供此接口以便主程序获得协议对象指针
	DLL_PUBLIC IProtocol* CreateIProtocol(char const* ProtocolCode);
}

//写入指令
struct WriteInfo
{
	string uid;//下发指令的标识
	int channelNo = -1;//通道号，对隧道协议无意义
	string value;//要写入的值，对隧道协议是要写入的完整数据
};

enum class DeviceState { DEFAULT = 0, CONNECT_OK, CONNECT_ERROR };

//设备
struct Device
{
	Device(IDeviceParam* p) :deviceParam(p) {}

	string deviceCode;//设备的代码，唯一标识一个设备，跨越接口的唯一
	int rate = 0;//每多少毫秒采集一次，此值不能高于所有通道设置的值
	IDeviceParam* deviceParam;//设备参数，与协议相关

	vector<Channel*> channels;//设备的通道
	LockedList<WriteInfo > writeList;//待写入的列表
	DeviceState _DeviceState = DeviceState::DEFAULT;

	int _rate = 0;//实际使用的值，由数采服务设置
	CMyTime _allTime;//上次全部采集的时间
	string _events;//未处理的事件，目前仅有变化全部上传

	Channel* FindByChannelNo(int channelNo)
	{
		for (auto& c : channels)
		{
			if (c->channelNo == channelNo)return c;
		}
		return NULL;
	}
	Channel* FindByParamCode(char const* paramCode)
	{
		for (auto& c : channels)
		{
			if (c->paramCode == paramCode)return c;
		}
		return NULL;
	}
	void ToTable(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)const
	{
		table.SetTitle(this->deviceCode.c_str());
		if (0 == channels.size())
		{
			//由于不知道实际结构，只显示标准表头
			Channel tmp(NULL);
			tmp.AddHtmlTalbeCols(protocolCode, table, bEdit);
		}
		else
		{
			channels[0]->AddHtmlTalbeCols(protocolCode, table, bEdit);
			for (auto v : channels)
			{
				table.AddLine();
				v->AddToHtmlTableLine(protocolCode, table, bEdit);
			}
		}
	}
	string ToString(string const& protocolCode, bool withDetail = true)const
	{
		stringstream ss;
		ss << "\t设备[" << deviceCode << "] deviceParam:" << deviceParam->ToString() << " rate: " << rate << " _rate:" << _rate << " _DeviceState:" << (int)_DeviceState << endl;

		if (!withDetail)
		{
			ss << "\t共有通道 " << channels.size() << " 个" << endl;
		}
		else
		{
			{
				ss << "\t共有通道 " << channels.size() << " 个：" << endl;
				CHtmlDoc::CHtmlTable2 table;
				ToTable(protocolCode, table, false);
				ss << table.MakeTextTable() << endl;
			}

			{
				LockedList<WriteInfo >* pWriteList = (LockedList<WriteInfo >*) & writeList;
				ss << "\t待写入 " << pWriteList->locked_size() << " 个：" << endl;
				if (pWriteList->locked_size() > 0)
				{
					CHtmlDoc::CHtmlTable2 table;
					table.AddCol("uid");
					table.AddCol("c");
					table.AddCol("v");
					pWriteList->lock();
					for (auto v : pWriteList->getForForeach())
					{
						table.AddLine();
						table.AddData(v.uid);
						table.AddData(v.channelNo);
						table.AddData(v.value);
					}
					pWriteList->unlock();
					ss << table.MakeTextTable() << endl;
				}
			}
		}

		return ss.str();
	}
};

enum class InterfaceState { DEFAULT = 0, CONNECT_OK, CONNECT_ERROR };

struct InterfaceDeviceConfig
{
	string protocolCode;//MODBUS，OPCUA，DLT645，S7
	string interfaceCode;//接口名
	vector<Device* > devices;
	ProtocolSvr* _ProtocolSvr = nullptr;//运行时协议服务
	InterfaceState _InterfaceState = InterfaceState::DEFAULT;

	string ToString(bool withDetail = true)const;

	string AllDeviceName()const
	{
		stringstream ss;
		for (auto& v : devices)
		{
			ss << v->deviceCode << ";";
		}

		return ss.str();
	}
};
