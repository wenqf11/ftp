//fuser 21/tcp -k
#include "server.h"
#include "transfer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>


int main(void)
{
  add_default_user();
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
    perror("error bind failed!\n");
    close(serverFD);
    exit(EXIT_FAILURE);
  }

  /*  listen for connections to server
  */
  if(-1 == listen(serverFD, 10))
  {
    perror("error listen failed!\n");
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
       perror("error accept failed!\n");
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
	   handle_client_request(clientFD, client);
    }
  }
  return 0;
}

void handle_client_request(int clientFD,struct sockaddr_in client)
{
  int login_flag;     //   logged in or not
  login_flag = login(clientFD);
  while(receive_client_msg(clientFD))
  {
    if(login_flag>0&&strncmp("QUIT", msg_buf, 4)==0)
    {
      send_client_msg(clientFD, serverMsg221, strlen(serverMsg221));
      close(clientFD);
      break;
    }
    else if(strncmp("PASV", msg_buf, 4)== 0)
    {
      do_pasv(clientFD, client);
    }
    else if(strncmp("PORT ", msg_buf, 5)== 0)
    {
      do_port(clientFD);
    }
    else if(strncmp("TYPE", msg_buf, 4)== 0)
    {
      if(strncmp("TYPE I", msg_buf,6) == 0)
      {
        send_client_msg(clientFD, TYPE_I_SUCCESS, strlen(TYPE_I_SUCCESS));
      }
      else
      {
      	send_client_msg(clientFD, serverMsg202, strlen(serverMsg202));
      }
    }
    else if(strncmp("RETR ", msg_buf, 5)== 0)
    {
      do_retr(clientFD);
    }
    else if(strncmp("STOR ", msg_buf, 5)== 0)
    {
      //handle_file(clientFD);
      send_client_msg(clientFD,serverMsg226, strlen(serverMsg226));
    }
    else if(strncmp("SYST", msg_buf, 5) == 0)
    {
        send_client_msg(clientFD, serverMsg215, strlen(serverMsg215));
    }
    else if(strncmp("PWD ",msg_buf, 4) == 0)
    {
      char  pwd_info[MAX_BUF];
      char  tmp_dir[MAX_BUF];
      snprintf(pwd_info, MAX_BUF, "257 \"%s\" is current location.\r\n", getcwd(tmp_dir, MAX_BUF));
      send_client_msg(clientFD, pwd_info, strlen(pwd_info));
    }
    else if(strncmp("ABOR ",msg_buf, 5) == 0)
    {
      close(clientFD);
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
  char username[MAX_BUF]={0};
  char password[MAX_BUF]={0};

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
    printf("client %d logged in, username:%s password:%s\n",getpid(),username,password);
    send_client_msg(clientFD, serverMsg230, strlen(serverMsg230));
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
      printf("client %d logged in, username:%s password:%s\n",getpid(),username,password); 	
      send_client_msg(clientFD, serverMsg230, strlen(serverMsg230));
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
	  return 1;
  }
  return 0;
}

int receive_client_msg(int clientFD)
{
	memset(msg_buf, 0, sizeof(msg_buf));
	int ret = read(clientFD, msg_buf, MAX_BUF);
	if (ret == 0)   //client closed
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

void do_pasv(int clientFD, struct sockaddr_in client)
{
  int data_transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (data_transfer_sock < 0)
  {
      printf("error create data socket!\n");
    return;
  }

  srand((int)time(0));
  int port = rand()  + 20000;

  memset(&data_transfer_addr, 0, sizeof(struct sockaddr_in));
  data_transfer_addr.sin_family = AF_INET;
  data_transfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  data_transfer_addr.sin_port = htons(port);    

  if (bind(data_transfer_sock, (struct sockaddr*)&data_transfer_addr, sizeof(struct sockaddr)) < 0)
  {
    printf("error bind data socket!\n");
    return;
  }
  listen(data_transfer_sock, 10);

  char pasv_msg[MAX_BUF];
  char port_str[8];
  char addr_info_str[30];
  int port1 = ntohs(data_transfer_addr.sin_port) / 256;
  int port2 = ntohs(data_transfer_addr.sin_port) % 256;
  long ip = inet_addr(inet_ntoa(client.sin_addr));  //get server's ip
                                              
  snprintf(addr_info_str, sizeof(addr_info_str), "%ld,%ld,%ld,%ld,", ip&0xff,ip>>8&0xff,ip>>16&0xff,ip>>24&0xff);
  snprintf(port_str, sizeof(port_str), "%d,%d", port1, port2);
  strcat(addr_info_str, port_str);
  snprintf(pasv_msg, MAX_BUF, "227 Entering Passive Mode (%s).\r\n", addr_info_str);
  send_client_msg(clientFD, pasv_msg, strlen(pasv_msg));
}

void get_ip(char *buffer,char *ip_address,int *port)
{
  char *p, *q, *r;
  int i;
  char port1[6] = {0};
  char port2[6] = {0};
  r = strchr(buffer, ' ') + 1;
  p = r;
  for (i = 0; i < 4; i++)
  {
    p = strchr(p,',');
    *p = '.';
  }
  *p = '\0';
  strcpy(ip_address, r);
  p = p + 1;
  if((q = strchr(p, ',')) != NULL)
  {
    *q = '\0';
    strcpy(port1, p);
    q = q + 1;
    strcpy(port2, q);
  }
  *port = atoi(port1) * 256 + atoi(port2);
} 
  
void do_port(int clientFD)
{
  int data_transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in client_data_addr;
  char ip[32]={0};
  int  port=0;
  get_ip(msg_buf, ip, &port);

  memset(&client_data_addr, 0, sizeof(struct sockaddr_in));
  client_data_addr.sin_family = AF_INET;
  client_data_addr.sin_port = htons(port);
  int res = inet_pton(AF_INET, ip, &client_data_addr.sin_addr);

  if (res == 0)
  {
    perror("not a valid ipaddress!\n");
    close(data_transfer_sock);
    exit(EXIT_FAILURE);
  }
  
  if (-1 == connect(data_transfer_sock, (const struct sockaddr *)&client_data_addr, sizeof(struct sockaddr_in)))
  {
    perror("error data socket connect to client failed!\n");
    close(data_transfer_sock);
    exit(EXIT_FAILURE);
  }

  ftp_data_sock = data_transfer_sock;
  send_client_msg(clientFD, PORT_SUCCESS, strlen(PORT_SUCCESS));
}