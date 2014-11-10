#include	"../App.h"

char PRT_STATE = FALSE;
char PRTBUF[1024];

int fd;

unsigned char buf3[]="商品名称(Name) ：       \r";
unsigned char buf2[]="客户存根联           请妥善保管\r";
unsigned char buf4[]="                 \r";
unsigned char buf5[]="商户号MER NO.：       \r";
unsigned char buf6[]="终端号：                 00008\r";
unsigned char buf7[]="操作员号：                  06\r";
unsigned char buf8[]={0x1B,0x63,0x34,0x08};


/*******************************************************************************************
函数名称	:	PRT_PaperState
函数功能	:	读取打印机是否缺纸
入口参数	:	无
出口参数	:	无
函数返回	:	有纸：TRUE；无纸或失败：FASLE；
其它说明	:	无
*******************************************************************************************/
int PRT_PaperState(void)
{
	unsigned char buffer[] = {0x10,0x04,0x04};
	unsigned char recv[16];
	int recvlen;
	int ret;
	memset(recv,0,16);
	write(fd,buffer,3);
	usleep(1000*50);
	if ((PRT_STATE) && (PRTBUF[0] == 0x00)) 
		return TRUE; 
		else return FALSE;
}

/*******************************************************************************************
函数名称	:	PRT_Black
函数功能	:	该指令可根据不同的电源需求调整打印黑度
入口参数	:	n的调整范围：0x08~0x40;缺省值： n=0x08
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_Black(char n)
{
	unsigned char buffer[]={0x10,0x05,0x05,n};
	write(fd,buffer,4);
	return TRUE;
}
/*******************************************************************************************
函数名称	:	PRT_Empty
函数功能	:	执行n 字符行空白打印
入口参数	:	n：多少行
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_Empty(char n)
{
    int i;
	unsigned char buffer[]={0x1B,0x66,0x01,n};
	write(fd,buffer,4);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_HZoom
函数功能	:	该命令可同时设置字符的宽度放大倍数和高度放大倍数，
					该命令后的所有字符将以基本宽度的n 倍和基本高度的 倍打印
入口参数	:	n：放大比例 取值：1~6
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_WHZoom(char n)
{
	if (n>6) return FALSE;
	unsigned char buffer[]={0x1B,0x57,n};
	write(fd,buffer,3);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_HZoom
函数功能	:	该命令后的所有字符将以基本高度的n 倍打印
入口参数	:	n：放大比例 取值：1~6
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_HZoom(char n)
{
	if (n>6) return FALSE;
	unsigned char buffer[]={0x1B,0x56,n};
	write(fd,buffer,3);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_WZoom
函数功能	:	该命令后的所有字符将以基本宽度的n 倍打印
入口参数	:	n：放大比例 取值：1~6
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_WZoom(char n)
{
	if (n>6) return FALSE;
	unsigned char buffer[]={0x1B,0x55,n};
	write(fd,buffer,3);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_LineSpace
函数功能	:	为后面的换行命令设置n 点行间距，该值表示两个字符行之间的空白点行数
入口参数	:	n：空行点数，默认为0x03
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_LineSpace(char n)
{
	unsigned char buffer[]={0x1B,0x31,n};
	write(fd,buffer,3);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_UnderLine
函数功能	:	是否打印下划线
入口参数	:	state：为TRUE时打印下划线，FASLE是不打印
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_UnderLine(char state)
{
	unsigned char buffer[]={0x1B,0x2B,state};
	
	write(fd,buffer,3);
	return TRUE;
}


/*******************************************************************************************
函数名称	:	PRT_PaperGo
函数功能	:	打印走纸N行
入口参数	:	n:走纸行数; 
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_PaperGo(char n)
{
	unsigned char buffer[]={0x1B,0x4B,n};
	write(fd,buffer,3);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_text
函数功能	:	打印字符
入口参数	:	cnt:数据个数,字节为单为,str:打印的数据指针
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_text(unsigned char * str, int cnt)
{
	write(fd,str,cnt);
	return TRUE;
}

/*******************************************************************************************
函数名称	:	PRT_SetTextDouble
函数功能	:	设置打印机字符比例倍高与倍宽
入口参数	:	
	WH字符比列, 16进制表示,前一位为宽度,后一位为高度 
	EG:0x00 正常比列,0x01:倍宽，0x02:倍高，0x03:倍宽倍高
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_SetTextDouble(unsigned char WH)
{
	int i;
	unsigned char buffer[3] = {0x1c, 0x21, 0x00};
	switch (WH)
	{
	case 0: WH = 0x00; break;
	case 1: WH = 0x04; break;
	case 2: WH = 0x08; break;
	case 3: WH = 0x0c; break;
	}
	buffer[2] = WH;
	write(fd,buffer,3);
	return 0;
}

/*******************************************************************************************
函数名称	:	PRT_SetTextWH
函数功能	:	设置打印机字符比列
入口参数	:	WH字符比列, 16进制表示,前一位为宽度,后一位为高度 EG:0x00 正常比列,0x11:放大一倍
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	height 在默认的情况下为30
*******************************************************************************************/
int PRT_SetTextWH(unsigned char WH)
{
	unsigned char buffer[3] = {0x1d, 0x21, 0x00};	
	buffer[2] = WH;
	write(fd,buffer,3);
	return 0;
}

