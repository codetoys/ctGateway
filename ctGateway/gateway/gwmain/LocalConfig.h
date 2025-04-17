//LocalConfig.h 本地配置模块

#pragma once

#include "myUtil.h" 
using namespace ns_my_std;

#include "WorkThread.h"

class CLocalConfig : public IWorkThread
{
public:
	CWorkerThread m_WorkerThread;
	CLocalConfig();
	~CLocalConfig();

private:
	thread* m_httpd_thread = nullptr;

	static void _start_httpd();
public:
	void start_httpd();
	virtual void worker_job()override;
	virtual void timer_job()override;

};
