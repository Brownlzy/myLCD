#include "myLCD.h"
#include <stdio.h>	//printf()
#include <stdlib.h> //exit()
#include <math.h>
#include <signal.h> //signal()

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		setDefaultNetworkName();
		DashBoard_myLCD();
	}
	else if(argc == 4)
	{
		setNetworkName(argv[1],argv[2],argv[3]);
		DashBoard_myLCD();
	}
	else
	{
		printf("command error!");
		exit(1);
	}
	return 0;
}
