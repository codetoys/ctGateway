//PlatformCT.cpp CT平台

#include "PlatformCT.h"
#include "mimetype.h"
#include "mydir.h"
#include <utime.h>
#include "DataManager.h"
#include "myZIP.h"
#include<openssl/md5.h>
#include "myOpenSSL.h"
#include "myGmSSL.h"

bool CCTPlatform::_Process_Sudo(BasePacket& baseRequest, cJSON* cjson_data)
{
	string cmd;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "cmd", cmd))
	{
		MqttReportError(baseRequest, 1, "未定义cmd");
		return false;
	}
	string output;
	int ret = GetShellOutput(cmd.c_str(), output);
	if (0 == ret)
	{
		MqttReportSucceed(baseRequest, output.c_str());
		return true;
	}
	else
	{
		MqttReportError(baseRequest, ret, "执行失败");
		return false;
	}
}

bool CCTPlatform::_Process_Upload(BasePacket& baseRequest, cJSON* cjson_data)
{
	string file_name;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "file_name", file_name))
	{
		MqttReportError(baseRequest, 1, "未定义file_name");
		return false;
	}
	int file_size;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "file_size", file_size))
	{
		MqttReportError(baseRequest, 2, "未定义file_size");
		return false;
	}
	string file_time;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "file_time", file_time))
	{
		MqttReportError(baseRequest, 4, "未定义file_time");
		return false;
	}
	string file_content;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "file_content", file_content))
	{
		MqttReportError(baseRequest, 5, "未定义file_content");
		return false;
	}
	string dir;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "dir", dir))
	{
		dir = ".";
	}
	if (dir.size() == 0)dir = ".";
	int content_start = 0;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "content_start", content_start))
	{
		content_start = 0;
	}
	int content_length = 0;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "content_length", content_length))
	{
		content_length = file_size;
	}

	char* buf = new char[file_content.size()];
	if (!buf)
	{
		thelog << "内存分配失败" << ende;
		MqttReportError(baseRequest, 5, "内存分配失败");
		return false;
	}

	int dec_size = CBase64::Base64Dec(buf, file_content.c_str(), file_content.size());

	stringstream ss;
	if (dec_size >= content_length && dec_size <= content_length + 2)//解码后的长度是3的整数倍，因此比实际数据可能多2
	{
		CEasyFile file;
		string fullname = dir + "/" + file_name;
		string tmpname = getTmpDir() + file_name + ".tmp";
		if (0 == content_start)
		{
			file.DeleteFile(tmpname.c_str());//确保临时文件不存在
		}
		if (!file.WriteFile(tmpname.c_str(), content_start, buf, content_length, 0 == content_start))
		{
			MqttReportError(baseRequest, 6, file.getMsg().c_str());
			delete[]buf;
			return false;
		}

		if (content_start + content_length >= file_size)
		{
			string file_checksum;
			bool bChecked = false;
			if (CJsonUtil::_GetJsonParam(cjson_data, "file_checksum", file_checksum))
			{
				//校验文件
				string checksum;
				int tmplength = getFileCheckSum(tmpname.c_str(), checksum);
				if (file_checksum != checksum)
				{
					ss << "文件校验错误 " << file_name << " 总长度 " << file_size << " 写入长度 " << tmplength << " 校验和 " << checksum << " 预期的校验和 " << file_checksum;
					MqttReportError(baseRequest, 7, ss.str().c_str());
					delete[]buf;
					return false;
				}
				bChecked = true;
			}

			if (!file.DeleteFile(fullname.c_str()))
			{
				MqttReportError(baseRequest, 8, file.getMsg().c_str());
				delete[]buf;
				return false;
			}
			string cp = "cp " + tmpname + " " + fullname;
			if (!file.RenameFile(tmpname.c_str(), fullname.c_str()) && 0 != myShellExecute(cp.c_str(), ""))
			{//既不能rename也不能cp
				MqttReportError(baseRequest, 9, file.getMsg().c_str());
				delete[]buf;
				return false;
			}

			struct utimbuf timebuf;
			timebuf.actime = CMyTools::TimeStringToTime2(file_time.c_str());
			timebuf.modtime = timebuf.actime;
			if (utime(fullname.c_str(), &timebuf) < 0)
			{
				ss << "修改文件日期错误 " << errno << " : " << strerror(errno);
				MqttReportError(baseRequest, 10, ss.str().c_str());
				delete[]buf;
				return false;
			}
			else
			{
				ss << "文件全部写入成功 " << file_name << " 总长度 " << file_size << " 开始位置 " << content_start << " " << (bChecked ? "校验通过" : "未校验（未提供校验码）");
				MqttReportSucceed(baseRequest, ss.str().c_str());
				delete[]buf;
				return true;
			}
		}
		else
		{
			ss << "文件写入成功 " << file_name << " 总长度 " << file_size << " 开始位置 " << content_start;
			MqttReportSucceed(baseRequest, ss.str().c_str());
			delete[]buf;
			return true;
		}
	}
	else
	{
		ss << "长度错误 解码后 " << dec_size << " 预期 " << content_length << "[" << buf << "]";
		MqttReportError(baseRequest, 8, ss.str().c_str());
		delete[]buf;
		return false;
	}
}

