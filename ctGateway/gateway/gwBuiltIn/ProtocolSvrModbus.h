//ProtocolSvrModbus.h 服务

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "LockedList.h" 
#include <thread>
#include <mutex>
#include <condition_variable>
#include "CModbus.h"
#include "../gwmain/gwconfig.h" 
#include "../interface/IProtocol.h"
#include "ProtocolStdWork.h"
#include "ChannelParam.h"
#include "DeviceParam.h"
#include "ProtocolParam.h"
#include <math.h>

//Modbus基类，具体使用CModbusTcpSvr、CModbusRtuSvr、CModbusRtuTcpSvr
class CModbusSvr : public IProtocol, public CStdWork::IStdWork
{
private:
	CModbus m_mobus;//modbus对象
	ProtocolParam_TCP_or_SerialPort m_ProtocolParam;
	map<string, DeviceParam_std_all> m_DeviceParams;//DeviceCode-DeviceParam
	map<string, map<int, ModbusChannelParam*> > m_device_channelparams;

	//处理地址、功能码和个数，部分需要做转换
	void _AddrFunCount(Channel* pChannel, int& addr, int& fun, int& count)
	{
		ModbusChannelParam* pChannelParam = dynamic_cast<ModbusChannelParam*>(pChannel->pChannelParam);
		addr = CMyTools::HexToLong(pChannelParam->paramAddr.c_str());
		if (0 == pChannelParam->functionCode.size())addr -= 1;//注意，分区表示的地址是基于1的，1区对应指令码02，3区对应04，4区对应03

		static int mapZoneFuncode[5] = { 1,2,-1,4,3 };

		fun = -1;//读的功能码
		if (0 == pChannelParam->functionCode.size())
		{
			if (pChannelParam->zone >= 0 && pChannelParam->zone <= 5)fun = mapZoneFuncode[pChannelParam->zone];
		}
		else
		{
			fun = CMyTools::HexToLong(pChannelParam->functionCode.c_str());
		}

		count = 1;//读取个数
		if ("float" == pChannel->dataType)count = 2;
		else if ("uint" == pChannel->dataType)count = 2;
		else if ("string" == pChannel->dataType)
		{
			count = pChannel->dataLength / 2;
			if (0 != pChannel->dataLength % 2)++count;
		}
		else count = 1;
	}
	//快速采集全部
	struct _channelInfo
	{
		Channel* pChannel = nullptr;
		int _addr;
		int _fun = -1;//读的功能码
		int _count = 1;//读取个数

