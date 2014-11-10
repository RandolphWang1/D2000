#include "../App.h"

#define	  COLDRESET    0X11  
#define   RECEIVE_LEN  0X12  

int i = 0, receive_len[1] = {0};
char receive_buf[50] = {0}; 

void coldreset()
{
	ioctl(Drivers[PSAM1].fd,COLDRESET);									//发送冷复位命令
	ioctl(Drivers[PSAM1].fd, RECEIVE_LEN, receive_len);					//获取命令执行返回信息长度

	read(Drivers[PSAM1].fd, receive_buf, receive_len[0]);				//读取返回信息 
	
	printf("\ncoldrest num is:");
	for(i=0; i<receive_len[0]; i++)
	{
		printf("0x%x ",receive_buf[i]); 	
	}
	printf("\n");
}

void random_num()
{
	char len_buf[2] = {0};
	char send[5] = {0x00,0x84,0x00,0x00,0x04};

	len_buf[0] = 10;
	len_buf[1] = 10;



	write(Drivers[PSAM1].fd, send, 5);							//发送16进制数
//	write(Drivers[PSAM1].fd, len_buf, 2);				

	ioctl(Drivers[PSAM1].fd, RECEIVE_LEN, receive_len);			//获取返回信息长度
	
	read(Drivers[PSAM1].fd, receive_buf, receive_len[0]);		//获取返回信息
	
	printf("\nrandom num is:");
	for(i=0; i<receive_len[0]; i++)
	{
		printf("0x%x ",receive_buf[i]); 	
	}
	printf("\n");
}

int main()
{	
	set_gpio('G',9,1);
	open_driver(PSAM1,O_RDWR);							//打开PSAM小卡设备

	if(Drivers[PSAM1].fd==-1)
	{
		printf("open device error\n");
		return;
	}

	coldreset();
	random_num();

	close(Drivers[PSAM1].fd);
}


