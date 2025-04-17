//ProtocolSvrModbus.cpp 服务

#include "ProtocolSvrModbus.h" 
#include "ProtocolSvrModbus_MCGS.h"

bool CModbusSvr::OnAfterLoadDeviceConfig(Device* pDevice)
{
	return CMcgsCsv::McgsLoadCSV(this, pDevice);
}
