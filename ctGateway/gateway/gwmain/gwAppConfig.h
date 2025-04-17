//gwAppConfig.h 配置文件

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h>
#include <cJSON_util.h>
#include "gwdefine.h"
#include "../interface/gwUtil.h"
#include "PreFilledKey.h"

constexpr char const* AppConfigFileName = "appConfig.json";
constexpr char const* RuntimeFileName = "runtime.txt";

//应用配置
struct AppConfig
{
	long max_log_file_size = 10 * 1024 * 1024;//日志文件最大大小
	long max_data_file_size = 10 * 1024 * 1024;//数据文件最大大小
	bool enableCompress = false;//启用压缩
	bool enableAllCrypt = false;//启用全部加密（部分数据始终是加密的）
	string aesKey = "123";//AES的key
	bool useGmSSL = false;//启用国密算法
	long key_seq = -1;//加密密钥标识，默认值-1

	long current_key_seq;//与当前key匹配的标识，此值不保存
	string current_key;//加密密钥，此值不保存

	vector<IConfigItem *> m_ConfigItems;
	AppConfig()
	{
		m_ConfigItems.push_back(new ConfigItem_long("max_log_file_size",&max_log_file_size));
		m_ConfigItems.push_back(new ConfigItem_long("max_data_file_size", &max_data_file_size));
		m_ConfigItems.push_back(new ConfigItem_bool("enableCompress", &enableCompress));
		m_ConfigItems.push_back(new ConfigItem_bool("enableAllCrypt", &enableAllCrypt));
		m_ConfigItems.push_back(new ConfigItem_bool("useGmSSL", &useGmSSL));
		m_ConfigItems.push_back(new ConfigItem_long("key_seq", &key_seq));
		m_ConfigItems.push_back(new ConfigItem_string("key", &aesKey));

		set_default_gm_key();
	}
	//设置为默认密钥
	void set_default_gm_key()
	{
		current_key_seq = -1;//恢复为默认值
		current_key = "123";
	}
	//重置配置
	static bool ResetConfig()
	{
		AppConfig tmp;
		return tmp.Save();
	}
	//还原到出厂设置
	static bool RestoreFactory()
	{
		CEasyFile file;
		if (file.IsFileExist(AppConfigFileName) && !file.DeleteFile(AppConfigFileName))
		{
			thelog << "未能删除应用配置文件 " << AppConfigFileName << ende;
			return false;
		}
		return true;
	}
	bool Load()
	{
		bool ret = true;
		
		//读取配置文件
		{
			string filename = AppConfigFileName;
			CEasyFile file;
			string str;
			if (!file.ReadFile(filename.c_str(), str))
			{
				thelog << "未能打开应用配置文件 " << filename << ende;
				return false;
			}

			cJSON* cjson = cJSON_Parse(str.c_str());

			if (NULL == cjson)
			{
				thelog << "解析平台配置出错 " << filename << endl << str << ende;
				return false;
			}
			ret = true;
			cJSON* cjson_data = cJSON_GetObjectItem(cjson, "data");
			if (NULL == cjson_data)
			{
				thelog << "解析平台配置出错，没有data项 " << ende;
				ret = false;
			}
			else
			{
				//CJsonUtil::_GetJsonParam(cjson_data, "max_log_file_size", max_log_file_size);
				//CJsonUtil::_GetJsonParam(cjson_data, "max_data_file_size", max_data_file_size);
				//CJsonUtil::_GetJsonParam(cjson_data, "enableCompress", enableCompress);
				//CJsonUtil::_GetJsonParam(cjson_data, "enableAllCrypt", enableAllCrypt);
				//CJsonUtil::_GetJsonParam(cjson_data, "key", key);
				for (auto& v : m_ConfigItems)
				{
					v->LoadConfigItem(cjson_data);
					thelog << v->name << " " << v->ToString(str) << endi;
				}
			}

			cJSON_free(cjson);
		}
		
		//检查
		if (max_log_file_size < 1024)
		{
			thelog << "参数max_log_file_size设置过小 " << max_log_file_size << ende;
			max_log_file_size = 1024;
		}
		if (max_data_file_size < 1024)
		{
			thelog << "参数max_data_file_size设置过小 " << max_data_file_size << ende;
			max_data_file_size = 1024;
		}
		
		//应用（仅日志，其它直接访问）
		thelog << "max_log_file_size " << max_log_file_size << " max_data_file_size " << max_data_file_size << endi;
		theLog.setMaxFileSize(max_log_file_size);
		
		//加载密钥
		if(!LoadGmKey(key_seq))
		{
			set_default_gm_key();
		}

		return ret;
	}
	//加载预充注密钥，如果不存在保持不变
	bool LoadGmKey(int seq)
	{
		CPreFilledKeyManager km;
		string tmpkey;
		if (km.ReadKeyBySEQ(seq, tmpkey))
		{
			key_seq = seq;
			current_key_seq = seq;
			current_key = tmpkey;
			return true;
		}
		return false;
	}
	bool Save()
	{
		string filename = AppConfigFileName;

		cJSON* cjson = cJSON_CreateObject();
		
		cJSON_AddStringToObject(cjson, "func", "appConfig");
		
		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");
		
		//cJSON_AddNumberToObject(cjson_data, "max_log_file_size", max_log_file_size);
		//cJSON_AddNumberToObject(cjson_data, "max_data_file_size", max_data_file_size);
		//cJSON_AddNumberToObject(cjson_data, "enableCompress", enableCompress);
		//cJSON_AddNumberToObject(cjson_data, "enableAllCrypt", enableAllCrypt);
		//cJSON_AddStringToObject(cjson_data, "key", key.c_str());
		for (auto& v : m_ConfigItems)
		{
			v->SaveConfigItem(cjson_data);
		}

		string str = cJSON_Print(cjson);

		CEasyFile file;
		if (!file.WriteFile(filename.c_str(), str.c_str()))
		{
			thelog << "写应用配置文件失败 " << filename << ende;
			return false;
		}
		
		cJSON_free(cjson);
		return true;
	}
};

