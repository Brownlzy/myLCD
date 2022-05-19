#include "myLCD.h"
#include <stdio.h>	//printf()
#include <stdlib.h> //exit()
#include <math.h>
#include <signal.h> //signal()

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		DashBoard_myLCD();
	}
	else
	{
		exit(1);
	}
	return 0;
}