bool CCTPlatform::_Process_Download(BasePacket& baseRequest, cJSON* _cjson_data)
{
	string file_name;
	if (!CJsonUtil::_GetJsonParam(_cjson_data, "file_name", file_name))
	{
		MqttReportError(baseRequest, 1, "未定义file_name");
		return false;
	}
	string dir;
	if (!CJsonUtil::_GetJsonParam(_cjson_data, "dir", dir))
	{
		dir = ".";
	}
	if (0 == dir.size())dir = ".";

	string private_data;
	if (!CJsonUtil::_GetJsonParam(_cjson_data, "private_data", private_data))
	{
		private_data = "";
	}

	string full_name = dir + "/" + file_name;

	string file_time;
	{
		struct stat file_stat;
		struct tm* t2;
		char buf[256];
		if (stat(full_name.c_str(), &file_stat) != 0)
		{
			MqttReportError(baseRequest, 1, (full_name + " 读取文件错误").c_str());
			return false;
		}
		t2 = localtime(&file_stat.st_mtime);
		strftime(buf, 128, "%Y-%m-%d %H:%M:%S", t2);
		file_time = buf;
	}
	CEasyFile easyfile;
	CBuffer buffer;
	if (!easyfile.ReadFile(full_name.c_str(), buffer))
	{
		MqttReportError(baseRequest, 1, (full_name + " 读取文件错误").c_str());
		return false;
	}
	int file_size = buffer.size();
	constexpr int max_size = 500 * 1024;//受服务器允许的最大值限制，选择最大值的一半比较合适
	unsigned char* buf = new unsigned char[max_size * 2 + 3];//编码后会变大
	for (int start = 0; start < file_size; start += max_size)
	{
		int length = max_size;
		if (file_size - start < max_size)length = file_size - start;
		int enc_size = CBase64::Base64Enc(buf, (unsigned char*)buffer.data() + start, length);
		buf[enc_size] = '\0';

		cJSON* cjson = CreateBaseJson("download", baseRequest.uid.c_str());
		cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");

		cJSON_AddStringToObject(cjson_data, "file_name", file_name.c_str());
		cJSON_AddNumberToObject(cjson_data, "file_size", file_size);
		cJSON_AddStringToObject(cjson_data, "file_time", file_time.c_str());
		if (private_data.size() > 0)cJSON_AddStringToObject(cjson_data, "private_data", private_data.c_str());
		cJSON_AddStringToObject(cjson_data, "file_content", (char*)buf);
		cJSON_AddNumberToObject(cjson_data, "content_start", start);
		cJSON_AddNumberToObject(cjson_data, "content_length", length);

		string json = cJSON_Print(cjson);
		cJSON_free(cjson);

		m_pMqttSvr->MqttPostMessage("download", "", json);

		SleepSeconds(1);
	}

	delete[] buf;
	return true;
}

