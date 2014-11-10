#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "App.h"


int fd_pwm;
const char *read_name1 = "/App/3s.wav";
FILE *fp_r;

struct pwm_config
{
	unsigned long  pwm_ctrl;
	unsigned long  pwm_div;
	unsigned long  pwm_period;
	unsigned long  pwm_data;
}pwm,*p;


unsigned int nFileLen;
unsigned char read_byte[1024];
int i=0;

set_pwm(unsigned long ctrl,unsigned long div, unsigned long	period,	unsigned long data)
{
	pwm.pwm_ctrl = ctrl;            //设置PWM工作于PWM模式
    pwm.pwm_div = div;             //设置PWM为4分频
    pwm.pwm_period = period;        //PWM周期
    pwm.pwm_data = data;            //PWM占空比
}

/*******************************************************************************************
函数名称	:	pwm_test
函数功能	:	pwm测试
入口参数	:	无
出口参数	:	无
函数返回	:	无
其它说明	:	无
*******************************************************************************************/
void pwm_test(int fd_key,int *buf)
{
	set_gpio(PWM_PORT,PWM_PORT_NUM,LOW);

    fd_pwm = open_driver(PWM,O_RDWR);

	
	
    fp_r = fopen(read_name1, "r");
    fseek(fp_r,0,SEEK_END);       //定位到文件末  
    nFileLen = ftell(fp_r);       //文件长度 
    fclose(fp_r);

	set_pwm(0,4,225,80);
	
    p = &pwm;

    ioctl(fd_pwm, 1, p);
    fp_r = fopen(read_name1, "r");
    nFileLen = nFileLen/1024;
    for(i=0;i<nFileLen;i++)
    {
		fread(read_byte, 1024, 1, fp_r);
        write(fd_pwm,read_byte,1024);

        buf[0]=0;buf[1]=99;
        read(fd_key,(char *)buf, 2);
        if(buf[1]==5) 
        {
            buf[1]=99;
            break;
        }
    }
    ioctl(fd_pwm, 2, p);
	set_gpio(PWM_PORT,PWM_PORT_NUM,HIGH);
	close(fd_pwm);
}
