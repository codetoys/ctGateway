
#pragma once

#include "modbuslib/modbus.h"
#include <iostream>
#include <stdio.h>

class CModbus
{
private:
	modbus_t* ctx = NULL;
	int slaveAddr = 1;
	bool isConnected = false;
public:
	bool IsConnected()const { return isConnected; }
	bool InitModbusTcp(char const* ip, int port)
	{
		UnInitModbus();

		thelog << "InitModbusTcp : " << ip << ":" << port << ende;
		ctx = modbus_new_tcp(ip, port);
		if (modbus_connect(ctx) == -1)
		{
			thelog << "Connexion failed : " << modbus_strerror(errno) << " : " << ip << ":" << port << ende;
			modbus_free(ctx);
			ctx = NULL;
			return false;
		}
		isConnected = true;
		return true;
	}
	bool InitModbusRtuTcp(char const* ip, int port)
	{
		UnInitModbus();

		ctx = modbus_new_rtutcp(ip, port);
		if (modbus_connect(ctx) == -1)
		{
			thelog << "Connexion failed : " << modbus_strerror(errno) << " : " << ip << ":" << port << ende;
			modbus_free(ctx);
			ctx = NULL;
			return false;
		}
		isConnected = true;
		return true;
	}
	bool InitModbusRtu(const char* serial_port, int baud, char parity, int data_bit, int stop_bit,int timeout_s)
	{
		UnInitModbus();

		ctx = modbus_new_rtu(serial_port, baud, parity, data_bit, stop_bit);
		if (modbus_connect(ctx) == -1)
		{
			thelog << "Connexion failed : " << modbus_strerror(errno) << " : " << serial_port << " baud " << baud<<" parity "<< parity<<" data_bit " << data_bit<<" stop_bit "<< stop_bit << ende;
			modbus_free(ctx);
			ctx = NULL;
			return false;
		}
		//modbus_set_response_timeout(ctx, timeout_s, 0);//设置响应时间
		isConnected = true;
		return true;
	}
	bool UnInitModbus()
	{
		if (ctx)
		{
			modbus_close(ctx);
			modbus_free(ctx);//关闭 清理 modbus ctx
			ctx = NULL;
			isConnected = false;
		}
		return true;
	}
	//设置从站
	bool SetSlave(int slaveAddr)
	{
		if (-1 == modbus_set_slave(ctx, slaveAddr))
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	//读取输出线圈
	bool Read01H0z(int addr, uint8_t* regBuffer, int n)
	{
		int bytes = n / 8;//所需的字节数
		if (0 != n % 8)++bytes;
		memset(regBuffer, 0, bytes);
		int regs = modbus_read_bits(ctx, addr, n, regBuffer);
		if (n == regs)
		{
			return true;
		}
		else
		{
			thelog << "modbus_read_bits failed : " << regs << " errno : " << errno << " " << modbus_strerror(errno) << ende;
			return false;
		}
	}
	//写输出线圈
	bool Write05H0FH0z(int addr, uint8_t* regBuffer, int n)
	{
		int regs = modbus_write_bits(ctx, addr, n, regBuffer);
		if (n == regs)
		{
			return true;
		}
		else
		{
			thelog << "modbus_write_bits failed : " << regs << " errno : " << errno << " " << modbus_strerror(errno) << ende;
			return false;
		}
	}
	//读取输入线圈
	bool Read02H1z(int addr, uint8_t* regBuffer, int n)
	{
		int bytes = n / 8;//所需的字节数
		if (0 != n % 8)++bytes;
		memset(regBuffer, 0, bytes);
		int regs = modbus_read_input_bits(ctx, addr, n, regBuffer);
		if (n == regs)
		{
			return true;
		}
		else
		{
			thelog << "modbus_read_input_bits failed : " << regs << " errno : " << errno << " " << modbus_strerror(errno) << ende;
			return false;
		}
	}
	//读取保持寄存器
	bool Read03H4z(int addr, uint16_t* regBuffer, int n)
	{
		//CMyTime t3;
		//t3.SetCurrentTime();
		memset(regBuffer, 0, n * sizeof(uint16_t));
		int regs = modbus_read_registers(ctx, addr, n, regBuffer);
		if (n == regs)
		{
			//thelog << "读取保持寄存器耗时 " << t3.GetTimeSpanUS() << " 微秒" << endi;
			return true;
		}
		else
		{
			thelog << "modbus_read_registers failed : " << regs << " errno : " << errno << " " << modbus_strerror(errno) << ende;
			return false;
		}
	}
	//读取输入寄存器
	bool Read04H3z(int addr, uint16_t* regBuffer, int n)
	{
		memset(regBuffer, 0, n * sizeof(uint16_t));
		int regs = modbus_read_input_registers(ctx, addr, n, regBuffer);
		if (n == regs)
		{
			return true;
		}
		else
		{
			thelog << "modbus_read_input_registers failed : " << regs << " errno : " << errno << " " << modbus_strerror(errno) << ende;
			return false;
		}
	}
	//写保持寄存器，先尝试一次写入，如果失败则循环写入
	bool Write06H10H4z(int addr, uint16_t const* regBuffer, int n)
	{
		if (n == modbus_write_registers(ctx, addr, n, regBuffer))return true;
		for (int i = 0; i < n; ++i)
		{
			if (-1 == modbus_write_register(ctx, addr, regBuffer[i]))
			{
				thelog << "modbus_write_register failed :  errno : " << errno << " " << modbus_strerror(errno) << ende;
				return false;
			}
		}
		return true;
	}

	/*
	* 功能：本函数完成modbus主站，读取input regs
	*/
	static int modbusRTUMaste1()
	{
		modbus_t* ctx;
		int slaveAddr = 1;
		//建立modbus buf
		uint8_t dest[1024]; //setup memory for data
		uint16_t* dest16 = (uint16_t*)dest;
		memset(dest, 0, 1024);
		//初始化Modbus实例
		ctx = modbus_new_rtu("/dev/ttySE1", 9600, 'N', 8, 1);// 使用9600-N-8，1个停止位
		if (modbus_connect(ctx) == -1)
		{
			std::cout << "Connexion failed : " << modbus_strerror(errno) << std::endl;
			fprintf(stderr, "Connexion failed: %s\n", modbus_strerror(errno));
			modbus_free(ctx);
			return -1;
		}
		modbus_set_slave(ctx, slaveAddr);//设置从站地址，当前为1
		modbus_set_response_timeout(ctx, 1, 0);//设置响应时间
		/*
		 * modbus_read_bits  读线圈  fc=1
		 * modbus_read_input_bits 读输入线圈 fc=2
		 * modbus_read_registers 读取保持寄存器 fc=3
		 * modbus_read_input_registers 读输入寄存器 fc=4
		 * modbus_write_bit 写一位数据（强置单线圈） fc=5
		 * modbus_write_register 写单寄存器（预置单寄存器） fc=6
		 * modbus_write_bits 写多位数据（强置多线圈） fc=15
		 * modbus_write_and_read_registers 读/写多个寄存器 fc=23
		*/
		int regs = modbus_read_input_registers(ctx/*libmodbus实例*/,
			0/*输入地址*/,
			8/*读取输入寄存器个数*/,
			dest16 /*获取从站值*/);
		if (regs > 0)
		{
			for (int i = 0; i < regs; ++i)
			{
				std::cout << i << " : " << dest16[i] << std::endl;
			}
		}
		else
		{
			std::cout << "modbus_read_input_registers failed : " << regs<<" errno : "<< modbus_strerror(errno) << std::endl;
		}
		modbus_close(ctx);
		modbus_free(ctx);//关闭 清理 modbus ctx
		return 0;
	}
	static int modbusTCPMasteTest()
	{
		thelog << "modbusTCPMasteTest" << endi;
		modbus_t* ctx;
		int slaveAddr = 1;
		//建立modbus buf
		uint8_t dest[1024]; //setup memory for data
		uint16_t* dest16 = (uint16_t*)dest;
		memset(dest, 0, 1024);
		//初始化Modbus实例
		ctx = modbus_new_tcp("192.168.42.2", 502);
		if (modbus_connect(ctx) == -1)
		{
			std::cout << "Connexion failed : " << modbus_strerror(errno) << std::endl;
			fprintf(stderr, "Connexion failed: %s\n", modbus_strerror(errno));
			modbus_free(ctx);
			return -1;
		}thelog << endi;
		modbus_set_slave(ctx, slaveAddr);//设置从站地址，当前为1
		thelog << endi;
		//modbus_set_response_timeout(ctx, 1, 0);//设置响应时间
		thelog << endi;
		/*
		 * modbus_read_bits  读线圈  fc=1
		 * modbus_read_input_bits 读输入线圈 fc=2
		 * modbus_read_registers 读取保持寄存器 fc=3
		 * modbus_read_input_registers 读输入寄存器 fc=4
		 * modbus_write_bit 写一位数据（强置单线圈） fc=5
		 * modbus_write_register 写单寄存器（预置单寄存器） fc=6
		 * modbus_write_bits 写多位数据（强置多线圈） fc=15
		 * modbus_write_and_read_registers 读/写多个寄存器 fc=23
		*/
		thelog << endi;
		int regs = modbus_read_input_registers(ctx/*libmodbus实例*/,
			1038/*输入地址*/,
			8/*读取输入寄存器个数*/,
			dest16 /*获取从站值*/);
		thelog <<regs<< endi;
		if (regs > 0)
		{
			for (int i = 0; i < regs; ++i)
			{
				std::cout << i << " : " << dest16[i] << std::endl;
			}
		}
		else
		{
			std::cout << "modbus_read_input_registers failed : " << regs << " errno : " << modbus_strerror(errno) << std::endl;
		}
		modbus_close(ctx);
		modbus_free(ctx);//关闭 清理 modbus ctx
		return 0;
	}
};
