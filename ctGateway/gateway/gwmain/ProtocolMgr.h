//协议管理

#pragma once

#include "ProtocolMgr.h"
#include <interface/IProtocol.h>

class CProtocolMgr
{
public:
	static string getAllProtocol();
	static ProtocolSvr* CreateProtocolSvr(string code);
};