#include "variable.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "App.h"
#include "HanziZiku.h"
#include "sign.h"

#define FIFO "/tmp/fifo"

int success = 0;
int times = 0;

int open_driver(int driver_num,int flag)
{
	char string[20];
	char string2[20][20]={"/dev/graylcd","/dev/key","/dev/sep4020_gpio","/dev/asyncio","/dev/power",
		"/dev/mifare","/dev/tlc549","/dev/pa1100","/dev/ttyS3","/mnt/system.conf","/dev/light","/dev/sep4020_pwm","/dev/psam0","/dev/psam1","/dev/ttyS2"};
	strcpy(Drivers[driver_num].driver_name,string2[driver_num]);
	sprintf(string,"%s",string2[driver_num]);
	Drivers[driver_num].fd=open(string,flag);
	return Drivers[driver_num].fd;
}
/*******************************************************************************************
  函数名称	:	signal_handler
  函数功能	:	关机处理函数
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void signal_handler(int num)	//处理函数
{
	poweroff = 1;		//关机操作
}

/*******************************************************************************************
  函数名称	:	InitAsyncio
  函数功能	:	初始化异步io驱动
  出口参数	:	无
  函数返回	:	成功:TRUE; 失败:FALSE
  其它说明	:	无
 *******************************************************************************************/
void InitAsyncio(void)		//初始化异步io驱动
{
	int oflags;
	open_driver(ASYNCIO,O_RDWR);
	signal(SIGIO, signal_handler);	//绑定action到  SIGIO signal
	fcntl(Drivers[ASYNCIO].fd, F_SETOWN, getpid());	//设置STDIN_FILENOD的owner为当前进程
	oflags = fcntl(Drivers[ASYNCIO].fd, F_GETFL);	//获取文件打开的方式
	fcntl(Drivers[ASYNCIO].fd, F_SETFL, oflags | FASYNC);	//设置STDIN_FILENO的文件状态标志（增加了FASYNC标志）.
}


void get_time(time_t timep, struct tm *p)
{
	time(&timep);
	p=localtime(&timep);
}

int write_gprs(char *string, int mode, char *expect_str, int timeout)
{
	struct timeval begintime, endtime;
	gettimeofday(&begintime, NULL );//读取系统时间
	if(mode == LOOP){
		tcflush(Drivers[TTYS3].fd,TCIFLUSH);
		while(1){
			write(Drivers[TTYS3].fd, string, strlen(string));
			usleep(100000);

			memset(string, 0, 1024);
			read_datas_tty(Drivers[TTYS3].fd,string,0,200000);
			//			printf("string = %s\n", string);
			if(strstr(string, expect_str)!=NULL) {
				break;
			}else{
				gettimeofday(&endtime, NULL );//读取系统时间
				if((endtime.tv_sec-begintime.tv_sec) > timeout) {
					return TIMEOUT;
				}
			}
		}
	} else if(mode == NORMAL){
		tcflush(Drivers[TTYS3].fd,TCIFLUSH);
		write(Drivers[TTYS3].fd, string, strlen(string));
		while(1){
			usleep(100000);

			memset(string, 0, 1024);
			read_datas_tty(Drivers[TTYS3].fd,string,0,200000);
			//			printf("string = %s\n", string);
			if(strstr(string, expect_str)!=NULL) {
				break;
			}else{
				gettimeofday(&endtime, NULL );//读取系统时间
				if((endtime.tv_sec-begintime.tv_sec) > timeout) {
					return TIMEOUT;
				}
			}
		}
	} 
	return 0;
}

/*******************************************************************************************
  函数名称	:	set_contrast
  函数功能	:	调节对比度
  入口参数	:	level:对比度参数
  出口参数	:	无	
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void set_contrast(unsigned long level)
{
	unsigned char cmd=14;
	paint_struct rect_struct= { level,0,0,0,(unsigned long) NULL };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);
}	     
/*******************************************************************************************
  函数名称	:	set_pixel
  函数功能	:	配置像素
  入口参数	:
x:屏幕横坐标
y:屏幕纵坐标
mode:屏幕显示模式(0:清除点	1:画点	2:反显点)
出口参数	:	无
函数返回	:	无
其它说明	:	无
 *******************************************************************************************/
void set_pixel(int x, int y, unsigned char mode)
{
	unsigned char cmd = SET_PIXEL + (mode << 4);
	paint_struct pt_struct = { x, y, 0, 0, (unsigned long) NULL };
	ioctl(Drivers[GRAYLCD].fd, cmd, &pt_struct);
}

/*******************************************************************************************
  函数名称	:	draw_rect
  函数功能	:	显示图片
  入口参数	:	xpos:屏幕起始横坐标 ypos:屏幕起始纵坐标
width:图片宽度 height:图片高度
pstr[]:数据
出口参数	:	无	
函数返回	:	无
其它说明	:	无
 *******************************************************************************************/
void draw_ellipse(unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=DRAW_ELLIPSE+(mode << 4);
	paint_struct rect_struct= { xpos, ypos, width, height, (unsigned char)mode };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);
}

void draw_circle(unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=DRAW_CIRCLE+(mode << 4);
	paint_struct rect_struct= { xpos, ypos, width, height, (unsigned char)mode };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);
}
void draw_rect(unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=DRAW_RECT+(mode << 4);
	paint_struct rect_struct= { xpos, ypos, width, height, (unsigned char)mode };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);
}	      

void draw_lcd(unsigned int arg , unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=arg+(mode << 4);
	paint_struct rect_struct= { xpos, ypos, width, height, (unsigned char)mode };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);
}	      

/*******************************************************************************************
  函数名称	:	fill_rect
  函数功能	:	
  入口参数	:     
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void fill_rect(unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=FILL_RECT+(mode << 4);
	paint_struct rect_struct= { xpos, ypos, width, height, (unsigned char)mode };
	ioctl(Drivers[GRAYLCD].fd, cmd, &rect_struct);

}

void lcdfill(unsigned long xpos, unsigned long ypos,
			unsigned long width, unsigned long height,unsigned char mode)
{
	unsigned char cmd=LCDFILL+(mode << 4);
	paint_struct rect_struct= { 0,0,0,0,0 };
	ioctl(Drivers[GRAYLCD].fd, cmd,&rect_struct);
}

/*******************************************************************************************
  函数名称	:	lcd_clear
  函数功能	:	清屏
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void lcd_clear()
{
	ioctl(Drivers[GRAYLCD].fd, CLEAR_LCD);
}

/*******************************************************************************************
  函数名称	:	LCD_OFF
  函数功能	:	关闭LCD设备
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void LCD_OFF()
{
	ioctl(Drivers[GRAYLCD].fd, LCD_CLOSE);
	close(Drivers[Light].fd);
}

void LcdUnzipZK(unsigned char *FontUnzip, unsigned long offset,unsigned char num)
{
	unsigned char i, total, oneline, zkchar;
	unsigned char *ptr;
	ptr = FontUnzip;
	total = 3 * num;
	oneline = 2 * num;
	for (i = 0; i < total; i++)
	{
		if (num == 3)
		      zkchar = ASCIIKu[offset];
		else
		      zkchar = HanziKu[offset];
		offset++;
		if (i < oneline)
		{
			*ptr = zkchar;
			ptr++;
		}
		else
		{
			*ptr = zkchar & 0x0f;
			ptr++;
			zkchar >>= 4;
			*ptr = zkchar & 0x0f;
			ptr++;
		}
	}
	return;
}

unsigned char LcdTextChinese(unsigned char xPos, unsigned char yPos, unsigned short ChineseCode)
{
	unsigned char msb, lsb, qh, wh, achar, Hanzk_state;
	unsigned long offset, Hanzk_visual_addr;
	unsigned char font[100];

	paint_struct pt_struct = { xPos, yPos, 12, 12, (unsigned long) font };
	lsb = ChineseCode & 0x00ff;
	msb = ChineseCode >> 8;
	if (msb > 128)
	{
		if (lsb > 128) 
		{
			Hanzk_state = 1;
			qh = msb - 0xa1;
			wh = lsb - 0xa1;
			offset = 94 * qh + wh;
			offset = offset * 18;
			qh++;
			if (qh == 1 || qh == 2 || qh == 3)
			      Hanzk_visual_addr = Hanzk_addr;
			if (qh == 4 || qh == 5)
			      Hanzk_state = 0;
			if (qh == 6)
			      Hanzk_visual_addr = Hanzk_sec6_offset;
			if (qh == 7 || qh == 8)
			      Hanzk_state = 0;
			if (qh == 9)
			      Hanzk_visual_addr = Hanzk_sec9_offset;
			if (qh > 9 && qh < 16)
			      Hanzk_state = 0;
			if (qh >= 16)
			      Hanzk_visual_addr = Hanzk_han_offset;
			if (Hanzk_state == 0)
			      offset = Hanzk_space;
			if (Hanzk_state == 1)
			      offset = offset + Hanzk_visual_addr;
			LcdUnzipZK((unsigned char *) pt_struct.ptr_data, offset, 6);	//查找中文字符点阵
			ioctl(Drivers[GRAYLCD].fd, GRAPHIC_FONT, &pt_struct);
		}
		xPos = xPos + 13;
	}
	else			// GBcode中相应字符并非为中文字符
	{
		achar = 0;		// GBCode have error
	}
	return achar;
}

unsigned char LcdTextChar(unsigned char xPos, unsigned char yPos, unsigned char charcode)
{
	unsigned char temp;
	unsigned long offset;
	unsigned char font[54];
	paint_struct pt_struct = { xPos, yPos, 6, 12, (unsigned long) font };
	unsigned char i;		/*0 - 31的字符码和字符码127是不可打印的 */

	temp = charcode - 32;
	offset = 9 * temp + ASII_addr;
	font[0] = xPos;
	font[1] = yPos;
	font[2] = 6;
	font[3] = 12;
	LcdUnzipZK((unsigned char *) pt_struct.ptr_data, offset, 3);
	ioctl(Drivers[GRAYLCD].fd, GRAPHIC_FONT, &pt_struct);
	return 1;
}

