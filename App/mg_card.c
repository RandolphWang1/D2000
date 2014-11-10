#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define  FORWARD			    0x01
#define	BACK		   			0x02
#define	NO_TRACK1				0x10
#define	NO_TRACK2				0x04
#define NO_TRACK3				0x08
#define  OK			   			0x05
#define  STATE		   			0x00
#define  SIZE1_DATA_TRK1        704
#define  SIZE1_DATA_TRK2        711
#define  SIZE2_DATA_TRK3     	711
#define  SIZE3_CARD_NUM1     	100
#define  SIZE3_CARD_NUM2     	50
#define  SIZE4_CARD_NUM3     	120

#define DEBUG
#ifdef DEBUG
# define debug	//printf
#else
#define debug(...) ((void)0)
#endif

static unsigned char convert_track(char direction);
static int convert_forward(char *data, int len, int track,int bit);
static int convert_back(char *data,int len, int track,int bit);

static char ChangeData(char data);
static char ChangeData1(char data);
static char check(char data);

unsigned char	data_trk1[SIZE1_DATA_TRK1];   				//轨1 原始数据    第2磁道数据编码最大记录长度为79个字符
unsigned char	data_trk2[SIZE1_DATA_TRK2];   				//轨2 原始数据    第2磁道数据编码最大记录长度为40个字符
unsigned char	data_trk3[SIZE2_DATA_TRK3];    				//轨3 原始数据    第3磁道数据编码最大记录长度为107个字符
char           card_num1[SIZE3_CARD_NUM1];              		//经过处理的轨3数据存储到card_num3[50]  
char           card_num2[SIZE3_CARD_NUM2];              		//经过处理的轨2数据存储到card_num2[50]
char           card_num3[SIZE4_CARD_NUM3];              		//经过处理的轨3数据存储到card_num3[50]   
int            track1_num=SIZE1_DATA_TRK1;	      		    	//记录的是轨3数据记录占用的空间长度（处理之后的长度)
int            track2_num=SIZE1_DATA_TRK2;       		       	//记录的是轨2数据记录占用的空间长度  (处理之后的长度)
int            track3_num=SIZE2_DATA_TRK3;	      		    	//记录的是轨3数据记录占用的空间长度（处理之后的长度)
unsigned char  result_value;  

int fd_mg,mgflag=0;
int dstate = 0 ,error=0;
char str4[20];
//char brush_card=0;

static char check(char data)
{
	int num = 0,i = 0;	
	for(i= 0; i < 7; i++)
	{
		if(data&(0x01 << i))
		      num ++;
	}
	if(num%2)
	      return OK;
	else
	      return 0;
}	

static char ChangeData1(char data)
{  
	data &= (~0x40);
	data = data + 0x20;
	return data;
}

static char ChangeData(char data)
{  
	data &= (~0x10);
	data = data + 0x30;
	return data;
}

static void ShowTrack1(char *back_track)
{
	int i = 0;
//	if(data_trk1[0] != 0x45) return;
	for(i = 0; i< track1_num; i++)	
	{
		*(back_track + i ) = ChangeData1(data_trk1[i]);
//		if(*(back_track + i )<0x20||*(back_track + i )>122) break;
	}
	*(back_track + i) = '\0';
}

static void ShowTrack2(char *back_track)
{

	int i = 0;
//	if(data_trk2[0] != 0x0b) return;
	for(i = 0; i<track2_num; i++)	
	{
		*(back_track + i ) = ChangeData(data_trk2[i]);
		/*
		if(*(back_track + i )<0x20||*(back_track + i )>122)
		{
			error=1;
			break;
		}
		*/
	}
	*(back_track + i) = '\0';
}		
static void ShowTrack3(char *back_track)
{
	int i;	
//	if(data_trk3[0] != 0x0b) return;
	for(i = 0; i<track3_num; i++)
	{
		*(back_track + i ) = ChangeData(data_trk3[i]);
//		if(*(back_track + i )<0x20||*(back_track + i )>122) break;
	}
	*(back_track + i) = '\0';
}




