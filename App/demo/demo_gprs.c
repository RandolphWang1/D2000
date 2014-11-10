#include "../App.h"

int fd;

int i;

char string[1024]={0};

int main()
{
	//	set_gpio(GPRS_PORT,GPRS_PORT_NUM,HIGH);

	//	sleep(1);
	fd=open_driver(TTYS3,O_RDWR);

	set_speed(fd,115200);

	set_Parity (fd, 8,1,'n'); 

	while(1)
	{
		memset(string,0,1024);

		scanf("%s",string);

		sprintf(string,"%s\r",string);


		//	memset(string,0,1024);

		tcflush(fd,TCIFLUSH);

		write(fd,string,strlen(string));

		sleep(2);
		//write(fd,"AT+QIOPEN=\"TCP\",\"55.210.240.28\",80\r",strlen("AT+QIOPEN=\"TCP\",\"55.210.240.28\",80\r"));         

		//		while(1)
		//		{
		read_datas_tty(fd,string,0,200000);

		printf("string=%s\n",string);

		//			scanf("%d",&i);

		//			if(i==1)break;
		//			else

		//			sleep(2);
		//		}
	}

} 
