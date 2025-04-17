//gwmain_t.cpp 网关程序主入口

#include "ProtocolDemo.h"

int main(int argc, char** argv)
{
	if (!InitActiveApp("ProtocolDemo", 0, argc, argv))exit(1);//日志大小加载配置后重新设置
	theLog.SetSource("ProtocolDemo");

	CSerialPortDemoSvr a;
	CTcpDemoSvr b;

	return 0;
}
