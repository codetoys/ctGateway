//ProtocolDemo.cpp 协议范例

#include "ProtocolDemo.h" 
#include "uuid.h" 
#include <gwBuiltIn/gwmain_test.h>

class QJ57B_1A
{
public:
	typedef unsigned char byte;
	typedef unsigned int uint;
	typedef unsigned long ulong;
	class bufferTools
	{
	public:
		//压缩的BCD码转int
		static int BCD2Int(byte bcd)
		{
			return (bcd >> 4) * 10 + (bcd & 0x0F);
		}
		//int转压缩的BCD码
		static byte Int2BCD(int n)
		{
			return (byte)(((n / 10) << 4) + (n % 10));
		}
		//byteorder为true表示高在前
		static uint Bytes2UInt(bool byteorder, byte* bytes, int length)
		{
			ulong ret = 0;
			if (byteorder)
			{
				for (int i = 0; i < length; ++i)
				{
					ret = ret * 256 + bytes[i];
				}
			}
			else
			{
				for (int i = length - 1; i >= 0; --i)
				{
					ret = ret * 256 + bytes[i];
				}
			}
			return (uint)ret;
		}
		static byte* UInt2Bytes(bool byteorder, uint n, byte* ret, int bytes)
		{
			uint x = n;
			if (byteorder)
			{
				for (int i = 0; i < bytes; ++i)
				{
					ret[bytes - i - 1] = (byte)(x % 256);
					x /= 256;
				}
			}
			else
			{
				for (int i = 0; i < bytes; ++i)
				{
					ret[i] = (byte)(x % 256);
					x /= 256;
				}
			}
			return ret;
		}
	};
	class QJ57B_1A_data
	{
	public:
		CMyTime ts;//时间戳，采集到的时间

		int year = 0;
		int month = 0;
		int day = 0;
		int hour = 0;
		int minute = 0;
		int second = 0;

		unsigned int Rt = 0;
		unsigned char Rt_point = 0;
		unsigned char Rt_unit = 0;

		unsigned int R20 = 0;
		unsigned char R20_point = 0;
		unsigned char R20_unit = 0;

		unsigned int Rate = 0;
		unsigned char Rate_point = 0;
		unsigned char Rate_unit = 0;

		unsigned int T = 0;
		unsigned char T_point = 0;
		unsigned char T_unit = 0;

