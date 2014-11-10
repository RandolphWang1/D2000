#include     <stdio.h>		/*标准输入输出定义 */
#include     <stdlib.h>		/*标准函数库定义 */
#include     <unistd.h>		/*Unix 标准函数定义 */
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h>		/*文件控制定义 */
#include     <termios.h>	/*PPSIX 终端控制定义 */
#include     <errno.h>		/*错误号定义 */
#include	"App.h"

//#define FALSE  -1
//#define TRUE   0


int speed_arr[] ={ B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300 };
int name_arr[] = { 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300 };

/*******************************************************************************************
  函数名称	:	set_speed
  函数功能	:	设置速度
  入口参数	:
fd:设备文件指针
speed:速度参数
出口参数	:	无
函数返回	:	无
其它说明	:	无
 *******************************************************************************************/
void set_speed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;
	tcgetattr(fd, &Opt);
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) 
	{
		if (speed == name_arr[i]) 
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0) 
			{
				perror("tcsetattr fd1");
				return;
			}
			tcflush(fd, TCIOFLUSH);
		}

	}
}


/*******************************************************************************************
  函数名称	:	read_datas_tty
  函数功能	:	
  入口参数	:	
fd:文件指针
 *rcv_buf:缓存
sec:等待秒数
usec:等待微秒数
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
int read_datas_tty(int fd, char *rcv_buf, int sec, int usec)
{
	int retval;
	fd_set rfds;
	struct timeval tv;
	int ret, pos;
	tv.tv_sec = sec;		//set the rcv wait time
	tv.tv_usec = usec;		//100000us = 0.1s
	pos = 0;
	while (1) 
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		retval = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1) 
		{
			perror("select()");
			break;
		} 
		else if (retval) 
		{
			ret = read(fd, rcv_buf + pos, 8192);
			pos += ret;
			if (rcv_buf[pos - 2] == '\r' && rcv_buf[pos - 2] == '\n') 
			{
				FD_ZERO(&rfds);
				FD_SET(fd, &rfds);
				retval = select(fd + 1, &rfds, NULL, NULL, &tv);
				if (!retval)
				      break;
			}
		} 
		else 
		      break;
	}
	return 1;
}


/*******************************************************************************************
  函数名称	:	set_Parity
  函数功能	:	
  入口参数	:	
fd:
datebits:
stopbits:
parity:
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
int set_Parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(fd, &options) != 0) 
	{
		perror("SetupSerial 1");
		return (FALSE);
	}
	options.c_cflag &= ~CSIZE;
	switch (databits) 
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr, "Unsupported data size\n");
			return (FALSE);
	}
	switch (parity) 
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	/* Enable parity checking */
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;	/* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
		case 'S':
		case 's':			/*as no parity */
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported parity\n");
			return (FALSE);
	}
	switch (stopbits) 
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported stop bits\n");
			return (FALSE);
	}
	/* Set input parity option */
	if (parity != 'n')
	      options.c_iflag |= INPCK;
	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 50;
	options.c_cc[VMIN] = 0;	/* Update the options and do it NOW */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(ICRNL | IXON);
	options.c_oflag &= ~(ICRNL | IXON);
	options.c_oflag &= ~OPOST;
	if (tcsetattr(fd, TCSANOW, &options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}

int gprs_test_reset(int fd,int language)
{
	char *string=NULL;
	char *ptr=NULL;
	char buffer[1000]={0};
	int i = 0;
	struct timeval begintime, endtime;

	if(language == 0)
		pos_menu3=pos_chnedit;
	else if(language == 1)
		pos_menu3=pos_ukedit;

	text_out(20,25,pos_menu3[17]);
	//    text_out(20,25,"检测SIM卡...");
	ptr = (char *)malloc(1024*sizeof(char));
	string = ptr;
	memset(string,0,1024);
	gettimeofday(&endtime, NULL );//读取系统时间
	while(1)
	{	tcflush(fd,TCIFLUSH);
		write(fd,"AT\r",strlen("AT\r"));
		read_datas_tty(fd,string,0,200000);
		printf("string=%s\n",string);
		if(strstr(string,"OK")!=NULL)
		{
			lcd_clear();
			text_out(18,25,pos_menu3[18]);
			break;
		}
		if((endtime.tv_sec-begintime.tv_sec) > 2)
		{
			i = 3;
			break;
		}
	}
	if(i==3){ lcd_clear(); text_out(18,25,pos_menu3[19]);}
	lcd_clear();
	text_out(20,25,pos_menu3[20]);
	//    memset(string,0,1024);
	tcflush(fd,TCIFLUSH);
	write(fd,"AT+CPIN?\r",strlen("AT+CPIN?\r"));
	for(i=0;i<3;i++)
	{
		read_datas_tty(fd,string,0,200000);
		printf("string=%s\n",string);
		if(strstr(string,"READY")!=NULL)
		{
			tcflush(fd,TCIFLUSH);
			write(fd,"AT+QIDNSIP=1\r",strlen("AT+QIDNSIP=1\r"));
			for(i=0;i<3;i++)
			{
				read_datas_tty(fd,string,0,200000);
				printf("string=%s\n",string);
				if(strstr(string,"OK")!=NULL)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[33] );
					break;
				}
			}
			break;
		}
	}
	if(i==3){ lcd_clear();text_out(18,25,pos_menu3[34] );sleep(1);return 0;}
	else
	{
		lcd_clear();
		text_out(18,25,pos_menu3[21]);
		while(1)
		{
			tcflush(fd,TCIFLUSH);
			write(fd,"AT+CREG?\r",strlen("AT+CREG?\r"));
			memset(string,0,1024);
			read_datas_tty(fd,string,0,200000);
			printf("string=%s\n",string);
			if(strstr(string,"CREG")!=NULL)
			{
				string=strrchr(string,',')+1;
				if(atoi(string)==1)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[22]);
					break;
				}
				else if(atoi(string)==0)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[23]);
					sleep(1);
					break;
				}
				else if(atoi(string)==2)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[23]);
				}
			}
		}
		lcd_clear();
		text_out(18,25,pos_menu3[24]);
		while(1)
		{
			tcflush(fd,TCIFLUSH);
			write(fd,"AT+CGREG?\r",strlen("AT+CGREG?\r"));

			memset(string,0,1024);
			read_datas_tty(fd,string,0,200000);
			printf("string=%s\n",string);
			if(strstr(string,"CGREG")!=NULL)
			{
				string=strrchr(string,',')+1;
				if(atoi(string)==1)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[25] );
					break;
				}
				else if(atoi(string)==0)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[26] );
					sleep(2);
					break;
				}
				else if(atoi(string)==2)
				{
					tcflush(fd,TCIFLUSH);
					write(fd,"AT+CGREG?\r",strlen("AT+CGREG?\r"));
					text_out(18,25,pos_menu3[26]);
				}
			}
		}
		if(atoi(string)==1)
		{
			lcd_clear(); 
			text_out(18,25,pos_menu3[27]);
			tcflush(fd,TCIFLUSH);
			write(fd,"AT+qiopen=\"TCP\",\"www.baidu.com\",\"80\"\r",strlen("AT+qiopen=\"TCP\",\"www.baidu.com\",\"80\"\r"));
			for(i=0;i<3;i++)
			{
				read_datas_tty(fd,string,0,200000);
				printf("string=%s\n",string);
				if(strstr(string,"OK")!=NULL||strstr(string,"ALREADY CONNECT"))break;
			}
			if(i==3){ lcd_clear(); text_out(18,25,pos_menu3[28]);sleep(2);return 0;}
			tcflush(fd,TCIFLUSH);
			write(fd,"AT+qistat\r",strlen("AT+qistat\r"));
			for(i=0;i<10;i++)
			{
				read_datas_tty(fd,string,0,200000);
				printf("string=%s\n",string);
				if(strstr(string,"CONNECT OK")!=NULL)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[29]);
					sleep(1);
					break;
				}
				else if(strstr(string,"IP IND")!=NULL)
				{
					lcd_clear();
					text_out(18,25,pos_menu3[30]);
					sleep(1);
				}
				else 
				{
					lcd_clear();
					text_out(18,25, pos_menu3[31]);
					sleep(2);
					break;
				}
			}
		}
	}
	return i;
}



int gprs1_test(int language)
{
	int fd;
	int i;
	char buf[500] = {0};
	fd = open("/dev/ttyS3", O_RDWR );         
	set_speed(fd,115200);
	set_Parity (fd, 8,1,'n');
	i=gprs_test_reset(fd,language);
	close(fd); 
	return i;
}


