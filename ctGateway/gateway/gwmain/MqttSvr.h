//MqttSvr.h MQTT服务

#pragma once

#include "myUtil.h" 
#include "mimetype.h"
#include "myZIP.h"
using namespace ns_my_std;
#include "LockedList.h" 
#include "mosquitto/mosquitto.h"
#include "mosquitto/mosquittopp.h" 
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gwconfig.h"
#include "DataManager.h"

class CMqttSvr
{
public:
	class CMqtt :public mosqpp::mosquittopp
	{
	public:
		bool m_bConnected = false;
		int qos = 0;//0，至多一次，不管订阅者是否收到；1，至少一次，收到订阅者确认才会删除，否则重发；2，确保一次，会避免重复
		string topic_reg;//设备发现用
		string topic_server;
		string topic_client;
		int last_publish_rc;//最后发布数据的错误码

		bool Publish(int* mid, const char* topic, int payloadlen = 0, const void* payload = NULL, bool retain = false)
		{
			//MOSQ_ERR_SUCCESS				on success.
			//MOSQ_ERR_INVAL				if the input parameters were invalid.
			//MOSQ_ERR_NOMEM				if an out of memory condition occurred.
			//MOSQ_ERR_NO_CONN				if the client isn’t connected to a broker.
			//MOSQ_ERR_PROTOCOL				if there is a protocol error communicating with the broker.
			//MOSQ_ERR_PAYLOAD_SIZE			if payloadlen is too large.
			//MOSQ_ERR_MALFORMED_UTF8		if the topic is not valid UTF - 8
			//MOSQ_ERR_QOS_NOT_SUPPORTED	if the QoS is greater than that supported by the broker.
			//MOSQ_ERR_OVERSIZE_PACKET		if the resulting packet would be larger than supported by the broker.

			DEBUG_LOG << "准备调用publish" << endi;
			last_publish_rc = publish(mid, topic, payloadlen, payload, qos, retain);
			DEBUG_LOG << "调用publish返回 " << last_publish_rc << endi;
			if (MOSQ_ERR_SUCCESS == last_publish_rc)
			{
				g_Global.last_platform_connection_send = time(NULL);
				return true;
			}
			else if (MOSQ_ERR_INVAL == last_publish_rc)thelog << "参数无效" << ende;
			else if (MOSQ_ERR_NOMEM == last_publish_rc)thelog << "内存不足" << ende;
			else if (MOSQ_ERR_NO_CONN == last_publish_rc)thelog << "未连接" << ende;
			else if (MOSQ_ERR_PROTOCOL == last_publish_rc)thelog << "协议错误" << ende;
			else if (MOSQ_ERR_PAYLOAD_SIZE == last_publish_rc)thelog << "负载超长" << ende;
			else if (MOSQ_ERR_MALFORMED_UTF8 == last_publish_rc)thelog << "无效的UTF-8" << ende;
			else if (MOSQ_ERR_QOS_NOT_SUPPORTED == last_publish_rc)thelog << "服务端不支持的QOS" << ende;
			else if (MOSQ_ERR_OVERSIZE_PACKET == last_publish_rc)thelog << "服务端不支持的负载大小" << ende;
			else thelog << "错误码：" << last_publish_rc << ende;
			return false;
		}

