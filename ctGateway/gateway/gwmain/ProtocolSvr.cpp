//协议处理基础功能

#include "ProtocolSvr.h"
#include "gwconfig.h"

bool ProtocolSvr::StartProtocolSvr()
{
	//启动子线程
	return this->m_WorkThread.StartWorkerThread(this, pIProtocol->m_pInertfaceDevice->interfaceCode.c_str(), pIProtocol->m_pInertfaceDevice->_ProtocolSvr->getIProtocol()->getIProtocolParam()->_rate);
}
