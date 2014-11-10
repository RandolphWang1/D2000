#include "../App.h"

int buf[2]={0};

int main()
{
	open_driver(KEY,O_RDONLY | O_NONBLOCK);						//打开键盘设备

	set_gpio(PWM_PORT,PWM_PORT_NUM,LOW);						//配置G口打开PWM放音

	while(1)
	{
		memset(buf,0,2);
		read(Drivers[KEY].fd, (char *) buf, 2);					//读取按键键值
		if(buf[0]==1)
		{
            ioctl(Drivers[KEY].fd,0,0);							//按键音

			printf("your key num is=%d\n\n",buf[1]);
			if(buf[1]==KEY_CANCEL)break;
		}
	}
	close(Drivers[KEY].fd);										//关闭键盘设备
	set_gpio(PWM_PORT,PWM_PORT_NUM,HIGH);						//配置G口关闭PWM放音
	return;
}
