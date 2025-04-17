//IProtocol.cpp 协议接口

#include "../interface/IProtocol.h"
#include "gwconfig.h"

string CProtocolPublic::GetSN()
{
	return g_Global.pCGwConfig->GetBaseInfo().sn;
}
bool CProtocolPublic::isPlatformConnected()
{
	return g_Global.bPlatformConnected;
}
bool CProtocolPublic::post_Message(char const* func, char const* message)
{
	return g_Global.pIPlatform->post_Message(func, message);
}
void CProtocolPublic::AddPushData(long conn, char const* deviceCode, int channelNo, char const* channelCode, char const* channelName, int64_t ts, bool isNumber, double numberValue, char const* strValue, int errorCode, char const* errorMessage)
{
	g_Global.pIPlatform->AddPushData(conn, deviceCode, channelNo,  channelCode,  channelName, ts, isNumber, numberValue, strValue, errorCode, errorMessage);
}
bool CProtocolPublic::post_DataMessage(long conn, char const* deviceCode, bool isAll, string events, long ms)
{
	return g_Global.pIPlatform->post_DataMessage(conn, deviceCode, isAll, events, ms);
}

bool CProtocolPublic::post_ReportWithFunc(char const* func, char const* message, char const* _interface, char const* _device, char const* _channel)
{
	return g_Global.pIPlatform->post_ReportWithFunc(func, message, _interface, _device, _channel);
}

