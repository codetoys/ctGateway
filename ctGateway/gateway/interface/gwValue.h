//gwValue.h 通用值类型

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "CharSetConv.h"
#include "gwUtil.h"

//值，依据dataType
struct ChannelValue
{
	bool _bitValue = false;
	uint16_t _uint16Value = 0;//有符号或无符号
	uint32_t _uint32Value = 0;
	float _f32Value = 0;
	string _strValue;//注意，是设备字符集

	bool bHasValue = false;
	//设置要写入的值，注意字符串被转换成了设备字符集
	void ChannelValue_SetWriteValue(string const& dataType, string const& charset, string const& data)
	{
		if ("float" == dataType)
		{
			_f32Value = atof(data.c_str());
		}
		else if ("boolean" == dataType)
		{
			if (0 == stricmp(data.c_str(), "true"))_bitValue = true;
			else if (0 == stricmp(data.c_str(), "false"))_bitValue = false;
			else
			{
				char* endptr;
				_bitValue = strtoul(data.c_str(), &endptr, 10);
			}
		}
		else if ("uint" == dataType)
		{
			char* endptr;
			_uint32Value = strtoul(data.c_str(), &endptr, 10);
		}
		else if ("short" == dataType)
		{
			char* endptr;
			int16_t tmp = strtol(data.c_str(), &endptr, 10);
			memcpy(&_uint16Value, &tmp, 2);
		}
		else if ("string" == dataType)
		{
			CodeConverter codeconverter;
			if (0 != charset.size() && codeconverter.initCodeConverter("UTF-8", charset.c_str()))
			{
				string str = data;
				if (!codeconverter.CharSetConvert(str, getTmpDir()))thelog << "转码失败" << ende;
				else
				{
					_strValue = str;
					//thelog << charset << " " << CMyTools::ToHex(str) << " 转换后字符串[" << str << "]" << endi;
				}
				codeconverter.closeCodeConverter();
			}
			else
			{
				thelog << "编码转换器初始化失败" << ende;
				_strValue = data;
			}
		}
		else if ("buffer" == dataType)
		{
			_strValue = data;
		}
		else
		{
			char* endptr;
			_uint16Value = strtoul(data.c_str(), &endptr, 10);
		};

		bHasValue = true;
	}
	void transferFloat(string const& transferRuleStr, char const* src, char* dst)
	{
		if (0 == transferRuleStr.size())
		{
			memcpy(dst, src, 4);
		}
		else if ("ABCD" == transferRuleStr)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
		}
		else if ("CDAB" == transferRuleStr)
		{
			dst[0] = src[2];
			dst[1] = src[3];
			dst[2] = src[0];
			dst[3] = src[1];
		}
		else if ("BADC" == transferRuleStr)
		{
			dst[0] = src[1];
			dst[1] = src[0];
			dst[2] = src[3];
			dst[3] = src[2];
		}
		else if ("DCBA" == transferRuleStr)
		{
			dst[0] = src[3];
			dst[1] = src[2];
			dst[2] = src[1];
			dst[3] = src[0];
		}
		else
		{
			memcpy(dst, src, 4);
		}
	}
	string transferString(string const& transferRuleStr, char const* src, int dataLength)
	{
		int regCount = dataLength / 2;
		if (0 != dataLength % 2)++regCount;

		char* buf = new char[regCount * 2];

		memcpy(buf, src, regCount * 2);
		transferRegist(transferRuleStr, buf, regCount);

		string ret = buf;
		delete[] buf;

		return ret;
	}
	//原地处理寄存器的字节序
	void transferRegist(string const& transferRuleStr, char* regs, int regCount)
	{
		if ("BA" == transferRuleStr)
		{
			for (int i = 0; i < regCount; ++i)
			{
				char tmp = regs[i * 2];
				regs[i * 2] = regs[i * 2 + 1];
				regs[i * 2 + 1] = tmp;
			}
		}
	}
	//注意字符串是设备编码
	void ChannelValue_LoadFromBuffer(string const& dataType, string const& transferRuleStr, int dataLength, uint16_t* regBuffer)
	{
		if ("float" == dataType)
		{
			transferFloat(transferRuleStr, (char*)regBuffer, (char*)&_f32Value);
		}
		else if ("boolean" == dataType)
		{
			_bitValue = ((uint8_t*)regBuffer)[0];
		}
		else if ("uint" == dataType)
		{
			//thelog << CMyTools::NumberToHex((uint32_t)655351) << endi;
			//thelog << CMyTools::NumberToHex(*(uint32_t*)regBuffer) << endi;
			for (int i = 0; i < 4; ++i)
			{
				memcpy(((char*)&_uint32Value) + i, ((char*)regBuffer) + i, 1);
			}
			//thelog << CMyTools::NumberToHex(_uint32Value) << endi;
		}
		else if ("string" == dataType)
		{
			_strValue = transferString(transferRuleStr, (char*)regBuffer, dataLength);
		}
		else//ushort short
		{
			_uint16Value = regBuffer[0];
		};

		bHasValue = true;
	}
	//写入缓冲区，如果缓冲区不足则失败
	bool ChannelValue_WriteToBuffer(string const& dataType, string const& transferRuleStr, uint16_t* regBuffer, int bufsize, int& regCount)
	{
		if ("float" == dataType)
		{
			transferFloat(transferRuleStr, (char*)&_f32Value, (char*)regBuffer);
			regCount = 2;
		}
		else if ("boolean" == dataType)
		{
			((uint8_t*)regBuffer)[0] = _bitValue;
			regCount = 1;
		}
		else if ("uint" == dataType)
		{
			memcpy(((char*)regBuffer), ((char*)&_uint32Value), 4);
			regCount = 2;
		}
		else if ("string" == dataType)
		{
			regCount = (_strValue.size() + 1) / 2;
			if (0 != (_strValue.size() + 1) % 2)++regCount;
			memset(regBuffer, 0, regCount * 2);
			memcpy(((char*)regBuffer), _strValue.c_str(), _strValue.size());
			transferRegist(transferRuleStr, (char*)regBuffer, regCount);
		}
		else//ushort short
		{
			regBuffer[0] = _uint16Value;
			regCount = 1;
		};
		return true;
	}
	//charset是原始数据的字符集，用于显示要转换为程序所用的字符集，中文本地编码一般是"GB18030"，如果数据已经失效则增加括号
	string ChannelValue_ShowValue(string const& dataType, string const& charset)const
	{
		if (bHasValue)return ChannelValue_ToStringUTF8(dataType, charset);
		else
		{
			return "(" + ChannelValue_ToStringUTF8(dataType, charset) + ")";
		}
	}
	//任何类型转换为string，UTF-8
	string ChannelValue_ToStringUTF8(string const& dataType, string const& charset)const
	{
		stringstream ss;
		if ("float" == dataType)
		{
			ss.precision(3);
			ss.setf(std::ios::fixed);
			ss << _f32Value;
		}
		else if ("boolean" == dataType)
		{
			ss << _bitValue;
		}
		else if ("uint" == dataType)
		{
			ss << _uint32Value;
		}
		else if ("string" == dataType)
		{
			if (charset.size() != 0)
			{
				CodeConverter codeconverter;
				if (!codeconverter.initCodeConverter(charset.c_str(), "UTF-8"))
				{
					thelog << "编码转换器初始化失败" << ende;
					ss << _strValue;
				}
				else
				{
					string str = _strValue;
					codeconverter.CharSetConvert(str, getTmpDir());
					ss << str;
					codeconverter.closeCodeConverter();
				}
			}
			else
			{
				ss << _strValue;
			}
		}
		else if ("buffer" == dataType)
		{
			ss << _strValue;
		}
		else if ("short" == dataType)
		{
			ss << static_cast<int16_t>(_uint16Value);
		}
		else
		{
			ss << _uint16Value;
		};
		return ss.str();
	}
	//任何类型转换为double
	double ChannelValue_ToDouble(string const& dataType)const
	{
		if ("float" == dataType)
		{
			return _f32Value;
		}
		else if ("boolean" == dataType)
		{
			return _bitValue;
		}
		else if ("uint" == dataType)
		{
			return _uint32Value;
		}
		else if ("string" == dataType)
		{
			return atof(_strValue.c_str());
		}
		else if ("short" == dataType)
		{
			return static_cast<int16_t>(_uint16Value);
		}
		else
		{
			return _uint16Value;
		}
	}
};
