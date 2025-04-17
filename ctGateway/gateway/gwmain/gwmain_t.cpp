//gwmain_t.cpp 网关程序主入口

#include "gwmain.h" 
#include "../gwBuiltIn/gwmain_test.h" 
#include "myprocess.h"
using namespace ns_my_std;

void touch(bool b)
{
	if(b)CProtocolPublic::isPlatformConnected();
}
#include <dlfcn.h>
#include "testso.h"

void test_fun_dlopen()
{
	cout << __FILE__ << __LINE__ << " test_fun_dlopen" << endl;
}

int so_test2(int argc, char** argv)
{
	void* so = dlopen("../../lib/libprotocol_demo.so", RTLD_LAZY | RTLD_DEEPBIND);

	if (NULL == so)
	{
		thelog << "dlopen失败 " << dlerror() << ende;
		return __LINE__;
	}
	else
	{
		thelog << "dlopen成功 libprotocol_demo.so" << endi;
	}
	dlclose(so);

	return 0;
}
int so_test(int argc, char** argv)
{
	thelog << "主程序调用 test_fun_dlopen ：" << endi;
	test_fun_dlopen();
	so_test2(argc, argv);

	thelog << "主程序加载 libtestso2.so ：" << endi;
	void* baseso = dlopen("../../lib/libtestso2.so", RTLD_LAZY | RTLD_GLOBAL);

	if (NULL == baseso)
	{
		thelog << "dlopen失败 " << dlerror() << ende;
		return __LINE__;
	}
	else
	{
		thelog << "主程序加载 libtestso.so ：" << endi;
		void* so = dlopen("../../lib/libtestso.so", RTLD_LAZY);

		if (NULL == so)
		{
			thelog << "dlopen失败 " << dlerror() << ende;
		}
		else
		{
			{
				void* module_main = dlopen(NULL, RTLD_LAZY);
				if (NULL == module_main)
				{
					thelog << "dlopen失败 " << dlerror() << ende;
				}
				else
				{
					thelog << "主程序加载符号 test_fun_dlopen ：" << endi;
					void (*pFun)() = (void (*)())dlsym(module_main, "test_fun_dlopen");
					if (NULL == pFun)
					{
						thelog << "dlsym失败 " << dlerror() << ende;
					}
					else
					{
						thelog << "主程序执行符号 test_fun_dlopen ：" << endi;
						pFun();
					}
					dlclose(module_main);
				}
			}
			{
				int (*pFun)();
				thelog << "主程序加载符号 so_a ：" << endi;
				pFun = (int (*)())dlsym(so, "so_a");
				if (NULL == pFun)
				{
					thelog << "dlsym失败 " << dlerror() << ende;
				}
				else
				{
					thelog << "主程序执行符号 so_a ：" << endi;
					thelog << pFun() << endi;
				}
			}

			{
				thelog << "主程序加载符号 getClass ：" << endi;
				Ia* (*pFun)() = (Ia * (*)())dlsym(so, "getClass");
				if (NULL == pFun)
				{
					thelog << "dlsym失败 " << dlerror() << ende;
				}
				else
				{
					thelog << "主程序执行符号 getClass ：" << endi;
					thelog << pFun()->data << " " << pFun()->fun() << endi;
				}
			}

			dlclose(so);
		}
		dlclose(baseso);
	}
	return 0;
}

