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


struct gpio_config 
{
	int port;
	int num;
	int data;
}gpio;

void set_gpio_dir(int fd,char port,int num,int data)
{
	struct gpio_config gpio;

	gpio.data=data;
	gpio.port=port;
	gpio.num=num;
	write(fd,&gpio,sizeof(struct gpio_config));
}
/*******************************************************************************************
函数名称	:	Net_deal
函数功能	:	GPIO处理程序
入口参数	:	无
出口参数	:	无
函数返回	:	无
其它说明	:	无
*******************************************************************************************/
void Net_deal(void)
{
    int fd_gpio_dir;
	int IO_NUM;
    fd_gpio_dir = open("/dev/sep4020_gpio_dir",O_RDWR);

	
    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 0;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 1;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 2;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 3;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 4;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 5;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 6;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 7;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    gpio.data = 1;
    gpio.port = 0x49;
    gpio.num = 8;	
    write(fd_gpio_dir, &gpio, sizeof(struct gpio_config));

    close(fd_gpio_dir);
	
}

/*******************************************************************************************
函数名称	:	Sleep_deal
函数功能	:	睡眠处理程序
入口参数	:	
	n:状态参数
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/
void Sleep_deal(int n)
{
    int fd_gpio,fd_light;

    fd_gpio = open("/dev/sep4020_gpio",O_RDWR);
    Net_deal();
    if(n==0)             //关闭IO
    {  
		set_gpio_dir(fd_gpio,'C',7,0);
		set_gpio_dir(fd_gpio,'G',8,0);
		set_gpio_dir(fd_gpio,'F',7,0);
		set_gpio_dir(fd_gpio,'G',10,0);
		set_gpio_dir(fd_gpio,'B',14,0);
		set_gpio_dir(fd_gpio,'B',3,0);


		
    }
    else
    {	
        set_gpio_dir(fd_gpio,'C',7,1);
		set_gpio_dir(fd_gpio,'B',14,1);
			

		
    }
}

/*******************************************************************************************
函数名称	:	open_pw
函数功能	:	以指定方式打开设备
入口参数	:	flags:选项参数
出口参数	:	无
函数返回	:	成功:文件指针	 失败:FALSE
其它说明	:	无
*******************************************************************************************/

int open_pw(int flags)		// 以指定方式打开 设备
{
    int pw;
    pw = open("/sys/power/state",flags);
    if (pw >= 0)
    return pw;
    else
    return -1;
}

/*******************************************************************************************
函数名称	:	sleep_server
函数功能	:	进入睡眠模式
入口参数	:	无
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/

int sleep_server()     //sep4020 sleep
{
    char tmp[6] = "mem";
	int pw_fd;	
	pw_fd = open_pw(O_WRONLY);
	write(pw_fd, tmp, strlen(tmp));
	close(pw_fd);
	return 0;
}