//应用运行时数据，动态保存到文件
struct RuntimeData
{
	//已确认数据序列号（存在的最小数据序列号）
	T_DATA_SEQ confirmed_data_sequence = 0;
	//数据序列号，单调递增
	T_DATA_SEQ last_data_sequence = 0;
	//成功发送的数据的序号
	T_DATA_SEQ last_sended_seq = 0;

	static T_DATA_SEQ NextSeq(T_DATA_SEQ seq)
	{
		return seq + 1;
	}
	static T_DATA_SEQ PrevSeq(T_DATA_SEQ seq)
	{
		return seq - 1;
	}
	T_DATA_SEQ Add_last_data_sequence()
	{
		return last_data_sequence=NextSeq(last_data_sequence);
	}

	static bool Reset()
	{
		CEasyFile file;
		if (!file.DeleteFile(RuntimeFileName))
		{
			thelog << "未能删除运行时配置文件 " << RuntimeFileName << ende;
			return false;
		}
		return true;
	}
	bool Load()
	{
		CEasyFile file;
		string str;
		if (!file.ReadFile(RuntimeFileName, str))
		{
			thelog << "未能打开运行时配置文件 " << RuntimeFileName << ende;
			return false;
		}
		StringTokenizer st(str, " ");
		if (st.size() > 0)last_sended_seq = atoll(st[0].c_str());
		if (st.size() > 1)last_data_sequence = atoll(st[1].c_str());
		if (st.size() > 2)confirmed_data_sequence = atoll(st[2].c_str());

		thelog << "last_sended_seq " << last_sended_seq << " last_data_sequence " << last_data_sequence << " confirmed_data_sequence " << confirmed_data_sequence << endi;
		return true;
	}
	bool Save()
	{
		CEasyFile file;
		char buf[256];
		sprintf(buf, "%llu %llu %llu", (unsigned long long)last_sended_seq, (unsigned long long)last_data_sequence, (unsigned long long)confirmed_data_sequence);
		if (!file.WriteFile(RuntimeFileName, buf))
		{
			thelog << "未能写入运行时配置文件 " << RuntimeFileName << ende;
			return false;
		}
		return true;
	}
};
class CGwAppConfig
{
public:
	AppConfig m_AppConfig;
	RuntimeData m_RuntimeData;
	bool LoadAppConfig()
	{
		m_RuntimeData.Load();
		m_AppConfig.Load();
		return true;//不能因为配置文件问题不能运行
	}
	static bool Reset()
	{
		RuntimeData::Reset();
		return AppConfig::ResetConfig();
	}
};