bool CCTPlatform::_Process_Missing(BasePacket& baseRequest, cJSON* cjson_data)
{
	string data_sequence;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "data_sequence", data_sequence))
	{
		MqttReportError(baseRequest, 1, "未定义data_sequence");
		return false;
	}
	StringTokenizer st(data_sequence, ",");

	bool bLoadRet = true;
	for (size_t i = 0; i < st.size(); ++i)
	{
		StringTokenizer st2(st[i], "-");
		T_DATA_SEQ from;
		T_DATA_SEQ to;
		from = atol(st2[0].c_str());
		if (st2.size() >= 2)
		{
			to = atol(st2[1].c_str());
		}
		else to = from;

		map<T_DATA_SEQ, string > map_data;
		bool bTmpRet = g_Global.pCDataManager->LoadData(from, to, map_data);

		thelog << bTmpRet << " 返回数据 " << map_data.size() << " 个" << endi;
		for (auto& m : map_data)
		{
			thelog << m.first << " size " << atol(m.second.c_str()) << endi;
			m_pMqttSvr->MqttPostDataMessage("push", "", m.second, m.first);
		}
		if (!bTmpRet)
		{
			bLoadRet = false;
			thelog << "数据不存在，程序需要重新启动" << ende;
		}
	}
	if (bLoadRet)
	{
		MqttReportSucceed(baseRequest, "处理成功");
		return true;
	}
	else
	{
		MqttReportError(baseRequest, 1, "处理失败");
		return false;
	}
}

bool CCTPlatform::_Process_Confirm(BasePacket& baseRequest, cJSON* cjson_data)
{
	T_DATA_SEQ data_sequence;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "data_sequence", data_sequence))
	{
		MqttReportError(baseRequest, 1, "未定义data_sequence");
		return false;
	}
	if (g_Global.pCDataManager->ConfirmData(data_sequence))
	{
		MqttReportSucceed(baseRequest, "处理成功");
		return true;
	}
	else
	{
		MqttReportError(baseRequest, 1, "处理失败");
		return false;
	}
}

bool CCTPlatform::_Process_Ip(BasePacket& baseRequest, cJSON* cjson_data)
{
	//string phyConnect;
	//string gateway;
	//if (!CJsonUtil::_GetJsonParam(cjson_data, "phyConnect", phyConnect))
	//{
	//	MqttReportError(baseRequest, 1, "未定义phyConnect");
	//	return false;
	//}
	//if (!CJsonUtil::_GetJsonParam(cjson_data, "gateway", gateway))
	//{
	//	MqttReportError(baseRequest, 1, "未定义gateway");
	//	return false;
	//}
	string ip;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "ip", ip))
	{
		ip = "";
	}
	string operate;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "operate", operate))
	{
		operate = "add";
	}

	CEasyFile file;
	string filedata;
	file.ReadFile(prepareShellFile, filedata);

	string cmdAdd = "ip addr add " + ip + " dev tether";
	string cmdDel = "ip addr del " + ip + " dev tether";
	string cmd;
	size_t pos;
	if ("add" == operate)
	{
		cmd = cmdAdd;
		if (filedata.find(cmd) == string::npos)
		{
			filedata += "\n" + cmd + "\n";
		}
	}
	else if ("del" == operate)
	{
		cmd = cmdDel;
		while ((pos = filedata.find(cmdAdd)) != string::npos)//注意要删除的是cmdAdd
		{
			filedata.erase(pos, cmdAdd.size());
		}
	}
	else if ("show" == operate)cmd = "ip addr show tether";

	//消除多余的回车换行
	while ((pos = filedata.find("\r")) != string::npos)
	{
		filedata.erase(pos, 1);
	}
	while ((pos = filedata.find("\n\n")) != string::npos)
	{
		filedata.erase(pos, 1);
	}

	if (!file.WriteFile(prepareShellFile, filedata.c_str()))
	{
		MqttReportSucceed(baseRequest, "保存失败");
	}

	string output;
	if (0 == GetShellOutput(cmd.c_str(), output))
	{
		MqttReportSucceed(baseRequest, (cmd + " 处理成功\n" + output).c_str());
		return true;
	}
	else
	{
		MqttReportError(baseRequest, 1, (cmd + " 处理失败\n" + output).c_str());
		return false;
	}
}

bool CCTPlatform::_Process_Connt(BasePacket& baseRequest, char const* messageBody)
{
	CEasyFile file;

	if (file.WriteFile(NorthConfigFile, messageBody))
	{
		MqttReportSucceed(baseRequest, " 处理成功");
		return true;
	}
	else
	{
		MqttReportError(baseRequest, 1, "处理失败");
		return false;
	}
}