void paint_rect(unsigned char *pdata, int x, int y, int xlen, int ylen)
{
	paint_struct pt_struct;
	pt_struct.arg1 = x;
	pt_struct.arg2 = y;
	pt_struct.arg3 = xlen;
	pt_struct.arg4 = ylen;
	pt_struct.ptr_data = (unsigned long) pdata;
	ioctl(Drivers[GRAYLCD].fd, PAINT_RECT, &pt_struct);
}


/*******************************************************************************************
  函数名称	:	text_string
  函数功能	:	显示符号点阵字符
  入口参数	:	
xPos:屏幕起始横坐标
yPos:屏幕起始纵坐标
GBCodePtr:许显示的字符串
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
int text_string(int xs,int ys,char *string,int length) //start(x,y)/string/length
{
	int i;
	for(i=0;i<length;i++)
	      paint_rect(ascDot+5*(string[i]-0x20),xs+i*5,ys,5,8);
}

/*******************************************************************************************
  函数名称	:	text_out
  函数功能	:	显示12＊12/12＊6点阵字符
  入口参数	:	
xPos:屏幕起始横坐标
yPos:屏幕起始纵坐标
GBCodePtr:许显示的字符串
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
 *******************************************************************************************/
unsigned char text_out(unsigned char xPos, unsigned char yPos, unsigned char *GBCodeptr)
{
	unsigned char achar = 1, x, y;
	unsigned short ttt;
	unsigned char countw = 0;	//用于计算输出下一个字符后屏幕是否会越界（起临时判断作用）
	unsigned char i = 12, j = 6;
	int buf[3] = { 0 };
	x = xPos;			//countw=xPos+1;
	y = yPos;			// 屏幕起点坐标为（1，0）
	countw = xPos;		// 屏幕起点坐标为（0，0）
	while ((*GBCodeptr) != 0) 
	{
		if (countw > 131) 
		{
			y = y + 16;
			x = 5;
			countw = 0;
		}
		if (*GBCodeptr > 128)	// display Chinese
		{
			if (*(GBCodeptr + 1) > 128)	//if (countw > 128) break;       // 屏幕起点坐标为（1，0）
			{
				if ((128 - countw) >= 12)	//if (countw > 131) break;         // 屏幕最后一个字符后面不用计算空格（最后一次因加i - 1，故可取到屏宽＋1
				{
					ttt = (*GBCodeptr << 8) + *(GBCodeptr + 1);
					if (x == 0)	//printf("%x\n",*GBCodeptr);
					x = 0x01;	//printf("%x\n",*(GBCodeptr+1));
					LcdTextChinese(x, y, ttt);
					x = x + i;
					GBCodeptr += 2;
				}
				countw = countw + i;
			} 
			else
			{
				achar = 0;	//GBCode have error
				return achar;
			}
		} 
		else			//display letter
		{
			if ((128 - countw) >= 6)	//if (countw > 128) break; // 屏幕起点坐标为（1，0）
			{
				LcdTextChar(x, y, *GBCodeptr);
				x = x + j;
				GBCodeptr++;
			}
			countw = countw + j;
		}
	}				//end of while
	return achar;
}

/******************************tcp_connection******************************/
void tcp_connection()
{
	int buf[3] = { 0 };
	char string[1024];
	char *string2;
	int i;
	int success = 0;
	int times = 0;
	struct timeval begintime, endtime;
	int len,flag=0;
	int fd_temp;
	int connecting_times = 0;


	while(1){
		buf[0] = 0; buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if(buf[0] == 1 && buf[1] == KEY_CANCEL){
			return ;
		}

		gettimeofday(&begintime, NULL );//读取系统时间
		strcpy(string, "AT+CSQ\r");
		state = write_gprs(string, NORMAL, "CSQ:", 1);
		if(state != TIMEOUT){
			string2=strrchr(string,':')+2;
			*(strchr(string2,','))='\0';
			fill_rect(60, 0, 20, 16, 0);
			text_out(60, 0, string2);
		} 
		strcpy(string, "AT+QINDI=1\r");
		state = write_gprs(string, NORMAL, "OK", 1);
		times++;
		strcpy(string, "AT+QISTAT\r");
		state = write_gprs(string, NORMAL, "OK", 1);
		if(strstr(string, "IP INITIAL")||strstr(string, "IP CLOSE")){
			//			strcpy(string, "AT+QIOPEN=\"TCP\",\"117.89.52.228\",10000\r");
			strcpy(string, "AT+QIOPEN=\"TCP\",\"119.75.217.56\",80\r");
			gettimeofday(&begintime, NULL );//读取系统时间 
			state = write_gprs(string, NORMAL, "OK", 1);
			if(strstr(string, "OK")&&strstr(string, "CONNECT FAIL")==NULL){
				printf("OPEN OK\n");
				while(1){
					buf[0] = 0; buf[1] = 99;
					read(Drivers[KEY].fd, (char *) buf, 2);
					if(buf[0] == 1 && buf[1] == KEY_CANCEL){
						return ;
					}

					strcpy(string, "AT+QISTAT\r");
					state = write_gprs(string, NORMAL, "STATE:", 30);
					if(strstr(string, "CONNECT OK")){
#if 0
						strcpy(string, "AT+QISEND\r");
						write_gprs(string, NORMAL, ">",1);
						strcpy(string, "prochip 1\r");
						state = write_gprs(string, NORMAL, "e", 30);
						if(state !=TIMEOUT){
							success++;
							gettimeofday(&endtime, NULL );//读取系统时间 
							sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
							fill_rect(100, 16, 20, 16, 0);
							text_out(100, 16, string);
							flag = 2;
							printf("communication finished\n");
							break;
						}else{
							gettimeofday(&endtime, NULL );//读取系统时间 
							sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
							fill_rect(100, 16, 20, 16, 0);
							text_out(100, 16, string);
							flag = 1;
							break;
						}
#endif
						success++;
						connecting_times = 0;
						gettimeofday(&endtime, NULL );//读取系统时间 
						sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
						fill_rect(100, 16, 20, 16, 0);
						text_out(100, 16, string);

						flag = 0;
						break;
					}else if(strstr(string, "PDP FAIL")){
						flag = 1; 
						break;
					}else if(state == TIMEOUT||strstr(string, "TCP CONNECTING")||strstr(string, "IP IND")){
						connecting_times++;
						if(connecting_times == 2)("/App/poweroff");
						else{
							gettimeofday(&endtime, NULL );//读取系统时间 
							if(endtime.tv_sec-begintime.tv_sec > 30){
								sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
								fill_rect(100, 16, 20, 16, 0);
								text_out(100, 16, string);
								flag = 1;
								break;
							}
						}
					}
				}
			} else{
				printf("OPEN ERROR\n");
				fill_rect(60, 16, 30, 64, 0);
				sprintf(string, "%d", times);
				text_out(60, 16, string);
				sprintf(string, "%d", success);
				text_out(60, 32, string);


				strcpy(string, "AT+QIDEACT\r");
				state = write_gprs(string, LOOP, "DEACT OK", 30);
				if(state!=0)times--;
				//write(Drivers[TTYS3].fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
			}

			if(flag == 0){
				strcpy(string, "AT+QICLOSE\r");
				state = write_gprs(string, LOOP, "CLOSE OK", 2);
				if(state!=0)times--;
				//write(Drivers[TTYS3].fd,"AT+QIDEACT\r",strlen("AT+QIDEACT"));
				//write(Drivers[TTYS3].fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
			}else if(flag == 1){
				strcpy(string, "AT+QIDEACT\r");
				state = write_gprs(string, LOOP, "DEACT OK", 2);
				if(state!=0)times--;

				//write(Drivers[TTYS3].fd,"AT+QIDEACT\r",strlen("AT+QIDEACT"));
				//write(Drivers[TTYS3].fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
			}else if(flag == 2){
				//			close(Drivers[TTYS3].fd);
				break;
			}
		}else if(strstr(string, "CONNECT OK")){
			strcpy(string, "AT+QICLOSE\r");
			state = write_gprs(string, LOOP, "CLOSE OK", 2);
			if(state!=0)times--;
		}else{
			strcpy(string, "AT+QIDEACT\r");
			state = write_gprs(string, LOOP, "DEACT OK", 2);
			if(state!=0)times--;
			//			write(Drivers[TTYS3].fd, "AT+CIPSHUT\r", strlen("AT+CIPSHUT\r"));
		}
		fill_rect(60, 16, 30, 64, 0);
		sprintf(string, "%d", times);
		text_out(60, 16, string);
		sprintf(string, "%d", success);
		text_out(60, 32, string);


	}
}
/*******************************************************************************************
  函数名称	:	show_page
  函数功能	:	显示菜单
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void show_page(int page_num)
{
	int i;
	lcd_clear();
	if(page_num==1)
	{
		for(i=0;i<4;i++)
		{
			text_out(0,i*16,pos_menu1[i]);
		}
		fill_rect(0,0,132,12,2);
		page=1;
	}
	else if(page_num==2)
	{
		for(i=0;i<4;i++)
		{
			text_out(0,i*16,pos_menu1[4+i]);
		}
		fill_rect(0,0,132,12,2);
		page=2;
	}
}

/*******************************************************************************************
  函数名称	:	show_function
  函数功能	:	菜单显示程序
  入口参数	:	fill:选择变量
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void show_function(int n)
{
	int i;
	lcd_clear();
	for(i=0;i<4;i++)
	{
		text_out(0,i*16,pos_menu2[(n-1)*4+i]);
	}
	fill_rect(0,0,132,12,2);
}


/*******************************************************************************************
  函数名称	:	show_function
  函数功能	:	显示PSAM小卡控制菜单
  入口参数	:	n:选项参数
fill:选择变量
出口参数	:	无
函数返回	:	无
其它说明	:	无
 *******************************************************************************************/
void show_psam(int a,int fill,int n)
{
	int i;
	lcd_clear();
	if (a == 1) 
	{
		for(i=0;i<4;i++)
		{
			text_out(0,i*16,pos_menu2[(n-1)*4+i]);
		}
	}
	else
	{
		n++;
		for(i=0;i<4;i++)
		{
			text_out(0,i*16,pos_menu2[(n-1)*4+i]);
		}
	}
//	text_out(0,48," 3.退出");
	fill_rect(0,0,132,12,2);
}

