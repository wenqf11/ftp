//fuser 21/tcp -k
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server.h"


int main(void)
{
  add_default_user();
  struct sockaddr_in server;
  struct sockaddr_in client;
  socklen_t sin_size = sizeof(struct sockaddr_in);

  /* create a socket
     IP protocol family (PF_INET)
     TCP protocol (SOCK_STREAM)
  */
  int serverFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  
  if(-1 ==  serverFD)
  {
    perror("can not create socket\n");
    exit(EXIT_FAILURE);
  }

  memset(&server, 0, sizeof(struct sockaddr_in));

  /* establish our address
     address family is AF_INET
     our IP address is INADDR_ANY (any of our IP addresses)
     the SERVER_PORT is 21
  */
  server.sin_family = AF_INET;
  server.sin_port = htons(SERVER_PORT);
  server.sin_addr.s_addr = INADDR_ANY;


  int on = 1;
  if(setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))==-1)
  {
     perror("setsockopt");
     exit(1);
  }
  /*  assign server socket to an address
  */
  if(-1 == bind(serverFD,(const struct sockaddr *)&server, sizeof(struct sockaddr_in)))
  {
    perror("error bind failed\n");
    close(serverFD);
    exit(EXIT_FAILURE);
  }

  /*  listen for connections to server
  */
  if(-1 == listen(serverFD, 10))
  {
    perror("error listen failed\n");
    close(serverFD);
    exit(EXIT_FAILURE);
  }


  while(1)
  {
    /*  accept a connection to server
    */
    int clientFD = accept(serverFD,(struct sockaddr *)&client,&sin_size);

    if(clientFD < 0)
    {
       perror("error accept failed\n");
       close(serverFD);
       exit(EXIT_FAILURE);
    }

    send_client_msg(clientFD,serverMsg220, strlen(serverMsg220));

    int pid = fork();
    if (pid == -1)
	  {
    	perror("error fork");
	  }
    else if (pid == 0)
    {
	    handle_client_request(clientFD);
      close(serverFD);
    }
  }
  close(serverFD);
  return 0;
}

void handle_client_request(int clientFD)
{
  int login_flag;     //   logged in or not
  login_flag = login(clientFD);
  while(receive_client_msg(clientFD)&&login_flag==1)
  {
    if(strncmp("QUIT ", msg_buf, 5)==0)
    {
      send_client_msg(clientFD, serverMsg221, strlen(serverMsg221));
      break;
    }
    else if(strncmp("PASV", client_Control_Info, 4)== 0)
    {
      handle_pasv(client_sock,client);
    }
    else if(strncmp("PORT", client_Control_Info, 4)== 0)
    {
      handle_pasv(client_sock,client);
    }
    else if(strncmp("TYPE", client_Control_Info, 4)== 0)
    {
      if(strncmp("TYPE I", client_Control_Info, 6) == 0)
      {
        translate_data_mode=FILE_TRANS_MODE_BIN;
      }
      send_client_msg(client_sock, serverInfo200, strlen(serverInfo200));
    }
    else if(strncmp("RETR", client_Control_Info, 4)== 0)
    {
      handle_file(client_sock);
      send_client_msg(client_sock,serverInfo226, strlen(serverInfo226));
    }
    else if(strncmp("STOR", client_Control_Info, 4)== 0)
    {
      handle_file(client_sock);
      send_client_msg(client_sock,serverInfo226, strlen(serverInfo226));
    }
    else if(strncmp("SYST", client_Control_Info, 4) == 0)
    {
        send_client_info(client_sock, serverInfo215, strlen(serverInfo215));
    }
    else if(strncmp("PWD ",msg_buf, 4) == 0)
    {
      char  pwd_info[MAX_BUF];
      char  tmp_dir[MAX_BUF];
      snprintf(pwd_info, MAX_BUF, "257 \"%s\" is current location.\r\n", getcwd(tmp_dir, MAX_BUF));
      send_client_msg(clientFD, pwd_info, strlen(pwd_info));
    }
    else if(strncmp("CLOSE ",msg_buf, 6) == 0)
    {
      printf("Client Quit!\n");
      shutdown(clientFD, SHUT_WR);  
    }
  }
}

