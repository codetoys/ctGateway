//LedControl.h LED控制

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;

#include "WorkThread.h"

class CLedControl : public IWorkThread
{
public:
	CWorkerThread m_WorkerThread;
private:
	struct led_state
	{
		int cmd = 0;//设置命令，0 常灭 1 常亮 2 闪烁
		int state = -1;//每个led的状态，0 常灭 1 常亮 2 闪烁
		int flash_step = 0;
		vector<int > flash_steps;//闪烁步骤
	};
	vector<led_state> leds;//所有led
	bool TurnOnTurnOff(int i, bool bOn);
public:
	CLedControl();
	~CLedControl();

	virtual void worker_job()override;
	virtual void timer_job()override {}


};
