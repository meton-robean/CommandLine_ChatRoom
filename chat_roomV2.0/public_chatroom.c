#include "chat.h"

int socketfd;
pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;
//void public_chatroom_inits(void);

void public_chatroom(void){

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
    printf("=========公共聊天室========\n" );
	char buf[BUF_SIZE]={0};
	int length=0;  
	sleep(10);  
    while(1){
    		length=read(socketfd,buf,BUF_SIZE);
		if(length<=0)
		{
			printf("服务器已关闭!\n");
			exit(EXIT_SUCCESS);	
			break;
		}  
		printf("%s\n",buf);  	
    }

}

int main(){

	public_chatroom();
	close(socketfd);
	return 0;
}

