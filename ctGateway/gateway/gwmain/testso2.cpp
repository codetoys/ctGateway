
#include "testso2.h"
#include "myUtil.h" 
using namespace ns_my_std;

char so_b() { return 'b'; }

extern "C" void test_fun_dlopen();

int Ca::fun()
{
	cout << "Ca::fun 调用 test_fun_dlopen" << endl;
	test_fun_dlopen();
	return 2;
}