/*******************************************************************************************
  函数名称	:   key_function	
  函数功能	:	数字功能键
  入口参数	:	
 *buf:缓冲数组
 出口参数	:	无
 函数返回	:	无
 其它说明	:	无
 *******************************************************************************************/
void key_function(int *buf,int *key_num,int flag)
{
	if(flag==1)
	{
		if(buf[1]==KEY_NUM1) { fill_mark=(*key_num-1)*4+1; }
		else if(buf[1]==KEY_NUM2) { fill_mark=(*key_num-1)*4+2; }
		else if(buf[1]==KEY_NUM3) { fill_mark=(*key_num-1)*4+3; }
		else if(buf[1]==KEY_NUM4) { fill_mark=(*key_num-1)*4+4; }
		else	return;
		buf[1]=1;
	}
	else
	{
		if(buf[1]==KEY_ENTER) { *key_num=0; }
		else if(buf[1]==KEY_NUM7) { *key_num=7; }
		else if(buf[1]==KEY_NUM8) { *key_num=8; }
		else if(buf[1]==KEY_NUM9) { *key_num=9; }
		else if(buf[1]==KEY_NUM1) { *key_num=1; }
		else if(buf[1]==KEY_NUM2) { *key_num=2; }
		else if(buf[1]==KEY_NUM3) { *key_num=3; }
		else if(buf[1]==KEY_NUM4) { *key_num=4; }
		else if(buf[1]==KEY_NUM5) { *key_num=5; }
		else if(buf[1]==KEY_NUM6) { *key_num=6; }
        else if(buf[1]==KEY_NUM0) { *key_num=0; }
		else return;
		buf[1]=1;
	}
}

void set_gpio(char port,int num,int data)
{
	struct gpio_config gpio;
	open_driver(GPIO,O_RDWR);

	gpio.port = port;
	gpio.num = num;
	gpio.data = data;

	tcflush(Drivers[GPIO].fd, TCIOFLUSH);
	write(Drivers[GPIO].fd,&gpio,sizeof(struct gpio_config));

	close(Drivers[GPIO].fd);
	return ;
}


