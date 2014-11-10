#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/msg.h>
#include <sys/types.h>

int dev_sms_fd,dev_sms_mask;
char center_phone[12]="13800270500";
char recive_phone[12]="13396062537";

struct Message{
	int which;
	char size_info[16];
	char info[128];
}gprs_msg[]={
	{0,"AT+CMGS=25\r","0A6E295EA662A58B66FF01"},    
	{1,"AT+CMGS=25\r","0A6E7F5EA662A58B66FF01"},
	{2,"AT+CMGS=25\r","0A514971675F3A5EA6FF01"},
	{3,"AT+CMGS=29\r","0E4E0D660E4EBA726995EF5165FF01"}
};

void serial_init(int fd)
{
	struct termios gprs_options;
	tcgetattr(fd,&gprs_options);
	gprs_options.c_cflag  |= (CLOCAL|CREAD);
	//input modle flag:ignore modle  enable receiver
	gprs_options.c_cflag &= ~CSIZE;
	gprs_options.c_cflag &= ~CRTSCTS;
	gprs_options.c_cflag |= CS8;       //8位数据位
	gprs_options.c_cflag &= ~CSTOPB;   //1位停止位 
	gprs_options.c_iflag |= IGNPAR;    //忽略校验错误 
	gprs_options.c_oflag = 0;           //无输出模式
	gprs_options.c_lflag = 0;           //本地模式禁用
	cfsetispeed(&gprs_options,B9600);
	cfsetospeed(&gprs_options,B9600);
	tcsetattr(fd,TCSANOW,&gprs_options);
	printf("serial_init is success!\n");
	return;
}

void send_message_tar(int fd,int which)
{
	char send_buf[128];
	char temp;
	int nread,nwrite;
	char recv_buf[128];
	int i=0;
//AT
	memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"AT\r");
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("1_nwrite= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("1_nread= %d,read= %s\n",nread,recv_buf);
//AT+CMGF=0
	memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"AT+CMGF=");
	strcat(send_buf,"0");
	strcat(send_buf,"\r");
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("2_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("2_nread= %d,read= %s\n",nread,recv_buf);

//AT+CMGS=25
    memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,gprs_msg[which].size_info);
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("3_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("3_nread= %d,read= %s\n",nread,recv_buf);

//send msg
    memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"80");
	strcat(send_buf,"19");
	strcat(send_buf,"86");
	strcat(send_buf,center_phone);
	strcat(send_buf,"F");
	strcat(send_buf,"1100D019");
	strcat(send_buf,"86");
	strcat(send_buf,recive_phone);
	strcat(send_buf,"F");
	strcat(send_buf,"008000");

	for(i;i<strlen(send_buf);i=i+2)
	{
		temp=send_buf[i];
		send_buf[i]=send_buf[i+1];
		send_buf[i+1]=temp;
	}
	strcat(send_buf,gprs_msg[which].info);
	strcat(send_buf,"\x1a");
	nwrite=write(fd,send_buf,strlen(send_buf));
	printf("4_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("4_nread= %d,read= %s\n",nread,recv_buf);
	if(strncmp(recv_buf,"ERROR",5)==0)
		exit(-1);
	printf("send message success!\n");
	
	return;
}
/*
void send_message(int fd,int which)
{
	int nread,nwrite;
	char send_buf[128];
	char recv_buf[128];
	char cmgs[16]={'\0'};
	strcat(cmgs,recive_phone);
//AT
	memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"AT\r");
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("1_nwrite= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("1_nread= %d,read= %s\n",nread,recv_buf);
//AT+CMGF=1
	memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"AT+CMGF=");
	strcat(send_buf,"1");
	strcat(send_buf,"\r");
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("2_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("2_nread= %d,read= %s\n",nread,recv_buf);

//AT+CMGS=18271809537
    memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,"AT+CMGS=");
	strcat(send_buf,cmgs);
	strcat(send_buf,"\r");
	nwrite = write(fd,send_buf,strlen(send_buf));
	printf("3_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("3_nread= %d,read= %s\n",nread,recv_buf);
//send msg
    memset(send_buf,0,sizeof(send_buf));
	strcpy(send_buf,gprs_msg[which].info);
	strcat(send_buf,"\x1a");
	nwrite=write(fd,send_buf,strlen(send_buf));
	printf("4_write= %d,write= %s\n",nwrite,send_buf);
	memset(recv_buf,0,sizeof(recv_buf));
	sleep(1);
	nread = read(fd,recv_buf,sizeof(recv_buf));
	printf("4_nread= %d,read= %s\n",nread,recv_buf);
	printf("send message success!\n");

	return;
}
*/

//void *pthread_sms(void *arg)
int main(int argc, const char *argv[])
{
	if((dev_sms_fd=open("/dev/ttyS3",O_RDWR|O_NOCTTY|O_NDELAY))<0)
	{
		perror("fail to open DEV_GPRS");
		exit(-1);
	}
	printf("open DEV_GPRS success\n");
	serial_init(dev_sms_fd);
	dev_sms_mask=1;
/*	while(1){
		pthread_mutex_lock(&mutex_sms);
		pthread_cond_wait(&cond_sms,&mutex_sms);
		printf("pthread_sms is wake up!dev_sms_mask=%c\n",dev_sms_mask);
		pthread_mutex_unlock(&mutex_sms);
*/
		if(1==dev_sms_mask)       //temperature      
			send_message_tar(dev_sms_fd,0);
/*		if(SMS_BRE==dev_sms_mask)       //unknown person
			send_message(dev_sms_fd,1);
		if(SMS_HUM==dev_sms_mask)       //humidity
			send_message(dev_sms_fd,2);
		if(SMS_ILL==dev_sms_mask)       //illumination
			send_message(dev_sms_fd,3);
*/
//	}
	return 0;
}
