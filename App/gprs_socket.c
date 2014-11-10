#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>

#include <netdb.h>

/*******************************************************************************************
函数名称	:	ConnectToServer
函数功能	:	连接服务器
入口参数	:	
	recv_data:获得数据
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/
static int ConnectToServer (char *recv_data)//连接到服务器并接受返回	2009-06-11 15:09:57
{
    int sockfd, numbytes; 															// 套接字描述符
    int flags = 0;
    struct sockaddr_in addr;
	int i,j,time = 0;
	struct timeval tv;	
    fd_set readfds,writefds;														//利用select函数来对进来的数据进行捕捉
	int ret;
	char *send_message ="GET /protest__1.php?test=prochip123 HTTP/1.1\nHOST:bbs.seu.edu.cn\n\n";
	char *temp_data=NULL;
	memset (&addr, 0, sizeof(struct sockaddr_in));        
	addr.sin_addr.s_addr = inet_addr("58.192.114.8");//("111.68.9.66");        
	addr.sin_family = AF_INET;        
	addr.sin_port = htons (80);
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)						//创建数据流套接字
	{
		perror("socket");
		exit(1);
	}
    flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags); //设置非阻塞 
	if(connect(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr))==-1)	//连接
	{
        FD_ZERO(&writefds);
        FD_SET( sockfd, &writefds );
        tv.tv_sec = 15;																	//设置超时时间
        tv.tv_usec = 0;
        ret = select(sockfd+1,NULL,&writefds,NULL,&tv);
        if(ret==0)
        {
            close(sockfd);			
            return -1;
        }
        else
        {	
            flags = fcntl(sockfd, F_GETFL, 0);
            flags &= ~O_NONBLOCK;
            fcntl(sockfd, F_SETFL, flags); //设置阻塞          
        }
	}
    FD_ZERO(&readfds);
	FD_SET(sockfd,&readfds);
	if (send(sockfd, send_message, strlen(send_message), 0) == -1)							//发送消息
	{
		perror("send");
		close(sockfd);
		return -1;
	}
	sleep(3);
	memset(recv_data,0,500);
	temp_data = recv_data;

	tv.tv_sec = 3;																	//设置超时时间
	tv.tv_usec = 500000;
	ret = select(sockfd+1,&readfds,NULL,NULL,&tv);
	if(FD_ISSET(sockfd,&readfds))
	{
		if ((recv(sockfd,temp_data,501,0)) == -1)
		{
			perror("recv");
			close(sockfd);
			return -1;
		}
	}
	else
	{	
		return -1;
	}
	close(sockfd);																			//关闭套接字
	return 0;
}

/*******************************************************************************************
函数名称	:	ExtractData
函数功能	:	获取数据
入口参数	:	
	sou:缓存
	des:缓存
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/
static int ExtractData(char *sou, char *des)				//提取有用的数据
{
	char *pointer = NULL;
	int j;
	
	pointer = strstr(sou,"string(10)");
	if (pointer != NULL)
	{
		pointer += 10;
		strcpy(des, pointer);
		
		j = 0;
				
		while(des[j]!='\n' && des[j]!='\r' && des[j] != '\0')
		   	j++;
		des[j] = '\0';
					
	}
}

/*******************************************************************************************
函数名称	:	gprs_test
函数功能	:	测试GPRS功能
入口参数	:	
	getmsg:缓存
出口参数	:	无
函数返回	:	成功:TRUE 失败:FALSE
其它说明	:	无
*******************************************************************************************/
int gprs_test(char *getmsg)
{
	int ret = -1;
	char recvdate[500] = {0,}; 
	ret=ConnectToServer(recvdate);
	ExtractData(recvdate, getmsg);	
	return 0;
}