		void GetFromFrame(unsigned char* frame)
		{
			year = bufferTools::BCD2Int(frame[3]);
			month = bufferTools::BCD2Int(frame[4]);
			day = bufferTools::BCD2Int(frame[5]);
			hour = bufferTools::BCD2Int(frame[6]);
			minute = bufferTools::BCD2Int(frame[7]);
			second = bufferTools::BCD2Int(frame[8]);
			Rt = bufferTools::Bytes2UInt(true, frame + 9, 4);
			Rt_point = frame[13];
			Rt_unit = frame[14];
			R20 = bufferTools::Bytes2UInt(true, frame + 15, 4);
			R20_point = frame[19];
			R20_unit = frame[20];
			Rate = bufferTools::Bytes2UInt(true, frame + 21, 4);
			Rate_point = frame[25];
			Rate_unit = frame[26];
			T = bufferTools::Bytes2UInt(true, frame + 27, 2);
			T_point = frame[29];
			T_unit = frame[30];
		}
		string toString()const
		{
			stringstream ss;
			ss << "20" << year << "年 " << month << "月" << day << "日 " << hour << ":" << minute << ":" << second;
			ss << " Rt:" << Rt << " " << Rt_point << " " << Rt_unit;
			ss << " R20:" << R20 << " " << R20_point << " " << R20_unit;
			ss << " Rate:" << Rate << " " << Rate_point << " " << Rate_unit;
			ss << " T:" << T << " " << T_point << " " << T_unit;
			return ss.str();
		}
		cJSON* _makeValue(string name, long value)
		{
			cJSON* cjson = cJSON_CreateObject();
			cJSON_AddStringToObject(cjson, "index", name.c_str());
			cJSON_AddNumberToObject(cjson, "val", value);
			return cjson;
		}
		string toJson(string sn)
		{
			CMyTime ts;
			ts.SetCurrentTime();

			cJSON* cjson = cJSON_CreateObject();
			cJSON_AddStringToObject(cjson, "func", "push");
			cJSON_AddStringToObject(cjson, "sn", sn.c_str());
			cJSON_AddNumberToObject(cjson, "ts", ts.GetTS_ms());
			cJSON_AddStringToObject(cjson, "uid", CUUID::generate_uuid().c_str());

			cJSON* jsonArray = cJSON_AddArrayToObject(cjson, "data");
			cJSON_AddItemToArray(jsonArray, _makeValue("year", year));
			cJSON_AddItemToArray(jsonArray, _makeValue("month", month));
			cJSON_AddItemToArray(jsonArray, _makeValue("day", day));
			cJSON_AddItemToArray(jsonArray, _makeValue("hour", hour));
			cJSON_AddItemToArray(jsonArray, _makeValue("minute", minute));
			cJSON_AddItemToArray(jsonArray, _makeValue("second", second));

			cJSON_AddItemToArray(jsonArray, _makeValue("rt", Rt));
			cJSON_AddItemToArray(jsonArray, _makeValue("rt_point", Rt_point));
			cJSON_AddItemToArray(jsonArray, _makeValue("rt_unite", Rt_unit));

			cJSON_AddItemToArray(jsonArray, _makeValue("r20", R20));
			cJSON_AddItemToArray(jsonArray, _makeValue("r20_point", R20_point));
			cJSON_AddItemToArray(jsonArray, _makeValue("r20_unite", R20_unit));

			cJSON_AddItemToArray(jsonArray, _makeValue("r", Rate));
			cJSON_AddItemToArray(jsonArray, _makeValue("r_point", Rate_point));
			cJSON_AddItemToArray(jsonArray, _makeValue("r_unite", Rate_unit));

			cJSON_AddItemToArray(jsonArray, _makeValue("t", T));
			cJSON_AddItemToArray(jsonArray, _makeValue("t_point", T_point));
			cJSON_AddItemToArray(jsonArray, _makeValue("t_unite", T_unit));

			string json = cJSON_Print(cjson);
			cJSON_free(cjson);
			return json;
		}
	};
	//处理数据，返回data
	static int ProcessData(byte * buffer,int length, QJ57B_1A_data & data)
	{
		if (length != 32)
		{
			thelog << "长度不正确" << ende;
			return __LINE__;
		}
		if (buffer[0] != 0xFC || buffer[1] != 0xFC || buffer[2] != 0xA0)
		{
			thelog << "开始标志不正确" << ende;
			return __LINE__;
		}
		if (buffer[32 - 1] != 0xFA)
		{
			thelog << "结束符不正确" << ende;
			return __LINE__;
		}
		else
		{
			data.GetFromFrame(buffer);
			thelog<<data.toString()<<endi;
		}

		return 0;
	}
};

void CDemoBase::recv_ms(string const& encodeType, int ms, bool once)
{
	char buf[1024];
	CMyTime t;
	t.SetCurrentTime();
	while (t.GetTimeSpanMS() < ms)
	{
		thelog << "recv_ms" << endi;
		long read_count = 0;

		if (!pConnection->Recv(buf, 1024, &read_count, ms))
		{
			thelog << "Recv 失败" << ende;
			break;
		}
		thelog << "Recv 返回 read_count=" << read_count << endi;
		if (0 == read_count)
		{
			thelog << "=================================================Recv 没有数据" << ende;
			break;
		}

		m_bReceivedData = true;//指示接收到了数据，设备正常

		thelog << m_pInertfaceDevice->devices.size() << endi;
		thelog << m_DeviceParam.deviceModel << endi;
		if (m_pInertfaceDevice->devices.size() > 0 && 0 == strcmp(DEVICE_MODEL_QJ57B_1A, m_DeviceParam.deviceModel.c_str()))
		{//指定设备型号的特殊处理
			string str = "收到数据 ";
			str += CMyTools::ToHex(buf, read_count);

			QJ57B_1A::QJ57B_1A_data frame;
			if (0 != QJ57B_1A::ProcessData((unsigned char*)buf, read_count, frame))
			{
				CProtocolPublic::post_Message("push", (str+"处理失败").c_str());
			}
			else
			{
				CProtocolPublic::post_Message("push", frame.toJson(CProtocolPublic::GetSN()).c_str());
			}
		}
		else
		{//通用处理
			string str;
			if ("hex" == encodeType)
			{
				str = CMyTools::ToHex(buf, read_count);
			}
			else
			{
				buf[read_count] = '\0';
				str = buf;
			}
			thelog << CMyTools::ToHex(buf, read_count) << endi;

			string deviveCode = "undefined";
			if (m_pInertfaceDevice->devices.size() > 0)deviveCode = m_pInertfaceDevice->devices[0]->deviceCode;
			CMyTime ts;
			ts.SetCurrentTime();
			CProtocolPublic::AddPushData((long)m_pInertfaceDevice, deviveCode.c_str(), 0, "", "", ts.GetTS_ms(),
				false, 0
				, str.c_str(), 0, "");
			CProtocolPublic::post_DataMessage((long)m_pInertfaceDevice, deviveCode.c_str(), false, "", 0);
		}

		if (once)break;
	}
	thelog << "recv_ms 返回" << endi;
}