		void on_connect(int rc)
		{
			thelog << "on_connect =================================================================" << rc << endi;
			g_Global.bPlatformConnected = true;
			if (MOSQ_ERR_SUCCESS == rc)
			{
				int ret;
				//订阅服务端下发的
				ret = subscribe(NULL, topic_server.c_str());
				if (MOSQ_ERR_SUCCESS != ret)
				{
					thelog << topic_server << " 订阅错误:" << mosqpp::strerror(ret) << endi;
				}
				else
				{
					thelog << topic_server << " 订阅成功" << endi;
				}
				//连接后注册，同时发送到设备发现和数据topic
				string reg = g_Global.pIPlatform->build_Reg();
				DEBUG_LOG << reg << endi;
				if (!Publish(NULL, topic_reg.c_str(), reg.size(), (const void*)reg.c_str()))
				{
					thelog << "注册失败 " << topic_reg << ende;
				}
				else
				{
					thelog << "注册成功 " << topic_reg << endi;
				}
				if (!Publish(NULL, topic_client.c_str(), reg.size(), (const void*)reg.c_str()))
				{
					thelog << "注册失败 " << topic_client << ende;
				}
				else
				{
					thelog << "注册成功 " << topic_client << endi;
				}
				m_bConnected = true;
			}
			else
			{
				thelog << topic_server << " 连接失败:" << mosqpp::strerror(rc) << endi;
			}
		}
		void on_disconnect()
		{
			thelog << "on_disconnect" << endi;
			m_bConnected = false;
			g_Global.bPlatformConnected = false;
		}
		virtual void on_error() 
		{
			thelog << "on_error ===================================================" << endi; 
			m_bConnected = false;
			g_Global.bPlatformConnected = false;
		}
		virtual void on_log(int level, const char* str) { thelog << "MQTT日志 " << level << " : " << str << endi; }
		void on_publish(int mid) { thelog << "on_publish " << mid << endi; }
		void on_subscribe(int mid, int qos_count, const int* granted_qos)//订阅回调函数
		{
			thelog << "订阅 mid: " << mid << endi;
		}
		void on_message(const struct mosquitto_message* message)//订阅主题接收到消息
		{
			g_Global.last_platform_connection_recv = time(NULL);

			//保留消息每次连接都会收到，如果是重启命令会直接导致不停重启
			if (message->retain)
			{
				DEBUG_LOG << "忽略retain消息" << endi;
				return;
			}

			//注意本函数会阻塞后续数据发送，所以应该尽快返回
			if (!message->payload)
			{
				DEBUG_LOG << "空消息" << endi;
				return;
			}
			bool res = false;
			mosqpp::topic_matches_sub(topic_server.c_str(), message->topic, &res);
			if (res)
			{
				DEBUG_LOG << "来自<" << message->topic << ">的消息：" << (message->payloadlen < 100 * 1024 ? (char*)message->payload : "长消息") << endi;
				g_Global.pIPlatform->m_ServerMesssageList.locked_push_back((char*)message->payload);
				g_Global.pIPlatform->m_WorkerThread.ActiveWorkerThread();
			}
		}
	};
private:
	class CMqttMessage
	{
	public:
		bool isData = false;//是否是通道数据
		T_DATA_SEQ data_seq = 0;//如果isData，这里是序列号
		string m_message;
		CMqttMessage() {}
		static CMqttMessage build(string const& msg)
		{
			CMqttMessage ret;
			ret.m_message = msg;
			ret.isData = false;
			return ret;
		}
		static CMqttMessage buildData(string const& msg, T_DATA_SEQ seq)
		{
			CMqttMessage ret;
			ret.data_seq = seq;
			ret.m_message = msg;
			ret.isData = true;
			return ret;
		}
	};
	CMqtt m_mqtt;//mqtt对象

	string m_send_topic;//发送的topic
	LockedList<CMqttMessage > m_messagelist;//待发送的队列
	thread* m_send_thread = nullptr;
	mutex m_send_mutex;//互斥锁
	condition_variable m_send_cv;//条件变量

	thread* m_timer_thread = nullptr;//定时循环

	time_t time_last_send_data = 0;//最后一次发送数据的时间

