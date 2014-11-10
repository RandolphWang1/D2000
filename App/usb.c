#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h> 
#include <signal.h>
#include <malloc.h>

/*
struct gpio_config 
{
	int port;
	int num;
	int data;
};
*/

/*******************************************************************************************
函数名称	:	InitializeGpio
函数功能	:	打开GPIO设备文件
入口参数	:	无
出口参数	:	无
函数返回	:	设备文件指针
其它说明	:	无
*******************************************************************************************/
/*
int InitializeGpio(void)
{
	int fd;
	fd = open("/dev/sep4020_gpio",O_RDWR);
	if(fd == -1)
	{
		exit(-1);
	}
	return fd;
}
*/

/*******************************************************************************************
函数名称	:	UninitializeGpio
函数功能	:	关闭GPIO设备
入口参数	:	无
出口参数	:	无
函数返回	:	无
其它说明	:	无
*******************************************************************************************/
/*
void UninitializeGpio (int fd)
{
    close(fd);
}
*/

/*******************************************************************************************
函数名称	:	SeetUsbGpioSta
函数功能	:	设置GPIO参数
入口参数	:	
	status:状态值
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/
/*
int SetUsbGpioSta(int status)  
{
    int fd;
    struct gpio_config  config;

    fd = InitializeGpio ();
    if (fd == -1)
    {
        return -1;
    }
    config.port = 0x43; 															//端口号请用大写的字母
    config.num = 5;
    config.data = status; 
    write(fd,&config, sizeof(struct gpio_config)); 														//写入状态值
    UninitializeGpio(fd);
}
*/
