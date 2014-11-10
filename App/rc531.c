#include <stdio.h>
#include <fcntl.h>

#define MODE_INITAUTHENTICA 0x05

#define MODE_GETNUM 0x0b

#define MI_OK 0x00

typedef struct
{
	unsigned char key[6];
	unsigned char mode;
	unsigned char* rtValue;
	unsigned char* buf;
	unsigned int keymode;
	unsigned char* cardid;
}key_buf;

typedef struct 
{
	unsigned int show;		//对应show_n显示的位置
	unsigned int count;		//对应n字符总位数
	unsigned int real;		//对应real_n和指针对应指向的位置
	unsigned char buf[100];
	unsigned int xPos;		//对应x坐标指向的位置
}rf_buf;

unsigned char buf2[16];
unsigned char buffer[16];
unsigned char rtValue2;
unsigned int j;
unsigned long tmp;
unsigned char mifare_buf[20];
unsigned int timeout = 10;
static int io_flag = 1;
int buf[3] = { 0 };
struct timeval begintime, currenttime;
int fd_mifare;

key_buf keyBuf2;

/*******************************************************************************************
  函数名称	:	rc531_test
  函数功能	:	射频卡测试
  入口参数	:	
fd:设备文件指针
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
char *rc531_test(int fd, int key_fd)
{
	char temp[4] = {0,};
	memset(keyBuf2.key, 0xff, 6);
	unsigned char cardid[4]={0};

	keyBuf2.buf = buf2;
	keyBuf2.rtValue = &rtValue2;
	keyBuf2.cardid = cardid;
	keyBuf2.keymode = 2;

	keyBuf2.mode = MODE_GETNUM;	//设置模式为获取射频卡卡号
	gettimeofday(&begintime, NULL);	//获取开始刷卡的时间
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(key_fd, (char *) buf, 2);
		if ((buf[0] == 1) && (buf[1]== 5)) 
		{
			sprintf(mifare_buf,"%s","1");
			ioctl(key_fd,0,0);
			show_function(1);
			return mifare_buf;
		}
		memset(&keyBuf2,0,6);
		if(read(fd,&keyBuf2,6)!=-1)
		{
			memset(mifare_buf, 0, 20);
			tmp = 0;
			for (j = 0; j < 8; j++)	//将16进制卡号转换成十进制数
			{
				tmp += *((keyBuf2.buf) + 3 - j) << j * 8;
			}
			sprintf(mifare_buf, "%u", (unsigned long) tmp);
			return mifare_buf;
		} 
		else 
		{
			gettimeofday(&currenttime, NULL);	//获取当前的时间
			if ((currenttime.tv_sec - begintime.tv_sec) > timeout)	//刷卡超时
			{
				memset(mifare_buf,0,20);
				return mifare_buf;
			}
			usleep(100000);
		}
	}
}

/*******************************************************************************************
  函数名称	:	rc531_write
  函数功能	:	写卡
  入口参数	:	
fd:设备文件指针
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
int rc531_write(int fd ,int key_fd,unsigned char* cardnum)
{
	unsigned char buf2[32] = {0, };
	int ret = 1;

	memset(keyBuf2.key, 0xff, 6);
	sprintf(buf2,"%d",atoi(cardnum));

	keyBuf2.buf = buf2;
	keyBuf2.rtValue = &rtValue2;

	keyBuf2.mode = MODE_INITAUTHENTICA;
	keyBuf2.keymode = 1;

	gettimeofday(&begintime, NULL);	//获取开始刷卡的时间

	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(key_fd, (char *) buf, 2);
		if ((buf[0] == 1) && (buf[1]== 5)) 
		{
			ioctl(key_fd,0,0);
			show_function(1);
			return 1;
		}
		ret=write(fd, &keyBuf2, 6 );	//读取射频卡卡号
		if (ret== MI_OK)
		{					
			return 0;	
		}
		else if(ret==-1)
		{
			return -1;		
		}
		else 
		{
			gettimeofday(&currenttime, NULL);	//获取当前的时间
			if ((currenttime.tv_sec - begintime.tv_sec) > timeout)	//刷卡超时
			{
				return -1;
			}
			usleep(100000);
		}
	}
}

/*******************************************************************************************
  函数名称	:	rc531_read
  函数功能	:	读卡
  入口参数	:	
cardnum1:卡号
cardnum2:卡号
cardnum3:卡号
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
int rc531_read(int fd, char *cardnum1, char *cardnum2, char *cardnum3, int key_fd)
{
	key_buf keyBuf1;
	unsigned char rtValue1;
	unsigned char buf1[16] = {0,};
	unsigned char cardid[4]={0};
	int ret = -1;
	int j;
	char temp[4] = {0,};

	memset(keyBuf1.key, 0xff, 6);

	keyBuf1.buf = buf1;
	keyBuf1.rtValue = &rtValue1;
	keyBuf1.cardid = cardid;

	keyBuf1.mode = MODE_GETNUM;
	keyBuf1.keymode = 1;

	gettimeofday(&begintime, NULL);	//获取开始刷卡的时间
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(key_fd, (char *) buf, 2);
		if ((buf[0] == 1) && (buf[1]== 5)) 
		{
			ioctl(key_fd,0,0);
			show_function(1);
			return 1;
		}
		lseek (fd, 5, SEEK_SET);
		if((read(fd, &keyBuf1, 6))!=-1)	//读取射频卡卡号
		{
			sprintf(cardnum1,"%s",keyBuf1.buf);

			memset(mifare_buf, 0, 20);
			tmp=0;
			for (j = 0; j < 8; j++)	//将16进制卡号转换成十进制数
			{
				tmp += *((keyBuf1.cardid) + 3 - j) << j * 8;
			}
			sprintf(cardnum3, "%u", (unsigned long) tmp);

			if (rtValue1 == MI_OK)
			{		
				return 0;	
			}
			else
			{
				return -1;		
			}
			break;
		} 
		else 
		{
			gettimeofday(&currenttime, NULL);							//获取当前的时间
			if ((currenttime.tv_sec - begintime.tv_sec) > timeout)	//刷卡超时
			{
				return -1;
			}
			usleep(100000);
		}
	}
}

