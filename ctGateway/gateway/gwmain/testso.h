
#pragma once
#include "testso2.h"

#define DLL_PUBLIC __attribute__ ((visibility("default")))
extern "C" 
{
	DLL_PUBLIC int so_a();
	DLL_PUBLIC Ia* getClass();
}
