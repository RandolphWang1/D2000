
int leap(int y)    /*判断平闰年 */
{ 
return (y%4==0&&y%100||y%400==0)?1:0; 
} 

int cal_num1(int year,int month)  /*根据年月确定当月1号是周几 */
{
    int f;
    if (month<3)
    { 
        month+=12; 
        year--; 
    } 
        f=(1+2*month+3*(month+1)/5+year+year/4-year/100+year/400)%7 +1; 
        return f;
    }

int cal_days(int year,int month)         /* 根据年月确定当月的天数 */
{ 
    int d; 
    switch (month)
    { 
        case 1: 
        case 3: 
        case 5: 
        case 7: 
        case 8: 
        case 10: 
        case 12:d=31;break; 
        case 4: 
        case 6:
        case 9:
        case 11:d=30;break;
        case 2:d=leap(year)?29:28;break;
    }
    return d;
}




int display_month(int week,int end)        //week：判断当前一号是周几  end：判断当月有多少天
{
    int i,j;
    int a[6][7];
    int counter = 0;
    char temp[2];
    memset(a,0,sizeof(a));
    for(i=1;i<end+1;i++)
    {   
        a[counter][week]=i;
        week++;
        if(week==7)
        {
            week=0;
            counter++;
        }
    }
            for(i=0;i<6;i++)
            {
                for(j=0;j<7;j++)
                {
                    if(a[i][j]==0);
                    else
                    {
                        sprintf(temp,"%2d",a[i][j]);
                        text_string(13+j*16,16+i*8,temp,2);
                    }
                }
            }  
}





/*******************************************************************************************
函数名称	:   calendar_server	
函数功能	:	系统设置程序
入口参数	:	无
出口参数	:	无
函数返回	:	无
其它说明	:	无
*******************************************************************************************/
void calendar_server()
{

 int fd_key;
 	char z,ch;
   int buf[3] = {0};
   int fd_gpio,light_line,tick;
   time_t timep;
   int tmp;
   struct tm *p;
	char s[10]={0};
   int y,m,i,week; 
   char *wday[]={"Sun","Mon","Tue","Wen","Thr","Fri","Sat"};
	char date[20]={0},q[10]={0};
	int year,month,day;
	int month_counter=0;
	int year_counter=0;
	int end;
	char temp[2];
	char head[128] = 
	{
      0x00,0x00,0x00,0x7f,0x49,0x49,0x49,0x49,
		0x49,0x49,0x7f,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x10,
      0x10,0x18,0x10,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x40,0x44,0x44,0x44,0x44,
      0x46,0x64,0x40,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x40,0x44,0x54,0x54,0x54,
      0x46,0x64,0x40,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0xFE,0xA2,0x9E,0x82,0x9E,
      0xA2,0xA2,0xFE,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x92,0x92,0xF2,0x9E,0x92,
      0x92,0xF2,0x80,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x88,0x88,0x7A,0x0A,0x0A,
      0x38,0x48,0x88,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	};
	lcd_open();
	lcd_clear();
	paint_rect(head,10,7,128,8);
	fill_rect(0,15,128,8,0);
    fd_key = open("/dev/key",O_RDONLY|O_NONBLOCK);        //非阻塞方式读取
	fd_gpio = open("/dev/gpio",O_RDWR|O_NONBLOCK);
	time(&timep);
	p=gmtime(&timep);
	year=1900+p->tm_year;
	month=1+p->tm_mon;
	day=p->tm_mday;
	sprintf(q,"%4d",year);
	strcat(date,q);
	strcat(date,"-");
	sprintf(q,"%2d",month);
	strcat(date,q);
	strcat(date,"-");
	sprintf(q,"%2d",day);
	strcat(date,q);
	strcat(date,"   ");
	strcat(date,wday[p->tm_wday]);
	text_string(30,0,date,16); 

	time(&timep);
	p=gmtime(&timep);
	week=(cal_num1(p->tm_year + 1900,p->tm_mon + 1))%7;
	end=cal_days(p->tm_year + 1900,p->tm_mon + 1);
	display_month(week,end);

	while(1)
	{
		buf[0]=0;
 	   read(fd_key, (char*)buf, 2);            //从驱动得到的键值保存在buf数组内，3可变
		if(buf[0]==1&&buf[1]==15)
			{
				fill_rect(0,16,128,64,0);	
				month_counter++;
					if((p->tm_mon + 1 + month_counter) > 12)
						{
							month_counter = -p->tm_mon;
							year_counter++;
						}
						sprintf(temp,"%4d",p->tm_year+year_counter+1900);
						text_string(30,0,temp,4); 
						sprintf(temp,"%2d",p->tm_mon+month_counter+1);
						text_string(55,0,temp,2); 
		 				week=(cal_num1(p->tm_year+year_counter + 1900,p->tm_mon + 1 + month_counter))%7;
						end=cal_days(p->tm_year+year_counter + 1900,p->tm_mon + 1 + month_counter);
						display_month(week,end);
			}     
		if(buf[0]==1&&buf[1]==10)
		{
		   fill_rect(0,16,128,64,0);
			month_counter--;
			if((p->tm_mon + 1 + month_counter) < 1)
				{
					year_counter--;
               month_counter = 11 - p->tm_mon;
				 }
				sprintf(temp,"%4d",p->tm_year+year_counter+1900);
				text_string(30,0,temp,4); 
				sprintf(temp,"%2d",p->tm_mon+month_counter+1);
				text_string(55,0,temp,2); 
				week=(cal_num1(p->tm_year + year_counter + 1900,p->tm_mon + 1 + month_counter))%7;
				end=cal_days(p->tm_year+ year_counter + 1900,p->tm_mon + 1 + month_counter);
				display_month(week,end);
		 }
      if(buf[0]==1&&buf[1]==9)
		{
			month_counter = 0;
			year_counter = 0;
			fill_rect(0,16,128,64,0);
			sprintf(temp,"%4d",p->tm_year+1900);
			text_string(30,0,temp,4); 
			sprintf(temp,"%2d",p->tm_mon+1);
			text_string(55,0,temp,2); 
			week=(cal_num1(p->tm_year + 1900,p->tm_mon + 1))%7;
			end=cal_days(p->tm_year + 1900,p->tm_mon + 1);
			display_month(week,end);
		}		
    }	
}
