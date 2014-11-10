#include "../App.h"

int timeout=10;
char buf[2112];
int i;

struct timeval begintime, endtime;

int main()
{
	set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,HIGH);				//mifate进入工作模式

	open_driver(MGCARD,O_RDONLY);						//打开磁条卡设备

	gettimeofday(&begintime, NULL);						//读取系统时间      

	while(1)
	{
		gettimeofday(&endtime, NULL );					//读取系统时间 

		if(read(Drivers[MGCARD].fd,buf,2112)!=-1)			//读取磁条卡信息
		{
			/*
			printf("\n");
			for(i = 753; i < 1035; i++){
				if(i%30 == 0)printf("\n");
				printf("%x   ",buf[i]);
			}
			*/
			covert_data(buf);					//转换磁条卡数据
			set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,LOW);		//mifare进入低功耗模式
			return 0;
		}

		if((endtime.tv_sec-begintime.tv_sec) > 10)
		{
			printf("\n\nCharge overtime!\n\n");
			close(Drivers[MGCARD].fd);
			set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,LOW);		//mifare进入低功耗模式
			return;
		}
	}
}
