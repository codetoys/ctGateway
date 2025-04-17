//协议管理

#include "ProtocolMgr.h"
#include "../gwBuiltIn/ProtocolSvrModbus.h"
#include "../gwBuiltIn/ProtocolSvrTunnel.h"
//#include "../gwBuiltIn/ProtocolSvrDLT645.h"
#include <dlfcn.h> 

string CProtocolMgr::getAllProtocol()
{
	string  ret;
	//ret += (PROTOCOL_CODE_DLT645_1997); ret += " ";
	//ret += (PROTOCOL_CODE_DLT645_2007); ret += " ";
	ret += (PROTOCOL_CODE_MODBUS_TCP); ret += " ";
	ret += (PROTOCOL_CODE_MODBUS_RTU_TCP); ret += " ";
	ret += (PROTOCOL_CODE_MODBUS_RTU); ret += " ";
	ret += (PROTOCOL_CODE_TCP_TUNNEL); ret += " ";
	ret += (PROTOCOL_CODE_SERIAL_PORT_TUNNEL);
	return ret;
}

ProtocolSvr* CProtocolMgr::CreateProtocolSvr(string code)
{
	//if (CDLT645_1997Svr::ProtocolCode() == code)return new ProtocolSvr(new CDLT645_1997Svr());
	//if (CDLT645_2007Svr::ProtocolCode() == code)return new ProtocolSvr(new CDLT645_2007Svr());

	if (CModbusTcpSvr::ProtocolCode() == code)return new ProtocolSvr(new CModbusTcpSvr());
	if (CModbusRtuTcpSvr::ProtocolCode() == code)return new ProtocolSvr(new CModbusRtuTcpSvr());
	if (CModbusRtuSvr::ProtocolCode() == code)return new ProtocolSvr(new CModbusRtuSvr());

	if (CTcpTunnelSvr::ProtocolCode() == code)return new ProtocolSvr(new CTcpTunnelSvr());
	if (CSerialPortTunnelSvr::ProtocolCode() == code)return new ProtocolSvr(new CSerialPortTunnelSvr());

	if (true)
	{
		string dirverfile = "../../lib/libprotocol_demo.so";
		void* so = dlopen(dirverfile.c_str(), RTLD_LAZY);

		if (NULL == so)
		{
			thelog << "dlopen失败 " << dlerror() << ende;
			return NULL;
		}
		{
			IProtocol* (*pFun_CreateIProtocol)(char const*);
			pFun_CreateIProtocol = (IProtocol * (*)(char const*))dlsym(so, "CreateIProtocol");
			if (NULL == pFun_CreateIProtocol)
			{
				thelog << "dlsym失败 " << dlerror() << ende;
				return NULL;
			}
			ProtocolSvr* ret= new ProtocolSvr(pFun_CreateIProtocol(code.c_str()));
			if (NULL != ret)
			{
				ret->getIProtocol()->m_DriverFile = dirverfile;
			}
			return ret;
		}
		//dlclose(so);
	}

	return NULL;
}

