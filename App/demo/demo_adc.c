#include "../App.h"

int buf[1]={0};

int main()
{
	while(1)
	{
		open_driver(TLC549,O_RDWR);					//打开TLC549设备
		read(Drivers[TLC549].fd, buf, 1);			//读取TLC549 AD值
		printf("ad=%d\n",buf[0]);
		sleep(1);
	}
}


