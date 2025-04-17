//gwmain_t.cpp 网关程序主入口

#include "myUtil.h" 
#include <string>
#include <stdio.h>
#include <iostream> 
#include <stdlib.h>
#include <cstring>
#include "myprocess.h"
#include "mosquitto/mosquitto.h"
#include "mosquitto/mosquittopp.h" 
#include <cJSON/cJSON.h> 
#include "mysocket.h" 
#include "SerialPort.h"
#include "CModbus.h"
#include "myZIP.h"
#include "myOpenSSL.h"
//extern "C"
//{
//#include "dlt645/dlt645.h"
//#include "dlt645/dlt645_port.h"
//}
#include "gwmain_test.h"
//#include "snap7client.h"
//#include <myOpen62541/myOpen62541.h>
using namespace ns_my_std;

class CSocket
{
public:
	static void test()
	{
		CMySocket s;
		if (!s.Connect("192.168.42.2", 2200))
		{
			cout << "连接失败" << endl;
		}
		else
		{
			char buf[1024];
			long count;
			if (!s.Recv(buf, 1024, &count))
			{
				cout << "接收失败" << endl;
			}
			else cout << "收到数据 " << count << endl;
		}
		s.Close();
	}
};
class CJson
{
public:
	static char* test()
	{
		cJSON* cjson_test = NULL;
		cjson_test = cJSON_CreateObject();
		cJSON_AddNullToObject(cjson_test, "a_null");
		cJSON_AddBoolToObject(cjson_test, "b_bool", true);
		cJSON_AddNumberToObject(cjson_test, "c_double", 1.2);
		cJSON_AddStringToObject(cjson_test, "d_string", "abc");
		return cJSON_Print(cjson_test);
	}
};

std::string g_subTopic = "test";
class mqtt_test :public mosqpp::mosquittopp
{
public:
	mqtt_test(const char* id) :mosquittopp(id) {}
	void on_connect(int rc) { thelog << "on_connect" << endi; }
	void on_disconnect() { thelog << "on_disconnect" << endi; }
	void on_publish(int mid) { thelog << "on_publish" << endi; }
	void on_subscribe(int mid, int qos_count, const int* granted_qos)//订阅回调函数
	{
		thelog << "订阅 mid: %d " << mid << endi;
	}
	void on_message(const struct mosquitto_message* message)//订阅主题接收到消息
	{
		bool res = false;
		mosqpp::topic_matches_sub(g_subTopic.c_str(), message->topic, &res);
		if (res)
		{
			std::string strRcv = (char*)message->payload;
			thelog << "来自<" << message->topic << ">的消息：" << strRcv << endi;
		}
	}
};
int testMqtt(int argc, char* argv[])
{
	mosqpp::lib_init();
	mqtt_test test("client6");

	int rc;
	char buf[1024] = "This is test";
	test.username_pw_set("user", "user1234");
	string host = "192.168.42.2";
	thelog << "连接到:" << host << endi;
	//rc = test.connect("192.168.0.27", 18883, 600);//本地IP 
	rc = test.connect(host.c_str(), 18883, 600);//本地IP 

	if (rc == MOSQ_ERR_ERRNO)
	{
		thelog << "连接错误:" << mosqpp::strerror(rc) << endi;//连接出错
	}
	else if (MOSQ_ERR_SUCCESS == rc)
	{
		thelog << "连接成功" << endi;//连接出错
		rc = test.subscribe(NULL, "test");
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "订阅错误:" << mosqpp::strerror(rc) << endi;//订阅出错
		}
		else
		{
			thelog << "订阅成功" << endi;//连接出错
		}
		//发布测试
		rc = test.loop_start();
		while (true)
		{
			thelog << "请输入消息：" << endi;
			if (NULL == fgets(buf, 1024, stdin))break;
			std::string topic1 = "test";
			rc = test.publish(NULL, topic1.c_str(), strlen(buf), (const void*)buf);
			if (rc == MOSQ_ERR_ERRNO)
			{
				thelog << "发送错误:" << mosqpp::strerror(rc) << endi;//发送出错
				SleepSeconds(5);
			}
		}
	}
	mosqpp::lib_cleanup();
	return 0;
}

