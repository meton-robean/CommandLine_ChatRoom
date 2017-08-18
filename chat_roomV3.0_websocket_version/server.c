#include "chat.h"
proto p[]={
  {PUBLIC_CHAT,      public_chat}, 					 //广播
  {PRIVATE_CHAT,     private_chat},					 //私聊
  {CLIENT_LOGIN,     server_check_login},		 //登陆验证
  {CLIENT_REGISTER,  register_new_client},	 //注册
  {CLIENT_ONLINE,    get_all_online_clients},//获取所有的在线用户
  {CLIENT_EXIT,      client_exit},					 //客户退出
  {ADMIN_KICK_CLIENT, admin_kick_client},		 //管理员踢出某个用户
  {ADMIN_SHUTUP_CLIENT, admin_shutup_client},//管理员踢出禁言某个用户
  {ADVANCED_CLIENT_TO_ADMIN, advanced_client_to_admin},//管理员提升某用户为管理员
  {DROP_CLIENT_TO_NORMAL, drop_client_to_normal}, //管理员把某用户降为普通用户
  {0,0}
};

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;//互斥锁pthread_mutex_t

static int id=CLIENT_ID; //init  client id
int chatroom_fd;

int main(int argc ,char *argv[])
{
	int socketfd,newfd;
	socketfd=system_init();
	//public_chatroom_inits(socketfd);
	connect_to_client(socketfd);
	return 0;
}

void public_chatroom_inits(int socketfd){
	int newfd;
    struct sockaddr_in   acc_addr;
	socklen_t  len=sizeof(struct sockaddr_in);
	int i=0;
	pthread_t tid;

	if((chatroom_fd=accept(socketfd,(struct sockaddr *)&acc_addr,&len))==-1)
		err("server accept()   error");  
	printf("   %s请求连接。chatroom_fd=%d \n",inet_ntoa(acc_addr.sin_addr),chatroom_fd);
	printf("==公共聊天室开启==\n" );
	/*
	if((pthread_create(&tid,NULL,public_chatroom_fun,&chatroom_fd))==-1)
		err("server pthread_create() 2  error"); 
	printf("here\n");
	*/
}


