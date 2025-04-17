//gwdefine.h 枚举和接口定义

#pragma once

#include "LockedList.h"
#include "WorkThread.h"
#include <interface/ProtocolCode.h>

#define DEVICE_MODEL_QJ57B_1A "QJ57B_1A"
struct AllDeviceModel
{
	static bool isInDeviceModels(char const* deviceModel)
	{
		if (NULL == deviceModel || '\0' == deviceModel[0])return false;
		if (0 == strcmp(deviceModel, DEVICE_MODEL_QJ57B_1A))return true;
		return false;
	}
};

constexpr int REALTIME_RATE = 1000;//实时采集的频率，毫秒
constexpr int DEFAULT_RATE = 60000;//默认采集的频率，毫秒

constexpr int EXIT_RELOAD = 100;//退出后重新运行程序
constexpr int EXIT_RESET = 101;//退出后清理程序设置
constexpr int EXIT_PLATFORM_CONNECT_TIMEOUT = 102;//平台连接超时(bUseDefaultNorthConfig=false)
constexpr int EXIT_DEFAULT_PLATFORM_CONNECT_TIMEOUT = 103;//默认平台连接超时(bUseDefaultNorthConfig=true)

//全局变量
class CGwConfig;
class IPlatform;
class CDataManager;
class CMqttSvr;
class ProtocolSvr;
struct GW_Global
{
	CGwConfig* pCGwConfig = nullptr;
	IPlatform* pIPlatform = nullptr;
	CDataManager* pCDataManager = nullptr;

	time_t satrt_time;//程序启动时间
	bool bUseDefaultNorthConfig = false;//使用默认北向配置
	bool bPlatformConnected = false;//平台已连接
	time_t last_platform_connection_send = 0;//上一次向平台成功发送的时间（技术上未必真的发生了发送，但长时间没有成功肯定出了问题）
	time_t last_platform_connection_recv = 0;//上一次收到平台数据的时间（平台可能从不下发数据，但只要收到说明平台连接正常）
};
extern GW_Global g_Global;//全局对象

typedef uint32_t T_DATA_SEQ; //数据序列号，一秒一个也够几十年了

//平台处理接口
class IPlatform : public IWorkThread
{
private:
	//工作线程的任务
	virtual void worker_job()override
	{
		string msg;
		while (m_ServerMesssageList.locked_TryGetBegin(msg))
		{
			bool bEncryped;
			MessageUnTranslate(msg, bEncryped);
			ProcessServerMessage(msg.c_str(), bEncryped);
			m_ServerMesssageList.locked_pop_front();
		}
	}
	//定时器线程的任务，会在激活工作线程之前执行（并不代表工作线程此时一定不在工作中）
	virtual void timer_job()override {}
public:
	CWorkerThread m_WorkerThread;
	IPlatform()
	{
	}
	LockedList<string> m_ServerMesssageList;//收到的服务器消息
public://接口
	virtual bool InitPlatform() = 0;
	virtual bool UninitPlatform() = 0;
	//平台是否连接成功
	virtual bool isPlatformConnected() = 0;
	//平台是否注册成功
	virtual bool isPlatformRegisted() = 0;
	//数据包压缩和加密
	virtual string& MessageTranslate(string func, string respondFunc, string& msg) = 0;
	//数据包解密和解压
	virtual string& MessageUnTranslate(string& msg, bool& bEncryped) = 0;
	//构造注册包
	virtual string build_Reg() = 0;
	//构造心跳包
	virtual string build_Hb() = 0;
	//构造状态包
	virtual string build_State() = 0;
	//发送报告包
	virtual bool post_Report(char const* message, char const* _interface = nullptr, char const* _device = nullptr, char const* _channel = nullptr) = 0;
	//发送报告包，指定func
	virtual bool post_ReportWithFunc(char const* func, char const* message, char const* _interface = nullptr, char const* _device = nullptr, char const* _channel = nullptr) = 0;
	//发送普通消息，任何格式，func仅用来做内部处理
	virtual bool post_Message(char const* func, char const* message) = 0;

	//添加数据，errorCode非零表示错误码，errorMessage为错误信息，用channelNo排序
	virtual void AddPushData(long conn, char const* deviceCode, int channelNo, char const* channelCode, char const* channelName, int64_t ts, bool isNumber, double numberValue, char const* strValue, int errorCode, char const* errorMessage) = 0;
	//获取数据个数
	virtual int GetDataCount(long conn) = 0;
	//发送指定设备的数据包并清空设备的数据
	virtual bool post_DataMessage(long conn, char const* deviceCode, bool isAll, string events, long ms) = 0;
private://接口
	//处理平台下发指令
	virtual bool ProcessServerMessage(char const* messageBody, bool bEncryped) = 0;
};
