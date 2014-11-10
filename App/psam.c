#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	  COLDRESET    0X11  
#define   RECEIVE_LEN  0X12  

int PSAM_coldreset(int fd,char *get_num)
{
	int i = 0, receive_len[1] = {0};
	char receive_buf[50] = {0}; 
	char len_buf[2] = {0};
	
	ioctl(fd, COLDRESET); 
	ioctl(fd, RECEIVE_LEN, receive_len);  

	read(fd, receive_buf, receive_len[0]); 
	
	for(i=0; i<receive_len[0]; i++)
	{
		printf("0x%x ",receive_buf[i]); 	
		get_num[i]=receive_buf[i];
	}
	
	return receive_len[0];
}

int PSAM_random(int fd,char *get_num)
{			
	int i = 0, receive_len[1] ={0};
	char receive_buf[50] = {0}; 
	char len_buf[2] = {0};
	char send[5] = {0x00,0x84,0x00,0x00,0x04};

	len_buf[0] = 10;
	len_buf[1] = 10;
	write(fd, send, 5); 
//	write(fd, len_buf, 2); 

	ioctl(fd, RECEIVE_LEN, receive_len); 
	
	read(fd, receive_buf, receive_len[0]);
	
	for(i=0; i<receive_len[0]; i++)
	{
		printf("0x%x ",receive_buf[i]); 	
		get_num[i]=receive_buf[i];
	}
	
	return receive_len[0];
}

int psam_test(char *get_num,int n,int mode)
{
	int  fd = 0,len;
	
	if(mode==1)fd = open("/dev/psam1", O_RDWR); 
	if(mode==2)fd = open("/dev/psam0", O_RDWR); //打开设备节点
	if(fd == -1)      
	{
	 	exit(-1);
	} 

	len=PSAM_coldreset(fd,get_num);
	if(n==1)len=PSAM_random(fd,get_num);
	
	close(fd);
	
	
	return len;
}