void fun()
{ 
	int i=0;
	for(;i<QUEUELEN;i++)
	 {
	  	if(clients[i].tid!=0)
         DEBUG("=========%lu\t%d\t%s\t%s\t%d\t%d\t%d\t\n",clients[i].tid,clients[i].sockfd,clients[i].client_name,clients[i].client_passwd,clients[i].client_id,clients[i].is_online,clients[i].admin);
  	}
}
//管理员提升某用户为管理员
void advanced_client_to_admin (send_info *send,int newfd)
{
	if(admin_opt_self (send,newfd))  return;//自己提升，不允许
	pthread_mutex_lock(&mutex); //互斥锁
		
  char  *tok_file[5];
  int file_fd;
  char buf[BUF_SIZE]={0};
  char all_buf[BUF_SIZE]={0};
  char ret_buf[100]={0};
  char ret_buf_to_admin[100]={0};
  char save_buf[BUF_SIZE]={0};
	if((file_fd=open(FILENAME,O_RDWR,0644))==-1)
		 err("server_check_login open file  failed!");
	int len=0;
	int j=0;
	int flag=0;
    char  id_buf[5];
    char *p[5];	
  while(1)
  {
	 	  lseek(file_fd,len,SEEK_SET);
	    /*off_t lseek(int fd, off_t offset, int whence);
	   使用lseek（）函数根据whence及offset两个参量重新定位打开的文件文件描述符fd的偏移量。whence有以下取值：
       SEEK_SET    偏移量设置为offset字节。
       SEEK_CUR    偏移量设置为当前位置加上offset字节。
       SEEK_END    偏移量设置为文件大小加上偏移字节大小。
   	   lseek（）函数允许的文件偏移量超出了该文件末尾的（但这并不改变文件的大小）设置。
       成功完成后，lseek（）返回的结果是从文件开头的字节偏移位置。否则，返回-1并设置errno以指示错误。
		  */
			if(read(file_fd,buf,BUF_SIZE)==0) break;
			/*number = read(handle, buffer ,n)　；
上述read调用函数中，各个参数的定义如下：
handle： 这是一个已经打开的文件句柄，表示从这个文件句柄所代表的文件读取数据。
buffer： 指缓冲区，即读取的数据会被放到这个缓冲区中去。
n： 表示调用一次read操作，应该读多少数量的字符。
number：表示系统实际所读取的字符数量。 */
		  strcpy(save_buf,buf);
			//DEBUG("advanced_client_to_admin  strlen(buf)=%d\n ",strlen(buf));
		  //	DEBUG("advanced_client_to_admin  buf=%s\n",buf);
			len+=strlen(buf)+2;			
			j=0;
			tok_file[j++]=strtok(buf,":"); 
			while(tok_file[j++]=strtok(NULL,":"));
      	  int i=0;
      for(;i<QUEUELEN;i++)
      {//DEBUG(" atoi(send->id) =%d\n",atoi(send->id));
            if(strcmp(clients[i].client_name,send->id)==0){
            	sprintf(id_buf,"%d",clients[i].client_id);
            	p[2]=id_buf;
            	//printf("id_buf=%s,p[2]=%s\n",id_buf,p[2]);//#debug
            	break;
            }	
            
      } 
			if(strcmp(tok_file[2],id_buf)==0)
	    { 
	    	lseek(file_fd,len-strlen(save_buf)-2,SEEK_SET);
	     	sprintf(tok_file[3],"1");//将权限位改为1，并回写到文件中
	     	sprintf(all_buf,"%s:%s:%s:%s",tok_file[0],tok_file[1],tok_file[2],tok_file[3]);
	     //	DEBUG("advanced_client_to_admin  strlen(all_buf)=%d\n ",strlen(all_buf));
	     //	DEBUG("advanced_client_to_admin  all_buf=%s\n",all_buf);
		    write(file_fd,all_buf,strlen(all_buf)+1);
			  write(file_fd,"\n",1);// write a enter  write  length=2
	     	  flag =1;
	        break;
	    }
	    else 
           flag =0;	
     
 			memset(buf,0,sizeof(buf));
  } 
  memset(ret_buf,0,sizeof(ret_buf));

  if(flag==1)  
  {
  	sprintf(ret_buf ,"advanced");
  	sprintf(ret_buf_to_admin,"您对\tid=%s\t 的用户提升为管理员成功.",send->id);
  	write(get_sockfd(send->id),ret_buf,strlen(ret_buf)+1);
  	write(newfd,ret_buf_to_admin,strlen(ret_buf_to_admin)+1);
  }
  else
  {
   	  sprintf(ret_buf ,"没有\tid=%s\t的用户,请您核对后操作.",send->id); 
   	  write(newfd,ret_buf,strlen(ret_buf)+1);
  }
  close(file_fd);
  pthread_mutex_unlock(&mutex);  
}
//把某个管理员降为普通用户
void drop_client_to_normal (send_info *send,int newfd) 
{
  if(admin_opt_self (send,newfd))  return;
  pthread_mutex_lock(&mutex); 
		
  char  *tok_file[5];
  int file_fd;
  char buf[BUF_SIZE]={0};
  char all_buf[BUF_SIZE]={0};
  char ret_buf[100]={0};
  char ret_buf_to_admin[100]={0};
  char save_buf[BUF_SIZE]={0};
	if((file_fd=open(FILENAME,O_RDWR,0644))==-1)
		 err("server_check_login open file  failed!");
	int len=0;
	int j=0;
	int flag=0;
    char  id_buf[5];
    char *p[5];	
  while(1)
  {
	 	  lseek(file_fd,len,SEEK_SET);
			if(read(file_fd,buf,BUF_SIZE)==0) break;
		  strcpy(save_buf,buf);
			//DEBUG("advanced_client_to_admin  strlen(buf)=%d\n ",strlen(buf));
		  //	DEBUG("advanced_client_to_admin  buf=%s\n",buf);
			len+=strlen(buf)+2;			
			j=0;
			tok_file[j++]=strtok(buf,":"); 
			while(tok_file[j++]=strtok(NULL,":"));
      	int i=0;
      for(;i<QUEUELEN;i++)
      	{//DEBUG(" atoi(send->id) =%d\n",atoi(send->id));
            if(strcmp(clients[i].client_name,send->id)==0){
            	sprintf(id_buf,"%d",clients[i].client_id);
            	p[2]=id_buf;
            	//printf("id_buf=%s,p[2]=%s\n",id_buf,p[2]);//#debug
            	break;
            }	
            
      	} 
		if(strcmp(tok_file[2],id_buf)==0)
	    { 
	    	lseek(file_fd,len-strlen(save_buf)-2,SEEK_SET);
	     	sprintf(tok_file[3],"0");//将权限位改为0，并回写到文件中
	     	sprintf(all_buf,"%s:%s:%s:%s",tok_file[0],tok_file[1],tok_file[2],tok_file[3]);
	     //	DEBUG("advanced_client_to_admin  strlen(all_buf)=%d\n ",strlen(all_buf));
	     //	DEBUG("advanced_client_to_admin  all_buf=%s\n",all_buf);
		    write(file_fd,all_buf,strlen(all_buf)+1);
			  write(file_fd,"\n",1);// write a enter  write  length=2
	     	  flag =1;
	        break;
	    }
	    else 
           flag =0;	
     
 			memset(buf,0,sizeof(buf));
  } 
  memset(ret_buf,0,sizeof(ret_buf));
 // DEBUG("get_sockfd(send->id)=%d\n",get_sockfd(send->id));
  if(flag==1)  
  {
  	sprintf(ret_buf ,"normal");
  	sprintf(ret_buf_to_admin,"您对\tid=%s\t 的用户降为为普通用户成功.",send->id);
  	write(get_sockfd(send->id),ret_buf,strlen(ret_buf)+1);
  	write(newfd,ret_buf_to_admin,strlen(ret_buf_to_admin)+1);
  }
  else
  {
   	  sprintf(ret_buf ,"没有\tid=%s\t的用户,请您核对后操作.",send->id); 
   	  write(newfd,ret_buf,strlen(ret_buf)+1);
  }
  close(file_fd);
  pthread_mutex_unlock(&mutex); 
}
//判断是否是对自己进行踢下线，禁言，提升或降低权限
int  admin_is_opt_self(send_info *send,int newfd)
{//DEBUG("admin_is_opt_self   %s\n",send->id);
 //DEBUG("newfd=   %d\n",newfd);
 //DEBUG("get_sockfd(send->id)=   %d\n",get_sockfd(send->id));
  int i=0; 
	for(;i<QUEUELEN;i++)
	 {
	  	if(get_sockfd(send->id)==newfd)
     {
           return 1;
     }   
   }
    return 0;
 }
 //如果是对自己进行踢下线，禁言，提升或降低权限  ，则不作任何操作，并提示用户