static int convert_back(char *data,int len, int track,int bit)
{
	int i = 0,now_postion = 0,real_num = 0;
	char *temp;																		
	char ascii_data = 0x00, tmp_data[107], track_data;
	char lrc[5] = {0, 0, 0, 0, 0};
	track_data = track;
	temp = data + len - 1;
	debug("\n");
	while(1)
	{
		if(*temp&track_data)													
		{
			temp --;
			now_postion ++;
			if(now_postion > len) {
				debug("DATA ERROR\n");
				return 0;
			}
		}
		else
		{
			while(ascii_data != 0x1f)
			{				
				ascii_data = 0x00;
				for(i = 0; i < bit; i++)
				{
					if(*temp & track_data){
						debug("%x  ",*temp);
						ascii_data |= (0x00<<i);
					} else{
						debug("%x  ",*temp);
						ascii_data |= (0x01<<i);
						lrc[i]++;
					}
					temp --;
				}
				debug("0x%02x	\n",ascii_data);
				if(ascii_data == 0x00) break;
				if(check(ascii_data))
				      tmp_data[real_num] = ascii_data;
				else{
					debug("CRC ERROR\n");
					return 0;
				}
				real_num ++;
				if(real_num%15 == 0)debug("\n");
			}
			ascii_data = 0x00;
			for(i = 0; i < bit; i++)
			{
				if(*temp & track_data){
					debug("%x  ",*temp);
					ascii_data |= (0x00<<i);
				} else{
					debug("%x  ",*temp);
					ascii_data |= (0x01<<i);
					lrc[i]++;
				}
				temp --;
			}
			debug("0x%02x	\n",ascii_data);
			debug("1 = %d\n2 = %d\n3 = %d\n4 = %d\n5 = %d\n", lrc[0], lrc[1], lrc[2], lrc[3], lrc[4]);
			debug("\n");
			for(i = 0 ;i < 4; i++) {
				if(lrc[i]%2 != 0){
					debug("LRC ERROR\n");
					return 0;
				}
			}
			memset(data,0x00,len);
			if(tmp_data[0] ==0x45 || tmp_data[0] == 0x0b);
			else {
				debug("SIGNIFICANT BIT ERROR\n");
				return 0;
			}
			i = 0;
			do
			{
				*(data + i) = tmp_data[i];
			}
			while((tmp_data[i++] != 0x1f)&&(i<120));
			dstate = FORWARD;     
			return i;
		}
	}
}

static int convert_forward(char *data, int len, int track,int bit)
{ 
	int i = 0,now_postion = 0,real_num = 0;
	char *temp;																			
	char ascii_data = 0x00, tmp_data[107], track_data;	
	char lrc[5] = {0, 0, 0, 0, 0};
	track_data = track;
	temp = data;	
	debug("\n");
	while(1)
	{
		if(*temp&track_data)														
		{
			temp ++;
			now_postion ++;
			if(now_postion > len-1){
				debug("DATA ERROR\n");
			       	return 0;
			}
		}
		else
		{
			while(ascii_data != 0x1f)												
			{				
				ascii_data = 0x00;
				for(i = 0;i < bit;i++)
				{
					if(*temp & track_data){
						debug("%x  ",*temp);
						ascii_data |= (0x00<<i);
					} else{
						debug("%x  ",*temp);
						ascii_data |= (0x01<<i);
						lrc[i]++;
					}
					temp ++;
				}
				debug("0x%02x	\n",ascii_data);
				if(ascii_data == 0x00)
				      break;
				if(check(ascii_data))
				      tmp_data[real_num] = ascii_data;
				else{
					debug("CRC ERROR\n");
					return 0;
				}
				real_num ++;
				if(real_num%15 == 0)debug("\n");
			}
			ascii_data = 0x00;
			for(i = 0;i < bit;i++)
			{
				if(*temp & track_data){
					debug("%x  ",*temp);
					ascii_data |= (0x00<<i);
				} else{
					debug("%x  ",*temp);
					ascii_data |= (0x01<<i);
					lrc[i]++;
				}
				temp ++;
			}
			debug("0x%02x	\n",ascii_data);
			debug("1 = %d\n2 = %d\n3 = %d\n4 = %d\n5 = %d\n", lrc[0], lrc[1], lrc[2], lrc[3], lrc[4]);
			debug("\n");
			for(i = 0 ;i < 4; i++) {
				if(lrc[i]%2 != 0){
					debug("LRC ERROR\n");
					return 0;
				}
			}
			memset(data,0x00,len);
			if(tmp_data[0] ==0x45 || tmp_data[0] == 0x0b);
			else {
				debug("SIGNIFICANT BIT ERROR\n");
				return 0;
			}
			i = 0;
			do	
			{
				*(data + i) = tmp_data[i];
			}
			while((tmp_data[i++] != 0x1f)&&(i<120));
			dstate = BACK;     
			return i;
		}
	}
}