/*******************************************************************************************
  函数名称	:	rc531_powerdown
  函数功能	:	射频卡服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/

void rc531_powerdown()
{
	open_driver(RC531,O_RDWR);
	if (Drivers[RC531].fd < 0) 
	{
		text_out(0,0,pos_menu3[2]);
		close(Drivers[RC531].fd);
		sleep(2);
		lcd_clear();
		return;
	}
	ioctl(Drivers[RC531].fd,1);
}

void  rc531_signal()
{
	/***************************************/
	fill_rect(103,35,3,3,1);    //center
	/***************************************/

	fill_rect(98,34,2,5,1); //first circle
	set_pixel(99,33,1);     //first point
	set_pixel(99,32,1);
	set_pixel(100,32,1);
	set_pixel(100,31,1);

	set_pixel(108,31,1);
	set_pixel(109,33,1);     
	set_pixel(108,32,1);
	set_pixel(109,32,1);

	set_pixel(100,41,1);     
	set_pixel(99,40,1);
	set_pixel(100,40,1);
	set_pixel(99,39,1);

	set_pixel(108,41,1);     
	set_pixel(108,40,1);
	set_pixel(109,40,1);
	set_pixel(109,39,1);

	fill_rect(109,34,2,5,1);
	/***************************************/
	fill_rect(91,32,2,9,1);//second circle
	fill_rect(116,32,2,9,1);

	fill_rect(92,30,2,2,1);
	set_pixel(93,29,1);

	fill_rect(115,30,2,2,1);
	set_pixel(115,29,1);

	fill_rect(115,41,2,2,1);
	set_pixel(115,43,1);

	fill_rect(92,41,2,2,1);
	set_pixel(93,43,1);
	/***************************************/
	fill_rect(85,32,2,9,1);//third circle
	fill_rect(122,32,2,9,1);

	fill_rect(86,29,2,3,1);
	fill_rect(86,41,2,3,1);
	fill_rect(121,29,2,3,1);
	fill_rect(121,41,2,3,1);

	set_pixel(87,28,1);    
	set_pixel(87,27,1);
	set_pixel(88,27,1);
	set_pixel(88,26,1);

	set_pixel(121,28,1);    
	set_pixel(120,27,1);
	set_pixel(121,27,1);
	set_pixel(120,26,1);

	set_pixel(120,46,1);    
	set_pixel(120,45,1);
	set_pixel(121,45,1);
	set_pixel(121,44,1);

	set_pixel(88,46,1);    
	set_pixel(87,45,1);
	set_pixel(88,45,1);
	set_pixel(87,44,1);
	/***************************************/

}
/*******************************************************************************************
  函数名称	:	rc531_server
  函数功能	:	射频卡服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void rc531_server(void)
{
	int buf[3] = { 0 };
	char cardnum1[30] = { 0, };
	char cardnum2[30] = { 0, };
	char cardnum3[30] = { 0, };
	char cardnum[16] = { 0, };
	char *rc531_get;
	char string[1024];

	time_t timep;
	struct tm *p = localtime(&timep);

	int temp=0,fd_fifo;
	fill_mark_signal = 2;
	show_function(1);
	page=1;

	fd_fifo = open(FIFO, O_WRONLY|O_NONBLOCK, 0);
	open_driver(RC531,O_RDWR);
	ioctl(Drivers[RC531].fd,0);
	if (Drivers[RC531].fd < 0) 
	{
		error();
	}
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if (state == 1) 
			{
				show_function(1);
				state = 0;
			}
			if(buf[1]==KEY_UP&&page==2)
			{
				lcd_clear();
				page=1;
				show_function(1);
			}
			if(buf[1]==KEY_DOWN&&page==1)
			{
				lcd_clear();
				page=2;
				text_out(0,0,pos_menu3[3]);
				text_out(0,16,pos_menu3[4]);
				text_out(0,32,pos_menu3[5]);
				fill_rect(0,0,132,12,2);
			}

			if (buf[1]==KEY_ENTER)	//ENTER
			{
				switch (fill_mark_signal)
				{
					case 1:
						{
							lcd_clear();
							text_out(0, 0, pos_open[0]);
							fill_rect(0,0,132,12,2);
							rc531_signal();
							while(1)
							{
								rc531_get = (char *) rc531_test(Drivers[RC531].fd, Drivers[KEY].fd);
								if(rc531_get!=NULL&&atoi(rc531_get)==1) break;
								if (strlen(rc531_get) != 0) 
								{
									ioctl(Drivers[KEY].fd,0,0);
									fill_rect(80,22,50,29,0);
									sprintf(cardnum1," %s%s",pos_menu3[10],rc531_get);
									fill_rect(35,30,132,12,0);
									text_out(0,30,cardnum1);
								}
								else
								{
									ioctl(Drivers[KEY].fd,0,0);
									fill_rect(0,16,132,64,0);
									text_out(5, 30, pos_open[1]);
									break;
								}
							}
							while(1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if(atoi(rc531_get)==1)break;
								if (buf[1]==KEY_CANCEL) //退出键
								{
									ioctl(Drivers[KEY].fd,0,0);
									show_function(1);
									buf[1]=99;
									break;
								}
							}
							break;
						}
					case 2:
						{
							lcd_clear();
							text_out(0, 0, pos_open[0]);
							fill_rect(0,0,132,12,2);
							rc531_signal();
							while(1)
							{
								temp = rc531_read(Drivers[RC531].fd,cardnum1,cardnum2,cardnum3,Drivers[KEY].fd);
								if(temp==0)
								{
									ioctl(Drivers[KEY].fd,0,0);
									fill_rect(80,22,50,29,0);
									sprintf(cardnum2," %s%s",pos_menu3[10], cardnum3);
									fill_rect(35,30,132,12,0);
									text_out(0,30,cardnum2);
									temp = write_card(Drivers[KEY].fd);
									if(temp==-1)
									{
										ioctl(Drivers[KEY].fd,0,0);
										fill_rect(0,16,132,64,0);
										text_out(5, 30, pos_open[1]);
										break;
									} else if(temp==1)
									      break;
								} else {
									if(temp==-1)
									{
										ioctl(Drivers[KEY].fd,0,0);
										fill_rect(0,16,132,64,0);
										text_out(5, 30, pos_open[1]);
										break;
									} else if(temp==1)
									      break;
								}
							}
							while(1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if(temp==1)break;
								if (buf[1]==KEY_CANCEL) //退出键
								{
									ioctl(Drivers[KEY].fd,0,0);
									show_function(1);
									buf[1]=99;
									break;
								}
							}
							break;
						}

					case 3:
						{
							lcd_clear();
							text_out(0, 0, pos_open[0]);
							fill_rect(0,0,132,12,2);
							rc531_signal();

							while(1)
							{
								temp = rc531_read(Drivers[RC531].fd,cardnum1,cardnum2,cardnum3,Drivers[KEY].fd);
								if(temp==0)
								{
									sprintf(cardnum2,"%d",atoi(cardnum1)+50);
									sprintf(cardnum1," %s:%d",pos_menu3[38],atoi(cardnum2));

									/*******************************server**************************/
									get_time(timep, p);
									sprintf(string, "%02d.%02d.%02d;+50;%d;", p->tm_hour, p->tm_min, p->tm_sec, atoi(cardnum2));
									write(fd_fifo, string, 20);

									temp=rc531_write(Drivers[RC531].fd,Drivers[KEY].fd,cardnum2);
									fill_rect(35,16,132,12,0);
									fill_rect(35,48,132,12,0);
									fill_rect(80,22,50,29,0);
									sprintf(cardnum2," %s%s",pos_menu3[10],cardnum3);
									text_out(0,16,cardnum2);
									sprintf(cardnum2," %s:%s",pos_menu3[39],"50");
									text_out(0,32,cardnum2);
									ioctl(Drivers[KEY].fd,0,0);
									text_out(0,48,cardnum1);
								}
								else 
								{
									if(temp==-1)
									{
										ioctl(Drivers[KEY].fd,0,0);
										fill_rect(0,16,132,64,0);
										text_out(5, 30, pos_open[1]);
										break;
									}
									else if(temp==1)break;
								}
							}
							while(1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if(temp==1)break;
								if (buf[1]==KEY_CANCEL) //退出键
								{
									ioctl(Drivers[KEY].fd,0,0);
									show_function(1);
									buf[1]=99;
									break;
								}
							}
							break;
						}
					case 4:
						{
							lcd_clear();
							text_out(0, 0, pos_open[0]);
							fill_rect(0,0,132,12,2);
							rc531_signal();

							while(1)
							{
								temp = rc531_read(Drivers[RC531].fd,cardnum1,cardnum2,cardnum3,Drivers[KEY].fd);
								if(temp==0)
								{
									sprintf(cardnum2,"%d",atoi(cardnum1)-10);
									sprintf(cardnum1," %s:%d",pos_menu3[38],atoi(cardnum2));

									/*********************server**********************/						
									get_time(timep, p);
									sprintf(string, "%02d.%02d.%02d;-10;%d;", p->tm_hour, p->tm_min, p->tm_sec, atoi(cardnum2));
									write(fd_fifo, string, 20);

									temp=rc531_write(Drivers[RC531].fd,Drivers[KEY].fd,cardnum2);
									fill_rect(35,16,132,12,0);
									fill_rect(35,48,132,12,0);
									fill_rect(80,22,50,29,0);
									sprintf(cardnum2," %s%s",pos_menu3[10],cardnum3);
									text_out(0,16,cardnum2);
									sprintf(cardnum2," %s:%s",pos_menu3[40],"10");
									text_out(0,32,cardnum2);
									text_out(0,48,cardnum1);
									ioctl(Drivers[KEY].fd,0,0);
								}
								else 
								{
									if(temp==-1)
									{
										ioctl(Drivers[KEY].fd,0,0);
										fill_rect(0,16,132,64,0);
										text_out(5, 30, pos_open[1]);
										break;
									}
									else if(temp==1)
									{
										lcd_clear();
										page=2;
										text_out(0,0,pos_menu3[3]);
										text_out(0,16,pos_menu3[4]);
										text_out(0,32,pos_menu3[5]);
										fill_rect(0,0,132,12,2);
										break;
									}
								}
								if(temp==2)break;
							}
							while(1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if(temp==1)break;
								if (buf[1]==KEY_CANCEL) //退出键
								{
									lcd_clear();
									page=2;
									buf[1]=99;
									ioctl(Drivers[KEY].fd,0,0);
									text_out(0,0,pos_menu3[3]);
									text_out(0,16,pos_menu3[4]);
									text_out(0,32,pos_menu3[5]);
									fill_rect(0,0,132,12,2);
									break;
								}
							}
							break;
						}
					case 5:
						{
							show_page(PAGE_TWO);
							ioctl(Drivers[RC531].fd,1);
							page=2;
							close(Drivers[RC531].fd);
							return;
						}
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) //退出键
			{
				show_page(PAGE_TWO);
				ioctl(Drivers[RC531].fd,1);
				close(Drivers[RC531].fd);
				break;
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:	gprs_server
  函数功能	:	gprs服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/

char center_phone[12]="13800270500";
char str[12]="0";
int flag=1;

struct Message{
	int which;
	char size_info[16];
	char info[128];
}gprs_msg[]={
	{0,"AT+CMGS=25\r","0A96F670B979D16280FF01"},    
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
	strcat(send_buf,str);
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
	{
		flag=0;
        printf("send message fail\n");
		exit(-1);
	}
	return;
}
void gprs_server(void)
{
	int buf[3] = { 0 };
	int gprs_set;
	int len,step=0,flag=0;
	int fd_temp;
	int count=0;
	char *string;
	char num[2];
	fill_mark_signal = 1;//修改
	show_function(2);

	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;

		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);

			if (buf[1]==KEY_ENTER) 
			{
				switch (fill_mark_signal) 
				{
					case 1:
                      //  system("./gprs");
   
                       memset(str,0,sizeof(str));
					//	serial_init(Drivers[TTYS3].fd);
						lcd_clear();
						text_out(3,0,"请输入接收方号码：");
						draw_rect(16,30,105,13,1);
						int n=1;
						while(n==1)
						{
							buf[0]=0;
							buf[1]=99;
							read(Drivers[KEY].fd,(char *)buf,2);
							if(buf[0]==1)
							{
								ioctl(Drivers[KEY].fd,0,0);
								count++;
								if(count>11)
									break;
								switch(buf[1])
								{
								case 19:
									strcat(str,"1");
									text_out(17+8*count,32,"1");
									break;

								case 17:
									strcat(str,"2");
									text_out(17+8*count,32,"2");
									break;

								case 16:
									strcat(str,"3");
									text_out(17+8*count,32,"3");
									break;
								case 14:
									strcat(str,"4");
									text_out(17+8*count,32,"4");
									break;
								case 13:
									strcat(str,"5");
									text_out(17+8*count,32,"5");
									break;
								case 11:
									strcat(str,"6");
									text_out(17+8*count,32,"6");
									break;
								case 8:
									strcat(str,"7");
									text_out(17+8*count,32,"7");
									break;
								case 9:
									strcat(str,"8");
									text_out(17+8*count,32,"8");
									break;
								case 7:
									strcat(str,"9");
									text_out(17+8*count,32,"9");
									break;
								case 3:
									strcat(str,"0");
									text_out(17+8*count,32,"0");
									break;
								case KEY_CANCEL:
									n=0;
									show_function(2);
									break;
								}
							}
						}
						if(n==1)
						{
							strcat(str,"\0");
							printf("num=%s\n",str);


							lcd_clear();
							text_out(20,10,"短信发送中...");
							send_message_tar(Drivers[TTYS3].fd,0);
						}	
						if(flag==0&&n==1){
								
                            count=0;
                        lcd_clear();
							text_out(0,0,"短信发送失败！");
						}else if(n==1&&flag==1){
								
                            count=0;
                        lcd_clear();
							text_out(0,0,"短信发送成功！");
						}
						
						show_function(2);
						buf[1]=0;
						break;
                    case 2:
                        system("./gprs_test");
                     /*   
                        memset(str,0,sizeof(str));
						serial_init(Drivers[TTYS3].fd);
						lcd_clear();
                        strcat(str,"atd");
						text_out(0,0,"请输入对方号码：");
						draw_rect(16,30,105,13,1);
						int i=1;
						while(i==1)
						{
							buf[0]=0;
							buf[1]=99;
							read(Drivers[KEY].fd,(char *)buf,2);
							if(buf[0]==1)
							{
								ioctl(Drivers[KEY].fd,0,0);
								count++;
								if(count>11)
									break;
								switch(buf[1])
								{
								case 19:
									strcat(str,"1");
									text_out(17+8*count,32,"1");
									break;

								case 17:
									strcat(str,"2");
									text_out(17+8*count,32,"2");
									break;

								case 16:
									strcat(str,"3");
									text_out(17+8*count,32,"3");
									break;
								case 14:
									strcat(str,"4");
									text_out(17+8*count,32,"4");
									break;
								case 13:
									strcat(str,"5");
									text_out(17+8*count,32,"5");
									break;
								case 11:
									strcat(str,"6");
									text_out(17+8*count,32,"6");
									break;
								case 8:
									strcat(str,"7");
									text_out(17+8*count,32,"7");
									break;
								case 9:
									strcat(str,"8");
									text_out(17+8*count,32,"8");
									break;
								case 7:
									strcat(str,"9");
									text_out(17+8*count,32,"9");
									break;
								case 3:
									strcat(str,"0");
									text_out(17+8*count,32,"0");
									break;
								case KEY_CANCEL:
									i=0;
									show_function(2);
									break;
								}
							}
						}
						if(i==1)
						{
                            strcat(str,";");
							printf("num=%s\n",str);
                           
                            write(Drivers[KEY].fd,str,strlen(str));

                         //   system("echo str >/dev/ttyS3");
							lcd_clear();
                            count=0;

							text_out(20,10,"电话拨打中...");
                        }*/
/*					case 2:
						if(step==3)break;
						show_page(PAGE_ONE);
						//			system("/etc/ppp/ppp-off");
*/						return;
                        
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) 
			{
				if(step==3)
				{
					step=0;
					lcd_clear();
					show_function(2);
				}
				else
				{
					//		system("/etc/ppp/ppp-off");
					show_page(PAGE_ONE);
					return;
				}
			}
		}
	}
}



