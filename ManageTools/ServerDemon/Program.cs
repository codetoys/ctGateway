using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using MqttToysBase;
using MqttToysModels.Models;

namespace ServerDemon
{
	internal class MqttClient : BaseMqttClient
	{
		private readonly Dictionary<string, GwInfo> m_GwInfos = [];
		//public BaseInfluxDBTools baseInfluxDBTools;
		public MqttClient() : base(Global.default_key, Global.keyMgr)
		{
			{
				DbContextOptionsBuilder<gatewayContext> optionsBuilder =new ();
				optionsBuilder.UseMySQL("server=127.0.0.1;userid=user1;pwd=user1;port=3306;database=gateway;sslmode=none;allowPublicKeyRetrieval=true;");
				using var db = new gatewayContext(optionsBuilder.Options);
				var keys = db.gw_keys;
				foreach (gw_key item in keys)
				{
					myUtils.Log(item.gw_sn);
				}
			}
			//baseInfluxDBTools = new BaseInfluxDBTools();
			if (Global.baseMySqlTools.ConnectMySql("127.0.0.1", "user1", "user1", "gateway"))
			{
				myUtils.Log("mysql连接成功");
			}
			else
			{
				myUtils.Log("mysql连接失败");
			}
			Global.keyMgr.LoadAllKeys(Global.baseMySqlTools.connection);
		}
		public override void OnMqttClientProcessMessage(string topic, SnMessage snMessage)
		{
			myUtils.Log(snMessage.sn);
			if (!m_GwInfos.TryGetValue(snMessage.sn, out GwInfo? gwInfo))
			{
				m_GwInfos[snMessage.sn] = new GwInfo(snMessage.sn);
				gwInfo = m_GwInfos[snMessage.sn];
			}
			Package.ProcessMessage(snMessage, gwInfo, true, null);//baseInfluxDBTools 占空间太大也没什么用，停用influxdb
			_ = Global.baseMySqlTools.InsertToMySql(topic, snMessage, gwInfo);
		}
		public override void ShowMqttState(string message)
		{
			myUtils.Log(message);
		}
	}
	internal class Program
	{
		private static void Main(string[] args)
		{
			myUtils.Log("ServerDemon 2024.09.13 16:39");

			bool bDebug = false;
			foreach (string arg in args)
			{
				myUtils.Log($"{arg}");
				if (arg == "-debug")
				{
					bDebug = true;
				}
			}

			if (!bDebug)
			{
				BaseInfluxDBTools.url = "http://127.0.0.1:8086";//非调试时的服务器，influxdb实际未用
			}
			myUtils.Log("调试模式：" + bDebug);
			myUtils.Log("BaseInfluxDBTools.url = " + BaseInfluxDBTools.url);

			while (true)
			{
				try
				{
					MqttClient mqttClient = new();
					mqttClient.mqttConfig.type = "MQTTS";//注意使用的证书文件是链接的ManageTools的证书文件
					mqttClient.mqttConfig.platform = "localtest";
					mqttClient.mqttConfig.hostname = "127.0.0.1";
					mqttClient.mqttConfig.port = "8883";
					mqttClient.mqttConfig.userid = "user";
					mqttClient.mqttConfig.upwd = "user1234";
					if (!mqttClient.Connect(out string errorMessage))
					{
						throw new Exception(errorMessage);
					}
					else
					{
						myUtils.Log("成功连接到 " + mqttClient.mqttConfig.ToShortString());
					}
					_ = mqttClient.SubscribeAll();
					while (true)
					{
						myUtils.ThreadSleep(10 * 1000);
						//if (!mqttClient.mqttClient.IsConnected)
						//{
						//	mqttClient.ReConnect();
						//}
					}
				}
				catch (Exception ex)
				{
					myUtils.Log(ex.ToString());
					if (bDebug)
					{
						break;//正式运行时循环
					}
				}
			}
		}
	}
}
