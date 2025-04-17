//PreFilledKey.h 预充注密钥管理

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "gwconfig.h"
#include <limits.h>

class CPreFilledKeyManager
{
private:
	char const* data_file_name = "prefilledkey.txt";//数据文件名
	//line基于1
	bool _ReadKey(bool bySEQ, long seq_or_line, string& ret_key)
	{
		ret_key = "";

		FILE* f = fopen(data_file_name, "r");
		if (!f)
		{
			thelog << "打开文件失败 " << data_file_name << endi;
			return false;
		}

		char buf[1024];
		long line = 0;
		while (fgets(buf, 1024, f))
		{
			++line;

			char buf_seq[32 + 1];
			char buf_key[64 + 1];
			buf_seq[0] = '\0';
			buf_key[0] = '\0';

			sscanf(buf, "%32s %64s", buf_seq, buf_key);
			//thelog << line << " : " << buf_seq << " : " << buf_key << endi;

			if (bySEQ)
			{
				if (seq_or_line != atol(buf_seq))
				{
					continue;
				}
			}
			else
			{
				if (line < seq_or_line)
				{
					continue;
				}
			}

			ret_key = buf_key;
			fclose(f);
			return true;
		}

		thelog << "指定的密钥不存在 " << bySEQ << " " << seq_or_line << ende;
		fclose(f);
		return false;
	}
public:
	bool ReadKeyBySEQ(long seq, string& ret_key)
	{
		return _ReadKey(true, seq, ret_key);
	}
	//line基于1
	bool ReadKeyByLine(long line, string& ret_key)
	{
		return _ReadKey(false, line, ret_key);
	}
};
