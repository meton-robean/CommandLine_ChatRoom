#include "chat.h"
int socketfd;  //套接字描述符
int client_ws_connfd; //客户端和网页链接websocket 句柄。全局变量； 在void inits_client_as_WSserver()中使用了。
//菜单提示的数据结构   
typedef struct {
	char cmd[40];			//format
	char explain[100];//description
	int  admin;				//authority
}usage;
int global_admin_flag=NORMAL_USER;   //用户的权限标记，默认为0
int globel_is_shutup;   						//用户是否被禁言的标记 ,默认为0
  
//0为普通用户具有的执行权限 ，1为管理员具有的执行权限 。
 usage help_menu[] = {
	{"format", 			        "\tdescription",0},
	{">信息", 			      "\t\t与所有在线用户聊天",0},
	{":用户名>信息",		    "\t选择用户聊天",0},
	{"--online",            "\t显示在线用户",0},
	{"--help" ,              "\t\t显示帮助信息",0},
	{"exit",		            "\t\t退出",0},
	{"#kick:user",          "\t用户下线",1},
	{"#shutup:user",        "\t禁止用户发言，5分钟后可自行解除禁止", 1},
	{"#advanded:user",	    "\t提升用户为管理员", 1},
	{"#normal:user", 	      "\t降级管理员为普通用户", 1},
	{0,0,0}
}; 

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc ,char *argv[])
{
    signal(SIGINT, signHandler); 
	inits(atoi(argv[1]));//初始化socket，挂起消息提醒的线程pthread_fun，运行处理用户输入字符串的write_data()函数
	close(socketfd);
	return 0;
}
//初始化client端的链接
void  inits(int port)
{

	//创建client端和server端的socket链接。
	struct sockaddr_in   server;
	if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
         perror("creat socket error\n");
         exit(1);
    }
	bzero(&server, sizeof(server));
	//memset(&server,0,sizeof(server));
	// bzero(&(server.sin_zero),8);  
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT);
	inet_pton(AF_INET,IP,&server.sin_addr);
	if(( connect(socketfd,(struct sockaddr *)&server,sizeof(struct sockaddr)))==-1)
	{
         perror("connect error\n");
         exit(1);
    } 
	reg_log(); //    login or register
	pthread_t tid;//线程标识符 pthread_t 
	if((pthread_create(&tid,NULL,pthread_fun,&socketfd))==-1)//建立了客户端的接收线程以及线程相应处理函数
	{
		perror("pthread_create error\n");
		exit(1);
	}
     
	inits_client_as_WSserver(port);//建立网页端和client端的ws链接
}

//建立网页端和client端的ws（websocket）链接，并把网页端数据经由client端发给server端。
void inits_client_as_WSserver(int port )
{
    struct sockaddr_in servaddr, cliaddr;  
    socklen_t cliaddr_len;  
    int listenfd, connfd; 
    char buf[REQUEST_LEN_MAX];  
    char *data;  
    char str[INET_ADDRSTRLEN];  
    char *secWebSocketKey;  
    int i,n;  
    int connected=0;//0:not connect.1:connected.  
    //int port= DEFEULT_SERVER_WS_PORT; 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);  
  
    bzero(&servaddr, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //ip addr
    servaddr.sin_port = htons(port);  //port 
      
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));  
  
    listen(listenfd, 20);  
  
    printf("Listen %d\nAccepting connections ...\n",port);  
    cliaddr_len = sizeof(cliaddr);  
    client_ws_connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);  
    printf("From %s at PORT %d\n",  
               inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),  
               ntohs(cliaddr.sin_port));

    while (1)  
      {  
      
        memset(buf,0,REQUEST_LEN_MAX);  
        n = read(client_ws_connfd, buf, REQUEST_LEN_MAX);   
        printf("---------------------\n");  
      
      
        if(0==connected)  
          {  
            printf("read:%d\n%s\n",n,buf);  
            secWebSocketKey=computeAcceptKey(buf);    
            shakeHand(client_ws_connfd,secWebSocketKey);  
            connected=1;  
            continue;  
          }  
  
        data=analyData(buf,n);  
        writedata(data);//把网页端的数据发给server端
        //response(client_ws_connfd,data);
    }  
    close(connfd);           
}

