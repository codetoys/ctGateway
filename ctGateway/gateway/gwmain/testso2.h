#pragma once

#define DLL_PUBLIC __attribute__ ((visibility("default")))
extern "C" 
{
	DLL_PUBLIC char so_b();
}

class Ia
{
public: int data;
   virtual int fun()=0;
};
class DLL_PUBLIC Ca : public Ia
{
public:
	virtual int fun()override;
};