bool CCTPlatform::_Process_Conf(BasePacket& baseRequest, char const* messageBody)
{
	CEasyFile file;

	if (file.WriteFile(deviceConfigFile, messageBody))
	{
		MqttReportSucceed(baseRequest, " 处理成功");
		return true;
	}
	else
	{
		MqttReportError(baseRequest, 1, "处理失败");
		return false;
	}
}

bool CCTPlatform::_Process_Loglevel(BasePacket& baseRequest, cJSON* cjson_data)
{
	string str;
	if (NULL == cjson_data || !CJsonUtil::_GetJsonParam(cjson_data, "loglevel", str))
	{
		MqttReportError(baseRequest, 1, "参数错误");
		return false;
	}
	if ("0" == str)G_IS_DEBUG = false;
	if ("1" == str)G_IS_DEBUG = true;
	return true;
}

bool CCTPlatform::_Process_Overtime(BasePacket& baseRequest, cJSON* cjson_data)
{
	int tmp;
	if (NULL == cjson_data || !CJsonUtil::_GetJsonParam(cjson_data, "overtime", tmp))
	{
		MqttReportError(baseRequest, 1, "参数错误");
		return false;
	}
	if (tmp >= 30)
	{
		m_overtime = tmp;
		MqttReportSucceed(baseRequest, " 处理成功");
	}
	else
	{
		MqttReportError(baseRequest, 1, "参数错误，应大于等于30");
	}
	return true;
}
bool CCTPlatform::_Process_Key(BasePacket& baseRequest, cJSON* cjson_data)
{
	string tmp;
	if (!CJsonUtil::_GetJsonParam(cjson_data, "key", tmp))
	{
		MqttReportError(baseRequest, 1, "参数错误");
		return false;
	}
	if (!g_Global.pCGwConfig->GetAppConfig().LoadGmKey(atol(tmp.c_str())))
	{
		MqttReportError(baseRequest, 2, "无效的预充注密钥");
		return false;
	}
	if (g_Global.pCGwConfig->GetAppConfig().Save())
	{
		MqttReportSucceed(baseRequest, " 处理成功");
	}
	else
	{
		MqttReportError(baseRequest, 3, "保存失败");
		return false;
	}
	return true;
}

string& CCTPlatform::MessageTranslate(string func, string respondFunc, string& msg)
{
	//压缩，注意必须先处理压缩，因为加密后数据混乱，压缩无效果
	if (g_Global.pCGwConfig->GetAppConfig().enableCompress)
	{
		TryCompressMessasge(msg);
	}
	//加密
	if (g_Global.pCGwConfig->GetAppConfig().enableAllCrypt || !(
		func == "push" || func == "reg" || func == "hb" 
		|| func == "connecting" || func == "connecting_failed" || func == "connected"
		))
	{
		string enocdedMsg;
		if (g_Global.pCGwConfig->GetAppConfig().useGmSSL)
		{
			string key = g_Global.pCGwConfig->GetAppConfig().current_key;//密钥
			if (CMyGmSSL::protect_encode(key, msg, enocdedMsg))
			{
				//格式为“G+密钥标识+空格+密文”
				char buf[64];
				sprintf(buf, "%ld ", g_Global.pCGwConfig->GetAppConfig().current_key_seq);//注意key标识后面有一个空格
				msg = "G";
				msg += +buf + enocdedMsg;
			}
		}
		else
		{
			string key = g_Global.pCGwConfig->GetAppConfig().aesKey;//密钥
			if (CMyOpenSSL::protect_encode(key, msg, enocdedMsg))
			{
				msg = "E" + enocdedMsg;
			}
		}
	}
	return msg;
}