bool testMqtts_connected = false;
void my_connect_callback(struct mosquitto* mosq, void* obj, int rc)
{
	if (MOSQ_ERR_SUCCESS == rc)
	{
		thelog << "连接成功" << endi;
		testMqtts_connected = true;
	}
	else
	{
		thelog << "连接错误:" << mosqpp::strerror(rc) << endi;//连接出错
	}
}
int pw_callback(char* buf, int size, int rwflag, void* userdata)
{
	thelog << "pw_callback" << endi;
	return 1;
}
int testMqtts(int argc, char* argv[])
{
	int rc;
	mosquitto_lib_init();
	mosquitto* mosq = mosquitto_new(NULL, true, NULL);

	if (true)
	{
		char const* ca = "./SSL/localtest/ca.pem";
		char const* client = "./SSL/localtest/client.pem";
		char const* clientkey = "./SSL/localtest/client.key";

		char cwd[1024];
		thelog << "使用MQTTS "<<getcwd(cwd,1024) << endi;
		rc = mosquitto_tls_set(mosq, ca, "/etc/ssl/certs/", client, clientkey, pw_callback);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_set错误，错误码：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
		rc = mosquitto_tls_set(mosq, ca, NULL, client, clientkey, pw_callback);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_set错误，错误码：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
		rc = mosquitto_tls_set(mosq, ca, NULL, client, clientkey, NULL);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_set错误，错误码：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
		rc = mosquitto_tls_set(mosq, NULL, "/etc/ssl/certs/", NULL, NULL, pw_callback);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_set错误，错误码：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
		bool set = true;
		thelog << "mosquitto_tls_insecure_set " << set << endi;
		rc = mosquitto_tls_insecure_set(mosq, set);//设为true禁用主机名验证
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_insecure_set错误，错误码：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
		rc = mosquitto_tls_set(mosq, ca, NULL, client, clientkey, pw_callback);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			thelog << "tls_set错误，错误码2：" << rc << " " << mosquitto_strerror(rc) << ende;
		}
	}

	mosquitto_username_pw_set(mosq, "user", "user1234");
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	rc = mosquitto_connect(mosq, "192.168.213.1", 8883, 60);
	mosquitto_loop(mosq, -1, 1);
	while (!testMqtts_connected)
	{
		SleepSeconds(1);
		thelog << "等待连接状态返回 " << endi;
		mosquitto_loop(mosq, -1, 1);
	}
	if (MOSQ_ERR_SUCCESS == rc)
	{
		thelog << "连接成功" << endi;//连接出错
		//发布测试
		while (true)
		{
			thelog << "请输入消息：" << endi;
			char buf[1024];
			if (NULL == fgets(buf, 1024, stdin))break;
			std::string topic1 = "test";
			rc = mosquitto_publish(mosq, NULL, topic1.c_str(), strlen(buf), (const void*)buf, 1, false);
			if (MOSQ_ERR_SUCCESS != rc)
			{
				thelog << "发送错误:" << mosqpp::strerror(rc) << endi;//发送出错
			}
		}
	}
	else
	{
		thelog << "连接错误:" << mosqpp::strerror(rc) << endi;//连接出错
	}
	mosquitto_lib_cleanup();
	thelog << "测试结束，程序退出" << endi;
	exit(0);
	return 0;
}
//void test_dlt645()
//{
//	int fd;
//	/* 打开串口 */
//	if (0 != openSerialPort(&fd))
//	{
//		printf("open dev fail!\n");
//		return;
//	}
//	if (0 != uart_setup(fd))
//	{
//		thelog << "初始化串口失败" << endi;
//		return;
//	}
//	dlt645_port_init(fd);
//	dlt645_read_test();
//}

int gwmain_test(int argc, char** argv)
{
	//DEBUG_LOG << endl << CJson::test() << endi;
	//CSocket::test();已验证
	//serial_port_test();//已验证
	//CModbus::modbusRTUMaste1();已验证
	//CModbus::modbusTCPMasteTest();//已验证
	//CMyZip::CZip_test(argc, argv);//已验证
	//testMqtt(argc, argv);//已验证
	//testMqtts(argc, argv);//已验证
	//CMyOpenSSL::aes_test();//已验证
	//test_dlt645();//已验证
	//snap7_test_main("192.168.100.25", 0, 1);//已验证
	//snap7_test_main("192.168.50.39", 0, 1);//已验证
	//myOpen62541::opcuatest();//已验证

	bool bTest = GetCommandParam(argc, argv, "-test");//是否是测试模式

	if (bTest)
	{
		pid_t parent_pid = getpid();//父进程PID
		pid_t pid = fork();
		if (pid < 0)
		{
			char buf[256];
			sprintf(buf, "fork error : %d : %s", errno, strerror(errno));
			thelog << buf << ende;
			return 1;
		}
		else if (0 == pid)
		{
			thelog.SetSource("TEST SERVER");
			thelog << "主进程 " << parent_pid << " 服务进程 " << getpid() << endi;

			modbus_t * ctx = modbus_new_tcp(NULL, 10503);//ubuntu上开启低端口会报权限不足，su也不行
			modbus_mapping_t * mb_mapping = modbus_mapping_new(100, 100, 100, 100);
			if (mb_mapping == NULL)
			{
				thelog << "modbus_mapping_new出错 " << modbus_strerror(errno) << ende;
				fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
				modbus_free(ctx);
				exit(0);
			}
			//设置初值
			{
				mb_mapping->start_registers = 0;
				for (int i = 0; i < 10; ++i)
				{
					mb_mapping->tab_registers[mb_mapping->start_registers + i] = i;
				}
			}
			while (CMyProcess::isProcessLive(parent_pid))
			{
				int s = modbus_tcp_listen(ctx, 5);
				if (s < 0)
				{
					thelog << "modbus_tcp_listen error : " << modbus_strerror(errno) << endi;
					SleepSeconds(1);
					continue;
				}
				modbus_tcp_accept(ctx, &s); 
				thelog << "s:" << s << endi;
				while (CMyProcess::isProcessLive(parent_pid))
				{
					uint8_t query[512];
					int rc = modbus_receive(ctx, query);
					if (rc > 0)
					{
						modbus_reply(ctx, query, rc, mb_mapping);
					}
					else if (rc == -1)
					{
						break;
					}
					//改变数据
					for (int i = 0; i < 10; ++i)
					{
						++mb_mapping->tab_registers[mb_mapping->start_registers + i];
					}
				}
				thelog << "对方断开或出错 " << modbus_strerror(errno) << endi;
				if (s != -1)
				{
					close(s);
				}
			}
			modbus_mapping_free(mb_mapping);
			modbus_close(ctx);
			modbus_free(ctx);
			thelog << "父进程已经终止，服务结束" << endi;
			exit(0);
		}
		else
		{
			SleepSeconds(5);
			thelog << "服务进程已创建" << endi;
		}
	}

	return 0;
}