int _main(int argc, char** argv, int last_ret)
{
	touch(false);

	if (sizeof(long) != 8)
	{
		thelog << "非64位程序！" << endi;
	}
	{
		//检测字节序，大端：低放高、高放低（网络字节序），小端：低放低、高放高（x86）
		int a = 1;
		if ((1 == *(unsigned char*)&a))
		{
			thelog << "字节序 Little-Endian" << endi;
		}
		else
		{
			thelog << "字节序 Big-Endian" << endi;
		}

		//检测中文
		string str = "中文中文";
		DEBUG_LOG << str << " str.size()=" << str.size() << " strlen(str.c_str())=" << strlen(str.c_str()) << endi;
	}

	G_IS_DEBUG = true;
	bool bAuto = GetCommandParam(argc, argv, "-auto");//是否是自动模式（用于设备）
	if (bAuto)G_IS_DEBUG = false;

	string sw_version = "1.0.0 2025.04.15 14:39";
	thelog << "网关程序版本 " << sw_version << endi;
	char path[1024];
	thelog << "工作目录 " << getcwd(path, 1024) << endi;

	gwmain_test(argc, argv);

	signal(SIGPIPE, sig_default);

	int ret = 0;
	CGwMain gwmain;
	if (EXIT_PLATFORM_CONNECT_TIMEOUT == last_ret)
	{
		thelog << "连接平台超时，改为默认平台" << endi;
		g_Global.bUseDefaultNorthConfig = true;
	}
	if (!gwmain.GwMainInit(sw_version.c_str()))
	{
		thelog << "初始化失败 清理日志然后重启" << ende;
		myShellExecute("rm *.log *.log.bz2", "");
		myShellExecute("shutdown -r now", "");
		return 1;
	}

	while (true)
	{
		if (bAuto)
		{
			SleepSeconds(5);
			time_t t1 = time(NULL);
			long span = 10 * 60;
			if (t1 - g_Global.last_platform_connection_recv > span && t1 - g_Global.last_platform_connection_send > span)
			{
				thelog << "平台收发空闲超时 " << span <<" 重启设备" << ende;
				myShellExecute("shutdown -r now", "");
				ret= 1;
				break;
			}
			long timeout = 5 * 60;
			//thelog << t1<<" "<< g_Global.satrt_time <<" " << timeout<<" "<< g_Global.bPlatformConnected << endi;
			if (t1 - g_Global.satrt_time > timeout && !g_Global.bPlatformConnected)
			{
				thelog << "平台连接超时 " << timeout << ende;
				if (g_Global.bUseDefaultNorthConfig)ret = EXIT_DEFAULT_PLATFORM_CONNECT_TIMEOUT;
				else ret = EXIT_PLATFORM_CONNECT_TIMEOUT;
				exit(ret);//不exit导致段错误，监控进程会认为是信号11
			}

			//对时，默认平台配置不需要对时，ubuntu测试需要sudo ./run.sh -auto
			static bool bNtpOK = false;
			if (!g_Global.bUseDefaultNorthConfig && !bNtpOK)
			{
				char const* ntpserver = g_Global.pCGwConfig->GetNorthConfig().ntp.c_str();
				if (strlen(ntpserver) > 0)
				{
					thelog << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX "<< ntpserver << endi;
					if (0 == myShellExecute("ntpdate", ntpserver))
					{
						bNtpOK = true;
					}
				}
			}
		}
		else
		{
			string input = UIInput("请输入消息：", "");//这里输入q会直接exit(0)
		}
	}
	gwmain.GwMainUnInit();

	return ret;
}
int main(int argc, char** argv)
{
	if (!InitActiveApp("gwmain", 0, argc, argv))exit(1);//日志大小加载配置后重新设置
	theLog.SetSource("WatchDog");
	//return so_test(argc, argv);

	int last_ret = 0;//上次运行的返回值，可能会带有重启后要操作的指令
	while (true)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			char buf[256];
			sprintf(buf,"fork error : %d : %s", errno, strerror(errno));
			thelog << buf << ende;
			return 1;
		}
		else if (0 == pid)
		{
			theLog.SetSource("main");
			try
			{
				exit(_main(argc, argv, last_ret));
			}
			catch (...)
			{
				thelog << "未处理的异常" << ende;;
			}
			return 1;
		}
		else
		{
			bool bTerm = false;
			int termSig = 0;
			last_ret = 0;
			CMyProcess::WaitProcessFinish(pid, bTerm, last_ret, termSig);
			char buf[256];
			sprintf(buf,"%s %d %d", (bTerm ? "被信号终止" : "返回"), last_ret, termSig);
			thelog << buf << ende;
			if (bTerm && 9 == termSig)
			{
				last_ret = 9;
				thelog << "手工终止（信号9），返回9，由外部决定后续是否重启" << ende;
				break;
			}
			if (!bTerm && 0 == last_ret)
			{
				thelog << "正常返回，由外部决定后续是否重启" << endi;
				break;
			}
			if (!bTerm && EXIT_DEFAULT_PLATFORM_CONNECT_TIMEOUT == last_ret)
			{
				thelog << "默认平台连接超时，强制重启设备" << endi;
				myShellExecute("shutdown -r now", "");
			}
			thelog << "被信号终止或返回非0，自动重启" << ende;

			//重启前的操作
			if (EXIT_RESET == last_ret)
			{
				thelog << "EXIT_RESET，重置并自动重启" << ende;
				CGwAppConfig::Reset();
			}
		}
	}
	return last_ret;
}