int   admin_opt_self(send_info *send,int newfd)
{
       char self[100]={0};
 	     if(admin_is_opt_self(send,newfd))
	     {
	      	     sprintf (self,"self");
               write(newfd,self,strlen(self)+1);
               return  1; 
  	   }
  	   else  return 0;
  }
   // 踢出某人(强制某人下线)
void admin_kick_client (send_info *send,int newfd)
{//DEBUG("///////////////////%s\n",send->id);
	    //fun() ;
      if(admin_opt_self (send,newfd))  return;
   	  char  *p[5]; 
   	 
      // DEBUG("p[2]=%s\n",p[2]);
     int  kick_client_sockfd=0;
     char ret_admin_buf[100]={0};
     char ret_kicked_buf[10]="exit";//向被踢的用户发送的信息
      char  id_buf[5];
      int i=0;
      for(;i<QUEUELEN;i++)
      {//DEBUG(" atoi(send->id) =%d\n",atoi(send->id));
            if(strcmp(clients[i].client_name,send->id)==0){
            	sprintf(id_buf,"%d",clients[i].client_id);
            	p[2]=id_buf;
            	//printf("id_buf=%s,p[2]=%s\n",id_buf,p[2]);//#debug
            	break;
            }	
            
      } 
   	  if(check_online(p)==0)//if 踢出用户不在线则提示管理员
   	  {//DEBUG("//////0000000///\n");
   	      memset(ret_admin_buf,0,sizeof(ret_admin_buf));
          sprintf(ret_admin_buf,"该用户不在线或不存在，请用-online查看所有的在线用户,并确保存在,方能踢出用户.");
          write(newfd,ret_admin_buf,strlen(ret_admin_buf)+1);    
          return; 
   	  }
      if((kick_client_sockfd=get_sockfd(send->id))!=0)
     	{ 
     		memset(ret_admin_buf,0,sizeof(ret_admin_buf));
     		sprintf(ret_admin_buf,"成功将用户 : %s 踢下线",clients[i].client_name);
        	write(kick_client_sockfd,ret_kicked_buf,strlen(ret_kicked_buf)+1);
        	write(newfd,ret_admin_buf,strlen(ret_admin_buf)+1);
      }
      
 }
 //禁言某个客户,只是禁言不退出，能接受信息，在一段时间中不能发送信息 。