/*******************************************************************************************
函数名称	:	PRT_SetLineHeight
函数功能	:	设置打印机行距
入口参数	:	height行距,取值0~255,当取值为0时使用打印默认的行高
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	height 在默认的情况下为30
*******************************************************************************************/
int PRT_SetLineHeight(unsigned char height)
{
	unsigned char buffer[3] = {0x1b, 0x33, 0x00};
	
	if (height == 0)
	{
		buffer[1] = 0x32;
		write(fd,buffer,2);
	} else
	{
		buffer[2] = height;
		write(fd,buffer,3);
	}
	return 0;
}


/*******************************************************************************************
函数名称	:	PRT_SetSpaceBetween
函数功能	:	设置打印机字符间距
入口参数	:	width间距,取值0~255
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_SetSpaceBetween(unsigned char width)
{
	unsigned char buffer[3] = {0x1b, 0x20, 0x00};
	
	buffer[2] = width;
	write(fd,buffer,3);
	return 0;
}

/*******************************************************************************************
函数名称	:	PRT_Alignment
函数功能	:	设置对齐方式
入口参数	:	type 对齐方式, 0x00 左对齐; 0x01 居中; 0x02 右对齐
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_Alignment(unsigned char type)
{
	unsigned char buffer[3] = {0x1B, 0x61, 0x00};
	buffer[2] = type;
	write(fd,buffer,3);
	return 0;
}

/*******************************************************************************************
函数名称	:	PRT_SetInit
函数功能	:	设置打印机为出厂状态
入口参数	:	无
出口参数	:	无
函数返回	:	成功:TRUE; 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int PRT_SetInit(void)
{
	unsigned char buffer[2] = {0x0B, 0x40};
	write(fd,buffer,2);
	return 0;
}

int main()
{
    int i=0,j;
    char input[10]={0};
    int buf[10]={0};

    fd=open_driver(TTYS2,O_RDWR);
    set_speed(Drivers[TTYS2].fd,115200);
    set_Parity (Drivers[TTYS2].fd, 8,1,'n');

    char string[1024]={0};
    
//  unsigned char buf3[]="商品名称(Name) ：       \r";

    while(1)
    {
        system("clear");
        i=0;j=0;
        memset(buf,0,10);
        memset(input,0,10);
        while(1)
        {
            scanf("%x",&buf[i]);
            if(buf[i]=='z')
            {
//                input[i]=='\n';
 //               buf[i]==0x0A;
                break;
            }
            i++;
        }
        for(j=0;j<10;j++)
        {
            printf("%c\n",buf[j]);
        }
        for(j=0;j<=i-1;j++)
        {
            input[j]=buf[j];
            printf("0x%02x\n",input[j]);
        }
       
        set_gpio(PRINTTER_PORT,PRINTER_PORT_NUM0,1);
        set_gpio(PRINTTER_PORT,PRINTER_PORT_NUM1,1);

        sleep(1);

        tcflush(Drivers[TTYS2].fd,TCIFLUSH);
        write(Drivers[TTYS2].fd,input,strlen(input)); 
        sleep(1);
        memset(string,0,1024);
        read_datas_tty(Drivers[TTYS2].fd,string,0,200000);
        if(strlen(string)!=0&&string[0]>' '&&string[0]<'z') printf("string=%s\n",string);

        sleep(2);

        set_gpio(PRINTTER_PORT,PRINTER_PORT_NUM0,0);
        set_gpio(PRINTTER_PORT,PRINTER_PORT_NUM1,0);
    
    }
}