	bool _SendMqttMessage(CMqttMessage const& msg)
	{
		bool bSendOK = false;
		for (int i = 0; i < 2; ++i)
		{
			if (!m_mqtt.m_bConnected)
			{
				thelog << "未连接，尝试重新连接 " << endi;
				int rc = m_mqtt.reconnect();
				if (MOSQ_ERR_SUCCESS != rc)
				{
					thelog << "重新连接MQTT服务器失败 " << rc << " : " << mosqpp::strerror(rc) << ende;
					break;
				}
			}
			if (m_mqtt.m_bConnected)
			{
				if (!m_mqtt.Publish(NULL, m_send_topic.c_str(), msg.m_message.size(), (const void*)msg.m_message.c_str()))
				{
					thelog << "发送失败 " << bSendOK << endi;
				}
				else
				{
					thelog << "发送成功 " << bSendOK << endi;
					bSendOK = true;
					break;
				}
			}
		}

		return bSendOK;
	}
	void _SendThread()
	{
		thelog << "SendThread" << endi;
		MqttInit();
		while (true)
		{
			//处理队列
			CMqttMessage mqttmessage;
			while (m_messagelist.locked_TryGetBegin(mqttmessage))
			{
				if (mqttmessage.isData)
				{
					T_DATA_SEQ from = g_Global.pCGwConfig->NextSeq(g_Global.pCGwConfig->GetLastSendedSeq());
					T_DATA_SEQ to = g_Global.pCGwConfig->PrevSeq(mqttmessage.data_seq);
					if (mqttmessage.data_seq > from)//序列号小可能是平台要求重发或之前未能发送
					{
						thelog << "发送积压数据 " << from << " - " << to << endi;

						string msg;
						if (g_Global.pCDataManager->LoadDataWithCache(from, to, msg))
						{
							m_messagelist.locked_push_front(CMqttMessage::buildData(msg, from));//塞入队列，下一个循环立即发送
						}
						else
						{
							g_Global.pCGwConfig->SetLastSendedSeq(from);//数据不存在，假装已经发送过
						}

						continue;
					}
				}

				if (_SendMqttMessage(mqttmessage))
				{
					time_last_send_data = time(NULL);
					if (mqttmessage.isData && mqttmessage.data_seq > g_Global.pCGwConfig->GetLastSendedSeq())g_Global.pCGwConfig->SetLastSendedSeq(mqttmessage.data_seq);//只有更大才修改

					m_messagelist.locked_pop_front();
				}
				else
				{
					thelog << "发送错误，未发送数据包：" << m_messagelist.locked_size() << endi;//发送出错
					if (!m_mqtt.m_bConnected || MOSQ_ERR_NO_CONN == m_mqtt.last_publish_rc)
					{
						thelog << "未连接，丢弃所有包（数据包已保存）" << ende;
						m_messagelist.locked_clear();
					}
					else
					{//其它错误，丢弃
						thelog << "其它错误，丢弃此包：" << mqttmessage.isData << " " << mqttmessage.data_seq << " " << mqttmessage.m_message.size() << ende;
						m_messagelist.locked_pop_front();
					}
				}
			}

			//等待数据
			unique_lock<mutex> lck(m_send_mutex);
			m_send_cv.wait(lck);
			DEBUG_LOG << "SendThread被激活" << endi;
		}
	}
	void _TimerThread()
	{
		DEBUG_LOG << "TimerThread" << endi;
		unsigned int i = 0;
		int hb_time = 60;//心跳间隔
		while (true)
		{
			++i;
			SleepSeconds(1);

			//定时激活发送线程，避免发送线程无限期等待
			if (0 == i % 10)
			{
				m_send_cv.notify_one();
			}
			//定时发送心跳
			if ((0 == i % hb_time) && (time(NULL) - time_last_send_data > hb_time))
			{
				MqttPostMessage("hb", "", g_Global.pIPlatform->build_Hb());
			}
		}
	}
	static void SendThread(CMqttSvr* pMe)
	{
		pMe->_SendThread();
	}
	static void TimerThread(CMqttSvr* pMe)
	{
		pMe->_TimerThread();
	}
public:
	bool isMqttConnected()const { return this->m_mqtt.m_bConnected; }
	bool MqttInit()
	{
		mosqpp::lib_init();

		char client_id[1024];//sn+随机数
		srand(time(NULL));
		sprintf(client_id, "%s_%03d", g_Global.pCGwConfig->GetSn().c_str(), rand() % 1000);
		thelog << "client_id[" << client_id << "]" << endi;

		m_mqtt.reinitialise(client_id, true);

		m_send_topic = "device_gateway/" + g_Global.pCGwConfig->GetSn() + "/client";

		m_mqtt.topic_reg = "device_gateway/reg";
		m_mqtt.topic_client = m_send_topic;
		m_mqtt.topic_server = "device_gateway/" + g_Global.pCGwConfig->GetSn() + "/server";

		int rc;

		if (0 == stricmp(g_Global.pCGwConfig->GetNorthConfig().type.c_str(), "MQTTS"))
		{
			//这些尚未测试成功，但类似的设置和证书客户端是成功的
			string certdir = "./SSL/";
			certdir += g_Global.pCGwConfig->GetNorthConfig().platform + "/";
			thelog << "使用MQTTS 证书路径：" << certdir << endi;
			rc = m_mqtt.tls_set((certdir + "ca.pem").c_str(), NULL, (certdir + "client.pem").c_str(), (certdir + "client.key").c_str(), NULL);
			if (MOSQ_ERR_SUCCESS != rc)
			{
				thelog << "tls_set错误，错误码：" << rc << " " << mosqpp::strerror(rc) << ende;
			}
			rc = m_mqtt.tls_insecure_set(true);//设为true禁用服务端证书验证
			if (MOSQ_ERR_SUCCESS != rc)
			{
				thelog << "tls_insecure_set错误，错误码：" << rc << " " << mosqpp::strerror(rc) << ende;
			}
			thelog << "禁用服务端证书验证" << endi;
		}

		m_mqtt.username_pw_set(g_Global.pCGwConfig->GetNorthConfig().userid.c_str(), g_Global.pCGwConfig->GetNorthConfig().upwd.c_str());

		string host = g_Global.pCGwConfig->GetNorthConfig().hostname.c_str();
		for (int i = 0; i < 10; ++i)
		{
			thelog << i << " 连接到:[" << host << "] " << g_Global.pCGwConfig->GetNorthConfig().port << endi;
			rc = m_mqtt.connect(host.c_str(), g_Global.pCGwConfig->GetNorthConfig().port, 60);
			if (rc == MOSQ_ERR_ERRNO)
			{
				thelog << "连接错误:" << mosqpp::strerror(rc) << endi;//连接出错
				SleepSeconds(10);
			}
			else break;
		}
		if (rc == MOSQ_ERR_ERRNO)
		{
			thelog << "连接错误:" << mosqpp::strerror(rc) << endi;//连接出错
		}
		else if (MOSQ_ERR_SUCCESS == rc)
		{
			thelog << "连接成功（注意这里不一定真的连接了）" << endi;//连接成功
		}
		else
		{
			thelog << "连接错误，错误码：" << rc << " " << mosqpp::strerror(rc) << ende;
		}

		//启动处理循环（内部是一个子线程），不论是否连接成功都要启动
		rc = m_mqtt.loop_start();

		return true;
	}
	void MqttStartProcessThread()
	{
		//启动子线程
		m_send_thread = new thread(SendThread, this);
		m_timer_thread = new thread(TimerThread, this);
	}
	void MqttUninit()
	{
		//需要首先停止子线程，不过一般设备都是直接断电重启的，所以无所谓
		m_mqtt.loop_stop();
		m_mqtt.disconnect();
		mosqpp::lib_cleanup();
	}
	bool MqttPostMessage(string func, string respondFunc, string str)
	{
		thelog << "MqttPostMessage " << func << " [" << respondFunc << "]" << endi;
		m_messagelist.locked_push_back(CMqttMessage::build(g_Global.pIPlatform->MessageTranslate(func, respondFunc, str)));
		m_send_cv.notify_one();
		return true;
	}
	bool MqttPostDataMessage(string func, string respondFunc, string str, T_DATA_SEQ seq)
	{
		thelog << "MqttPostDataMessage " << func << " [" << respondFunc << "] " << seq << endi;
		m_messagelist.locked_push_back(CMqttMessage::buildData(g_Global.pIPlatform->MessageTranslate(func, respondFunc, str), seq));
		m_send_cv.notify_one();
		return true;
	}
};