		bool operator<(_channelInfo const& tmp)const { return _addr < tmp._addr; }
	};
	struct _ChannelGroup
	{
		int addr;//起始地址
		int count;//总个数
		list<_channelInfo> m_channels;
	};
	map<Device*, map<int, list<_ChannelGroup > > > m_PreparedDeviceChannelGroups;//预处理出来的设备-func-通道组
	void _PrepareDevice(Device* pDevice)
	{
		map<int, vector<_channelInfo > >mapFunAddrChannel;

		//分析，理出每个命令的按地址排序的数据
		for (Channel* channel : pDevice->channels)
		{
			DEBUG_LOG << channel->channelNo << " " << channel->paramCode << " " << channel->processType << endi;
			if (!channel->CanRead())
			{
				thelog << "不可读" << endi;
				continue;
			}

			_channelInfo tmp;
			tmp.pChannel = channel;
			_AddrFunCount(tmp.pChannel, tmp._addr, tmp._fun, tmp._count);

			mapFunAddrChannel[tmp._fun].push_back(tmp);
		}
		//对每个命令合并操作
		for (auto& vFunChannels : mapFunAddrChannel)
		{
			//先按照地址排序
			sort(vFunChannels.second.begin(), vFunChannels.second.end());

			list<_ChannelGroup >& groups= m_PreparedDeviceChannelGroups[pDevice][vFunChannels.first];//存放合并出的分组
			for (auto& vChannel : vFunChannels.second)
			{
				constexpr int maxcount = 100;//连续操作的最大限制，具体值取决于设备，甚至可能完全不支持
				bool bNewGroup = false;
				
				if (0 == groups.size())
				{
					bNewGroup = true;
				}
				else
				{
					_ChannelGroup& lastGroup = *groups.rbegin();
					int tmpcount = vChannel._addr + vChannel._count - lastGroup.addr;

					if (vChannel._addr >= lastGroup.addr && vChannel._addr<= lastGroup.addr+ lastGroup.count+1 && tmpcount <= maxcount)
					{
						if (tmpcount > lastGroup.count)lastGroup.count = tmpcount;
						lastGroup.m_channels.push_back(vChannel);
					}
					else
					{
						bNewGroup = true;
					}
				}

				if (bNewGroup)
				{
					_ChannelGroup tmpgroup;
					tmpgroup.addr = vChannel._addr;
					tmpgroup.count = vChannel._count;
					tmpgroup.m_channels.push_back(vChannel);
					groups.push_back(tmpgroup);
				}
			}
		}
	}
public://CStdWork::IStdWork
	//确认Interface已经连接，未连接则连接
	virtual bool _EnsureInterfaceConnected()override
	{
		if (!m_mobus.IsConnected())
		{
			thelog << "需要初始化连接" << endi;

			bool retConn = false;
			ProtocolParam_TCP_or_SerialPort* pParam = &m_ProtocolParam;
			if (PROTOCOL_CODE_MODBUS_TCP == m_pInertfaceDevice->protocolCode)
			{
				retConn = m_mobus.InitModbusTcp(pParam->ip.c_str(), pParam->port);
			}
			else if (PROTOCOL_CODE_MODBUS_RTU_TCP == m_pInertfaceDevice->protocolCode)
			{
				retConn = m_mobus.InitModbusRtuTcp(pParam->ip.c_str(), pParam->port);
			}
			else if (PROTOCOL_CODE_MODBUS_RTU == m_pInertfaceDevice->protocolCode)
			{
				retConn = m_mobus.InitModbusRtu(pParam->serial_port.c_str(), pParam->baud, pParam->parity, pParam->data_bit, pParam->stop_bit, pParam->timeout_s);
			}
			else
			{
				thelog << "未知的protocolCode [" << m_pInertfaceDevice->protocolCode << "]" << ende;
			}
			if (!retConn)
			{
				thelog << "Interface连接失败" << ende;
				m_pInertfaceDevice->_InterfaceState = InterfaceState::CONNECT_ERROR;
			}
			else
			{
				thelog << "Interface连接成功" << endi;
				m_pInertfaceDevice->_InterfaceState = InterfaceState::CONNECT_OK;
			}

			if (!m_mobus.IsConnected())
			{
				string msg = "无法连接设备（接口），包含的设备：";
				for (auto& device : m_pInertfaceDevice->devices)
				{
					msg += device->deviceCode;
					msg += " ";
				}
				thelog << msg << ende;
				g_Global.pIPlatform->post_Report(msg.c_str(), m_pInertfaceDevice->interfaceCode.c_str());
				return false;
			}
		}
		else
		{
			//thelog << "设备已经初始化" << endi;
		}

		return true;
	}
	virtual void _DisconnectInterface()override
	{
		m_mobus.UnInitModbus();
	}
	virtual bool _SetDevice(Device* pDevice)override
	{
		return m_mobus.SetSlave(m_DeviceParams[pDevice->deviceCode].stationCode);
	}
	//写入
	virtual bool _WriteChannel(Device* pDevice, Channel* pChannel)override
	{
		ModbusChannelParam* pChannelParam = dynamic_cast<ModbusChannelParam*>(pChannel->pChannelParam);
		int addr;
		int fun = -1;//读的功能码
		int count = 1;//读取个数
		_AddrFunCount(pChannel, addr, fun, count);

		uint16_t regBuffer[1024];//读取和写入缓冲区，部分类型使用
		//处理写入
		thelog << "需要写入" << endi;
		switch (fun)
		{
		case 1://输出线圈，0区
			int writeCount;
			pChannel->GetWriteValue().ChannelValue_WriteToBuffer(pChannel->dataType, pChannel->transferRuleStr, regBuffer, 1024, writeCount);
			if (m_mobus.Write05H0FH0z(addr, (uint8_t*)regBuffer, writeCount))
			{
				pChannel->FinishWriteValue(true);
			}
			else
			{
				pChannel->FinishWriteValue(false);
				return false;
			}
			break;
		case 2://离散量输入，1区，不可写
			break;
		case 3://保持寄存器，4区
		{
			int writeCount;
			pChannel->GetWriteValue().ChannelValue_WriteToBuffer(pChannel->dataType, pChannel->transferRuleStr, regBuffer, 1024, writeCount);
			if (m_mobus.Write06H10H4z(addr, regBuffer, writeCount))
			{
				pChannel->FinishWriteValue(true);
			}
			else
			{
				pChannel->FinishWriteValue(false);
				return false;
			}
		}
		break;
		case 4://输入寄存器，3区，不可写
			break;
		default:
		{
			thelog << "未知的命令码 " << pChannelParam->functionCode << ende;
			pChannel->SetChannelState(ChannelState::CONFIG_ERROR);
			return false;
		}
		break;
		}

		return true;
	}
	//采集单个通道并处理变化
	virtual bool _ReadChannel(bool must, Device* pDevice, Channel* pChannel, bool& bNotThreshold)override
	{
		ModbusChannelParam* pChannelParam = dynamic_cast<ModbusChannelParam*>(pChannel->pChannelParam);
		int addr;
		int fun = -1;//读的功能码
		int count = 1;//读取个数
		_AddrFunCount(pChannel, addr, fun, count);

		uint16_t regBuffer[1024];//读取和写入缓冲区，部分类型使用
		//采集数据
		bool readRet;
		if (1 == fun)readRet = m_mobus.Read01H0z(addr, (uint8_t*)regBuffer, count);
		else if (2 == fun)readRet = m_mobus.Read02H1z(addr, (uint8_t*)regBuffer, count);
		else if (3 == fun)readRet = m_mobus.Read03H4z(addr, regBuffer, count);
		else if (4 == fun)readRet = m_mobus.Read04H3z(addr, regBuffer, count);
		else
		{
			thelog << "未知的命令码 " << pChannelParam->functionCode << ende;
			pChannel->SetChannelState(ChannelState::CONFIG_ERROR);
			return false;
		}
		if (readRet)pChannel->SetChannelState(ChannelState::OK);
		else
		{
			thelog << "==================" << pDevice->deviceCode << " " << pChannel->channelNo << " 出错" << ende;
			pChannel->SetChannelState(ChannelState::READ_ERROR);
			return false;
		}

		//处理结果数据
		pChannel->_changed = false;
		if (!must && pChannel->reportStrategy.needCheckChanged() && pChannel->HasValue())
		{
			ChannelValue newvalue;
			newvalue.ChannelValue_LoadFromBuffer(pChannel->dataType, pChannel->transferRuleStr, pChannel->dataLength, regBuffer);
			if ("string" != pChannel->dataType)
			{
				double d_new = newvalue.ChannelValue_ToDouble(pChannel->dataType);
				double d_old = pChannel->GetValue().ChannelValue_ToDouble(pChannel->dataType);
				double diff = fabs(d_new - d_old);//差异的绝对值
				int percent;
				if (0 == d_old)percent = (0 == d_new ? 0 : 100);
				else percent = fabs(diff) * 100 / fabs(d_old);
				//thelog << d_new << " " << d_old << " " << percent << "% " << pChannel->collectorRate << " " << CMyTools::NumberToHex(d_old) << " " << CMyTools::NumberToHex(d_new) << " " << (d_old != d_new) << endi;
				if ((pChannel->collectorRate > 0 && percent < pChannel->collectorRate)
					|| (0 == pChannel->collectorRate && d_old == d_new))
				{
					if (!must)
					{
						bNotThreshold = true;
						return true;
					}
				}
				else
				{
					pChannel->_changed = true;
				}
			}
			else
			{
				if (newvalue._strValue == pChannel->GetValue()._strValue)
				{
					if (!must)
					{
						bNotThreshold = true;
						return true;
					}
				}
				else
				{
					pChannel->_changed = true;
				}
			}
		}
		pChannel->SetValueFromBuffer(regBuffer);
		pChannel->_new_report = true;
		if (ReportStrategy_CHANGED_ACQU_ALL == pChannel->reportStrategy.reportStrategy && pChannel->_changed)
		{
			if (pDevice->_events.size() != 0)pDevice->_events += " ";
			pDevice->_events += pChannel->paramCode;
		}

		return true;
	}
	virtual bool _FastReadAllChannel(Device* pDevice, int& countReadOK, bool bReally)override
	{
		//对每个命令合并操作
		for (auto& vFunChannels : m_PreparedDeviceChannelGroups[pDevice])
		{
			//对每个分组用合并指令采集
			for (auto& vGroup : vFunChannels.second)
			{
				int addr = vGroup.addr;
				int fun = vFunChannels.first;//读的功能码
				int count = vGroup.count;//读取个数

				uint16_t regBuffer[1024];//读取和写入缓冲区，部分类型使用
				//采集数据
				bool readRet;
				if (1 == fun)readRet = m_mobus.Read01H0z(addr, (uint8_t*)regBuffer, count);
				else if (2 == fun)readRet = m_mobus.Read02H1z(addr, (uint8_t*)regBuffer, count);
				else if (3 == fun)readRet = m_mobus.Read03H4z(addr, regBuffer, count);
				else if (4 == fun)readRet = m_mobus.Read04H3z(addr, regBuffer, count);
				else
				{
					thelog << "未知的命令码 " << fun << ende;
					continue;
				}

				if (!bReally)return false;//仅仅用来保持连接，所以随便读一个就可以了

				if (readRet)
				{
					for (auto& channel : vGroup.m_channels)
					{
						Channel* pChannel = channel.pChannel;

						pChannel->SetChannelState(ChannelState::OK);
						pChannel->SetValueFromBuffer(regBuffer + (channel._addr - addr));
						pChannel->_new_report = true;

						++countReadOK;
					}
				}
				else
				{
					thelog << "==================" << " 批量读取出错" << ende;
					return false;
				}
			}
		}

		return true;
	}
public://IProtocol
	CModbusSvr(char const* code, bool isSerialPort) :IProtocol(code, isSerialPort) {}
	virtual IProtocolParam* getIProtocolParam()override
	{
		return &m_ProtocolParam;
	}
	virtual IDeviceParam* getIDeviceParam(char const* deviceCode)override
	{
		return &m_DeviceParams[deviceCode];
	}
	ModbusChannelParam* _getIChannelParam(char const* deviceCode, int channelNo)
	{
		ModbusChannelParam* ret = new ModbusChannelParam();
		m_device_channelparams[deviceCode][channelNo] = ret;
		return ret;
	}
	void _clearChannelParams(char const* deviceCode)
	{
		for (auto& v : m_device_channelparams[deviceCode])
		{
			delete v.second;
		}
		m_device_channelparams[deviceCode].clear();
	}
	virtual IChannelParam* getIChannelParam(char const* deviceCode, int channelNo)override
	{
		return _getIChannelParam(deviceCode, channelNo);
	}
	virtual void protocol_process_job()override
	{
		CStdWork stdwork(this->m_pInertfaceDevice);
		stdwork.StdWork(this);
	}
	virtual void protocol_process_timer_job()override {}
	virtual ~CModbusSvr()override
	{}
	virtual bool ProtoSvrInit(InterfaceDeviceConfig* p)override
	{
		m_pInertfaceDevice = p;

		//预处理出快速采集所需的分组
		for (auto& v : m_pInertfaceDevice->devices)
		{
			auto& d = *v;

			_PrepareDevice(&d);
		}

		return true;
	}
	virtual bool OnAfterLoadDeviceConfig(Device* pDevice)override;
	virtual bool ProtoSvrUnInit()override
	{
		//需要首先停止子线程，不过一般设备都是直接断电重启的，所以无所谓

		return true;
	}
};

class CModbusTcpSvr : public CModbusSvr
{
public:
	static char const* ProtocolCode() { return PROTOCOL_CODE_MODBUS_TCP; }
	CModbusTcpSvr() :CModbusSvr(ProtocolCode(), false) {}
};
class CModbusRtuTcpSvr : public CModbusSvr
{
public:
	static char const* ProtocolCode() { return PROTOCOL_CODE_MODBUS_RTU_TCP; }
	CModbusRtuTcpSvr() :CModbusSvr(ProtocolCode(), false) {}
};
class CModbusRtuSvr : public CModbusSvr
{
public:
	static char const* ProtocolCode() { return PROTOCOL_CODE_MODBUS_RTU; }
	CModbusRtuSvr() :CModbusSvr(ProtocolCode(), true) {}
};