string& CCTPlatform::MessageUnTranslate(string& msg, bool& bEncryped)
{
	bEncryped = false;
	//thelog << "=============================" << msg.size() << endl << "[" << msg << "]" << endi;
	if ('E' == msg[0])
	{
		bEncryped = true;
		string deocdedMsg;
		string key = g_Global.pCGwConfig->GetAppConfig().aesKey;//密钥

		if (CMyOpenSSL::protect_decode(key, msg.c_str() + 1, deocdedMsg))
		{
			msg = deocdedMsg;
		}
	}
	if ('G' == msg[0])
	{//格式为“G+密钥标识+空格+密文”
		bEncryped = true;

		string deocdedMsg;
		string key = g_Global.pCGwConfig->GetAppConfig().current_key;//密钥
		long msg_key_seq = atol(msg.c_str()+1);
		if (msg_key_seq == g_Global.pCGwConfig->GetAppConfig().current_key_seq)
		{
			thelog << "msg_key_seq 一致 " << msg_key_seq << endi;
			key = g_Global.pCGwConfig->GetAppConfig().current_key;
		}
		else
		{
			CPreFilledKeyManager km;
			if (!km.ReadKeyBySEQ(msg_key_seq, key))
			{
				thelog << "msg_key_seq 不存在 " << msg_key_seq << endi;
				return msg = "解密错误：密钥标识不存在 " + msg;
			}
			thelog << "msg_key_seq 存在 " << msg_key_seq << endi;
		}

		char const* seq_end = strchr(msg.c_str() + 1, ' ');
		if (CMyGmSSL::protect_decode(key, seq_end+1, deocdedMsg))
		{
			msg = deocdedMsg;
		}
	}
	TryUnCompressMessasge(msg);
	return msg;
}

bool CCTPlatform::_Process_Restart(BasePacket& baseRequest)
{
	int ret = myShellExecute("shutdown -r now", "");
	if (0 == ret)
	{
		MqttReportSucceed(baseRequest);
		return true;
	}
	else
	{
		MqttReportError(baseRequest, ret, "执行失败");
		return false;
	}
}

bool CCTPlatform::_Process_Reload(BasePacket& baseRequest)
{
	MqttReportSucceed(baseRequest, "重新加载");
	exit(EXIT_RELOAD);
	return false;
}

bool CCTPlatform::_Process_ResetConf(BasePacket& baseRequest)
{
	CEasyFile file;

	if (!file.IsFileExist(deviceConfigFile) || file.DeleteFile(deviceConfigFile))
	{
		MqttReportSucceed(baseRequest, " 处理成功");
		exit(EXIT_RESET);
	}
	else
	{
		MqttReportError(baseRequest, 1, "处理失败");
		return false;
	}
}

bool CCTPlatform::_Process_Acquire(BasePacket& baseRequest)
{
	g_Global.pCGwConfig->ReAcquire();
	MqttReportSucceed(baseRequest, "请等待数据上报");
	return true;
}

bool CCTPlatform::_Process_Show(BasePacket& baseRequest)
{
	MqttReportSucceed(baseRequest, g_Global.pCGwConfig->GetDeviceConfig().ToString().c_str());
	return true;
}

bool CCTPlatform::_Process_EnableCompress(BasePacket& baseRequest)
{
	g_Global.pCGwConfig->GetAppConfig().enableCompress = true;
	g_Global.pCGwConfig->GetAppConfig().Save();
	MqttReportSucceed(baseRequest, "已启用压缩");
	return true;
}

bool CCTPlatform::_Process_EnableCrypt(BasePacket& baseRequest)
{
	g_Global.pCGwConfig->GetAppConfig().enableAllCrypt = true;
	g_Global.pCGwConfig->GetAppConfig().Save();
	MqttReportSucceed(baseRequest, "已启用加密");
	return true;
}

