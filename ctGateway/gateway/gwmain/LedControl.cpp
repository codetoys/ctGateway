//LedControl.cpp LED控制

#include "LedControl.h" 
#include "gwconfig.h" 
#include "MqttSvr.h" 

CLedControl::CLedControl()
{
	leds.resize(6);

	for (auto& v : leds)
	{
		v.flash_steps.push_back(0);
		v.flash_steps.push_back(1);
	}

	system("echo 218 > /sys/class/gpio/export");
	system("echo 160 > /sys/class/gpio/export");
	system("echo 172 > /sys/class/gpio/export");
	system("echo 223 > /sys/class/gpio/export");
	system("echo 147 > /sys/class/gpio/export");
	system("echo 142 > /sys/class/gpio/export");

	system("echo out > /sys/class/gpio/gpio218/direction");
	system("echo out > /sys/class/gpio/gpio160/direction");
	system("echo out > /sys/class/gpio/gpio172/direction");
	system("echo out > /sys/class/gpio/gpio223/direction");
	system("echo out > /sys/class/gpio/gpio147/direction");
	system("echo out > /sys/class/gpio/gpio142/direction");
}
CLedControl::~CLedControl()
{
	system("echo 218 > /sys/class/gpio/unexport");
	system("echo 160 > /sys/class/gpio/unexport");
	system("echo 172 > /sys/class/gpio/unexport");
	system("echo 223 > /sys/class/gpio/unexport");
	system("echo 147 > /sys/class/gpio/unexport");
	system("echo 142 > /sys/class/gpio/unexport");
}
bool CLedControl::TurnOnTurnOff(int i, bool bOn)
{
	if (bOn)
	{
		if (0 == i)system("echo 1 > /sys/class/gpio/gpio218/value");
		if (1 == i)system("echo 1 > /sys/class/gpio/gpio160/value");
		if (2 == i)system("echo 1 > /sys/class/gpio/gpio172/value");
		if (3 == i)system("echo 0 > /sys/class/gpio/gpio223/value");
		if (4 == i)system("echo 0 > /sys/class/gpio/gpio147/value");
		if (5 == i)system("echo 0 > /sys/class/gpio/gpio142/value");
	}
	else
	{
		if (0 == i)system("echo 0 > /sys/class/gpio/gpio218/value");
		if (1 == i)system("echo 0 > /sys/class/gpio/gpio160/value");
		if (2 == i)system("echo 0 > /sys/class/gpio/gpio172/value");
		if (3 == i)system("echo 1 > /sys/class/gpio/gpio223/value");
		if (4 == i)system("echo 1 > /sys/class/gpio/gpio147/value");
		if (5 == i)system("echo 1 > /sys/class/gpio/gpio142/value");
	}
	return true;
}

void CLedControl::worker_job()
{
	//平台指示灯
	if (!g_Global.pIPlatform->isPlatformConnected())leds[0].cmd = 0;
	else
	{
		if (g_Global.pIPlatform->isPlatformRegisted())leds[0].cmd = 1;
		else leds[0].cmd = 2;
	}
	//设备指示灯
	size_t i = 0;
	for (auto& _interface : g_Global.pCGwConfig->GetDeviceConfig().interfaces)
	{
		for (auto& _device : _interface->devices)
		{
			if (i < 5)
			{
				if (InterfaceState::CONNECT_OK != _interface->_InterfaceState)leds[i + 1].cmd = 0;
				else
				{
					if (DeviceState::CONNECT_OK == _device->_DeviceState)leds[i + 1].cmd = 1;
					else leds[i + 1].cmd = 2;
				}
			}

			++i;
		}
	}

	for (size_t i = 0; i < leds.size(); ++i)
	{
		led_state& led = leds[i];
		//thelog << "############################################### " << i << " " << led.cmd << " " << led.flash_step << endi;
		if (0 == led.cmd || 1 == led.cmd)
		{
			if (led.cmd == led.state)continue;
			led.state = led.cmd;
			TurnOnTurnOff(i, led.state);
		}
		else if (2 == led.cmd)
		{
			if (led.flash_step < 0 || led.flash_step >= (int)led.flash_steps.size())
			{
				led.flash_step = 0;
			}
			//thelog << led.flash_step << " " << led.flash_steps[led.flash_step] << endi;
			led.state = led.flash_steps[led.flash_step];
			TurnOnTurnOff(i, led.state);
			++led.flash_step;
		}
	}
}
