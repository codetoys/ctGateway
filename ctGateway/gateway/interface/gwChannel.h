//通道

#pragma once

#include "myUtil.h" 
#include "htmldoc.h" 
#include "cJSON_util.h" 
using namespace ns_my_std;
#include "CharSetConv.h"
#include "gwValue.h"
#include "ProtocolCode.h"

constexpr int ReportStrategy_DEFAULT = 0;
constexpr int ReportStrategy_CYCLE = 1;
constexpr int ReportStrategy_CHANGED = 2;
constexpr int ReportStrategy_DEPEND = 5;
constexpr int ReportStrategy_CHANGED_ACQU_ALL = 6;

struct ReportStrategy
{
	int reportStrategy = ReportStrategy_DEFAULT;

	bool needCheckChanged()const { return ReportStrategy_CHANGED == reportStrategy || ReportStrategy_CHANGED_ACQU_ALL == reportStrategy; }

	string ToString()const
	{
		if (ReportStrategy_DEFAULT == reportStrategy)return "-";
		else if (ReportStrategy_CYCLE == reportStrategy)return "v";
		else if (ReportStrategy_CHANGED == reportStrategy)return "变化";
		else if (ReportStrategy_DEPEND == reportStrategy)return "依赖";
		else if (ReportStrategy_CHANGED_ACQU_ALL == reportStrategy)return "变化全部";
		else
		{
			char buf[64];
			sprintf(buf, "%d", reportStrategy);
			return buf;
		}
	}
};

enum class ChannelState { DEFAULT = 0, OK, READ_ERROR, CONFIG_ERROR };
enum class ChannelWriteState { DEFAULT = 0, OK, WRITE_ERROR };

class IChannelParam
{
public:
	virtual ~IChannelParam() {}
	//自定义参数的输出
	virtual std::string _ToString()const = 0;
	//从配置json中加载
	virtual bool LoadConfig(string const& protocolCode, cJSON* cjson_resolveParamConfigVOList_item) = 0;
	//输出表格头
	virtual void AddHtmlTalbeCols(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit) = 0;
	//输出表格数据
	virtual void AddToHtmlTableLine(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit) = 0;
};

class Channel
{
public:
	int channelNo = 0;//通道号，唯一标识一个通道，与外部交互时可能用channelNo或paramCode来标识通道
	string channelComment;//通道注释
	string paramCode;//点位编码，唯一标识一个通道
	string paramName;//名称
	string dataType;//有ushort short ubyte byte uint int ulong long float double boolean
	int dataLength = 0;//字符串和数组使用（数组尚未支持）
	string charsetCode;//字符串编码 gbk utf-8，ChannelValue存储的是原始数据，也就是这个编码的数据，显示时要转换为程序所需的编码，与平台交互则需要转换为协商的编码，一般是utf-8
	int processType = 0;//操作类型1-可读 2-可写 3-可读写
	int collectorRate = 0;//采集频率，毫秒，或变化百分比，或依赖的channelNo
	ReportStrategy reportStrategy;//上报策略 1-按采集频率上报 2-变化上报 3-监听上报(尚未支持) 4-条件上报（尚未支持） 5-依赖上报（随dependParam上报）6-变化触发全量
	string dependParamCode = "";//依赖的点位编码，会转换为channelNo放在collectorRate
	string transferRuleStr;//字节顺序ABCD

	IChannelParam* pChannelParam;//通道的自定义参数

	bool _changed = false;//检测到数据改变（仅对需要检测变化的类型）
	bool _new_report = false;//新上报值（即数据需要放入上报数据包）
private:
	ChannelValue _value;
	CMyTime _valueTime;//读取的时间
	ChannelState _ChannelState = ChannelState::DEFAULT;
	ChannelValue _writeValue;//此值有效时将执行写入，写入完成后状态写入_ChannelWriteState
	CMyTime _writeTime;//写入的时间
	ChannelWriteState _ChannelWriteState = ChannelWriteState::DEFAULT;

public:
	Channel(IChannelParam* p) :pChannelParam(p) {}
	void SetChannelState(ChannelState state) { _ChannelState = state; }
	int GetChannelState()const { return (int)_ChannelState; }
	bool HasValue()const { return _value.bHasValue; }
	ChannelValue& GetValue() { return _value; }
	CMyTime const& GetValueTime()const { return _valueTime; }
	void SetValue(string const& value)
	{
		_value._strValue = value;
		_value.bHasValue = true;
		_valueTime.SetCurrentTime();
	}
	void SetValueFromBuffer(uint16_t* regBuffer)
	{
		_value.ChannelValue_LoadFromBuffer(dataType, transferRuleStr, dataLength, regBuffer);
		_valueTime.SetCurrentTime();
	}

