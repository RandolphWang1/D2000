#include<stdio.h>
#include<time.h>
#include<sys/types.h>
#include<unistd.h>
#include"12x24.h"
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>     /*标准函数库定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
#include <math.h>
#include "App.h"

//#define UP    15
//#define DOWN  10
#define LEFT   10
#define RIGHT  15
#define CLEAR   5
#define CANCEL 100

int pre_year = -1;
int pre_mon = -1;
int pre_day = -1;
int pre_hour = -1;
int pre_min = -1;
int pre_wday = -1;
int powerstate=0;
int time_state=0;
int show,show1,siglv;
int gprs_signal=0;
int bufshow[3]={0};
int fd_show;
int shutdown;

void paint_string(unsigned char *string,int length,int x)
{
	int var;
	for(var = 0;var < length;var++)
	{
		paint_rect(num+36*(string[var]-0x20),x+16*var,16,12,24);
	}
}
void updata_cyear(int year,int n)
{
    char dyear[5];
    sprintf(dyear,"%04d",year);
	if(n==1){fill_rect(25,50,24,16,0);text_out(25,50,dyear,2);}
    else {fill_rect(5,50,24,16,0);text_out(5,50,dyear,2);}
}

void updata_year(int year,int n)
{	
	char dyear[5];
	sprintf(dyear,"%02d",1900+year);
    if(n==1){fill_rect(25,50,24,16,0);text_out(25,50,dyear,2);}
    else {fill_rect(5,50,24,16,0);text_out(5,50,dyear,2);}

}

void updata_month(int month,int n)
{
	char dmonth[3];
    sprintf(dmonth,"%02d",month);
	if(n==1){fill_rect(63,50,12,16,0);text_out(63,50,dmonth,2);}
    else {fill_rect(38,50,12,16,0);text_out(38,50,dmonth,2);}
}

void updata_day(int day,int n)
{
	char dday[3];
	sprintf(dday,"%02d",day);
	if(n==1){fill_rect(85,50,12,6,0);text_out(85,50,dday,2);}
    else   {fill_rect(59,50,12,16,0);text_out(59,50,dday,2);}
}

void updata_hour(int hour)
{
	char dhour[3];
	sprintf(dhour,"%02d",hour);
	paint_string(dhour,2,30);
}

void updata_min(int min)
{
	char dmin[3];
	sprintf(dmin,"%02d",min);
	paint_string(dmin,2,72);
}

void  mdelay(int n)                                //自定义延时
{
    int i,j;
    if((shutdown==1&&n)||show==1||show1==1);
    else
    {
        for(i=0;i<400;i++)
        for(j=0;j<100;j++)
        {
            read(fd_show,(char *)bufshow,2);
 //           if((bufshow[1]==8||bufshow[1]==11||bufshow[1]==12||bufshow[1]==17)&&n==0)
            if(bufshow[1]==KEY_F2||bufshow[1]==KEY_F1||bufshow[1]==KEY_ENTER)
            {
                i=400;
                shutdown=1;
                break;
            }
            if(bufshow[0]==1&&n==0)
            {
                i=400;
                break;
            }
        }
    }
}

int time_change(int num,int *cal,int i)
{
    switch(num)
    {
        case 19:num=1;break;
        case 17:num=2;break;
        case 16:num=3;break;
        case 14:num=4;break;
		case 13:num=5;break;
        case 11:num=6;break;
        case  8:num=7;break;
        case  9:num=8;break;
        case  7:num=9;break;
        case  3:num=0;break;
        case  4:num=CANCEL;break;
        default:num=11;
    }
    if(num==CANCEL){ cal[i]=0; time_state=0;return 12; }
    if(num==11)return 11;
    if(i==2)
    {
        if(time_state==0){cal[i]=num;time_state++;}
        else 
        {
 //           printf("time_state=%d\n",time_state);
            cal[i]=cal[i]*10+num;
            time_state++;
        }
    }
    else if(time_state==0)
    {
        cal[i]=num;
        time_state=1;
    }
    else if(time_state==1)
    {
        cal[i]=cal[i]*10+num;
        time_state=0;
    }
    return num;
}

void calendar_change(int language)
{
    show=2;
    show1=2;
    int i=0,j;
    int num;
    char string[30];
    if(language == 0)
	  pos_menu3=pos_chnedit;
    else if(language == 1)
	  pos_menu3=pos_ukedit;

   lcd_clear();

    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    int cal[5]={p->tm_hour,p->tm_min,p->tm_year+1900,p->tm_mon+1,p->tm_mday};
    
    text_out(0,0,pos_menu3[0]);
    text_out(50,50,"/");
    text_out(74,50,"/");
    text_out(98,50,"/");
    i=0;
    updata_min(p->tm_min);
    updata_year(p->tm_year,1);
	updata_month(p->tm_mon+1,1);
	updata_day(p->tm_mday,1);
    while(1)
    {
        num=11;
        bufshow[0]=0;
        bufshow[1]=99;
        if(i==0) { fill_rect(30,16,30,24,0); mdelay(0); updata_hour(cal[i]);   mdelay(0); }
        if(i==1) { fill_rect(72,16,30,24,0); mdelay(0); updata_min(cal[i]);    mdelay(0); }
        if(i==2) { fill_rect(25,50,24,9,0);  mdelay(0); updata_cyear(cal[i],1); mdelay(0); }
        if(i==3) { fill_rect(63,50,12,9,0);  mdelay(0); updata_month(cal[i],1);mdelay(0); }
        if(i==4) { fill_rect(85,50,12,9,0);  mdelay(0); updata_day(cal[i],1);  mdelay(0); }
        if(bufshow[0]==1)
        {
            num=time_change(bufshow[1],cal,i);
            if(bufshow[1]==KEY_ENTER)
            {
                sprintf(string,"date %02d%02d%02d%02d%04d",cal[3],cal[4],cal[0],cal[1],cal[2]);
                system(string);
                system("hwclock -w");
                lcd_clear();
                text_out(0,0,pos_menu3[1]);
                sleep(1);
                break;
            }
            if(bufshow[1]==CLEAR)break;
            if(bufshow[1]==LEFT){ i--; time_state=0; }
            if(bufshow[1]==RIGHT){ i++; time_state=0; }
            if(i<0)i=4;
            if(i>4)i=0;
            if(num<=9&&num>=0)
            {
                if(cal[0]>23)cal[0]=0;
                if(cal[1]>59)cal[1]=0;
                if((cal[2]<1900||cal[2]>2035)&&time_state==4)
                {
                    cal[2]=1900+p->tm_year;
                    time_state=0;
                }
				else if(time_state==4)time_state=0;
                if(cal[3]>12)cal[3]=1;
                j=cal_days(cal[2],cal[3]);
                if(cal[4]> j)cal[4]=1;
                if(cal[0]<0)cal[0]=23;
                if(cal[1]<0)cal[1]=59;
                if(cal[3]<=0)cal[3]=1;
                if(cal[4]<1)cal[4]= j;

            }
        }
    }
}

void updata_wday(int wwday,int flag)
{
    char *wday1[]={"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
    char *wday2[]={"Sun","Mon","Tue","Wen","Thr","Fri","Sat"};
    if(flag==0) {fill_rect(116,50,24,16,0);text_out(92,50,wday1[wwday]);}
    if(flag==1) {fill_rect(134,50,24,16,0);text_out(110,50,wday2[wwday]);}
}

void TLC549_thread(int fd_tlc549)
{
    int i;
	char buff[4]={0};
    while(1)
    {
        read(fd_tlc549,buff,4);
        read(fd_tlc549,buff,4);
        if(show==0)
        {
           if(buff[0]<250&&buff[0]>200)
           {
               if(powerstate!=3&&bufshow[0]!=1){powerstate=3;show=1;} 
           }
           else if(buff[0]<=200&&buff[0]>=120)
           {
               if(powerstate!=2&&bufshow[0]!=1){powerstate=2;show=1;}
           }
           else if(buff[0]<120)
           {
               if(powerstate!=1&&bufshow[0]!=1){powerstate=1;show=1;}
           }
        }
        mdelay(1);
        pthread_exit(0);
    } 
}


void gprs_thread(int fd)
{
    char *string=(char *)malloc(1024*sizeof(char));
    int i;
    char *p;
    if(show1==0)
    {
        memset(string,0,1024);
        tcflush(fd,TCIFLUSH);
        write(fd,"AT+CSQ\r",strlen("AT+CSQ\r"));         //信号质量检测
        read_datas_tty(fd,string,0,200000);

        if(strchr(string,',')==NULL||strstr(string,"99")!=NULL)
        {
            siglv=0;
            show1=0;
        }
        else
        {
            strcpy(strchr(string,','),"\0");
            if(toascii(string[8])=='0')i=0;
            else
            {
                p=strchr(string,':')+2;
                i=atoi(p);
            }
            if(i<=10&&i>0)siglv=1;
            else if(i>10&&i<=20)siglv=2;
            else if(i>20&&i<=30)siglv=3;
            else siglv=0;
            show1=1;
        }

        mdelay(1);
    }
    pthread_exit(0);
}

void show_dot(int flag,int fd_gprs)
{
	struct timeval begintime, endtime;
    pthread_t id1;
	time_t timep;
	struct tm *p;
    int i,j,z=0;
	gettimeofday(&begintime, NULL );//读取系统时间 
	while(1)
	{
		gettimeofday(&endtime, NULL );//读取系统时间 
		if((endtime.tv_sec-begintime.tv_sec) > 7 && z==0)
		{
			z=1;
			close(Drivers[Light].fd);
		}
        if(shutdown==1)
        {
            break;
        }
		time(&timep);
		p = localtime(&timep);
		if(p->tm_year!=pre_year)
		{
			pre_year = p->tm_year;
			updata_year(p->tm_year,0);
		}
		if(p->tm_mon!=pre_mon)
		{
			pre_mon = p->tm_mon;
			updata_month(p->tm_mon+1,0);
		}
		if(p->tm_mday!=pre_day)
		{
			pre_day = p->tm_mday;
			updata_day(p->tm_mday,0);
		}
		if(p->tm_hour!=pre_hour)
		{
			pre_hour = p->tm_hour;
			updata_hour(p->tm_hour);
		}
		if(p->tm_min!=pre_min)
		{
			pre_min = p->tm_min;
			updata_min(p->tm_min);
		}
		if(p->tm_wday!=pre_wday)
		{
			pre_wday = p->tm_wday;
			updata_wday(p->tm_wday,flag);
		}
        if(show==1)
        {
               //电量检测
            if(powerstate==3)
            {   
                fill_rect(125,4,4,3,1);
                fill_rect(120,4,4,3,1);
                fill_rect(115,4,4,3,1);
            }
            else if(powerstate==2)
            {
                fill_rect(125,4,4,3,1);
                fill_rect(120,4,4,3,1);
                fill_rect(115,4,4,3,0);
            }
            else if(powerstate==1)
            {
                fill_rect(127,4,4,3,1);
                fill_rect(120,4,4,3,0);
                fill_rect(115,4,4,3,0);
            }
            show=2;
        }
        if(show1==1)
        {
            //信号检测
            if(siglv==3)
            {
                fill_rect(13,0,8,8,1);
                fill_rect(13,0,2,4,2);
                fill_rect(15,0,1,8,2);
                fill_rect(18,0,1,8,2);
                fill_rect(16,0,2,2,2);
            }
            else if(siglv==2)
            {
                fill_rect(13,0,8,8,1);
                fill_rect(13,0,2,4,2);
                fill_rect(15,0,1,8,2);
                fill_rect(18,0,1,8,2);
                fill_rect(16,0,2,2,2);
                fill_rect(19,0,2,7,2);
            }
            else if(siglv==1)
            {
                fill_rect(13,0,8,8,1);
                fill_rect(13,0,2,4,2);
                fill_rect(15,0,1,8,2);
                fill_rect(18,0,1,8,2);
                fill_rect(16,0,2,2,2);
                fill_rect(19,0,2,7,2);
                fill_rect(16,0,2,7,2);
				fill_rect(16,0,2,2,0);
            }
            else if(siglv==0)
            {
                fill_rect(13,0,8,8,1);
                fill_rect(13,0,2,8,2);
                fill_rect(15,0,1,8,2);
                fill_rect(18,0,1,8,2);
                fill_rect(16,0,2,8,2);
                fill_rect(19,0,2,8,2);
            }
            show1=2;
        }
            paint_rect(num+36*26,60,16,12,24);
            mdelay(1);
            mdelay(1);
            fill_rect(60,16,12,24,0);
            mdelay(1);
            mdelay(1);
			if(show1!=2)pthread_create(&id1,NULL,(void *) gprs_thread,(int *)fd_gprs);
            pthread_join(id1,NULL);
	}
}



int show_time(int fd_key,int fd_tlc549,int fd_gprs,int flag)
{
    powerstate=4;
    show=0;
    show1=0;
    bufshow[0]=0;
    bufshow[1]=99;
    shutdown=0;
    fd_show=fd_key;
    gprs_signal=0;
    pthread_t id1,id2;
    set_speed(fd_gprs,115200);
    set_Parity (fd_gprs, 8,1,'n'); 
    pthread_create(&id1,NULL,(void *) TLC549_thread,(int *)fd_tlc549);
    pthread_create(&id2,NULL,(void *) gprs_thread,(int *)fd_gprs);
    lcd_clear();
    draw_rect(113,2,18,7,1);
    draw_rect(112,4,1,3,1);//电池

    fill_rect(0,0,12,8,2);
    fill_rect(0,2,7,6,2); 
    fill_rect(9,2,3,6,2);


	text_out(30,50,"/");
    text_out(51,50,"/");
    show_dot(flag,fd_gprs);
    pre_year = -1;
    pre_mon = -1;
    pre_day = -1;
    pre_hour = -1;
    pre_min = -1;
    pre_wday = -1;
    return bufshow[1];
}
