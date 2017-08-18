#include "chat.h"
int socketfd;  //套接字描述符
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
	inits();
	close(socketfd);
	return 0;
}
//初始化链接
void  inits()
{
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
	if((pthread_create(&tid,NULL,pthread_fun,&socketfd))==-1)
		/*int pthread_create (pthread_t * thread_id， 					__const pthread_attr_t * __attr,
 					void *(*__start_routine) (void *),
					void *__restrict __arg)
					线程标识符 pthread_t 
第一个参数为指向线程标识符的指针；
第二个参数用来设置线程属性，若取NULL，则生成默认属性的子线程；
第三个参数是线程运行函数的起始地址，该函数是线程体函数，即子线程将完成的工作；
第四个参数用来设定线程体函数的参数 ；若函数体不需要参数，则最后一个参数设为NULL。
*/
		print_err("client pthread_create() error");		
   	writedata();
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
		if(ch=='2')
		{
			int get_ret=0;
  		while(1)
			{ 
				login();
				read(socketfd,&get_ret,sizeof(get_ret));
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
					exit(EXIT_SUCCESS);	
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
  isvalid (send.name);
 	printf("密码  ：");
  isvalid (send.passwd);
  send.type=CLIENT_LOGIN;
	write(socketfd,&send,sizeof(send));

}
// 接收数据
void *pthread_fun(void *arg)
{
	char buf[BUF_SIZE]={0};
	int length=0;
	while(1)
	{
		length=read(socketfd,buf,BUF_SIZE);
		if(length<=0)
		{
			printf("服务器已关闭!\n");
			exit(EXIT_SUCCESS);	
			break;
		}
		if(strcmp("exit",buf)==0)
		{
		    printf("您被管理员踢下线了,请遵守聊天规则.\n");
		    exit(EXIT_SUCCESS);	
		    break;
	  }
	  else if(strcmp("shutup",buf)==0)
    {
           printf("您被管理员禁言了,5分钟后可发言,请遵守聊天规则.\n");
           globel_is_shutup =1;
           continue;
    }
    else if(strcmp("advanced",buf)==0)
    {
           printf("您被管理员提升为管理员.\n");
           global_admin_flag =ADMIN_USER;//1管理员
           show_menu();
           continue;
    }
    else if(strcmp("normal",buf)==0)
    {
           printf("您被管理员降为普通用户.\n");
           global_admin_flag =NORMAL_USER;//0普通用户  
           show_menu();
           continue;
    }
    else if(strcmp("self",buf)==0)
    {
           printf("您不能对把自己踢下线，禁言，降为普通用户.\n");  
           continue;
    } 
  		printf("%s\n",buf);
		memset(buf,0,sizeof(buf)); // clear  buf
	}
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
	    strcpy(send->id,strtok(src+1,">"));
		  send->type=PRIVATE_CHAT; 
			strcpy(send->buf,strtok(NULL,">"));
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
//发送数据
void writedata()
{
  char buf[BUF_SIZE]={'\0'};
  send_info  send; 
 	while(1)
	{
		gets(buf); 
		parse_input_buf(buf, &send); 
		if(strcmp("--help",buf)==0) continue;	
		if(!globel_is_shutup)
    	write(socketfd, &send, sizeof(send));
    else 
			wait_minutes ();
	  if(strcmp("exit",buf)==0) 
	  	{   
	  		close(socketfd);
	  	  exit(EXIT_SUCCESS);
	  	}
		memset(buf,0,sizeof(buf));
	}
}

//忽略ctrl +c 键的处理函数
void signHandler(int signNo)
{
		DEBUG("singal:%d \n",signNo);
    printf("退出聊天室请按输入 exit ，谢谢.\n");
}