static unsigned char convert_track(char direction)      			//对原始数据进行处理
{
	int i;
	result_value=0x0;
	track1_num=SIZE1_DATA_TRK1;
	track2_num=SIZE1_DATA_TRK2;
	track3_num=SIZE2_DATA_TRK3;
	if(direction == FORWARD)
	{	
		if(!(track1_num = convert_forward((char *)data_trk1, track1_num,0x01,7)))
		      result_value |= NO_TRACK1;
		if(!(track2_num = convert_forward((char *)data_trk2, track2_num,0x01,5)))
		      result_value |= NO_TRACK2;
		if(!(track3_num = convert_forward((char *)data_trk3, track3_num,0x01,5)))
		      result_value |= NO_TRACK3;
	}
	else if(direction == BACK)
	{	
		if(!(track1_num = convert_back((char *)data_trk1, track1_num,0x01,7)))
		      result_value |= NO_TRACK1;
		if(!(track2_num = convert_back((char *)data_trk2, track2_num,0x01,5)))
		      result_value |= NO_TRACK2;
		if(!(track3_num = convert_back((char *)data_trk3, track3_num,0x01,5)))
		      result_value |= NO_TRACK3;
	}
//	for(i = 2 ; i< 5 ; i++)
//	{
	//	if((result_value & (1 << i) != 0)){
	//	      sdebug(card_num1,"%s","Read error");
	//	}
	//	error=0;
		ShowTrack1(card_num1);
		ShowTrack2(card_num2);
		ShowTrack3(card_num3);
//	}
	debug("First Track Data:%s\n",card_num1);
	debug("Second Track Data:%s\n",card_num2);
	debug("Third Track Data:%s\n",card_num3);	
	result_value |= direction;
	return result_value;
}

