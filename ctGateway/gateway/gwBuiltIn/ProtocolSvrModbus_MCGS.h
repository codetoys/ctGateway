//MCGS.h MCGS组态软件导出csv文件的导入程序

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "CSV.h"
#include "CharSetConv.h"
#include "ProtocolSvrModbus.h"

//请使用全局变量g_Global.pCGwConfig->

class CMcgsCsv
{
public:
	//文件名规则为deviceCode+".csv"
	static bool McgsLoadCSV(CModbusSvr* pIProtocol, Device* pDevice)
	{
		string charset = "GB18030";
		string filename = pDevice->deviceCode+".csv";
		CCSV csv;
		if (!csv.OpenCSV(filename.c_str(), true))
		{
			thelog << "未能打开设备配置文件 " << filename << endi;
			return true;
		}

		CodeConverter codeconverter;
		if (!codeconverter.initCodeConverter(charset.c_str(), "UTF-8"))
		{
			thelog << "编码转换器初始化失败：" << errno << " : " << strerror(errno) << ende;
			return false;
		}

		pIProtocol->_clearChannelParams(pDevice->deviceCode.c_str());
		pDevice->channels.clear();

		vector<string > lineFields;
		string tmp_field;
		bool tmp_isLindend;
		while (csv._ReadField(tmp_isLindend, tmp_field))
		{
			lineFields.push_back(tmp_field);
			if (!tmp_isLindend)
			{
				continue;
			}

			if (lineFields.size() < 11)
			{
				thelog << "字段数不足11，忽略" << endi;
				lineFields.clear();
				continue;
			}

			if (lineFields[0].size()>0 && lineFields[0][0]>='0' && lineFields[0][0] <= '9')
			{
			}
			else
			{
				thelog << "不是以数字开始，忽略" << endi;
				lineFields.clear();
				continue;
			}

			if (!codeconverter.CharSetConvert(lineFields[1], getTmpDir()))
			{
				thelog << "编码转换失败 " << CMyTools::ToHex(lineFields[1]) << ende;
			}
			if (!codeconverter.CharSetConvert(lineFields[3], getTmpDir()))
			{
				thelog << "编码转换失败 " << CMyTools::ToHex(lineFields[3]) << ende;
			}
			if (!codeconverter.CharSetConvert(lineFields[4], getTmpDir()))
			{
				thelog << "编码转换失败 " << CMyTools::ToHex(lineFields[4]) << ende;
			}
			if (!codeconverter.CharSetConvert(lineFields[5], getTmpDir()))
			{
				thelog << "编码转换失败 " << CMyTools::ToHex(lineFields[5]) << ende;
			}

			int channelNo = atoi(lineFields[0].c_str());
			ModbusChannelParam* pChannelParam = pIProtocol->_getIChannelParam(pDevice->deviceCode.c_str(), channelNo);
			Channel* pChennel = new Channel(pChannelParam);
			Channel& tmp_chennel = *pChennel;

			tmp_chennel.channelNo = channelNo;
			tmp_chennel.paramCode = CMyTools::ToString(tmp_chennel.channelNo);
			tmp_chennel.paramName = lineFields[1];
			tmp_chennel.channelComment = lineFields[3];
			
			if (strstr(lineFields[3].c_str(), "DF"))tmp_chennel.dataType = "float";
			else if (strstr(lineFields[3].c_str(), "WUB"))tmp_chennel.dataType = "ushort";
			else if (strstr(lineFields[3].c_str(), "DUB"))tmp_chennel.dataType = "uint";
			else if (strstr(lineFields[3].c_str(), "STR"))
			{
				tmp_chennel.dataType = "string";
				tmp_chennel.transferRuleStr = "BA";
				char const* p = strstr(lineFields[3].c_str(), "_");
				if (NULL == p)
				{
					thelog << "格式不符合预期，忽略" << endi;
					lineFields.clear();
					continue;
				}
				tmp_chennel.dataLength = atoi(p + 1);
				tmp_chennel.charsetCode = charset;
			}
			else
			{
				if ("[0区]输出继电器" == lineFields[5])tmp_chennel.dataType = "boolean";
				else if ("[1区]输入继电器" == lineFields[5])tmp_chennel.dataType = "boolean";
				else
				{
					thelog << "未知格式，忽略 " << lineFields[5] << endi;
					lineFields.clear();
					continue;
				}
			}
			
			pChannelParam->paramAddr = CMyTools::NumberToHex((unsigned short int)atoi(lineFields[7].c_str()));
			
			if ("只读" == lineFields[4])tmp_chennel.processType = 1;
			else if ("只写" == lineFields[4])tmp_chennel.processType = 2;
			else if ("读写" == lineFields[4])tmp_chennel.processType = 3;
			else
			{
				thelog << "未知格式，忽略 " << lineFields[4] << endi;
				lineFields.clear();
				continue;
			}

			if ("[0区]输出继电器" == lineFields[5])
			{
				pChannelParam->zone = 0;
			}
			else if ("[1区]输入继电器" == lineFields[5])
			{
				pChannelParam->zone = 1;
			}
			else if ("[3区]输入寄存器" == lineFields[5])
			{
				pChannelParam->zone = 3;
			}
			else if ("[4区]输出寄存器" == lineFields[5])
			{
				pChannelParam->zone = 4;
			}
			else
			{
				thelog << "未知格式，忽略 " << lineFields[5] << endi;
				lineFields.clear();
				continue;
			}

			//处理采集频率，转换为上报类型
			constexpr int dependBase = 50000;//依赖上报的范围
			int tmp_report = atoi(lineFields[9].c_str());
			if (tmp_report >= 100 && tmp_report < 200)
			{
				tmp_chennel.reportStrategy.reportStrategy = ReportStrategy_CHANGED;
				tmp_chennel.collectorRate = tmp_report - 100;
			}
			else if (tmp_report >= 200 && tmp_report < 300)
			{
				tmp_chennel.reportStrategy.reportStrategy = ReportStrategy_CHANGED_ACQU_ALL;
				tmp_chennel.collectorRate = tmp_report - 200;
			}
			else if (tmp_report >= dependBase && tmp_report < dependBase+10000)
			{
				tmp_chennel.reportStrategy.reportStrategy = ReportStrategy_DEPEND;
				tmp_chennel.collectorRate = tmp_report - dependBase;
			}
			else
			{
				tmp_chennel.reportStrategy.reportStrategy = ReportStrategy_CYCLE;
				tmp_chennel.collectorRate = 0;
			}

			//thelog << lineFields[3] << " : " << tmp_chennel.ToString() << endi;
			pDevice->channels.push_back(pChennel);

			lineFields.clear();
		}

		codeconverter.closeCodeConverter();
		csv.CloseCSV();

		return true;
	}
};