void admin_shutup_client(send_info *send,int newfd)
{ 
  if(admin_opt_self (send,newfd))  return;
      char  *p[5]; 
      p[2] =send->id; 
      int  shutup_client_sockfd=0;
      char ret_admin_buf[100]={0};
      char ret_shutup_client_buf[10]="shutup";//向被踢的用户发送的信息
      char  id_buf[5];
      int i=0;
      for(;i<QUEUELEN;i++)
      {//DEBUG(" atoi(send->id) =%d\n",atoi(send->id));
            if(strcmp(clients[i].client_name,send->id)==0){
            	sprintf(id_buf,"%d",clients[i].client_id);
            	p[2]=id_buf;
            	//printf("id_buf=%s,p[2]=%s\n",id_buf,p[2]);//#debug
            	break;
            }	
            
      } 
   	  if(check_online(p)==0)//if 被禁言用户不在线则提示管理员
   	  {
   	     	memset(ret_admin_buf,0,sizeof(ret_admin_buf));
          sprintf(ret_admin_buf,"该用户不在线或不存在，请用-online查看所有的在线用户,并确保存在,方能禁言用户.");
          write(newfd,ret_admin_buf,strlen(ret_admin_buf)+1);    
          return; 
   	 	 }
      
    if((shutup_client_sockfd=get_sockfd(send->id))!=0)
     	{ 
     		memset(ret_admin_buf,0,sizeof(ret_admin_buf));
     		sprintf(ret_admin_buf,"成功将用户 %s 禁言",clients[i].client_name);
        write(shutup_client_sockfd,ret_shutup_client_buf,strlen(ret_shutup_client_buf)+1);
        write(newfd,ret_admin_buf,strlen(ret_admin_buf)+1);
      }
}
//判断该用户是否在线
int check_online(char  *tok_file[]) // check client is online  true return 1 or return 0
{ //DEBUG("   check_online     tok_file[2]=%s\n",tok_file[2]);
	int i=0;
	for(i=0;i<QUEUELEN;i++)
	{
	if( ( clients[i].client_id==atoi(tok_file[2])) && clients[i].is_online==1  )
			return 1;//online 
	}
	//检查完所有在线用户后,则说明该用户没有在线  return 0!!!!!!!!!!!!! 
	 return 0;  // no online 
} 
//初始化 用户结构数组
int  init_clents(char  *tok_file[])
{	
	pthread_mutex_lock(&mutex);
	int i=0;
	if(check_online(tok_file))  // if online return 1 or init and return 0
		return 1;
	for(i=0;i<QUEUELEN;i++)
	{
		if(clients[i].client_id==0 && clients[i].is_online!=1)
		{//DEBUG("init_clents     =====%s\n",tok_file[2]);
			strcpy( clients[i].client_name , tok_file[0]);
			strcpy( clients[i].client_passwd,tok_file[1]);
			clients[i].client_id=atoi(tok_file[2]);
			clients[i].is_online=1; 
			if(atoi(tok_file[3])==1)  
			 clients[i].admin=ADMIN_USER;//1
			else
       clients[i].admin=NORMAL_USER; //0
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
//	fun() ;
	return  0;
}

//注册新用户
void  register_new_client(send_info *send,int newfd)
{
	pthread_mutex_lock(&mutex);
	int    ret=0;
	char   all_buf[BUF_SIZE];
	char   read_buf[BUF_SIZE];
	int fd, n,m;
	if(( fd=open(FILENAME,O_CREAT|O_RDWR|O_APPEND,0644))==-1)
		err("open file  failed or create file failed!\n");
	int len=0;
	int  is_account_exited_flag=0;
	int j=0;
	
 char  *tok_file[5];
	while(1)
	{
		lseek(fd,len,SEEK_SET);
		if(read(fd,read_buf,BUF_SIZE)==0)
		{ 
			id++;
			break;
		}
		else{ 
			  j=0;
			 char str[BUF_SIZE]={0};
			 strcpy(str,read_buf); 
		   tok_file[j++]=strtok(read_buf,":");
		   while(tok_file[j++]=strtok(NULL,":")); 
		   if(strcmp(tok_file[0],send->name)==0)
		   {
            is_account_exited_flag=REGIST_EXITED;
             break;
       }
 			 id=atoi(tok_file[2]);   
			 len+=strlen(str)+2;
			 bzero(str,sizeof(str));
		}

	} 
	if(is_account_exited_flag==REGIST_EXITED)
	{
			  	ret=REGIST_EXITED;//1  exited
			  	write(newfd,&ret,sizeof(int));
	}
	else
	{		
		  sprintf(all_buf,"%s:%s:%d:%d",send->name,send->passwd,id,ADMIN_USER);
			n=write(fd,all_buf,strlen(all_buf)+1);
			m=write(fd,"\n",1);// write a enter  write  length=2
			if(m!=0 && n!=0)
			{
				ret=id;
				write(newfd,&ret,sizeof(int));
			}
			else   
			{
				  ret=REGIST_FALIED;//0  falied
		  		write(newfd,&ret,sizeof(int));
			} 
	}
	pthread_mutex_unlock(&mutex);
	close(fd);
}

 //验证登陆
void server_check_login(send_info *send,int newfd)
{
	  pthread_mutex_lock(&mutex);
	//================normal users login read info from file to check========================   
	int fd; char buf[BUF_SIZE]={0};
	if((fd=open(FILENAME,O_RDONLY,0644))==-1)
		 err("server_check_login open file  failed!");
	int len=0;
  int ret=0;
  char  *tok_file[5];
  int j=0;
	while(1)
	{
		lseek(fd,len,SEEK_SET);
		if(read(fd,buf,BUF_SIZE)==0) break;
		len+=strlen(buf)+2;
		j=0;
		tok_file[j++]=strtok(buf,":"); 
		while(tok_file[j++]=strtok(NULL,":"));
		if( strcmp(tok_file[0],send ->name) ==0 )
		{
			 if( strcmp(tok_file[1],send ->passwd)==0)
        {
				 			bzero(&ret,sizeof(ret));
							pthread_mutex_unlock(&mutex);
							if(init_clents(tok_file)) //  online ret 2  
							ret = NORMAL_USER_LOGIN_FAILED_ONLINE ;//2 已经在线
							else
							{  
								  if(atoi(tok_file[3])==1) 
								  ret= ADMIN_LOGIN_SUCCESS;// 3
								  else 
							    ret= NORMAL_USER_LOGIN_SUCCESS;//1   登录成功
							}
						  break;
		    }
		    else //密码错误
		     {	 
		         bzero(&ret,sizeof(ret));
		         ret=NORMAL_USER_LOGIN_PASSWD_ERROR ;//4  passwd error
		         break;
		     }
		}else{//没有此账号
		   bzero(&ret,sizeof(ret));
		   ret= NORMAL_USER_LOGIN_FAILED ;//0
		}
		memset(buf,0,sizeof(buf));
	}
	
	write(newfd,&ret,sizeof(ret)); // 1 success 0 failed  2 online  4  passwd error
	pthread_mutex_unlock(&mutex);
	close(fd);
}
//对客户端退出的处理fun
void  client_exit(send_info *send,int exit_client_socket_fd)
{
	   int i=0;
		for(i=0;i<QUEUELEN;i++)
		{
			if(clients[i].sockfd==exit_client_socket_fd)
			{ 
				break;
			}
		}
		//把该用户的有关信息置为空
		fprintf(stderr,"client name:\t%s\tid:\t%d\t exited!\n",clients[i].client_name,clients[i].client_id);
    memset(&clients[i],0,sizeof(clients[i]));
		close(exit_client_socket_fd);
	//  fun() ;
}
//获取所有的在线用户
void get_all_online_clients (send_info *send,int newfd)
{
     int i=0;
     char buf[BUF_SIZE]={'\0'};
     char str[BUF_SIZE]={'\0'};
     char  no_client_online[40]={"no client online ,only you !"};
   	for(i=0;i<QUEUELEN;i++) 
   	{
   		     //不包含自己
           if( (clients[i].is_online==1) && (clients[i].sockfd!=newfd) )
           {
               sprintf(buf,"%s\t",clients[i].client_name);
               strcat(str,buf);
           }
    }
      if(strcmp(str,"")==0)//没有用户在线
      write(newfd,no_client_online,strlen(no_client_online)+1);  
      else 
     	write(newfd,str,strlen(str)+1);  
}
//获取用户的sockfd(即accept的返回值)
int  
get_sockfd(char dest[])
{  
	int i=0;
	for(i=0;i<QUEUELEN;i++)
	{
		if(clients[i].client_id==atoi(dest) || strcmp(clients[i].client_name,dest)==0) 
		{
			return  clients[i].sockfd;
		}               
	}
	return 0;
}
//解析客户端传过来的字符串数据结构中的type 成员来匹配调用不同的函数。
void parse_buf(send_info *send,int newfd)
{  //DEBUG("send->type=%d\n",send->type);
	  int i=0;
    for(i=0;i<p[i].fun_flag!=0;i++)
    {
        if(send->type==p[i].fun_flag)
        {
            p[i].fun(send,newfd);	
            break; 
        }
     }   
}
//格式化字符串 (加上时间，用户昵称和ID等)
void format_buf(char *dest,char *content,int newfd)
{  
	    int i=0;
      time_t  timep;
     for(;i<QUEUELEN;i++)
     { 
      	  if(clients[i].sockfd ==newfd)break;
     }
      time(&timep);
      sprintf(dest,"%s\t%s \n %s",clients[i].client_name,ctime(&timep),content);  
} 
//私聊
void private_chat ( send_info *send,int newfd)
{
			char dest[BUF_SIZE]={'\0'};
			int tag_id=0;
			if((tag_id=get_sockfd(send->id))!=0)
      {
 	      format_buf (dest,send->buf,newfd);
 			  write(tag_id,dest,strlen(dest)+1);  
 			}
 			else
 				DEBUG("没有此用户，请核对!\n");

}


//群聊（不包含自己）
void public_chat (send_info *send ,int newfd)
{
    int j=0;
    char str[BUF_SIZE]={'\0'};
    for(j=0;j<QUEUELEN;j++)
		{
			 if(clients[j].sockfd==0 || clients[j].sockfd==newfd) 
			  	continue;
			 else
			 {
			  	  format_buf (str,send->buf,newfd);
				  write(clients[j].sockfd,str,strlen(str)+1);
			 }
		}
}


void public_chat_V2( send_info *send,int newfd)//公共聊天室响应函数
{
	//int chatroom_fd=*((int *)arg);
	//send_info *send;
	int fd=newfd;
	time_t timep;
	pthread_mutex_lock(&mutex);
	int length;
    char str[BUF_SIZE]={'\0'};
    time(&timep);
	//printf("%s\n",send->buf);
    //sprintf(str,"%s  %s\n %s",send->name,ctime(&timep),send->buf);  
    format_buf (str,send->buf,newfd);
	printf("%s\n",str);
	write(chatroom_fd,str,strlen(str)+1);	
	//memset(&send,0,sizeof(send));
    pthread_mutex_unlock(&mutex);

}

// 线程读函数
void *pthreads(void *arg)
{
	int newfd=*((int *)arg);
	char buf[BUF_SIZE]={0};
	int length=0;
	send_info   send,*p;
    p=&send;
 	while(1)
	{
			length=read(newfd,&send,sizeof(send));
			if(length<=0)
			{
				printf("server receive msg is empty !\n");
				break;	
			}
			parse_buf(p, newfd);
			memset(&send,0,sizeof(send));
	}
     client_exit(p, newfd);
}

// print error
void  err(char *err_msg)
{
	perror(err_msg);
	exit(EXIT_FAILURE);
}   
//忽略ctrl +c 键的处理函数
void signHandler(int signNo)
{
    DEBUG("singal:%d \n",signNo);
    printf("若要真关闭服务器，请按CRTL+\\即可关闭。\n");
}
//初始化连接
int system_init()
{
    int socketfd;
	struct sockaddr_in  server_addr ;
	if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	 {
        perror("socket error\n");
        exit(1);
    } 

    int opt,optlen=sizeof(optlen);
	if( (setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&opt,optlen))==-1)
		err(" server setsockopt()  error ");

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	inet_pton(AF_INET,IP,&server_addr.sin_addr);

	
	if(-1==(bind(socketfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))))
	 {
        perror("bind error");
        exit(1);
    }

	if( (listen(socketfd,QUEUELEN))==-1)
	  {
        perror("listen error\n");
        exit(1);
    }
	fprintf(stderr,"waiting client connection....\n");
	// return 新的socket 描述符 
	return socketfd ;
 }


 //等待客户端连接
void  connect_to_client(int socketfd )
{
	int newfd;
    struct sockaddr_in   acc_addr;
	socklen_t  len=sizeof(struct sockaddr_in);
	int i=0;
	while(1)
	{
    // signal(SIGINT, signHandler); // server ctrl +c   ,必要时可以用ctrl + \来关闭服务器
    // signal(SIGPIPE,SIG_IGN);// if client  read  write close  server write 接收到 SIGPIPE
		if((newfd=accept(socketfd,(struct sockaddr *)&acc_addr,&len))==-1)
			err("server accept()   error");  
		printf("   %s请求连接。\n",inet_ntoa(acc_addr.sin_addr));	
		           
		for(i=0;i<QUEUELEN;i++)
		{
			if(clients[i].sockfd==0)
			{
				clients[i].sockfd=newfd;
			  DEBUG("clients[i]sockfd=%d\n",clients[i].sockfd);
				break;
			}
		} 
			
		if((pthread_create(&clients[i].tid,NULL,pthreads,&clients[i].sockfd))==-1)
			err("server pthread_create() 2  error"); 
	}
	close(socketfd);
	close(newfd);
 }