/*******************************************************************************************
  函数名称	:	pwm_server
  函数功能	:	pwm服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void pwm_server(void)		//pwm 语音放音
{
	int buf[3] = { 0 };
	fill_mark_signal = 2;
	show_function(3);

	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);

		if (buf[0] == 1)
		{
			key_function(buf,&fill_mark_signal,0);
			ioctl(Drivers[KEY].fd,0,0);
			if (state == 1)
			{
				show_function(3);
				state = 0;
			}
			if (buf[1]==KEY_ENTER) 
			{
				switch (fill_mark_signal) 
				{
					case 1:
						pwm_test(Drivers[KEY].fd,buf);
						break;
					case 2:
						show_page(PAGE_ONE);
						return;
						break;
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL)
			{
				show_page(PAGE_ONE);
				break;
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:	mgcard_server
  函数功能	:	磁条卡测试
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void mgcard_server(void)	//磁条卡测试
{
	int buf[3] = { 0 },back_mark = -1, i = 0, j = 0;
	//char buffer[10000] = {0};
	char *buffer;
	fill_mark_signal = 2;
	show_function(6);

	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1) 
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,HIGH);
			if (state == 1)									//进入屏保状态下 重绘界面
			{
				show_function(6);
				state = 0;
			}
			if (buf[1]==KEY_ENTER)
			{
				switch (fill_mark_signal)
				{
					case 1:
						lcd_clear();
						text_out(0, 0, pos_open[20+0]);
						fill_rect(0,0,132,12,2);
						open_driver(MGCARD,O_RDONLY);
						back_mark = mg_test(Drivers[MGCARD].fd,Drivers[KEY].fd);
						if(back_mark == 2)break;
						if (back_mark == 0)
						{
							ioctl(Drivers[KEY].fd,0,0);
							buffer=&str4[0];
							i=strlen(buffer);
							if(i<=0)text_out(15,30,pos_open[21+1]);
							else
							{
								text_out(0,16,pos_menu3[10]);
								if(strlen(buffer)<=17) text_out(15, 32, buffer);
								else 
								{
									text_out(20,48,buffer+17);
									*(buffer+17)='\0';
									text_out(20,32,buffer);
								}
							}
						}
						if (back_mark == 1)
						{
							text_out(15, 30, pos_open[20+2]);
						}
						if(back_mark==3)
						{
							text_out(15,30,pos_menu3[11]);
						}
						while (1)
						{
							buf[0] = 0;
							buf[1] = 99;
							read(Drivers[KEY].fd, (char *) buf, 2);
							if ((buf[0] == 1) && (buf[1]==KEY_CANCEL))
							{
								ioctl(Drivers[KEY].fd,0,0);
								show_function(6);
								buf[1] = 99;
								close(Drivers[MGCARD].fd);
								break;
							}
						}
						break;
					case 2:
						show_page(PAGE_ONE);
						return;
						break;
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) 
			{
				set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,LOW);				//磁条卡低功耗
				show_page(PAGE_ONE);
				break;
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:	TLC549_server
  函数功能	:	电量检测程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void TLC549_server(void)	//电量检测
{
	int i,j;
	int buf[3] = { 0 };
	char buff[1] = { 0 };
	char string[5] = { 0 };
	fill_mark_signal = 2;
	show_function(4);
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1) 
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if (state == 1) 
			{
				show_function(4);
				state = 0;
			}

			if (buf[1]==KEY_ENTER) 
			{
				switch (fill_mark_signal) 
				{
					case 1:
						lcd_clear();
						text_out(0, 0,pos_open[23+0]);
						while (1) 
						{
							buf[0] = 0;
							buf[1] = 99;
							read(Drivers[TLC549].fd, buff, 1);
							sprintf(string, "%d", buff[0]);
							fill_rect(59,27,20,10,0);
							text_out(59, 27, string);
							for(i=0;i<400;i++)
							{
								for(j=0;j<300;j++)
								{
									read(Drivers[KEY].fd, (char *) buf, 2);
									if ((buf[0] == 1) && (buf[1]==KEY_CANCEL)) 
									{
										ioctl(Drivers[KEY].fd,0,0);
										i=400;
										show_function(4);
										buf[1] = 100;
										break;
									}
								}
							}
							if(buf[1]==100)break;
						}
						break;
					case 2:
						show_page(PAGE_ONE);
						return;
						break;
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) 
			{
				show_page(PAGE_ONE);
				break;
			}
		}
	}
}

int psam_show()
{
	int buf[3] = { 0 };
	fill_mark_signal = 5;
	lcd_clear();
	text_out(0,0,pos_menu1[18]);
	text_out(0,16,pos_menu1[8]);
	text_out(0,32,pos_menu1[9]);
	text_out(0,48,pos_menu1[19]);
	fill_rect(0,0,132,12,2);
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)	//检测到有按键操作；
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if(buf[1]==KEY_CANCEL) 
			{
				show_page(PAGE_ONE);
				set_gpio('G',9,0);
				return 3;
			}
			if(fill_mark_signal==1||fill_mark_signal==2||fill_mark_signal==3)return fill_mark_signal;
		}
	}
}


/*******************************************************************************************
  函数名称	:	psam_server
  函数功能	:	psam卡服务程序
  入口参数	:	
psam_select:psam选项
出口参数	:	无
函数返回	:	无
其它说明	:	无
 *******************************************************************************************/
