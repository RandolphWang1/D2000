#include "../App.h"

int arg;

int main()
{
	set_gpio(RC531_PORT,RC531_PORT_NUM,LOW);

    open_driver(RC531, O_RDWR);	//打开射频卡设备

	if(Drivers[RC531].fd<0)
	{
		printf("open device error");

		set_gpio(RC531_PORT,RC531_PORT_NUM,HIGH);

		return;
	}

	printf("Please select a process\n");

	printf("1:mifare test\n2:write card\n3:read card\n");

	scanf("%d",&arg);

	switch(arg)
	{
		case 1: Rc531_test();

		case 2: write_card();break;

		case 3:	read_card();break;
	}
	close(Drivers[RC531].fd);

	set_gpio(RC531_PORT,RC531_PORT_NUM,LOW);

	return;
}