void covert_data(char *buf)
{
	int i;
	unsigned char	data1[SIZE1_DATA_TRK1];   				//轨2 原始数据    第2磁道数据编码最大记录长度为40个字符
	unsigned char	data2[SIZE1_DATA_TRK2];   				//轨2 原始数据    第2磁道数据编码最大记录长度为40个字符
	unsigned char	data3[SIZE2_DATA_TRK3];    				//轨3 原始数据    第3磁道数据编码最大记录长度为107个字符

	dstate = FORWARD;
	for(i=0;i<704;i++)
	      data_trk1[i]=buf[i];
#if 0
	i=0;
	while(data_trk1[i]!=0x01)
	{
		data_trk1[i]=0x01;
		i++;
		if(i>752) break;
	}
	i=752;
	while(data_trk1[i]!=0x01)
	{
		data_trk1[i]=0x01;
		i--;
		if(i<0) break;
	}
#endif


	for(i=704;i<704+704;i++)
	      data_trk2[i-704]=buf[i];
#if 0
	i=0;
	while(data_trk2[i]!=0x01)
	{
		data_trk2[i]=0x01;
		i++;
		if(i>299) break;
	}
	i=299;
	while(data_trk2[i]!=0x01)
	{
		data_trk2[i]=0x01;
		i--;
		if(i<0) break;
	}
#endif
	for(i=704+704;i<704+704+704;i++)
	      data_trk3[i-704-704]=buf[i];
#if 0
	i=0;
	while(data_trk3[i]!=0x01)
	{
		data_trk3[i]=0x01;
		i++;
		if(i>734) break;
	}
	i=734;
	while(data_trk3[i]!=0x01)
	{
		data_trk3[i]=0x01;
		i--;
		if(i<0) break;
	}
#endif
	for(i=0;i<704;i++)
	      data1[i]=data_trk1[i];
	for(i=0;i<704;i++)
	      data2[i]=data_trk2[i];
	for(i=0;i<704;i++)
	      data3[i]=data_trk3[i];
	convert_track(FORWARD); 
	memset(str4,0x00,20);
	for(i=1;i<strlen(card_num2);i++)
	{
		if(card_num2[i]!='=')
		      str4[i-1]=card_num2[i];
		else
		      break;
	}
	dstate = BACK;
	if(str4[0]!=0x00 || dstate == FORWARD);
	else {
		for(i=0;i<704;i++)
		      data_trk1[i]=data1[i];

		for(i=0;i<7;i++)
		      data_trk2[i]=0x1;
		for(i=0;i<704;i++)
		      data_trk2[i+7]=data2[i];

		for(i=0;i<7;i++)
		      data_trk3[i]=0x1;
		for(i=0;i<704;i++)
		      data_trk3[i+7]=data3[i];

		convert_track(BACK);
		memset(str4,0x00,20);
		for(i=1;i<strlen(card_num2);i++)
		{
			if(card_num2[i]!='=')
			      str4[i-1]=card_num2[i];
			else
			      break;
		}
		if(str4[0]!=0x00 || dstate == BACK);
		else
		      debug("BRUSH ERROR\n");
	}
	debug("brush card is %s\n",str4);
}

/*******************************************************************************************
  函数名称	:	mg_test
  函数功能	:	测试mg卡
  入口参数	:	无
  出口参数	:	无
  函数返回	:	成功:TRUE 失败:FALSE
  其它说明	:	无
 *******************************************************************************************/
int mg_test(int fd,int key_fd)
{
	int num=0,count=0,timeout=10,buff[3]={0};
	char buf[2112],a;

	int i,j,k=0;	
	struct timeval begintime, endtime;

	gettimeofday(&begintime, NULL);//读取系统时间      
	memset(buf,0x00,2112);
	bzero(str4,20);
	bzero(data_trk1,SIZE1_DATA_TRK1);
	bzero(data_trk2,SIZE1_DATA_TRK2);
	bzero(data_trk3,SIZE2_DATA_TRK3);
	bzero(card_num1,SIZE3_CARD_NUM1);
	bzero(card_num2,SIZE3_CARD_NUM2);
	bzero(card_num3,SIZE4_CARD_NUM3);

	while(1)
	{
		count++;
		gettimeofday(&endtime, NULL );//读取系统时间 

		buf[0] = 0;
		buf[1] = 99;
		read(key_fd, (char *) buff, 2);
		
		if(read(fd,buf,2112)!=-1)
		{
			for(i=0;i<2112;i++)
			      debug("%x",buf[i]);
			covert_data(buf);
			return 0;
		}
		if ((buff[0] == 1) && (buff[1]== 5)) 
		{
			ioctl(key_fd,0,0);
			show_function(6);
			close(fd);
			return 2;
		}
		if((endtime.tv_sec-begintime.tv_sec) > timeout)
		{
			debug("\n\nCharge overtime!\n\n");
			close(fd);
			return 3;
			break;
		}
	}
}