int login(int clientFD)
{
  /*
  * listen for "USER" command from client
  */
  while(1)
  {
    if(receive_client_msg(clientFD) && strncmp("USER ", msg_buf, 5)==0)
    {
      break;
    }
    else 
    {
      send_client_msg(clientFD,serverMsg202, strlen(serverMsg202));
    }
  }


  /*
  * extract username from command
  */
  int anonymous;             // anonymous user or not
  int length = strlen(msg_buf);
  char username[MAX_BUF];
  char password[MAX_BUF];

  int i;
  for(i=5; i<length; i++)
  {
    username[i-5] = msg_buf[i];
  }
  username[i-6] = '\0';

  if(strncmp(username, "anonymous",9)==0)
  {
    anonymous = 1;
  }
  else 
  {
    anonymous = 0;
  }

  if(anonymous == 1)  // anonymous user
  {
    send_client_msg(clientFD,serverMsgAnms331,strlen(serverMsgAnms331));


    /*
    * listen for "PASS" command from client
    */
    while(1)
    {
      if(receive_client_msg(clientFD) && strncmp("PASS ", msg_buf, 5)==0)
      {
        break;
      }
      else
      {
        send_client_msg(clientFD,serverMsg202, strlen(serverMsg202));
      }
    }

    /*
    * extract username from command
    */
    length = strlen(msg_buf);
    for(i = 5; i < length; i++)
    {
      password[i-5] = msg_buf[i];
    }
    password[i-6] = '\0';
    send_client_msg(clientFD, serverMsg230, strlen(serverMsg230));
    printf("client %d logged in, username:%s password:%s",getpid(),username,password);
    return 1;
  }
  else      //cNon-anonymous user
  {
    send_client_msg(clientFD,serverMsg331,strlen(serverMsg331));

    /*
    * listen for "PASS" command from client
    */
    while(1)
    {
      if(receive_client_msg(clientFD) && strncmp("PASS ", msg_buf, 5)==0)
      {
        break;
      }
      else
      {
        send_client_msg(clientFD,serverMsg202, strlen(serverMsg202));
      }
    }

    int flag = 0;     //allow to login or not
    length = strlen(msg_buf);

    for(i = 5; i < length; i++)
    {
      password[i-5] = msg_buf[i];
    }
    password[i-6] = '\0';

    for(i = 0; i < numOfUser; i++)
    if (strcmp(default_user[i],username)==0 && strcmp(default_pwd[i],password)==0)
    {
      flag = 1;
      break;
    }


    if(flag == 1)  
    {
      send_client_msg(clientFD, serverMsg230, strlen(serverMsg230));
      printf("client %d logged in, username:%s password:%s",getpid(),username,password);
      return 2;
    }
    else
    {
      send_client_msg(clientFD, serverMsg530, strlen(serverMsg530));
      return 0;
    }
  }

}

int send_client_msg(int clientFD,char* msg,int length)
{
  if(write(clientFD, msg, length)<0)
  {
	  perror("error send message to client\n");
  }
  return 0;
}

int receive_client_msg(int clientFD)
{
	int ret = read(clientFD, msg_buf, MAX_BUF);
	if (ret == 0)   //客户端关闭了
	{
		printf("client closed");
		return 0;
	}
	else if (ret == -1)
	{
		perror("error read message from client");
		return 0;
	}
  printf("client %d send a message:%s",getpid(),msg_buf);
  return 1;
}

void add_default_user()
{
  FILE *fp;
  char user[MAX_BUF],password[MAX_BUF];
  if((fp = fopen("user.txt","a+")) == NULL)
  {
    printf("error in add default user\n");
    exit(1);
  }
  fscanf(fp, "%d", &numOfUser);
  int i;
  for(i=0; i<numOfUser; i++)
  {
    fscanf(fp, "%s %s",user, password);
    strcpy(default_user[i], user);
    strcpy(default_pwd[i], password);
  }
  fclose(fp);
}