//显示菜单项
void show_menu()
{
	int i = 0;
	printf("*********************************************\n");
	for(; help_menu[i].cmd[0] != 0; i++)
	{
		  if(global_admin_flag==ADMIN_USER) //管理员
		  	printf("*\t%s\t\t%s\n", help_menu[i].cmd, help_menu[i].explain);
		  else if(help_menu[i].admin==NORMAL_USER)//普通用户
		   	printf("*\t%s\t\t%s\n", help_menu[i].cmd, help_menu[i].explain);
	}
	printf("********************************************\n");
}
//进行选择注册或登陆 。
void reg_log()
{
	char ch;
	printf("1：注册;\t2：登录\n");
	printf("请选择:");
	while(1)
	{
		ch=getchar();
		if(ch=='2') //2代表登录
		{
			int get_ret=0;
  		while(1)
			{ 
				login();//输入用户密码，检测是否符合输入格式要求，接着将用户密码写入一个send_info结构体，发送给服务器
				read(socketfd,&get_ret,sizeof(get_ret));//读服务器的响应
				if(get_ret==NORMAL_USER_LOGIN_SUCCESS)//1
				{
					printf("用户您已成功登录。欢迎您！\n\n");
					global_admin_flag =NORMAL_USER;//0普通用户
			    	show_menu();//显示菜单
					break;
				}
				else if(get_ret==ADMIN_LOGIN_SUCCESS)//3
				{ 
					printf("管理员您已成功登录。欢迎您！\n\n");
					global_admin_flag =ADMIN_USER;//1管理员
			    	show_menu();
					break;
				}
				else if(get_ret==NORMAL_USER_LOGIN_FAILED_ONLINE)//2
                {
					printf("登录失败，该用户在线!\n");
                    exit(EXIT_SUCCESS);
					break;
                }
                else if(get_ret==NORMAL_USER_LOGIN_PASSWD_ERROR) //4
          	        printf("密码错误，请重新登录！\n");
                else//0
					printf("用户名错误，请重新登录！\n");
			}
			break;
		}
		else if(ch=='1')
		{
			 int get_ret=0;
			 while(1)
			{
				register_client();
				read(socketfd,&get_ret,sizeof(int));
			  if(get_ret==REGIST_EXITED)//账号已经存在 1
           		printf("该用户已存在，请重新输入！\n ");
 				else if(get_ret==REGIST_FALIED)//注册失败 0
					 printf("注册失败，请重新输入!\n ");
				else 
				{
					printf("注册成功,帐号ID为：%d ,请登陆。\n ",get_ret);
					exit(EXIT_SUCCESS);//第一次注册成功后，客户端程序自动离开，我们需要重新登录	
					break;
				}
			}
			break;
		}
		else
		{
			printf("输入错误，请重新选择.\n");
		}
		//清空输入流.
		for(  ;   (ch=getchar())!='\n'  && ch !=EOF; )
			continue;
	}      
}

