
#include "testso.h"
#include <stdio.h>

int so_a() 
{
	printf("testso.cpp so_a : %c\n", so_b());
	return 4; 
}

Ca ca;
Ia* getClass()
{
	printf("testso.cpp getClass ：%c\n", so_b());
	return &ca;
}