void psam_server()	//psam test
{
	int psam_select;
	int buf[3] = { 0 },count_reset = 0, j = 0,len;
	char buff[1] = { 0 }, get_num[50] = {0};
	char string[5] = { 0 };
	char psam_buf[50];
	fill_mark_signal = 4;
	set_gpio('G',9,1);
	while(1)
	{
		while(1)
		{
			psam_select=psam_show();
			if (psam_select == 1)
			{
				fd_psam = open("/dev/psam1", O_RDWR);	//打开小卡设备节点
				if ((fd_psam == -1) || (no_small_psam == 1)) 
				{
					lcd_clear();
					text_out(0, 0, pos_open[25+0]);
					sleep(2);
					close(fd_psam);
				}
				else break;
			}
			if (psam_select == 2) 
			{
				fd_psam = open("/dev/psam0", O_RDWR);	//打开大卡设备节点
				if ((fd_psam == -1) || (no_big_psam == 1)) 
				{
					lcd_clear();
					text_out(0, 0, pos_open[25+1]);
					sleep(2);
					close(fd_psam);
				}
				else break;
			}
			if(psam_select == 3)
			{
				show_page(PAGE_ONE);
				set_gpio('G',9,0);
				return;
			}
		}
		show_psam(psam_select, fill_mark_signal,7);
		while (1) 
		{
			buf[0] = 0;
			buf[1] = 99;
			read(Drivers[KEY].fd, (char *) buf, 2);
			if (buf[0] == 1) 
			{
				ioctl(Drivers[KEY].fd,0,0);
				key_function(buf,&fill_mark_signal,0);
				if (state == 1) 
				{
					show_psam(psam_select, fill_mark_signal,7);
					state = 0;
				}
				if (buf[1]==KEY_ENTER)
				{
					switch (fill_mark_signal) 
					{
						case 1:
							close(fd_psam);
							if (psam_select == 2)
							      fd_psam = open("/dev/psam0", O_RDWR);
							if (psam_select == 1)
							      fd_psam = open("/dev/psam1", O_RDWR);
							if(fd_psam==-1)
							{
								lcd_clear();
								text_out(0, 0, pos_open[25+1]);
								sleep(2);
								lcd_clear();
								show_psam(psam_select, fill_mark_signal,7);
								break;
							}

							lcd_clear();
							text_out(0, 0, pos_open[25+2]);
							bzero(get_num, 50);
							count_reset++;
							len=psam_test(get_num,0,psam_select);
							for (j = 0; j < len; j++) 
							{
								bzero(psam_buf, 2);
								sprintf(psam_buf, "%02x", get_num[j]);
								text_out(8 + (j % 6) * 16, 16 + (j / 6) * 16,
											psam_buf);
							}
							while (1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if ((buf[0] == 1) && (buf[1]==KEY_CANCEL)) 
								{
									ioctl(Drivers[KEY].fd,0,0);
									lcd_clear();
									show_psam(psam_select, fill_mark_signal,7);
									buf[1] = 99;
									break;
								}
							}
							break;
						case 2:
							close(fd_psam);
							if (psam_select == 2)
							      fd_psam = open_driver(PSAM0,O_RDWR);
							if (psam_select == 1)
							      fd_psam = open_driver(PSAM1,O_RDWR);;
							if(fd_psam==-1)
							{
								lcd_clear();
								text_out(0, 0, pos_open[25+1]);
								sleep(2);
								lcd_clear();
								show_psam(psam_select, fill_mark_signal,7);
								break;
							}
							lcd_clear();
							text_out(0, 0, pos_open[25+3]);
							if (count_reset == 0)
							      text_out(0, 16, pos_open[25+4]);
							else
							{
								bzero(get_num, 50);
								//                                psam_small_test(fd_psam, 2, get_num);
								len=psam_test(get_num,1,psam_select);
								for (j = 0; j < len; j++)
								{
									bzero(psam_buf, 2);
									sprintf(psam_buf, "%02x", get_num[j]);
									text_out(8 + (j % 6) * 16, 16 + (j / 6) * 16, psam_buf);
								}
							}
							while (1)
							{
								buf[0] = 0;
								buf[1] = 99;
								read(Drivers[KEY].fd, (char *) buf, 2);
								if ((buf[0] == 1) && (buf[1]==KEY_CANCEL)) 
								{
									ioctl(Drivers[KEY].fd,0,0);
									show_psam(psam_select, fill_mark_signal,7);
									buf[1] = 99;
									break;
								}
							}
							break;
						case 3:
							close(fd_psam);
							break;
						default:
							break;
					}
					if(fill_mark_signal==3)break;
				}
				if (buf[1]==KEY_CANCEL) 
				{
					close(fd_psam);	
					break;
				}
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:	usb_server
  函数功能	:	usb服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void usb_server(void)		//USB test
{
	int buf[3] = { 0 };
	fill_mark_signal = 2;
	show_function(9);
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if (state == 1)
			{
				show_function(9);
				state = 0;
			}
			if (buf[1]==KEY_ENTER) 
			{
				switch (fill_mark_signal) 
				{
					case 1:
						lcd_clear();		
						text_out(0,0,pos_open[30+0]);
						sleep(2);
						set_gpio(USB_PORT,USB_PORT_NUM,HIGH);
						show_function(9);
						break;
					case 2:
						set_gpio(USB_PORT,USB_PORT_NUM,LOW);
						lcd_clear();
						text_out(0,0,pos_open[30+1]);
						sleep(2);
						set_gpio(USB_PORT,USB_PORT_NUM,LOW);
						show_function(9);
						break;
					case 3:
						show_function(10);
						return;
						break;
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL)
			{
				show_function(10);
				break;
			}
		}			//end while(1)
	}
}
/*******************************************************************************************
  函数名称	:	update_server
  函数功能	:	升级程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void update_server()
{
	int buf[3] = { 0 }; 
	show_function(10);
	fill_mark_signal = 1;
	while (1)
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if (buf[1]==KEY_ENTER)    //ENTER
			{
				switch (fill_mark_signal) 
				{
					case 1:
						sdcard_server();
						show_function(10);
						break;
					case 2:
						buf[1]=5;
						break;
					case 3:
						buf[1]=5;
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) //退出键
			{
				show_page(PAGE_TWO);
				break;
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:	language_set
  函数功能	:	语言设置程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void language_set()
{
	int buf[3] = { 0 };
	int signal = 2;
	show_function(11);
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&signal,0);
			if (state == 1)
			{
				show_function(11);
				state = 0;
			}
			if (buf[1]==KEY_ENTER)
			{
				switch (signal) 
				{
					case 1:
						p.symbol[LANGUAGE]=0;
						pos_open= pos_chnopen;
						pos_menu1=pos_chnmenu1;
						pos_menu2=pos_chnmenu2;
						pos_menu3=pos_chnedit;
						lcd_clear();		
						text_out(0,0,pos_open[32+0]);
						sleep(1);
						show_function(11);
						break;
					case 2:
						p.symbol[LANGUAGE]=1;
						pos_open= pos_ukopen;
						pos_menu1=pos_ukmenu1;
						pos_menu2=pos_ukmenu2;
						pos_menu3=pos_ukedit;
						lcd_clear();
						text_out(0,0," language changed");
						sleep(1);
						show_function(11);
						break;
					default:
						break;
				}
			}
			if (buf[1]==KEY_CANCEL) 
			{
				open_driver(SYSCONF,O_RDWR);
				lseek(Drivers[SYSCONF].fd,p.size[2],SEEK_SET);
				sprintf((char *)buf,"%d",p.symbol[LANGUAGE]);
				write(Drivers[SYSCONF].fd,buf,1);
				close(Drivers[SYSCONF].fd);

				show_function(12);
				break;
			}
		}			//end while(1)
	}
}

/*******************************************************************************************
  函数名称	:	printer_server
  函数功能	:	打印机服务程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void printer_server(void)	//打印机测试
{
	int buf[2] = { 0 };
	fill_mark_signal = 2;
	show_function(5);
	int zz=0;
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if (buf[0] == 1)	//检测到有按键操作；
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&fill_mark_signal,0);
			if(buf[1]==KEY_CANCEL) 
			{
				show_page(PAGE_ONE);
				return;
			}
			if (buf[1]==KEY_ENTER)	//按键键值＝12；
			{
				switch (fill_mark_signal) 
				{
					case 1:
                        printf("print test\n");
						if(printer_test(p.symbol[LANGUAGE])==0)break;
						else
						{
							show_function(5);
							break;
						}
					case 2:
						show_page(PAGE_ONE);
						return;
						break;
				}
			}				
		}
	}//end while (1) 
}


/*******************************************************************************************
  函数名称	:   security_server	
  函数功能	:	系统设置程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void security_server()
{
	int buf[3] = { 0 };
	int num[7];
	int fd,size,count=0;
	char string[50];
	if(open("/mnt/text.txt",O_RDONLY,0666)==-1)return;
	lcd_clear();
	text_out(0,0,pos_open[33+0]);
	draw_rect(16,30,100,12,1);	
	while (1)
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if(buf[0]==1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			text_out(34+11*count,32,"*");
			key_function(buf,&num[count],0);
			count++;
			if(count==6)
			{
				int i;
				for(i=0;i<6;i++)
				{
                    printf("%d\n",num[i]);
					sprintf(string,"echo %d >> /mnt/secret",num[i]);
					system(string);
				}
				count=0;
				system("diff /mnt/secret /mnt/text.txt >>  /mnt/judge");
				fd = open("/mnt/judge",O_RDONLY,0666 );
				size=read( fd, string, 10);
				close(fd);
				system("rm /mnt/judge /mnt/secret");
				if(size==0)break;
				else
				{
					lcd_clear();
					text_out(0,0,pos_open[33+1]);
					sleep(2);
					lcd_clear();
					text_out(0,0,pos_open[33+2]);
					draw_rect(16,30,100,12,1);	
				}
			}
		}
	}
}

/*******************************************************************************************
  函数名称	:   security_change	
  函数功能	:   密码修改程序	
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void security_change()
{
	int buf[3] = { 0 };
	char num[7];
	int fd,size,count=0;
	char string[50];
	int state=0,state2=0;
	lcd_clear();
	if(fd=open("/mnt/text.txt",O_RDONLY,0666)==-1)
	{
		text_out(0,0,pos_open[33+3]);
		state=1;
	}
	else
	      text_out(0,0,pos_open[33+4]);
	draw_rect(16,30,100,12,1);	
	while (1) {
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		if(buf[0]==1)
		{
			ioctl(Drivers[KEY].fd,0,0);
			if(buf[1]==KEY_CANCEL)
			{
				lcd_clear();
				show_function(12);
				break;
			}
			text_out(34+11*count,32,"*");
			count++;
			switch(buf[1])
			{ 
				case 19:num[count-1]=1;break;
				case 17:num[count-1]=2;break;
				case 16:num[count-1]=3;break;
				case 14:num[count-1]=4;break;
				case 13:num[count-1]=5;break;
				case 11:num[count-1]=6;break;
				case  8:num[count-1]=7;break;
				case  9:num[count-1]=8;break;
				case  7:num[count-1]=9;break;
				case  3:num[count-1]=0;break;
				case  1:num[count-1]=23;break;
			}
			if(count==6||num[0]==23)
			{
				int i;
				for(i=0;i<6;i++)
				{
					if(state==2){ sprintf(string,"echo %d >> /mnt/tmp2.txt",num[i]); if(i==5)state=3; }
					else sprintf(string,"echo %d >> /mnt/tmp.txt",num[i]);
					system(string);
				}
				if(state==1)
				{
					if(num[0]==23)
					{
						system("rm -rf /mnt/text.txt /mnt/judge /mnt/tmp.txt /mnt/tmp2.txt");
						break;
					}
					lcd_clear();
					text_out(0,0,pos_menu3[12]);
					draw_rect(16,30,100,12,1);	
					// set_timer(tick);
					state=2;
				}
				count=0;
				if(state==0||state==3)
				{
					if(state==3) system("diff /mnt/tmp.txt /mnt/tmp2.txt >>  /mnt/judge");
					if(state==0) {	system("diff /mnt/tmp.txt /mnt/text.txt >> /mnt/judge");state2=1;}
					fd = open("/mnt/judge",O_RDONLY,0666 );
					size=read( fd, string, 10);
					close(fd);
					if(size==0)
					{
						if(state==0)
						{
							lcd_clear();
							text_out(0,0,pos_menu3[13]);
							system("rm -rf /mnt/tmp.txt /mnt/judge");
							draw_rect(16,30,100,12,1);	
							state=1;
							state2=1;
						}
						else
						{
							lcd_clear();
							text_out(0,0,pos_open[33+5]);
							sleep(2);
							system("mv /mnt/tmp.txt /mnt/text.txt");
							system("rm -rf /mnt/judge /mnt/tmp.txt /mnt/tmp2.txt");
							break;
						}
					}
					else
					{
						lcd_clear();
						text_out(0,0,pos_open[33+7]);
						sleep(2);
						lcd_clear();
						text_out(0,0,pos_open[33+8]);
						draw_rect(16,30,100,12,1);	
						system("rm -rf /mnt/judge /mnt/tmp.txt /mnt/tmp2.txt");
						if(state2==1)state=0;
						else	state=1;
					}
				}
			}
		}
	}
	if(num[0]==23)
	{
		lcd_clear();
		text_out(0,0,pos_menu3[14]);
		sleep(2);
	}
}
/*******************************************************************************************
  函数名称	:	configure_server
  函数功能	:	系统设置程序
  入口参数	:	无
  出口参数	:	无
  函数返回	:	无
  其它说明	:	无
 *******************************************************************************************/
void configure_server()
{
	int level=p.symbol[CONTRAST];
	int lightlv=p.symbol[LIGHT];
	int size1=p.size[0];
	int size2=p.size[1];
	int n=0;
	char buf1[20];
	int buf[3] = { 0 };
	struct timeval begintime, endtime;
	page=3;
	fill_mark_signal = 2;
	show_function(12);
	gettimeofday(&begintime, NULL );//读取系统时间 
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);

		gettimeofday(&endtime, NULL );//读取系统时间 
		if((endtime.tv_sec-begintime.tv_sec) > 7)
		{
			buf[0]=1;
			buf[1]=KEY_CANCEL;
			close(Drivers[Light].fd);
		}
		if (buf[0] == 1) 
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&n,0);
			if(buf[1]==KEY_F1)
			{
				lcd_clear();
				show_page(PAGE_ONE);
				return;
			}
			if (state == 1)
			{
				lcd_clear();
				show_function(12);
				state = 0;
			}
			if (buf[1]==KEY_ENTER) 
			{
				open_driver(SYSCONF,O_RDWR);
				switch (n) 
				{
					case 6:
						lcd_clear();
						text_out(0,0,pos_open[42+0]);
						text_out(0,16,"  0              20");
						fill_rect(0,0,132,12,2);
						draw_rect(16,30,102,10,1);	
						fill_rect(17,31,(level-10)*5,8,1);
						while (1) 
						{
							buf[0] = 0;
							buf[1] = 99;
							read(Drivers[KEY].fd, (char *) buf, 2);
							if (buf[0] == 1)
							{
								ioctl(Drivers[KEY].fd,0,0);
								if (state == 1) 
								{
									lcd_clear();
									text_out(0,0,pos_open[42+1]);
									text_out(0,16,"  0              20");
									draw_rect(16,30,102,10,1);	
									fill_rect(17,31,(level-10)*5,8,1);
									state = 0;
								}
								if (buf[1]==KEY_DOWN)
								{
									fill_rect(17,31,(level-10)*5,8,0);
									level--;
									if(level<10)level=10;
									fill_rect(17,31,(level-10)*5,8,1);
								}	
								if (buf[1]==KEY_UP)
								{
									level++;
									if(level>30)level=30;
									fill_rect(17,31,(level-10)*5,8,1);
								}		  
								if (buf[1]==KEY_CANCEL) 
								{
									show_function(12);
									close(Drivers[SYSCONF].fd);
									buf[1]=0;
									break;
								}
								if(buf[1]==KEY_ENTER)
								{
									show_function(12);
									close(Drivers[SYSCONF].fd);
									buf[1]=0;
									lcd_clear();
									text_out(15,25,pos_menu3[15]);
									sleep(2);
									show_function(12);
									break;
								}
								p.symbol[CONTRAST]=level;
								set_contrast(p.symbol[CONTRAST]);
								sprintf(buf1,"%d",p.symbol[CONTRAST]);
								lseek(Drivers[SYSCONF].fd,p.size[0],SEEK_SET);
								write(Drivers[SYSCONF].fd,buf1,2); 
							}	
						}
						break;
					case 1:
						lcd_clear();
						security_change();	
						show_function(12);
						break;
					case 3:
						lcd_clear();
						text_out(0,0,pos_open[42+2]);
						text_out(0,16,"  0              20");
						fill_rect(0,0,132,12,2);
						draw_rect(16,30,102,10,1);	
						fill_rect(17,31,(lightlv/12)*5,8,1);
						while (1) 
						{
							buf[0] = 0;
							buf[1] = 99;
							read(Drivers[KEY].fd, (char *) buf, 2);
							if (buf[0] == 1) 
							{
								ioctl(Drivers[KEY].fd,0,0);
								if (state == 1)
								{
									lcd_clear();
									text_out(0,0,pos_open[42+3]);
									text_out(0,16,"  0              20");
									draw_rect(16,30,102,10,1);	
									fill_rect(17,31,(lightlv/12)*5,8,1);
									state = 0;
								}
								if (buf[1]==KEY_DOWN)
								{
									fill_rect(17,31,(lightlv/12)*5,8,0);
									lightlv=lightlv-12;
									if(lightlv<0)lightlv=0;
									fill_rect(17,31,(lightlv/12)*5,8,1);
								}	
								if (buf[1]==KEY_UP)
								{
									lightlv=lightlv+12;
									if(lightlv>=240)lightlv=240;
									fill_rect(17,31,(lightlv/12)*5,8,1);
								}		  
								if (buf[1]==KEY_CANCEL) 
								{
									show_function(12);
									buf[1]=0;
									break;
								}
								if(buf[1]==KEY_ENTER)
								{
									show_function(12);
									close(Drivers[SYSCONF].fd);
									buf[1]=0;
									lcd_clear();
									text_out(15,25,pos_menu3[15]);
									sleep(2);
									show_function(12);
									break;
								}
								p.symbol[LIGHT]=lightlv;
								ioctl(Drivers[Light].fd,p.symbol[LIGHT],0);
								sprintf(buf1,"%d",p.symbol[LIGHT]);
								lseek(Drivers[SYSCONF].fd,p.size[1],SEEK_SET);
								write(Drivers[SYSCONF].fd,buf1,3); 
							}	
						}
						break;
					case 2:
						language_set();
						break;
					case 4:
						calendar_server(Drivers[KEY].fd,fill_mark_signal);
						break;
					case 5:
						calendar_change(p.symbol[LANGUAGE]);
						show_function(12);
						break;
					default:
						break;
				}
				n=0;
			}
			if(buf[1]==KEY_CANCEL) 
			{
				buf[1]=show_time(Drivers[KEY].fd,Drivers[TLC549].fd,Drivers[TTYS3].fd,p.symbol[LANGUAGE],buf);
				open_driver(Light,O_RDWR);
				ioctl(Drivers[Light].fd,p.symbol[LIGHT],0);

				if(buf[1]==KEY_F1){show_page(PAGE_ONE);}
				if(buf[1]==KEY_F2||buf[1]==1){configure_server();}
				break;
			}
			gettimeofday(&begintime, NULL );//读取系统时间 
		}
	}
}

