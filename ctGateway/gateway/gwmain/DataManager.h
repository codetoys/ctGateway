//DataManager.h 数据管理

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "gwconfig.h"
#include <limits.h>

class CDataManager
{
private:
	FILE* file=NULL;//当前文件

	char const* data_file_name = "data.dat";//当前文件
	char const* data_file_name2 = "data2.dat";//旧文件
	bool GetSeq(char const* filename, T_DATA_SEQ& min_seq, T_DATA_SEQ& max_seq)
	{
		min_seq = numeric_limits<T_DATA_SEQ>::max();
		max_seq = 0;

		FILE* f = fopen(filename, "r");
		if (!f)
		{
			//thelog << "打开文件失败 " << filename << endi;
			return false;
		}
		
		char buf[1024];
		while (fgets(buf, 1024, f))
		{
			//thelog << buf << endi;
			bool bNotNumber = false;
			char* p;
			for (p = buf; *p != '\0' && *p != '\r' && *p != '\n'; ++p)
			{
				if (*p < '0' || *p>'9')
				{
					bNotNumber = true;
					break;
				}
			}
			if (bNotNumber || p == buf)continue;//p==buf表明没有有效的数字

			T_DATA_SEQ seq = atol(buf);
			//thelog << seq << endi;
			if (seq < min_seq)min_seq = seq;
			if (seq > max_seq)max_seq = seq;
		}

		fclose(f);

		//thelog << min_seq << " " << max_seq << endi;
		return true;
	}
	static bool _WriteData(FILE* f, T_DATA_SEQ data_sequence, char const* _data)
	{
		string data = _data;
		TryCompressMessasge(data);
		
		char buf[256];
		sprintf(buf, "%llu\n", (unsigned long long)data_sequence);
		if (fputs(buf, f) < 0)return false;
		if (fputs(data.c_str(), f) < 0)return false;
		if (fputs("\n", f) < 0)return false;
		if (fflush(f) < 0)return false;
		return true;
	}
public:
	//不需要析构，因为程序只会因为断电结束
	void CDataManagerInit()
	{
		thelog << "已确认的数据标识 "<< g_Global.pCGwConfig->GetConfirmedSeq() <<" last "<< g_Global.pCGwConfig->GetLastDataSeq()<<" 已发送 "<< g_Global.pCGwConfig->GetLastSendedSeq() << endi;

		//thelog << "获取最小最大数据标识--------这部分可能无意义，因为没有考虑循环" << endi;
		//{
		//	g_Global.pCGwConfig->SetLastDataSeq(0);

		//	T_DATA_SEQ min_seq=0;
		//	T_DATA_SEQ max_seq=0;
		//	T_DATA_SEQ min_seq2=0;
		//	T_DATA_SEQ max_seq2 = 0;
		//	GetSeq(data_file_name, min_seq, max_seq);
		//	GetSeq(data_file_name2, min_seq2, max_seq2);

		//	T_DATA_SEQ new_min = min(min_seq, min_seq2);
		//	if (new_min - 1 != g_Global.pCGwConfig->GetConfirmedSeq())
		//	{
		//		thelog << "部分未确认数据已丢失，从 " << g_Global.pCGwConfig->GetConfirmedSeq() + 1 << " 到 " << new_min - 1 << ende;
		//		g_Global.pCGwConfig->SetConfirmedSeq(new_min - 1);
		//	}
		//	g_Global.pCGwConfig->SetLastDataSeq(max(max_seq, max_seq2));
		//	thelog << "最小值 " << new_min << " 序列 " << g_Global.pCGwConfig->GetLastDataSeq() << " 已确认 " << g_Global.pCGwConfig->GetConfirmedSeq() << endi;
		//}
	}
	//添加一条数据，保存到文件
	void AddData(T_DATA_SEQ data_sequence, char const* data)
	{
		if (!file)
		{
			file = fopen(data_file_name, "a");
		}
		_WriteData(file, data_sequence, data);
		if (ftell(file) > g_Global.pCGwConfig->GetMaxDataFile()/2)
		{
			fclose(file);
			remove(data_file_name2);
			rename(data_file_name, data_file_name2);
			file = fopen(data_file_name, "a");
		}
	}
	//确认数据，清除已经确认的
	bool ConfirmData(T_DATA_SEQ data_sequence)
	{
		g_Global.pCGwConfig->SetConfirmedSeq(data_sequence);
	
		map<T_DATA_SEQ, string > datas;
		LoadData(1, numeric_limits<T_DATA_SEQ>::max(), datas, false);
		thelog << "加载到 " << datas.size() << "条数据，已确认序列号 "<< data_sequence << endi;
		
		remove(data_file_name2);
		FILE* f = fopen(data_file_name2, "a");
		if (!f)
		{
			thelog << "打开文件失败 " << data_file_name2 << ende;
			return false;
		}
		for (auto& data : datas)
		{
			if (data.first <= data_sequence)thelog << "需要丢弃 " << data.first << endi;
			else
			{
				thelog << "未确认 " << data.first << endi;
				_WriteData(f, data.first, data.second.c_str());
			}
		}
		fclose(f);

		return true;
	}
	//加载数据（以供重新发送），如果某条数据不存在，返回false，注意，并不首先清空ret
	bool LoadData(T_DATA_SEQ from, T_DATA_SEQ to, map<T_DATA_SEQ, string >& ret, bool allFiles = true)
	{
		vector<char const*> files;
		files.push_back(data_file_name2);
		if (allFiles)files.push_back(data_file_name);
		for (auto & filename : files)
		{
			FILE* f = fopen(filename, "r");
			if (!f)
			{
				//thelog << "打开文件失败 " << filename << endi;
				continue;
			}

			char buf[1024];
			T_DATA_SEQ seq=0;
			string message;
			while (fgets(buf, 1024, f))
			{
				//thelog << buf << endi;
				bool bNotNumber = false;
				char* p;
				for (p = buf; *p != '\0' && *p != '\r' && *p != '\n'; ++p)
				{
					if (*p < '0' || *p>'9')
					{
						bNotNumber = true;
						break;
					}
				}
				if (bNotNumber || p == buf)
				{
					message += buf;
				}
				else
				{
					if (seq >= from && seq <= to && Trim(message).size() > 0)ret[seq] = message;
					seq = atol(buf);
					message = "";
				}
			}
			if (seq > 0 && message.size() > 0)
			{
				if (seq >= from && seq <= to && Trim(message).size() > 0)ret[seq] = TryUnCompressMessasge(message);
			}

			fclose(f);
		}
		//检查是否存在缺失
		for (T_DATA_SEQ x = from; x <= to; ++x)
		{
			if (ret.end() == ret.find(x))return false;
		}
		return true;
	}
	//带缓存的加载数据，ret返回from的数据
	bool LoadDataWithCache(T_DATA_SEQ from, T_DATA_SEQ to, string & ret)
	{
		static map<T_DATA_SEQ, string > data_cache;
		map<T_DATA_SEQ, string >::iterator it = data_cache.find(from);
		if (it != data_cache.end())
		{
			ret = it->second;
			return true;
		}

		//缓存数据，每个文件每次读取最多maxcount条
		constexpr int maxcount = 100;
		data_cache.clear();
		if (to - from + 1 > maxcount)LoadData(from, from + maxcount - 1, data_cache);
		else LoadData(from, to, data_cache);

		it = data_cache.find(from);
		if (it != data_cache.end())
		{
			ret = it->second;
			return true;
		}
		return false;
	}
};
