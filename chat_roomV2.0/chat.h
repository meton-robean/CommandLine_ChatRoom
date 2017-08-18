#ifndef CHAT_H
#define CHAT_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
//#include<linux/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<signal.h>
#include<time.h>		 //for asctime() and time()
#include<ctype.h>    //for ispunct()
 

#define QUEUELEN     100  //max of  client
#define BUF_SIZE     256  
#define IP        "127.0.0.1"
#define PORT         1234             // port
#define FILENAME     "userinfo"       // file name for save users info（name:passwd:id:authority） 
#define CLIENT_ID    1000    // client id 
//========================functions================================================
#define PUBLIC_CHAT        			1  //群聊
#define PRIVATE_CHAT       			2  //私聊
#define CLIENT_LOGIN       			3  //登陆验证
#define CLIENT_REGISTER    			4  //注册
#define CLIENT_ONLINE      			5  //所用的在线用户
#define CLIENT_EXIT        			6  //退出
#define ADMIN_KICK_CLIENT       7  //管理员踢出某用户
#define ADMIN_SHUTUP_CLIENT     8  //管理员禁止某用户聊天
#define ADVANCED_CLIENT_TO_ADMIN   9  //管理员提升某用户为管理员
#define DROP_CLIENT_TO_NORMAL    10  //管理员把某用户降为普通用户

//====================================================================
#define NORMAL_USER_LOGIN_FAILED  			 0 //普通用户登录失败
#define NORMAL_USER_LOGIN_SUCCESS 			 1 //普通用户登录成功
#define NORMAL_USER_LOGIN_FAILED_ONLINE  2 //普通用户已经在线
#define ADMIN_LOGIN_SUCCESS              3 //管理员登录成功
#define NORMAL_USER_LOGIN_PASSWD_ERROR   4 //普通用户登录密码错误

#define REGIST_FALIED   0   //注册失败
#define REGIST_EXITED   1   //注册的用户已经存在
#define NORMAL_USER     0   //普通用户
#define ADMIN_USER      1   //管理员

// ===========client attr==========================================
typedef struct{
	pthread_t tid;					//线程的描述符,unsigned long int ,printf用%lu
	int  sockfd;  					//accept的返回的客户端的新的套接字描述符
	char client_name[25]; 	// 账号
	char client_passwd[25]; //密码
	int client_id;					//用户ID
  	int is_online;					// 在线状态 1 在线 0 不在线
  	int admin;              //用户权限，1为管理员，0为普通用户
}client_info;
client_info clients[QUEUELEN];

//===========客户发送的数据结构=====================================
 typedef struct send_info{
	int  type;						//类型
	char id[25];  				//对方id
	char buf[BUF_SIZE]; 	//内容
	char name[25];				//用户名（昵称）
	char passwd[25];			//密码
	}send_info;
//================函数功能的协议====================================
typedef struct {
     int  fun_flag; //function flag
     void (*fun)();// function pointer variable
}proto;
//====================debug============================

#define CHAT_DEBUG
#ifdef  CHAT_DEBUG
#define DEBUG(message...) fprintf(stderr, message)
#else
#define DEBUG(message...)
#endif

// ========fun=========client.c====================
void  print_err(char *msg);
void  reg_log();
void  register_client();
void  login();
void  inits();
void  writedata();
void  show_menu();
void  wait_minutes();
void  signHandler(int signNo);
void *pthread_fun(void *arg);

//  ======fun=======server.c======================
int 	 system_init();
void   connect_to_client(int socketfd );
void   err(char *err_msg);
int    init_clents(char  *tok_file[]);
void   register_new_client(send_info *send,int newfd);
void   server_check_login(send_info *send,int newfd);
void   client_exit(send_info *send,int newfd);
int    get_sockfd(char dest[]);
void   private_chat ( send_info *send,int newfd);
void   public_chat (send_info *send ,int newfd);
void   get_all_online_clients (send_info *send ,int newfd);
void   admin_kick_client (send_info *send,int newfd);
void   admin_shutup_client(send_info *send,int newfd);
void   advanced_client_to_admin (send_info *send,int newfd);
void   drop_client_to_normal (send_info *send,int newfd) ;
int    admin_is_opt_self(send_info *send,int newfd);
int    admin_opt_self(send_info *send,int newfd);

//void public_chatroom_fun(void *arg);
void public_chatroom_inits(int socketfd);
void public_chat_V2( send_info *send,int newfd);
#endif 