void read_variable(int variable)
{
	char buf[20];
	if(Drivers[SYSCONF].fd==0)
	{
		open_driver(SYSCONF,O_RDONLY);

		read( Drivers[SYSCONF].fd, buf, 20);
		strcpy(strchr(buf,':'),"\0");
		p.size[0]=strlen(buf)+1;
		lseek(Drivers[SYSCONF].fd,p.size[0],SEEK_SET);                             //读取对比度参数

		read(Drivers[SYSCONF].fd,buf,20);
		strcpy(strchr(buf,':'),"\0");
		p.size[1]=strlen(buf)+p.size[0]+1;
		lseek(Drivers[SYSCONF].fd,p.size[1],SEEK_SET);                             //读取对比度参数

		read(Drivers[SYSCONF].fd,buf,20);                                      //读取语言设定参数
		strcpy(strchr(buf,':'),"\0");
		p.size[2]=strlen(buf)+p.size[1]+1;
	}
	memset(buf,0,20);
	lseek(Drivers[SYSCONF].fd,p.size[variable],SEEK_SET);              
	read(Drivers[SYSCONF].fd,buf,3);                        
	p.symbol[variable]=atoi(buf);
}

void configure_init()			//初始化手持机基本设备，读取系统参数（LCD，键盘，磁条卡，异步IO，IC卡，GPRS，亮度、对比度、系统语言参数）
{
	char string[1024];
	char *string2;
	int i;
	set_gpio('G',9,0);								//关闭PSAM
	set_gpio(GPRS_PORT,GPRS_PORT_NUM,HIGH);						//启动GPRS模块
	open_driver(GRAYLCD,O_RDWR);				//打开LCD设备
	open_driver(KEY,O_RDONLY | O_NONBLOCK);		//打开键盘设备
	ioctl(Drivers[KEY].fd,NULL,NULL);					//按键音
	set_gpio(MGCARD_PORT,MGCARD_PORT_NUM,LOW);						//上电默认高电平 置低降低磁条卡设备功耗 
	InitAsyncio();								//初始化异步IO
	open_driver(POWER,O_RDONLY);
	rc531_powerdown();							//rc531进入低功耗模式
	open_driver(TLC549,O_RDWR);					//打开设备TLC549


	/*********************************************************************************************/
	/*	
		text_out(20,25,"初始化GPRS...");

	//	set_gpio('C',7,1,0);						//启动GPRS模块

	sleep(6);
	lcd_clear();
	text_out(18,25,"GPRS初始化完成");
	sleep(1);
	lcd_clear();

	gprs1_test(p.symbol[LANGUAGE]);								//检测sim卡信号

	open_driver(TTYS3, O_RDWR);					//打开串口3
	*/
	//    gprs1_test();								//检测sim卡信号
	/*********************************************************************************************/

	read_variable(CONTRAST);					//读取对比度数值
	read_variable(LIGHT);						//读取亮度数值
	read_variable(LANGUAGE);					//读取系统语言参数

	if(p.symbol[LANGUAGE]==0)					//设置系统语言
	{   
		pos_open= pos_chnopen;
		pos_menu1=pos_chnmenu1;
		pos_menu2=pos_chnmenu2;
		pos_menu3=pos_chnedit;
	}
	if(p.symbol[LANGUAGE]==1)
	{
		pos_open= pos_ukopen;
		pos_menu1=pos_ukmenu1;
		pos_menu2=pos_ukmenu2;
		pos_menu3=pos_ukedit;
	}
	close(Drivers[SYSCONF].fd);
	set_contrast(p.symbol[CONTRAST]);			//配置对比度
	open_driver(Light,O_RDWR);
	ioctl(Drivers[Light].fd,p.symbol[LIGHT],0); //配置亮度

#if 0
	int fd = open("/dev/ttyS3", O_RDWR);
	set_speed(fd,115200);
	set_Parity (fd, 8,1,'n');

	struct timeval begintime, endtime;
	int len,flag=0;
	int fd_temp;

	lcd_clear();
	text_out(0, 0, "信号：");
	text_out(0, 16, "拨号次数：");
	text_out(0, 32, "成功次数：");
	gettimeofday(&begintime, NULL );//读取系统时间
	while(1){

		while(1) {	
			tcflush(fd,TCIFLUSH);
			write(fd,"AT\r",strlen("AT\r"));
			memset(string, 0, 1024);
			read_datas_tty(fd,string,0,200000);
			printf("string=%s\n",string);
			if(strstr(string,"OK")!=NULL)
			{
				//				lcd_clear();
				//				text_out(18,25,"GPRS初始化完成");
				break;
			}
			gettimeofday(&endtime, NULL );//读取系统时间
			if((endtime.tv_sec-begintime.tv_sec) > 3)
			{
				lcd_clear();
				text_out(20, 25, "GPRS初始化失败");
				close(Drivers[TTYS3].fd);
				sleep(1);
				system("/App/poweroff");
				sleep(4);
				open_driver(TTYS3, O_RDWR);
				gettimeofday(&begintime, NULL );//读取系统时间
				lcd_clear();
				text_out(0, 0, "信号：");
				text_out(0, 16, "拨号次数：");
				text_out(0, 32, "成功次数：");

			}
		}

		//	lcd_clear();
		tcflush(fd,TCIFLUSH);
		write(fd,"AT+CSQ\r",strlen("AT+CSQ\r"));
		memset(string, 0, 1024);
		read_datas_tty(fd,string,0,200000);
		printf("string = %s\n",string);
		string2=strrchr(string,':')+2;
		*(strchr(string2,','))='\0';
		fill_rect(60, 0, 20, 16, 0);
		text_out(60, 0, string2);

#if 0
		if(open("/var/run/syslogd.pid",O_RDONLY)==-1) system("/sbin/syslogd");
		if(open("/var/run/ppp0.pid",O_RDONLY)==-1);
		else
		      system("/etc/ppp/ppp-off");

		system("rm /var/log/messages");
		system("rm /tmp/txt");
		system("pppd call gprs&");
		times++;
		while(1)
		{
			system("cat /var/log/messages > /tmp/txt");
			fd_temp=open("/tmp/txt",O_RDONLY);
			lseek(fd_temp,0-1024,SEEK_END);
			read(fd_temp,string,1024);
			if(strstr(string,"PPP Connection established")!=NULL){
				success ++;
				system("/App/demo_App/client2 117.89.79.151");
				//				sleep(2);
				system("/etc/ppp/ppp-off");
				close(fd_temp);
				for(i = 0; i < 4; i++){
					printf("%d\n", i);
					sleep(1);
				}
				break;
			} else if(strstr(string, "Connection terminated")|| strstr(string, "Exit")){
				close(fd_temp);
				for(i = 0; i < 4; i++){
					printf("%d\n", i);
					sleep(1);
				}
				break;	
			}
			sleep(1);
		}
#endif

#if 1
		times++;
		tcflush(fd,TCIFLUSH);
		//		write(fd, "AT+CIPSTATUS\r", strlen("AT+CIPSTATUS\r"));
		write(fd, "AT+QISTAT\r", strlen("AT+QISTAT\r"));
		usleep(500000);
		memset(string, 0, 1024);
		read(fd,string,1024);
		printf("string = %s\n",string);

		if(strstr(string, "IP INITIAL")||strstr(string, "IP CLOSE"));
		else{
			tcflush(fd,TCIFLUSH);
			//			write(fd, "AT+CIPSHUT\r", strlen("AT+CIPSHUT\r"));
			write(fd, "AT+QIDEACT\r", strlen("AT+QIDEACT\r"));
		}
		sleep(1);

		tcflush(fd,TCIFLUSH);
		//		write(fd,"AT+CIPSTART=\"TCP\",\"119.75.217.56\",80\r",strlen("AT+CIPSTART=\"TCP\",\"119.75.217.56\",80\r"));
		write(fd,"AT+QIOPEN=\"TCP\",\"117.89.54.44\",10000\r",strlen("AT+QIOPEN=\"TCP\",\"117.89.54.44\",10000\r"));
		gettimeofday(&begintime, NULL );//读取系统时间 

		while(1){
			//			usleep(200000);
			memset(string, 0, 1024);
			read(fd,string,1024);
			printf("string = %s\n",string);
			if(strstr(string, "OK"))break;
			else if(strstr(string, "ERROR"))break;
		}

		if(strstr(string, "OK")&&strstr(string, "CONNECT FAIL")==NULL){
			printf("OPEN OK\n");
			while(1){
				tcflush(fd,TCIFLUSH);
				//				write(fd, "AT+CIPSTATUS\r", strlen("AT+CIPSTATUS\r"));
				write(fd, "AT+QISTAT\r", strlen("AT+QISTAT\r"));
				usleep(100000);
				memset(string, 0, 1024);
				read(fd,string,1024);
				printf("string = %s\n",string);
				if(strstr(string, "CONNECT OK")){
					tcflush(fd,TCIFLUSH);
					write(fd, "AT+QISEND\r", strlen("AT+QISEND\r"));
					while(1){
						usleep(100000);
						memset(string, 0, 1024);
						read(fd,string,1024);
						printf("string = %s\n", string);
						if(strstr(string,">"))
						      break;
					}
					tcflush(fd,TCIFLUSH);
					write(fd, "prochip 1\r", strlen("prochip 1\r"));
					while(1){
						usleep(100000);
						memset(string, 0, 1024);
						read(fd,string,1024);
						printf("string = %s\n", string);
						if(strstr(string,"e")){
							success++;
							gettimeofday(&endtime, NULL );//读取系统时间 
							sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
							fill_rect(100, 16, 20, 16, 0);
							text_out(100, 16, string);
							flag = 2;
							printf("communication finished\n");
							break;
						}else{
							gettimeofday(&endtime, NULL );//读取系统时间 
							if(endtime.tv_sec-begintime.tv_sec > 30){
								sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
								fill_rect(100, 16, 20, 16, 0);
								text_out(100, 16, string);
								flag = 1;
								break;
							}
						}
					}
					if(flag == 2)break;
				}else if(strstr(string, "PDP FAIL")){
					flag = 1; break;
				}
				else{
					gettimeofday(&endtime, NULL );//读取系统时间 
					if(endtime.tv_sec-begintime.tv_sec > 30){
						sprintf(string, "%d", endtime.tv_sec-begintime.tv_sec);
						fill_rect(100, 16, 20, 16, 0);
						text_out(100, 16, string);
						flag = 1;
						break;
					}
				}
				sleep(1);
			}
			fill_rect(60, 16, 30, 64, 0);
			sprintf(string, "%d", times);
			text_out(60, 16, string);
			sprintf(string, "%d", success);
			text_out(60, 32, string);

			if(flag == 0){
				while(1){
					sleep(1);
					tcflush(fd,TCIFLUSH);
					//write(fd,"AT+QIDEACT\r",strlen("AT+QIDEACT"));
					//write(fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
					write(fd,"AT+QICLOSE\r",strlen("AT+QICLOSE\r"));
					sleep(1);
					memset(string, 0, 1024);
					read(fd,string,1024);
					printf("string = %s\n",string);
					if(strstr(string, "CLOSE OK")){
						sleep(1);
						break; 
					}
				}
			}else if(flag == 1){
				while(1){
					sleep(1);
					tcflush(fd,TCIFLUSH);
					//write(fd,"AT+QIDEACT\r",strlen("AT+QIDEACT"));
					//write(fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
					write(fd,"AT+QIDEACT\r",strlen("AT+QIDEACT\r"));
					sleep(1);
					memset(string, 0, 1024);
					read(fd,string,1024);
					printf("string = %s\n",string);
					if(strstr(string, "DEACT OK")){
						sleep(1);
						break; 
					}
				}
			}else if(flag == 2);


		} else{
			printf("OPEN error\n");
			fill_rect(60, 16, 30, 64, 0);
			sprintf(string, "%d", times);
			text_out(60, 16, string);
			sprintf(string, "%d", success);
			text_out(60, 32, string);

			while(1){
				tcflush(fd,TCIFLUSH);
				//write(fd,"AT+QIDEACT\r",strlen("AT+QIDEACT"));
				//write(fd,"AT+CIPSHUT\r",strlen("AT+CIPSHUT\r"));
				write(fd,"AT+QIDEACT\r",strlen("AT+QIDEACT\r"));
				sleep(1);
				memset(string, 0, 1024);
				read(fd,string,1024);
				printf("string = %s\n",string);
				if(strstr(string, "DEACT OK"))break; 
			}
		}

#endif
	}
#endif
}

