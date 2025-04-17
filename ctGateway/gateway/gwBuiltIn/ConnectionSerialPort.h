//ConnectionSerialPort.h 串口连接

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;
#include "ConnectionBase.h"
#include "ProtocolParam.h"
#include <termio.h>

class CSerialPortConnection :public IConnection
{
private:
	int fd = -1;
	ProtocolParam_TCP_or_SerialPort* protocolParam;

	static speed_t _Bund(int bund)
	{
		if (50 == bund)return B50;
		if (75 == bund)return B75;
		//if (100 == bund)return B100;
		if (134 == bund)return B134;
		if (150 == bund)return B150;
		if (200 == bund)return B200;
		if (300 == bund)return B300;
		if (600 == bund)return B600;
		if (1200 == bund)return B1200;
		if (1800 == bund)return B1800;
		if (2400 == bund)return B2400;
		if (9600 == bund)return B9600;
		if (19200 == bund)return B19200;
		if (38400 == bund)return B38400;
		if (57600 == bund)return B57600;
		if (115200 == bund)return B115200;
		thelog << "未知的波特率 " << bund << ende;
		return B0;
	}
	/* 115200, 8, N, 1 */
	int _setup_serial_port()
	{
		struct termios options;

		// 获取原有串口配置
		if (tcgetattr(fd, &options) < 0)
		{
			thelog << "获取串口设置失败 " << strerror(errno) << ende;
			return -1;
		}
		memset(&options, 0, sizeof(options));//网关上有问题，全清了

		// 修改控制模式，保证程序不会占用串口
		options.c_cflag |= CLOCAL;//忽略解制-解调器状态行

		// 修改控制模式，能够从串口读取数据
		options.c_cflag |= CREAD;//启用接收装置

		// 不使用流控制
		options.c_cflag &= ~CRTSCTS;

		// 设置数据位
		options.c_cflag &= ~CSIZE;//字符大小屏蔽
		if (5 == protocolParam->data_bit)options.c_cflag |= CS5;//CS5 CS6 CS7 CS8
		if (6 == protocolParam->data_bit)options.c_cflag |= CS6;//CS5 CS6 CS7 CS8
		if (7 == protocolParam->data_bit)options.c_cflag |= CS7;//CS5 CS6 CS7 CS8
		if (8 == protocolParam->data_bit)options.c_cflag |= CS8;//CS5 CS6 CS7 CS8

		// 设置奇偶校验位
		if ('N' == protocolParam->parity)
		{
			options.c_cflag &= ~PARENB;//PARENB 奇偶校验
			options.c_iflag &= ~INPCK;//INPCK 输入的奇偶校验
		}
		else
		{
			options.c_cflag |= PARENB;
			options.c_iflag |= INPCK;
			//options.c_iflag &= ~INPCK;//5G网关接收数据就会导致发送异常，跟这个无关
			if ('E' == protocolParam->parity)
			{
				options.c_cflag &= ~PARODD;
			}
			if ('O' == protocolParam->parity)
			{
				options.c_cflag |= PARODD;
			}
		}	

		// 设置停止位
		if (2 == protocolParam->stop_bit)options.c_cflag &= CSTOPB;//设置CSTOPB 停止位2 ，否则为1
		if (1 == protocolParam->stop_bit)options.c_cflag &= ~CSTOPB;//设置CSTOPB 停止位2 ，否则为1

		// 设置最少字符和等待时间
		options.c_cc[VMIN] = 0;     // 读数据的最小字节数
		options.c_cc[VTIME] = 1;   //等待第1个数据，单位是100ms，通常是8位无符号字节

		// 修改输出模式，原始数据输出
		options.c_oflag &= ~OPOST;
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

		// 设置波特率
		speed_t speed = _Bund(protocolParam->baud);

		thelog << "波特率设置 " << protocolParam->baud << " B值:" << speed << endi;
		cfsetispeed(&options, speed);
		cfsetospeed(&options, speed);

		// 清空终端未完成的数据
		tcflush(fd, TCIFLUSH);

		// 设置新属性
		if (tcsetattr(fd, TCSANOW, &options) < 0)
		{
			thelog << "修改串口设置失败 " << strerror(errno) << ende;
			return -1;
		}

		return 0;
	}
	bool _OpenSerialPort()
	{
		/* 打开串口 */
		fd = open(protocolParam->serial_port.c_str(), O_RDWR | O_NOCTTY |  O_NONBLOCK);//O_NDELAY|
		if (fd < 0)
		{
			thelog << "打开串口失败 " << strerror(errno) <<ende;
			return false;
		}
		else
		{
			fcntl(fd, F_SETFL, FNDELAY);//设置文件标志（此代码目的不明确）
		}

		/* 设置串口 */
		int n = _setup_serial_port();
		if (n < 0)
		{
			thelog << "串口设置失败 " << errno << " : " << strerror(errno) << ende;
			close(fd);
			fd = -1;
			return false;
		}

		return true;
	}
public:
	CSerialPortConnection(ProtocolParam_TCP_or_SerialPort* p) :protocolParam(p) {}
public://IConnection
	virtual ~CSerialPortConnection() override {}
	virtual bool IConnection_isConnected()override { return fd >= 0; }
	virtual bool EnsureConnected(InterfaceDeviceConfig* pInterface)override
	{
		if (NULL==protocolParam)
		{
			thelog << "====================================================无法连接，并非ProtocolParam_TCP_or_SerialPort类型"<< endi;
		}
		else if (fd<0)
		{
			thelog << "====================================================连接 " << protocolParam->serial_port << endi;
			if (!_OpenSerialPort())
			{
				thelog << "====================================================连接失败 " << protocolParam->serial_port << ende;
			}
			else
			{
				thelog << "====================================================连接成功 " << protocolParam->serial_port << endi;
			}
		}
		return fd >= 0;
	}
	virtual bool Send(char const* data, int len)override
	{
		if (fd < 0)
		{
			thelog << "串口出错或尚未打开" << ende;
			return false;
		}
	
		int n = write(fd, data, len);
		if (n < 0)
		{
			thelog << "串口写入失败 " << errno << " : " << strerror(errno) << ende;
			return false;
		}
		thelog << "-----------------------------串口写入 " << n << " " << CMyTools::ToHex((char*)data, len) << endi;
		return true;
	}
	virtual bool Recv(char* buf, int len, long* recvLen, int timeout_ms)override
	{
		thelog << "Recv" << endi;

		*recvLen = 0;

		if (fd < 0)
		{
			thelog << "串口出错或尚未打开" << ende;
			return false;
		}

		CMyTime t;
		t.SetCurrentTime();
		while (t.GetTimeSpanMS() <= timeout_ms && *recvLen < len)
		{
			//thelog << "read" << endi;
			int n = read(fd, buf + *recvLen, len - *recvLen);
			//thelog << "read 返回 " << n << endi;
			if (n > 0)
			{
				*recvLen += n;
			}
			else if (0 == n || (-1 == n && EAGAIN == errno))
			{
				if (*recvLen > 0)
				{
					//return true;//已经有数据，大概率是数据帧已经完整，可以返回
				}
			}
			else
			{
				thelog << "串口读取失败 " << n << " : " << strerror(errno) << ende;
				if (*recvLen > 0)
				{
					return true;//已经有数据，认为正确
				}
				else
				{
					close(fd);
					fd = -1;
					return false;
				}
			}
		}
		thelog << "read 超时返回" << endi;
		return true;//超时返回
	}
public:
	int GetFD() { return fd; }
	void CloseSerialPort()
	{
		if (fd >= 0)
		{
			close(fd);
			fd = -1;
		}
	}
};
