#include "../App.h"

int fd_pwm;
FILE *fp;
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

int main()
{
	set_gpio(PWM_PORT,PWM_PORT_NUM,LOW);//配置PWM端口

	open_driver(PWM,O_RDWR);			//打开PWM设备
	
    fp = fopen("/App/3s.wav", "r");		//打开音频文件
    fseek(fp,0,SEEK_END);				//定位到文件末  
    nFileLen = ftell(fp);				//获取文件长度 
    fclose(fp);							//关闭文件

	set_pwm(0,4,225,80);				//设置PWM（工作模式，分频数，周期，占空比）

    p = &pwm;

    ioctl(Drivers[PWM].fd, 1, p);						//初始化PWM端口
    fp = fopen("/App/3s.wav", "r");
    nFileLen = nFileLen/1024;
    for(i=0;i<nFileLen;i++)
    {
        fread(read_byte, 1024, 1, fp);					//读取文件1024数据
        write(Drivers[PWM].fd,read_byte,1024);			//将数据写入PWM
    }

    ioctl(Drivers[PWM].fd, 2, p);						//关闭PWM通道

	set_gpio(PWM_PORT,PWM_PORT_NUM,HIGH);//配置PWM端口


	close(Drivers[PWM].fd);								
}

