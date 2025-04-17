//ChannelParam.h 通道参数

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include <cJSON/cJSON.h> 
#include "../interface/gwChannel.h"

class ModbusChannelParam :public IChannelParam
{
public:
	string paramAddr;//hex，使用zone时基于1（实际命令中的地址需要减1），使用功能码时基于0
	int zone = -1;//0、1、3、4，功能码未定义时使用
	string functionCode;//功能码

	//自定义参数的输出
	virtual std::string _ToString()const
	{
		stringstream ss;
		ss << paramAddr << "(" << CMyTools::HexToLong(paramAddr.c_str()) << ")";
		return ss.str();
	}
	//从配置json中加载
	virtual bool LoadConfig(string const& protocolCode, cJSON* cjson_resolveParamConfigVOList_item)
	{
		bool ret = true;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "paramAddr", paramAddr))ret = false;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "functionCode", functionCode))ret = false;

		return ret;
	}
	//输出表格头
	virtual void AddHtmlTalbeCols(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.AddCol("地址hex", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT);
		table.SetColFormInput(table.AddCol("地址dec", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4);
		table.SetColFormInput2(table.AddCol("PLC区", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4, "0区输出线圈 1区输入中继 3区输入寄存器 4区保持寄存器");
		table.SetColFormInput2(table.AddCol("功能码hex", CHtmlDoc::CHtmlDoc_DATACLASS_RIGHT), 4, "01输出线圈 02输入线圈 03保持寄存器 04输入寄存器");
	}
	//输出表格数据
	virtual void AddToHtmlTableLine(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.AddData(paramAddr);
		table.AddData(CMyTools::HexToLong(paramAddr.c_str()));
		table.AddData(zone);
		table.AddData(functionCode);
	}
};

class TunnelChannelParam :public IChannelParam
{
public:
	string paramCommand;//隧道协议的指令

	//自定义参数的输出
	virtual std::string _ToString()const
	{
		stringstream ss;
		ss << paramCommand;
		return ss.str();
	}
	//从配置json中加载
	virtual bool LoadConfig(string const& protocolCode, cJSON* cjson_resolveParamConfigVOList_item)
	{
		bool ret = true;
		if (!CJsonUtil::_GetJsonParam(cjson_resolveParamConfigVOList_item, "paramCommand", paramCommand))ret = false;

		return ret;
	}
	//输出表格头
	virtual void AddHtmlTalbeCols(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.AddCol("paramCommand");
	}
	//输出表格数据
	virtual void AddToHtmlTableLine(string const& protocolCode, CHtmlDoc::CHtmlTable2& table, bool bEdit)
	{
		table.AddData(paramCommand);
	}
};