bool CCTPlatform::_Process_List(BasePacket& baseRequest, cJSON* _cjson_data)
{
	string inputdir;
	if (NULL == _cjson_data || !CJsonUtil::_GetJsonParam(_cjson_data, "dir", inputdir))
	{
		inputdir = ".";
	}
	if (0 == inputdir.size())inputdir = ".";

	cJSON* cjson = CreateBaseJson("list", baseRequest.uid.c_str());

	cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");

	cJSON* cjson_file_list = cJSON_AddArrayToObject(cjson_data, "file_list");

	stringstream ss;
	try
	{
		struct dirent* drip;
		DIR* dp;
		string fullname;
		if ((dp = opendir(inputdir.c_str())) == NULL)
		{
			ss << "Error open dir " << inputdir << " : " << strerror(errno);
			throw 1;
		}

		while ((drip = readdir(dp)) != NULL)
		{
			if (strcmp(drip->d_name, ".") == 0 || strcmp(drip->d_name, "..") == 0)
				continue;

			fullname = inputdir + "/" + drip->d_name;

			bool isDir = CDir::IsDir(fullname.c_str());

			string checksum;
			int file_size = -1;
			string file_time;
			{
				struct stat file_stat;
				struct tm* t2;
				char buf[256];
				if (stat(fullname.c_str(), &file_stat) != 0)
				{
					ss << "Error stat " << fullname << " : " << strerror(errno);
					throw 2;
				}
				t2 = localtime(&file_stat.st_mtime);
				strftime(buf, 128, "%Y-%m-%d %H:%M:%S", t2);
				file_time = buf;
			}

			if (!isDir)
			{
				file_size = getFileCheckSum(fullname.c_str(), checksum);
			}
			else
			{
				checksum = "dir";
			}

			cJSON* cjson_file = cJSON_CreateObject();
			cJSON_AddStringToObject(cjson_file, "file_name", drip->d_name);
			cJSON_AddStringToObject(cjson_file, "is_dir", (isDir ? "1" : "0"));
			cJSON_AddNumberToObject(cjson_file, "file_size", file_size);
			cJSON_AddStringToObject(cjson_file, "file_time", file_time.c_str());
			cJSON_AddStringToObject(cjson_file, "file_checksum", checksum.c_str());

			cJSON_AddItemToArray(cjson_file_list, cjson_file);

		}
		closedir(dp);
	}
	catch (...)
	{
	}
	string json = cJSON_Print(cjson);
	cJSON_free(cjson);

	m_pMqttSvr->MqttPostMessage("list", "", json);
	return true;
}

bool CCTPlatform::_Process_Reg(BasePacket& baseRequest, cJSON* cjson_data)
{
	int code;
	if (NULL == cjson_data || !CJsonUtil::_GetJsonParam(cjson_data, "code", code))
	{
		MqttReportError(baseRequest, 1, "参数错误，未定义code");
		return false;
	}
	if (0 == code)this->m_bRegisted = true;
	MqttReportError(baseRequest, code, "收到平台注册应答");
	return true;;
}

bool CCTPlatform::__WriteChannel(BasePacket& baseRequest, InterfaceDeviceConfig* pInterface, Channel* pChannel, string const& value)
{
	thelog << "通道找到 " << pChannel->channelNo << endi;
	if (pChannel->CanWrite())
	{
		//if (pChannel->HasWriteValue())
		//{
		//	MqttReportError(baseRequest, 3, "上次写入尚未执行 ", pChannel->channelNo);
		//	return false;
		//}
		//else
		{
			pChannel->SetWriteValue(pChannel->dataType, pChannel->charsetCode, value);
			pInterface->_ProtocolSvr->ProtoSvrActive();
		}
	}
	else
	{
		thelog << "不支持写入 " << pChannel->ToString() << ende;
		MqttReportError(baseRequest, 4, "不支持写入 ", pChannel->channelNo);
		return false;
	}
	return true;
}

