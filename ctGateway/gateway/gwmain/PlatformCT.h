//PlatformCT.h CT平台

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h> 
#include "uuid.h"
#include "MqttSvr.h"
#include "gwconfig.h"
#include <list>
#include <map>
#include <future>
#include <pthread.h>

class CCTPlatform : public IPlatform
{
private:
	CMqttSvr _MqttSvr;
	CMqttSvr* m_pMqttSvr = &_MqttSvr;
	bool m_bRegisted = false;//是否完成了注册过程

	struct report_channel
	{
		string channelCode;
		string channelName;
		int64_t ts;
		bool isNumber;
		double numberValue;
		string strValue;
		int errorCode;
		string errorMessage;
	};
	map<long, list<pair<string, map<int, report_channel > > > > m_map_datas;//连接识别码-设备-通道数据
	int m_overtime = 60;//处理超时
public:
private:
	//基本协议包
	struct BasePacket
	{
		string func;
		string ts;
		string uid;
	};
	//创建基本数据包
	cJSON* CreateBaseJson(char const* func, char const* uid = NULL)
	{
		CMyTime ts;
		ts.SetCurrentTime();
	
		cJSON* cjson = cJSON_CreateObject();
		cJSON_AddStringToObject(cjson, "func", func);
		cJSON_AddNumberToObject(cjson, "ts", ts.GetTS_ms());
		if(NULL!=uid)cJSON_AddStringToObject(cjson, "uid", uid);
		else cJSON_AddStringToObject(cjson, "uid", CUUID::generate_uuid().c_str());

		return cjson;
	}
	//上报成功状态
	bool MqttReportSucceed(BasePacket& baseRequest)
	{
		return MqttRespond(baseRequest, 1, 0, "Succeed");
	}
	//上报成功状态和信息
	bool MqttReportSucceed(BasePacket& baseRequest,char const * output)
	{
		return MqttRespond(baseRequest, 1, 0, output);
	}
	//上报出错状态
	bool MqttReportError(BasePacket& baseRequest, int error_code, char const* error_message, int x)
	{
		return MqttRespond(baseRequest, 0, error_code, error_message, x);
	}
	//上报出错状态
	bool MqttReportError(BasePacket& baseRequest, int error_code, char const* error_message)
	{
		return MqttRespond(baseRequest, 0, error_code, error_message);
	}
	//上报状态
	bool MqttRespond(BasePacket& baseRequest, int state, int error_code, char const* message, int x)
	{
		stringstream ss;
		ss << message << x;
		return MqttRespond(baseRequest, state, error_code, ss.str().c_str());
	}
	//上报状态
	bool MqttRespond(BasePacket& baseRequest, int state, int error_code, char const* message)
	{
		cJSON* cjson = CreateBaseJson("respond");

		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		cJSON_AddStringToObject(cjson_data, "func", baseRequest.func.c_str());
		cJSON_AddStringToObject(cjson_data, "uid", baseRequest.uid.c_str());
		cJSON_AddNumberToObject(cjson_data, "state", state);
		cJSON_AddNumberToObject(cjson_data, "error_code", error_code);
		cJSON_AddStringToObject(cjson_data, "message", message);

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);