	int GetChannelWriteState()const { return (int)_ChannelWriteState; }
	bool HasWriteValue()const { return _writeValue.bHasValue; }
	ChannelValue& GetWriteValue() { return _writeValue; }
	void SetWriteValue(string const& dataType, string const& charset, string const& value)
	{
		_writeValue.ChannelValue_SetWriteValue(dataType, charsetCode, value);
		_ChannelWriteState = ChannelWriteState::DEFAULT;
	}
	void FinishWriteValue(bool succeed)
	{
		if (succeed)
		{
			_writeValue.bHasValue = false;
			_writeTime.SetCurrentTime();
			_ChannelWriteState = ChannelWriteState::OK;
		}
		else
		{
			_ChannelWriteState = ChannelWriteState::WRITE_ERROR;
		}
	}
	string ProcessTypeStr()const
	{
		if (1 == processType)return "只读";
		else if (2 == processType)return "只写";
		else if (3 == processType)return "读写";
		else
		{
			stringstream ss;
			ss << processType;
			return ss.str();
		}
	}
	bool CanRead()const
	{
		return 1 == processType || 3 == processType;
	}
	bool CanWrite()const
	{
		return 2 == processType || 3 == processType;
	}
	string ToString()const
	{
		stringstream ss;
		ss << "\t\t" << channelNo << " : " << channelComment << " : " << paramCode << " : " << dataType << " " << dataLength << " " << pChannelParam->_ToString() << " processType: " << processType
			<< " s:" << static_cast<int>(_ChannelState) << " v:" << _value.ChannelValue_ShowValue(dataType, charsetCode)
			<< " w:" << _writeValue.ChannelValue_ShowValue(dataType, charsetCode) << " ws:" << static_cast<int>(_ChannelWriteState);
		return ss.str();
	}

	void AddHtmlTalbeCols(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.SetColFormInput(table.AddCol("channelNo", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4, true);
		table.SetColFormInput(table.AddCol("paramCode"), 16);
		table.SetColFormInput(table.AddCol("paramName"), 16);
		table.AddCol("channelComment");
		table.SetColFormInput2(table.AddCol("数据类型"), 8, "float uint boolean string ushort");
		table.SetColFormInput(table.AddCol("长度", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 2);
		if (pChannelParam)pChannelParam->AddHtmlTalbeCols(protocolCode, table, bEdit);
		if (!bEdit)
		{
			table.AddCol("s", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
			table.AddCol("值", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
			table.AddCol("v", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
		}
		table.SetColFormInput(table.AddCol("字符集", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 8);
		table.SetColFormInput2(table.AddCol("pT", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4, "只读 读写 只写");
		table.SetColFormInput(table.AddCol("采集间隔", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 6);
		table.SetColFormInput2(table.AddCol("上报策略", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4, "1-周期采集 2-变化上报 5-依赖上报 6-变化全报");
		table.SetColFormInput2(table.AddCol("字节序"), 4, "ABCD CDAB BADC DCBA");
		if (!bEdit)
		{
			table.AddCol("s", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
			table.AddCol("写", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
			table.AddCol("w", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
		}
	}
	void AddToHtmlTableLine(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.AddData(channelNo);
		table.AddData(paramCode);
		table.AddData(paramName);
		table.AddData(ReportStrategy_DEPEND == reportStrategy.reportStrategy ? channelComment + " " + dependParamCode : channelComment);
		table.AddData(dataType);
		table.AddData(dataLength);
		pChannelParam->AddToHtmlTableLine(protocolCode, table, bEdit);
		if (!bEdit)
		{
			table.AddData(static_cast<int>(_ChannelState));
			table.AddData(_value.ChannelValue_ShowValue(dataType, charsetCode));
			table.AddData(_valueTime.hasTime() ? _valueTime.GetTimeSpanS() : -1);
		}
		table.AddData(charsetCode);
		table.AddData(ProcessTypeStr());
		table.AddData(collectorRate);
		table.AddData(reportStrategy.ToString());
		table.AddData(transferRuleStr);
		if (!bEdit)
		{
			table.AddData(static_cast<int>(_ChannelWriteState));
			table.AddData(_writeValue.ChannelValue_ShowValue(dataType, charsetCode));
			table.AddData(_writeTime.hasTime() ? _writeTime.GetTimeSpanS() : -1);
		}
	}
};
