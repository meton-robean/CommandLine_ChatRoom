/* server.c */  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
  
#include "lib/base64.h"  
#include "lib/sha1.h"  
#include "lib/intLib.h"  
  
  
#define REQUEST_LEN_MAX 1024  
#define DEFEULT_SERVER_PORT 8000  
#define WEB_SOCKET_KEY_LEN_MAX 256  
#define RESPONSE_HEADER_LEN_MAX 1024  
#define LINE_MAX 256  
  
  
void shakeHand(int connfd,const char *serverKey);  
char * fetchSecKey(const char * buf);  
char * computeAcceptKey(const char * buf);  
char * analyData(const char * buf,const int bufLen);  
char * packData(const char * message,unsigned long * len);  
void response(const int connfd,const char * message);  
  
int main(int argc, char *argv[])  
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
    int port= DEFEULT_SERVER_PORT;  
  
    if(argc>1)  
      {  
        port=atoi(argv[1]);  
      }  
    if(port<=0||port>0xFFFF)  
      {  
        printf("Port(%d) is out of range(1-%d)\n",port,0xFFFF);  
        return;  
      }  
    listenfd = socket(AF_INET, SOCK_STREAM, 0);  
  
    bzero(&servaddr, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
    servaddr.sin_port = htons(port);  
      
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));  
  
    listen(listenfd, 20);  
  
    printf("Listen %d\nAccepting connections ...\n",port);  
    cliaddr_len = sizeof(cliaddr);  
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);  
    printf("From %s at PORT %d\n",  
               inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),  
               ntohs(cliaddr.sin_port));  
  
    while (1)  
      {  
      
        memset(buf,0,REQUEST_LEN_MAX);  
        n = read(connfd, buf, REQUEST_LEN_MAX);   
        printf("---------------------\n");  
      
      
        if(0==connected)  
          {  
            printf("read:%d\n%s\n",n,buf);  
            secWebSocketKey=computeAcceptKey(buf);    
            shakeHand(connfd,secWebSocketKey);  
            connected=1;  
            continue;  
          }  
  
        data=analyData(buf,n);

        response(connfd,data);  
    }  
    close(connfd);  
}  


/*--------------------------------------------------------------------------------------------------*/
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