bool CCTPlatform::_Process_Write(BasePacket& baseRequest, cJSON* cjson_data)
{
	bool ret = true;
	for (int i = 0; i < cJSON_GetArraySize(cjson_data); ++i)
	{
		cJSON* cjson_data_item = cJSON_GetArrayItem(cjson_data, i);
		string deviceCode;
		int channelNo;
		string paramCode;//channelNo存在则不检查paramCode，paramCode为空则使用channelNo
		string value;
		if (!CJsonUtil::_GetJsonParam(cjson_data_item, "deviceCode", deviceCode))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_data_item, "channelNo", channelNo) && !CJsonUtil::_GetJsonParam(cjson_data_item, "paramCode", paramCode))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_data_item, "value", value))ret = false;
		thelog << "指令：向设备[" << deviceCode << "] channelNo[" << channelNo << "] paramCode[" << paramCode << "] 写入[" << value << "]" << endi;

		InterfaceDeviceConfig* pInterface;
		Device* pDevice = g_Global.pCGwConfig->GetDeviceConfig().FindDevice(deviceCode.c_str(), &pInterface);
		if (!pDevice)
		{
			MqttReportError(baseRequest, 1, ("未知的设备" + deviceCode).c_str());
		}
		else
		{
			thelog << "设备找到" << endi;
			Channel* pChannel = (paramCode.size() > 0 ? pDevice->FindByParamCode(paramCode.c_str()) : pDevice->FindByChannelNo(channelNo));
			if (!pChannel)
			{
				string err = "未知的 paramCode[";
				err += paramCode + "]或 channelNo ";
				MqttReportError(baseRequest, 2, err.c_str(), channelNo);
			}
			else
			{
				__WriteChannel(baseRequest, pInterface, pChannel, value);
			}
		}
	}

	return ret;
}
bool CCTPlatform::_Process_Send(BasePacket& baseRequest, cJSON* cjson_data)
{
	string deviceCode;
	if (NULL == cjson_data || !CJsonUtil::_GetJsonParam(cjson_data, "deviceCode", deviceCode))
	{
		MqttReportError(baseRequest, 1, "参数错误，未定义deviceCode");
		return false;
	}
	InterfaceDeviceConfig* pInterface;
	Device* pDevice = g_Global.pCGwConfig->GetDeviceConfig().FindDevice(deviceCode.c_str(), &pInterface);
	if (!pDevice)
	{
		MqttReportError(baseRequest, 1, ("未知的设备" + deviceCode).c_str());
		return false;
	}
	else
	{
		thelog << "设备找到" << endi;
		string dataValue;
		if (!CJsonUtil::_GetJsonParam(cjson_data, "dataValue", dataValue))
		{
			MqttReportError(baseRequest, 1, "没有dataValue");
			return false;
		}
		thelog << "指令：向设备[" << deviceCode << "] 写入[" << dataValue << "]" << endi;
		WriteInfo tmp;
		tmp.uid = baseRequest.uid;
		tmp.value = dataValue;
		pDevice->writeList.locked_push_back(tmp);
	}
	return true;
}
void CCTPlatform::AddPushData(long conn, char const* deviceCode, int channelNo, char const* channelCode, char const* channelName, int64_t ts, bool isNumber, double numberValue, char const* strValue, int errorCode, char const* errorMessage)
{
	auto  connDatas = m_map_datas.find(conn);
	if (connDatas == m_map_datas.end())
	{
		list<pair<string, map<int, report_channel> > > tmp;
		connDatas = m_map_datas.insert(make_pair(conn, tmp)).first;
	}

	pair < string, map<int, report_channel> >* pDevice = nullptr;

	for (auto& d : connDatas->second)
	{
		if (d.first == deviceCode)
		{
			pDevice = &d;
		}
	}
	if (!pDevice)
	{
		pair < string, map<int, report_channel> > tmp;
		tmp.first = deviceCode;
		m_map_datas[conn].push_back(tmp);
		pDevice = &*m_map_datas[conn].rbegin();
	}

	report_channel channel;

	channel.channelCode = channelCode;
	channel.channelName = channelName;
	channel.ts = ts;
	channel.isNumber = isNumber;
	channel.numberValue = numberValue;
	if (strValue)channel.strValue = strValue;
	channel.errorCode = errorCode;
	if (errorMessage)channel.errorMessage = errorMessage;

	pDevice->second[channelNo] = channel;
}
int CCTPlatform::GetDataCount(long conn)
{
	auto  connDatas = m_map_datas.find(conn);
	if (connDatas == m_map_datas.end())return 0;

	int count = 0;

	for (auto& d : connDatas->second)
	{
		count += d.second.size();
	}

	return count;
}
bool CCTPlatform::post_DataMessage(long conn, char const* deviceCode, bool isAll, string events, long ms)
{
	T_DATA_SEQ seq = g_Global.pCGwConfig->Add_last_data_sequence();

	cJSON* cjson = CreateBaseJson("push");

	cJSON_AddNumberToObject(cjson, "identify", seq);
	cJSON* cjson_data = cJSON_AddArrayToObject(cjson, "data");

	auto  connDatas = m_map_datas.find(conn);
	if (connDatas == m_map_datas.end())
	{
		thelog << deviceCode << " 上报数据个数 " << 0 << endi;
		cJSON_AddStringToObject(cjson, "info", "nothing to push");
	}
	else
	{
		cJSON* cjson_device = cJSON_CreateObject();

		cJSON_AddStringToObject(cjson_device, "deviceCode", deviceCode);
		cJSON_AddNumberToObject(cjson_device, "type", (isAll ? 1 : 0));
		cJSON_AddStringToObject(cjson_device, "events", events.c_str());
		if (ms > 0)cJSON_AddNumberToObject(cjson_device, "ms", ms);
		cJSON* cjson_count = cJSON_AddNumberToObject(cjson_device, "count", 0);
		cJSON* cjson_dv = cJSON_AddArrayToObject(cjson_device, "dataValues");

		for (auto& d : connDatas->second)
		{
			if (d.first != deviceCode)continue;

			thelog << deviceCode << " 上报数据个数 " << d.second.size() << endi;
			cJSON_SetNumberValue(cjson_count, d.second.size());
			for (auto& mc : d.second)
			{
				auto c = mc.second;
				cJSON* cjson_channel = cJSON_CreateObject();

				cJSON_AddStringToObject(cjson_channel, "paramCode", c.channelCode.c_str());
				cJSON_AddStringToObject(cjson_channel, "paramName", c.channelName.c_str());
				cJSON_AddNumberToObject(cjson_channel, "ts", c.ts);

				if (c.errorCode != 0)
				{
					cJSON_AddNumberToObject(cjson_channel, "errorCode", c.errorCode);
					cJSON_AddStringToObject(cjson_channel, "errorMessage", c.errorMessage.c_str());
				}
				else
				{
					if (c.isNumber)cJSON_AddNumberToObject(cjson_channel, "val", c.numberValue);
					else cJSON_AddStringToObject(cjson_channel, "val", c.strValue.c_str());
				}

				cJSON_AddItemToArray(cjson_dv, cjson_channel);
			}

			d.second.clear();
		}

		cJSON_AddItemToArray(cjson_data, cjson_device);
	}

	char* json = cJSON_PrintUnformatted(cjson);//cJSON_Print是带格式的
	string msg = json;
	free(json);
	cJSON_free(cjson);

	g_Global.pCDataManager->AddData(seq, msg.c_str());

	return this->m_pMqttSvr->MqttPostDataMessage("push", "", msg, seq);
}
bool CCTPlatform::post_Message(char const* func, char const* message)
{
	return this->m_pMqttSvr->MqttPostMessage(func, "", message);
}
string CCTPlatform::build_State()
{
	g_Global.pCGwConfig->GetBaseInfo().Update();

	cJSON* cjson = CreateBaseJson("state");

	cJSON* cjson_data = cJSON_AddObjectToObject(cjson, "data");

	cJSON_AddNumberToObject(cjson_data, "gw_state", 0);
	cJSON_AddStringToObject(cjson_data, "mobile_type", "5G");
	cJSON_AddStringToObject(cjson_data, "csq", g_Global.pCGwConfig->GetBaseInfo().csq.c_str());

	cJSON* cjson_interface_state = cJSON_AddArrayToObject(cjson_data, "interface_state");
	for (auto const& i : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
	{
		cJSON* cjson_interface = cJSON_CreateObject();

		cJSON_AddStringToObject(cjson_interface, "interface_code", i->interfaceCode.c_str());
		cJSON_AddNumberToObject(cjson_interface, "interface_state", static_cast<int>(i->_InterfaceState));

		cJSON* cjson_device_state = cJSON_AddArrayToObject(cjson_interface, "device_state");
		for (auto const& d : i->devices)
		{
			cJSON* cjson_device = cJSON_CreateObject();

			cJSON_AddStringToObject(cjson_device, "device_code", d->deviceCode.c_str());
			cJSON_AddNumberToObject(cjson_device, "device_state", static_cast<int>(d->_DeviceState));

			cJSON* cjson_channel_state = cJSON_AddArrayToObject(cjson_device, "channel_state");
			for (auto const& c : d->channels)
			{
				cJSON* cjson_channel = cJSON_CreateObject();

				cJSON_AddNumberToObject(cjson_channel, "channel_no", c->channelNo);
				cJSON_AddStringToObject(cjson_channel, "channel_code", c->paramCode.c_str());
				cJSON_AddStringToObject(cjson_channel, "channel_name", c->paramName.c_str());
				cJSON_AddNumberToObject(cjson_channel, "channel_state", c->GetChannelState());
				cJSON_AddNumberToObject(cjson_channel, "channel_write_state", c->GetChannelWriteState());

				cJSON_AddItemToArray(cjson_channel_state, cjson_channel);
			}

			cJSON_AddItemToArray(cjson_device_state, cjson_device);
		}

		cJSON_AddItemToArray(cjson_interface_state, cjson_interface);
	}

	string json = cJSON_Print(cjson);
	cJSON_free(cjson);
	return json;
}