		DEBUG_LOG << "报告状态：" << endl << json << endi;
		m_pMqttSvr->MqttPostMessage("respond", baseRequest.func, json);
		return true;
	}

	//平台命令处理：resatrt
	bool _Process_Restart(BasePacket& baseRequest);
	//平台命令处理：reload
	bool _Process_Reload(BasePacket& baseRequest);
	//平台命令处理：reset
	bool _Process_ResetConf(BasePacket& baseRequest);
	//平台命令处理：show
	bool _Process_Acquire(BasePacket& baseRequest);
	//平台命令处理：show
	bool _Process_Show(BasePacket& baseRequest);
	//平台命令处理：enablecompress
	bool _Process_EnableCompress(BasePacket& baseRequest);
	//平台命令处理：enableCrypt
	bool _Process_EnableCrypt(BasePacket& baseRequest);

	//平台命令处理：list
	bool _Process_List(BasePacket& baseRequest, cJSON* cjson_data);

	//平台命令处理：reg
	bool _Process_Reg(BasePacket& baseRequest, cJSON* cjson_data);
	bool __WriteChannel(BasePacket& baseRequest, InterfaceDeviceConfig* pInterface, Channel* pChannel, string const& value);
	//平台命令处理：Write
	bool _Process_Write(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：Send
	bool _Process_Send(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：sudo
	bool _Process_Sudo(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：upload
	bool _Process_Upload(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：download
	bool _Process_Download(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：missing
	bool _Process_Missing(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：confirm
	bool _Process_Confirm(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：ip
	bool _Process_Ip(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：connt
	bool _Process_Connt(BasePacket& baseRequest, char const* messageBody);
	//平台命令处理：conf
	bool _Process_Conf(BasePacket& baseRequest, char const* messageBody);
	//平台命令处理：loglevel
	bool _Process_Loglevel(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：overtime
	bool _Process_Overtime(BasePacket& baseRequest, cJSON* cjson_data);
	//平台命令处理：key
	bool _Process_Key(BasePacket& baseRequest, cJSON* cjson_data);

	//根据func分别处理
	bool _ProcessServerMessage(char const* messageBody, cJSON* cjson, BasePacket& basePacket)
	{
		bool ret;
		//先判断无需data的指令，最后处理需要data的
		if ("notreg" == basePacket.func)
		{
			ret = m_pMqttSvr->MqttPostMessage("reg", "", build_Reg());
		}
		else if ("state" == basePacket.func)
		{
			ret = m_pMqttSvr->MqttPostMessage("state", "", build_State());
		}
		else if ("restart" == basePacket.func)
		{
			ret = _Process_Restart(basePacket);
		}
		else if ("reload" == basePacket.func)
		{
			ret = _Process_Reload(basePacket);
		}
		else if ("reset" == basePacket.func)
		{
			ret = _Process_ResetConf(basePacket);
		}
		else if ("acquire" == basePacket.func)
		{
			ret = _Process_Acquire(basePacket);
		}
		else if ("show" == basePacket.func)
		{
			ret = _Process_Show(basePacket);
		}
		else if ("enablecompress" == basePacket.func)
		{
			ret = _Process_EnableCompress(basePacket);
		}
		else if ("enablecrypt" == basePacket.func)
		{
			ret = _Process_EnableCrypt(basePacket);
		}
		else
		{
			cJSON* cjson_data = cJSON_GetObjectItem(cjson, "data");
			if (NULL == cjson_data)
			{
				if ("list" == basePacket.func)
				{
					ret = _Process_List(basePacket, NULL);
				}
				else
				{
					MqttReportError(basePacket, 1, "请求中没有data");
					ret = false;
				}
			}
			else
			{
				if ("reg" == basePacket.func)
				{
					ret = _Process_Reg(basePacket, cjson_data);
				}
				else if ("write" == basePacket.func)
				{
					ret = _Process_Write(basePacket, cjson_data);
				}
				else if ("send" == basePacket.func)
				{
					ret = _Process_Send(basePacket, cjson_data);
				}
				else if ("list" == basePacket.func)
				{
					ret = _Process_List(basePacket, cjson_data);
				}
				else if ("sudo" == basePacket.func)
				{
					ret = _Process_Sudo(basePacket, cjson_data);
				}
				else if ("upload" == basePacket.func)
				{
					ret = _Process_Upload(basePacket, cjson_data);
				}
				else if ("download" == basePacket.func)
				{
					ret = _Process_Download(basePacket, cjson_data);
				}
				else if ("missing" == basePacket.func)
				{
					ret = _Process_Missing(basePacket, cjson_data);
				}
				else if ("confirm" == basePacket.func)
				{
					ret = _Process_Confirm(basePacket, cjson_data);
				}
				else if ("ip" == basePacket.func)
				{
					ret = _Process_Ip(basePacket, cjson_data);
				}
				else if ("connt" == basePacket.func)
				{
					ret = _Process_Connt(basePacket, messageBody);
				}
				else if ("conf" == basePacket.func)
				{
					ret = _Process_Conf(basePacket, messageBody);
				}
				else if ("loglevel" == basePacket.func)
				{
					ret = _Process_Loglevel(basePacket, cjson_data);
				}
				else if ("overtime" == basePacket.func)
				{
					ret = _Process_Overtime(basePacket, cjson_data);
				}
				else if ("key" == basePacket.func)
				{
					ret = _Process_Key(basePacket, cjson_data);
				}
				else
				{
					MqttReportError(basePacket, 1, "未知请求");
					ret = false;
				}
			}
		}

		return ret;
	}
public://IPlatform
	virtual bool InitPlatform()override
	{
		//启动MQTT处理
		m_pMqttSvr->MqttStartProcessThread();
		return this->m_WorkerThread.StartWorkerThread(this, "平台", 1000 * 30);
	}
	virtual bool UninitPlatform()override
	{
		m_pMqttSvr->MqttUninit();
		return true;
	}
	virtual bool isPlatformConnected()override { return m_pMqttSvr->isMqttConnected(); }
	virtual bool isPlatformRegisted()override { return m_bRegisted; }
	virtual string& MessageTranslate(string func, string respondFunc, string& msg)override;
	virtual string& MessageUnTranslate(string& msg, bool& bEncryped)override;
	virtual string build_Reg()override
	{
		cJSON* cjson = CreateBaseJson("reg");

		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		cJSON_AddStringToObject(cjson_data, "sn", g_Global.pCGwConfig->GetSn().c_str());
		cJSON_AddStringToObject(cjson_data, "version", g_Global.pCGwConfig->GetBaseInfo().sw_version.c_str());//与现有平台兼容
		cJSON_AddStringToObject(cjson_data, "sw_version", g_Global.pCGwConfig->GetBaseInfo().sw_version.c_str());
		cJSON_AddStringToObject(cjson_data, "model", g_Global.pCGwConfig->GetBaseInfo().model.c_str());
		cJSON_AddStringToObject(cjson_data, "spec", g_Global.pCGwConfig->GetBaseInfo().model.c_str());
		cJSON_AddStringToObject(cjson_data, "iccid", g_Global.pCGwConfig->GetBaseInfo().iccid.c_str());
		cJSON_AddStringToObject(cjson_data, "cardNo", g_Global.pCGwConfig->GetBaseInfo().iccid.c_str());
		cJSON_AddStringToObject(cjson_data, "latitude", g_Global.pCGwConfig->GetBaseInfo().latitude.c_str());
		cJSON_AddStringToObject(cjson_data, "longitude", g_Global.pCGwConfig->GetBaseInfo().longitude.c_str());
		cJSON_AddNumberToObject(cjson_data, "useGmSSL", g_Global.pCGwConfig->GetAppConfig().useGmSSL);
		cJSON_AddNumberToObject(cjson_data, "key_seq", g_Global.pCGwConfig->GetAppConfig().key_seq);
		cJSON_AddNumberToObject(cjson_data, "current_key_seq", g_Global.pCGwConfig->GetAppConfig().current_key_seq);
		cJSON_AddNumberToObject(cjson_data, "data_sequence", g_Global.pCGwConfig->GetConfirmedSeq());
		cJSON_AddStringToObject(cjson_data, "seq", CUUID::generate_uuid().c_str());
		cJSON_AddStringToObject(cjson_data, "start_time", CMyTools::TimeToString_Time(g_Global.satrt_time).c_str());
		string uptime;
		if (0 == GetShellOutput("uptime", uptime))cJSON_AddStringToObject(cjson_data, "uptime", uptime.c_str());

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);
		return json;
	}
	virtual string build_Hb()override
	{
		CMyTime ts;
		ts.SetCurrentTime();

		cJSON* cjson = CreateBaseJson("hb");

		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		cJSON_AddNumberToObject(cjson_data, "hb", 1);
		cJSON_AddNumberToObject(cjson_data, "ts", ts.GetTS_ms());
		cJSON_AddStringToObject(cjson_data, "seq", CUUID::generate_uuid().c_str());

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);
		return json;
	}
	virtual bool post_Report(char const* message, char const* _interface = nullptr, char const* _device = nullptr, char const* _channel = nullptr)override
	{
		cJSON* cjson = CreateBaseJson("report");

		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		if (_interface)cJSON_AddStringToObject(cjson_data, "interface_code", _interface);
		if (_device)cJSON_AddStringToObject(cjson_data, "device_code", _device);
		if (_channel)cJSON_AddStringToObject(cjson_data, "channel_code", _channel);
		cJSON_AddStringToObject(cjson_data, "messsage", message);

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);
		
		return this->m_pMqttSvr->MqttPostMessage("report", "", json);
	}
	virtual bool post_ReportWithFunc(char const* func, char const* message, char const* _interface = nullptr, char const* _device = nullptr, char const* _channel = nullptr)override
	{
		cJSON* cjson = CreateBaseJson(func);

		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		if (_interface)cJSON_AddStringToObject(cjson_data, "interface_code", _interface);
		if (_device)cJSON_AddStringToObject(cjson_data, "device_code", _device);
		if (_channel)cJSON_AddStringToObject(cjson_data, "channel_code", _channel);
		cJSON_AddStringToObject(cjson_data, "messsage", message);

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);

		return this->m_pMqttSvr->MqttPostMessage(func, "", json);
	}

	virtual void AddPushData(long conn, char const* deviceCode, int channelNo, char const* channelCode, char const* channelName, int64_t ts, bool isNumber, double numberValue, char const* strValue, int errorCode, char const* errorMessage)override;
	virtual int GetDataCount(long conn)override;
	virtual bool post_DataMessage(long conn, char const* deviceCode, bool isAll, string events, long ms)override;
	virtual bool post_Message(char const* func, char const* message)override;
	virtual string build_State()override;
	virtual bool ProcessServerMessage(char const* messageBody, bool bEncryped)override
	{
		cJSON* cjson = cJSON_Parse(messageBody);

		if (NULL == cjson)
		{
			thelog << "不是有效的json数据： " << messageBody << ende;
			return false;
		}

		BasePacket basePacket;
		if (!CJsonUtil::_GetJsonParam(cjson, "func", basePacket.func))
		{
			thelog << "没有func： " << messageBody << ende;
			return false;
		}
		thelog << "收到下发指令 " << basePacket.func << endi;

		if (!bEncryped)
		{
			if ("reset" == basePacket.func || "sudo" == basePacket.func
				|| "upload" == basePacket.func || "download" == basePacket.func
				|| "key" == basePacket.func)
			{
				MqttReportError(basePacket, 401, "unauthorized");
				return false;
			}
		}

		CJsonUtil::_TryGetJsonParam(cjson, "ts", basePacket.ts);
		CJsonUtil::_TryGetJsonParam(cjson, "uid", basePacket.uid);

		promise<bool> p;
		auto future = p.get_future();

		thread* process_thread = new thread([&p, this, messageBody, cjson, &basePacket]
			{
				p.set_value(_ProcessServerMessage(messageBody, cjson, basePacket));
			});
		
		bool ret;
		time_t t1 = time(NULL);
		while (true)
		{
			if (time(NULL) - t1 < m_overtime)
			{
				auto status = future.wait_for(chrono::milliseconds(1));
				if (status == std::future_status::ready)
				{
					ret = future.get();
					break;
				}
			}
			else
			{
				thelog << "处理超时" << basePacket.func << ende;
				pthread_cancel(process_thread->native_handle());//终止线程（C++11并没有提供可用方法）
				MqttReportError(basePacket, m_overtime, "处理超时，强制结束。请提高超时设置。");
				break;
			}
		}
		process_thread->join();

		cJSON_free(cjson);

		return ret;
	}

};