void system_show()
{
	int key_num=0;
	int buf[3] = { 0 };
	struct timeval begintime, endtime;
    printf("system_show\n");
	gettimeofday(&begintime, NULL );//读取系统时间 
	while (1) 
	{
		buf[0] = 0;
		buf[1] = 99;
		read(Drivers[KEY].fd, (char *) buf, 2);
		gettimeofday(&endtime, NULL );//读取系统时间 
		if((endtime.tv_sec-begintime.tv_sec) > 7)
		{
			buf[0]=1;
			buf[1]=KEY_CANCEL;

			close(Drivers[Light].fd);
		}

		if (buf[0] == 1)	
		{
			ioctl(Drivers[KEY].fd,0,0);
			key_function(buf,&key_num,0);
			if (state == 1) 
			{
				state = 0;
			}
			/************************************************************************************************/
			//F1-F2菜单切换
			switch(buf[1])
			{
				case KEY_F1:
					if(page==1)break;
					else
					{
						lcd_clear();
						show_page(PAGE_ONE);
						fill_mark= KEY_ENTER;
						break;
					}
				case KEY_F2:
					lcd_clear();
					configure_server();
					break;
			}
			key_function(buf,&page,MENU_FLAG); 
			/************************************************************************************************/
			if(buf[1] == KEY_CANCEL)										//判断退出时间界面时要显示的菜单
			{
				fill_mark=show_time( Drivers[KEY].fd, Drivers[TLC549].fd,Drivers[TTYS3].fd, p.symbol[LANGUAGE] );
				open_driver(Light,O_RDWR);
				ioctl(Drivers[Light].fd,p.symbol[LIGHT],0);
				if(fill_mark == KEY_F1 || page == 1){ show_page(PAGE_ONE); }
				if(fill_mark == KEY_ENTER && page == 2){ show_page(PAGE_TWO); }
				if(fill_mark == KEY_F2 || page == 3){ configure_server(); }
			}
			if(buf[1] == KEY_UP)
			{
				if(page!=1)
				{
					show_page(PAGE_ONE);
					page=1;
				}
			}
			if(buf[1]==KEY_DOWN)
			{
				if(page!=2)
				{
					show_page(PAGE_TWO);
				}
			}
			switch (key_num) 
			{
				case 1:
					pwm_server();
					break;
				case 2:
					TLC549_server();
					break;
				case 3:
					psam_server();
					break;
				case 4:
					gprs_server();
					break;
				case 5:
					printer_server();
					break;
				case 6:
					mgcard_server();
					break;
				case 7:
					rc531_server();
					break;
				case 9:
					update_server();
					break;
				case 8:
					LCD_OFF();
					close(Drivers[TTYS3].fd);
					Sleep_deal(0);
					ioctl(Drivers[RC531].fd,1);
					sleep_server();
					set_gpio('C',7,1);
					open_driver(GRAYLCD,O_RDWR);
					Sleep_deal(1);
					open_driver(Light,O_RDWR);
					ioctl(Drivers[Light].fd,p.symbol[LIGHT],0);
					system("hwclock -s");
					text_out(20,25,pos_menu3[16]);
					sleep(7);
					open_driver(TTYS3, O_RDWR);					//打开串口3
					lcd_clear();
					//			gprs1_test();								//检测sim卡信号
					//			lcd_clear();
					show_page(PAGE_TWO);
					break;
				case 10:
					configure_server();
					break;
			}		
			key_num=0;
			gettimeofday(&begintime, NULL);//读取系统时间      
		}

	}
	close(Drivers[GRAYLCD].fd);
	close(Drivers[TTYS3].fd);
	close(Drivers[TLC549].fd);
}

void time_show()
{
	int key_num;
	open_driver(TTYS3, O_RDWR);					//打开串口3
	key_num=show_time(Drivers[KEY].fd,Drivers[TLC549].fd,Drivers[TTYS3].fd,p.symbol[LANGUAGE]);
	open_driver(Light,O_RDWR);
	ioctl(Drivers[Light].fd,p.symbol[LIGHT],0);

	if(key_num==KEY_F1||key_num==1){show_page(PAGE_ONE);}
	if(key_num==KEY_F2){configure_server();}
}