void CDemoBase::send_recv(string const& encodeType, string const& str)
{
	int buflen = str.size();
	unsigned char* buf = new unsigned char[buflen + 1];
	if (buf)
	{
		int len;
		if ("hex" == encodeType)
		{
			len = CMyTools::HexToBuffer((unsigned char*)str.c_str(), str.size(), buf);
			thelog << "发送数据：" << CMyTools::ToHex((char*)buf, len) << endi;
		}
		else
		{
			strcpy((char*)buf, str.c_str());
			len = str.size();
			thelog << "发送数据：" << buf << endi;
		}
		if (!pConnection->Send((char*)buf, len))
		{
			thelog << "发送出错 " << this->m_pInertfaceDevice->interfaceCode << " " << getIProtocolParam()->ToString() << ende;
		}
		else
		{
			thelog << "发送成功" << endi;
			recv_ms(encodeType, 1000, true);
			thelog << "接收完成" << endi;
		}
		delete[]buf;
	}
	else
	{
		thelog << "申请内存失败，无法发送 " << ende;
	}
}

void CDemoBase::process_send_command()
{
	for (auto& d : m_pInertfaceDevice->devices)
	{
		WriteInfo writeinfo;
		while (d->writeList.locked_TryGetBegin(writeinfo))
		{
			send_recv(m_DeviceParam.encodeType, writeinfo.value);

			d->writeList.locked_pop_front();
		}
	}
}

void CDemoBase::protocol_process_job()
{
	bool bConnecting = false;//连接中，循环尝试连接只在第一次上报
	while (true)
	{
		if (!CProtocolPublic::isPlatformConnected())
		{
			thelog << "平台尚未连接" << ende;
			SleepSeconds(5);
			continue;
		}
		if (!pConnection->IConnection_isConnected())
		{
			thelog << "***************************连接 " << this->m_pInertfaceDevice->interfaceCode << " " << getIProtocolParam()->ToString() << endi;
			if(!bConnecting)CProtocolPublic::post_ReportWithFunc("connecting", "正在连接", m_pInertfaceDevice->interfaceCode.c_str());
			bConnecting = true;
			if (!pConnection->EnsureConnected(this->m_pInertfaceDevice))
			{
				thelog << "====================================================连接失败 " << this->m_pInertfaceDevice->interfaceCode << " " << getIProtocolParam()->ToString() << ende;
				this->m_pInertfaceDevice->_InterfaceState = InterfaceState::CONNECT_ERROR;
				string msg = "无法连接设备（接口），包含的设备：";
				for (auto& device : m_pInertfaceDevice->devices)
				{
					msg += device->deviceCode;
					msg += " ";
				}
				thelog << msg << ende;
				//post_ReportWithFunc("connecting_failed", msg.c_str(), m_pInertfaceDevice->interfaceCode.c_str());
				//SleepSeconds(5);
				continue;
			}
			thelog << "====================================================连接成功 " << this->m_pInertfaceDevice->interfaceCode << " " << getIProtocolParam()->ToString() << endi;
			CProtocolPublic::post_ReportWithFunc("connected", "连接成功", m_pInertfaceDevice->interfaceCode.c_str());
			bConnecting = false;
			this->m_pInertfaceDevice->_InterfaceState = InterfaceState::CONNECT_OK;
		}

		//发送每个指令并接收一次（其实只有一个设备）
		m_bReceivedData = false;
		for (auto& d : m_pInertfaceDevice->devices)
		{
			thelog << "先处理下发写指令" << endi;
			process_send_command();

			thelog << "再处理周期写指令" << endi;
			for (auto& c : d->channels)
			{
				TunnelChannelParam* pChannelParam = m_channelparams[c->channelNo];
				send_recv(m_DeviceParam.encodeType, pChannelParam->paramCommand);
			}

			thelog << "剩余时间全部用来接收" << endi;
			while (d->_allTime.GetTimeSpanMS() < d->_rate)
			{
				thelog << d->_allTime.GetTimeSpanMS() << " " << d->_rate << endi;
				recv_ms(m_DeviceParam.encodeType, 1000, false);

				thelog << "处理下发写指令" << endi;
				process_send_command();
			}

			d->_allTime.SetCurrentTime();
		}
		for (auto& d : m_pInertfaceDevice->devices)
		{
			d->_DeviceState = (m_bReceivedData ? DeviceState::CONNECT_OK : DeviceState::CONNECT_ERROR);//因为无法区分具体设备，所以任何一个设备有数据都被当作正常
		}
	}
}