//判断用户名是否输入了非法字符eg:空格，标点或特殊符号
void isvalid (char   *str)
{
   while(1)
   { 
         scanf("%s",str);
         int  i=0 ,flag=-1;
 			   for(i=0;str[i]!=0;i++)
			   { 
			         if(ispunct(str[i]))
                {
  		          	 flag=i;
  		         		 break;		   
  		         }
		     }
		      if (flag!=-1)
		      {
		        printf("对不起，您输入了非法字符，请重新输入！\n");
		    	  bzero(str,sizeof(str));
		    	}
		    	else break;
 	}
}
//注册新用户
void register_client()
{
	pthread_mutex_lock(&mutex);
	send_info  send;
	printf("用户名：\n");
	isvalid (send.name);
 	printf("密码  ：\n");
	isvalid (send.passwd);
	send.type=CLIENT_REGISTER;
	write(socketfd,&send,sizeof(send));
	pthread_mutex_unlock(&mutex);
} 
//登陆
void  login()
{
	send_info  send;
	printf("用户名："); 
  isvalid (send.name);//输入用户名，并且检测是否符合输入要求的格式
 	printf("密码  ：");
  isvalid (send.passwd);
  send.type=CLIENT_LOGIN;
  write(socketfd,&send,sizeof(send));

}
// 接收数据
void *pthread_fun(void *arg)
{
	char buf[BUF_SIZE]={0};
	char *data2web;
	int length=0;
	while(1)
	{
		length=read(socketfd,buf,BUF_SIZE);
		if(length<=0)
		{
			printf("服务器已关闭!\n");
			sprintf(data2web,"服务器已关闭!\n");
			response(client_ws_connfd,data2web);
			exit(EXIT_SUCCESS);	
			break;
		}
		if(strcmp("exit",buf)==0)
		{
		    printf("您被管理员踢下线了,请遵守聊天规则.\n");
		    sprintf(data2web,"您被管理员踢下线了,请遵守聊天规则.\n");
		    response(client_ws_connfd,data2web);
		    exit(EXIT_SUCCESS);	
		    break;
	    }
	  else if(strcmp("shutup",buf)==0)
       {
           printf("您被管理员禁言了,5分钟后可发言,请遵守聊天规则.\n");
           sprintf(data2web,"您被管理员禁言了,5分钟后可发言,请遵守聊天规则.\n");
           response(client_ws_connfd,data2web);
           globel_is_shutup =1;
           continue;
       }
     else if(strcmp("advanced",buf)==0)
       {
           printf("您被管理员提升为管理员.\n");
           sprintf(data2web,"您被管理员提升为管理员.\n");
           response(client_ws_connfd,data2web);
           global_admin_flag =ADMIN_USER;//1管理员
           show_menu();
           continue;
    	}
    else if(strcmp("normal",buf)==0)
    	{
           printf("您被管理员降为普通用户.\n");
           sprintf(data2web,"您被管理员降为普通用户.\n");
           response(client_ws_connfd,data2web);
           global_admin_flag =NORMAL_USER;//0普通用户  
           show_menu();
           continue;
    	}
    else if(strcmp("self",buf)==0)
    	{
           printf("您不能对把自己踢下线，禁言，降为普通用户.\n");  
           sprintf(data2web,"您不能对把自己踢下线，禁言，降为普通用户.\n");
           response(client_ws_connfd,data2web);
           continue;
    	} 
  		printf("%s\n",buf);//输出聊天信息
  		sprintf(data2web,buf);
  		response(client_ws_connfd,data2web);
		memset(buf,0,sizeof(buf)); // clear  buf
	}
	//while循环结尾
	close(socketfd);
	pthread_exit(NULL);
	exit(EXIT_SUCCESS);	
}

//打印错误信息
void  print_err(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

//解析字符串，包装填充数据结构
void parse_input_buf(char *src,  send_info  *send)
{
  switch(src[0]){
	case '>' : 
		 send->type=PUBLIC_CHAT;
		 strcpy(send->buf,src+1);
		 break;
	case ':' :
	    strcpy(send->id,strtok(src+1,">"));//id 为用户名
		send->type=PRIVATE_CHAT; 
		strcpy(send->buf,strtok(NULL,">"));//代表聊天内容的字符串
		 break;
  case '-' :
       if(strcmp(src,"--online")==0)
          send->type=CLIENT_ONLINE;  
       else if(strcmp(src,"--help")==0)
          show_menu();
  		 break;
  case '#':
		   if(global_admin_flag==ADMIN_USER)//必须是管理员才可得到执行
		    {
		    	  char  *tok_buf[2];
		     	  tok_buf[0]=strtok(src,":");
		        tok_buf[1]=strtok(NULL,":");
		       	//DEBUG("tok_buf[0] %s\n",tok_buf[0]);
		        //DEBUG("tok_buf[1] %s\n",tok_buf[1]);
   		      if(strcmp(tok_buf[0],"#kick")==0)
		              send->type=ADMIN_KICK_CLIENT;
		        else if(strcmp(tok_buf[0],"#shutup")==0)
                  send->type=ADMIN_SHUTUP_CLIENT;  
            else if(strcmp(tok_buf[0],"#advanded")==0)
                  send->type=ADVANCED_CLIENT_TO_ADMIN;  
            else if(strcmp(tok_buf[0],"#normal")==0)
                 send->type=DROP_CLIENT_TO_NORMAL;  
            strcpy(send->id,tok_buf[1]);   
 		    }
		   break;		 
	default :
	   send->type=0;
		 strcpy(send->buf,src); 
		 break;
	}
} 
//禁言后需等待的时间 处理函数,5分钟
void wait_minutes()
{
     sleep(5*60);
     globel_is_shutup =0;
}
//发送数据给server端
void writedata(char *data)
{
  		char buf[BUF_SIZE]={'\0'};
  		send_info  send; 
  		//buf=data;
  		memcpy(buf,data,sizeof(buf));
		parse_input_buf(buf, &send); //解析client输入的字符串，放到send_info send结构体中
		//if(strcmp("--help",buf)==0) continue;	
		if(!globel_is_shutup)//如果该用户没有被禁言
    		write(socketfd, &send, sizeof(send));
    	else 
			wait_minutes ();//等待5分钟
	  	if(strcmp("exit",buf)==0) 
	  	{   
	  		close(socketfd);
	  	    exit(EXIT_SUCCESS);
	  	}
		memset(buf,0,sizeof(buf));//每一回合都把buf清掉
}

//忽略ctrl +c 键的处理函数
void signHandler(int signNo)
{
		DEBUG("singal:%d \n",signNo);
    	printf("退出聊天室请按输入 exit ，谢谢.\n");
}

/*------------------------------websocket 相关函数------------------------------------------------*/

//取出握手报文的websocket 的key
char * fetchSecKey(const char * buf)  
{  
  char *key;  
  char *keyBegin;  
  char *flag="Sec-WebSocket-Key: ";  
  int i=0, bufLen=0;  
  
  key=(char *)malloc(WEB_SOCKET_KEY_LEN_MAX);  
  memset(key,0, WEB_SOCKET_KEY_LEN_MAX);  
  if(!buf)  
    {  
      return NULL;  
    }  
   
  keyBegin=strstr(buf,flag);  
  if(!keyBegin)  
    {  
      return NULL;  
    }  
  keyBegin+=strlen(flag);  
  
  bufLen=strlen(buf);  
  for(i=0;i<bufLen;i++)  
    {  
      if(keyBegin[i]==0x0A||keyBegin[i]==0x0D)  
    {  
      break;  
    }  
      key[i]=keyBegin[i];  
    }  
    
  return key;  
}  
 

//计算 AcceptKey
char * computeAcceptKey(const char * buf)  
{  
  char * clientKey;  
  char * serverKey;   
  char * sha1DataTemp;  
  char * sha1Data;  
  short temp;  
  int i,n;  
  const char * GUID="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";  
   
  
  if(!buf)  
    {  
      return NULL;  
    }  
  clientKey=(char *)malloc(LINE_MAX);  
  memset(clientKey,0,LINE_MAX);  
  clientKey=fetchSecKey(buf);  
   
  if(!clientKey)  
    {  
      return NULL;  
    }  
  
   
  strcat(clientKey,GUID);  
  
  sha1DataTemp=sha1_hash(clientKey);  
  n=strlen(sha1DataTemp);  
  
  
  sha1Data=(char *)malloc(n/2+1);  
  memset(sha1Data,0,n/2+1);  
   
  for(i=0;i<n;i+=2)  
    {        
      sha1Data[i/2]=htoi(sha1DataTemp,i,2);      
    }   
  
  serverKey = base64_encode(sha1Data, strlen(sha1Data));   
  
  return serverKey;  
}  
 
//握手
void shakeHand(int connfd,const char *serverKey)  
{  
  char responseHeader [RESPONSE_HEADER_LEN_MAX];  
  
  if(!connfd)  
    {  
      return;  
    }  
  
  if(!serverKey)  
    {  
      return;  
    }  
  
  memset(responseHeader,'\0',RESPONSE_HEADER_LEN_MAX);  
  
  sprintf(responseHeader, "HTTP/1.1 101 Switching Protocols\r\n");  
  sprintf(responseHeader, "%sUpgrade: websocket\r\n", responseHeader);  
  sprintf(responseHeader, "%sConnection: Upgrade\r\n", responseHeader);  
  sprintf(responseHeader, "%sSec-WebSocket-Accept: %s\r\n\r\n", responseHeader, serverKey);  
   
  printf("Response Header:%s\n",responseHeader);  
  
  write(connfd,responseHeader,strlen(responseHeader));  
}


//取出websock报文的data payload 
char * analyData(const char * buf,const int bufLen)  
{  
  char * data;  
  char fin, maskFlag,masks[4];  
  char * payloadData;  
  char temp[8];  
  unsigned long n, payloadLen=0;  
  unsigned short usLen=0;  
  int i=0;   
  
  
 if (bufLen < 2)   
   {  
     return NULL;  
   }  
  
  fin = (buf[0] & 0x80) == 0x80; // 1bit，1表示最后一帧    
  if (!fin)  
   {  
       return NULL;// 超过一帧暂不处理   
   }  
  
   maskFlag = (buf[1] & 0x80) == 0x80; // 是否包含掩码    
   if (!maskFlag)  
   {  
       return NULL;// 不包含掩码的暂不处理  
   }  
  
   payloadLen = buf[1] & 0x7F; // 数据长度   
   if (payloadLen == 126)  
   {        
     memcpy(masks,buf+4, 4);        
     payloadLen =(buf[2]&0xFF) << 8 | (buf[3]&0xFF);    
     payloadData=(char *)malloc(payloadLen);  
     memset(payloadData,0,payloadLen);  
     memcpy(payloadData,buf+8,payloadLen);  
    }  
    else if (payloadLen == 127)  
    {  
     memcpy(masks,buf+10,4);    
     for ( i = 0; i < 8; i++)  
     {  
         temp[i] = buf[9 - i];  
     }   
  
     memcpy(&n,temp,8);    
     payloadData=(char *)malloc(n);   
     memset(payloadData,0,n);   
     memcpy(payloadData,buf+14,n);//toggle error(core dumped) if data is too long.  
     payloadLen=n;      
     }  
     else  
     {     
      memcpy(masks,buf+2,4);      
      payloadData=(char *)malloc(payloadLen);  
      memset(payloadData,0,payloadLen);  
      memcpy(payloadData,buf+6,payloadLen);   
     }  
  
     for (i = 0; i < payloadLen; i++)  
     {  
       payloadData[i] = (char)(payloadData[i] ^ masks[i % 4]);  
     }  
   
     printf("data(%d):%s\n",payloadLen,payloadData);  
     return payloadData;  
}  
  
char *  packData(const char * message,unsigned long * len)  
 {  
         char * data=NULL;  
     unsigned long n;  
  
     n=strlen(message);  
            if (n < 126)  
            {  
          data=(char *)malloc(n+2);  
          memset(data,0,n+2);      
          data[0] = 0x81;  
          data[1] = n;  
          memcpy(data+2,message,n);  
          *len=n+2;  
            }  
            else if (n < 0xFFFF)  
            {  
          data=(char *)malloc(n+4);  
          memset(data,0,n+4);  
          data[0] = 0x81;  
          data[1] = 126;  
          data[2] = (n>>8 & 0xFF);  
          data[3] = (n & 0xFF);  
          memcpy(data+4,message,n);      
          *len=n+4;  
            }  
            else  
            {  
       
                // 暂不处理超长内容    
          *len=0;  
            }  
    
  
        return data;  
 }  
  
void response(int connfd,const char * message)  
{  
  char * data;  
  unsigned long n=0;  
  int i;  
  if(!connfd)  
    {  
      return;  
    }  
  
  if(!data)  
    {  
      return;  
    }  
  data=packData(message,&n);   
   
  if(!data||n<=0)  
    {  
      printf("data is empty!\n");  
      return;  
    }   
   
  write(connfd,data,n);  
    
} 


