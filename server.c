#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
  init();
  struct sockaddr_in client;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  int port = SERVER_PORT;
  strcpy(ftp_path,"/tmp/");
  int i = 0;

  for(i = 0; i < argc; i++)
  {
    if(strncmp(argv[i],"-port",5) == 0)
    {
      port = atoi(argv[i+1]);
      i++;
    }
    else if(strncmp(argv[i],"-root",5) == 0)
    {
      strcpy(ftp_path,argv[i+1]);
      i++;
    }
  }
  if(ftp_path[strlen(ftp_path)-1] != '/')
    strcat(ftp_path,"/");
  strcpy(pwd, ftp_path);


  /* create a socket
     IP protocol family (PF_INET)
     TCP protocol (SOCK_STREAM)
  */
  serverFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  
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
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;

  int on = 1;
  if(setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))==-1)
  {
     perror("setsockopt");
     close(serverFD);
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
    close(clientFD);
  }
  close(serverFD);
  close(ftp_data_sock);
  return 0;
}

void handle_client_request(int clientFD,struct sockaddr_in client)
{
  int login_flag;     //   logged in or not
  login_flag = login(clientFD);
  while(receive_client_msg(clientFD))
  {
    if(strncmp("QUIT", msg_buf, 4)==0)
    {
      char buf[MAX_BUF] = {0};
      char buf2[MAX_BUF] = {0};
      if(numOfDownloadFiles != 0)
      {
        snprintf(buf, MAX_BUF, "221-You have download %ld bytes in %ld files,\r\n", bytesOfDownload, numOfDownloadFiles);
      }
      if(numOfUploadFiles != 0)
      {
        snprintf(buf2, MAX_BUF, "221-upload %ld bytes in %ld files\r\n", bytesOfUpload, numOfUploadFiles);
      }
      strcat(buf, buf2);
      strcat(buf,serverMsg221);
      send_client_msg(clientFD, buf, strlen(buf));
      close(clientFD);
      close(serverFD);
      close(connect_data_sock);
      close(ftp_data_sock);
      exit(0);
    }
    else if(login_flag > 0 &&strncmp("PASV", msg_buf, 4)== 0)
    {
      do_pasv(clientFD, client);
    }
    else if(login_flag > 0 && strncmp("PORT ", msg_buf, 5)== 0)
    {
      do_port(clientFD);
    }
    else if(login_flag > 0 && strncmp("TYPE", msg_buf, 4)== 0)
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
    else if(login_flag > 0 && strncmp("RETR ", msg_buf, 5)== 0)
    {
      do_retr(clientFD);
    }
    else if(login_flag > 0 && strncmp("STOR ", msg_buf, 5)== 0)
    {
      do_stor(clientFD);
    }
    else if(login_flag > 0 && strncmp("SYST", msg_buf, 4) == 0)
    {
        send_client_msg(clientFD, serverMsg215, strlen(serverMsg215));
    }
    else if(login_flag > 0 && strncmp("PWD", msg_buf, 3) == 0)
    {
      char pwd_info[MAX_BUF];
      snprintf(pwd_info, MAX_BUF, "257 \"%s\" is current location.\r\n", pwd);
      send_client_msg(clientFD, pwd_info, strlen(pwd_info));
    }
    else if(login_flag > 0 && strncmp("MKD ", msg_buf, 4) == 0)
    {
      char pwd_info[MAX_BUF], dir[MAX_BUF], path[MAX_BUF];
      int length = strlen(msg_buf);
      int i;
      for(i = 4; i < length; i++)
      {
        dir[i-4] = msg_buf[i];
      }
      dir[i-5] = '\0';
      strcpy(path, pwd);
      strcat(path, dir);
      if(path[strlen(path)-1] != '/') strcat(path, "/");
      mkdir_r(path);
      snprintf(pwd_info, MAX_BUF, "257 \"%s\" directory created.\r\n", path);
      send_client_msg(clientFD, pwd_info, strlen(pwd_info));
    }
    else if(login_flag > 0 && strncmp("CWD ", msg_buf, 4) == 0)
    {
      char  cwd_info[MAX_BUF], path[MAX_BUF];
      int i;
      memset(path, 0, sizeof(path));
      int length = strlen(msg_buf);
      for(i = 4; i < length; i++)
      {
        path[i-4] = msg_buf[i];
      }
      path[i-5] = '\0';
      if(access(path, 00) ==0) 
      {
        strcpy(pwd, path);
        snprintf(cwd_info, MAX_BUF, "200 directory changed to \"%s\".\r\n", path);
      }
      else
      {
        snprintf(cwd_info, MAX_BUF, "431 \"%s\" No such directory .\r\n", path);
      }
      send_client_msg(clientFD, cwd_info, strlen(cwd_info));
    }
    else if(login_flag > 0 && strncmp("CDUP", msg_buf, 4) == 0)
    {
      char  pwd_info[MAX_BUF];
      if(strlen(pwd)==1)
      {
        snprintf(pwd_info, MAX_BUF, "431 \"%s\" No parent directory.\r\n", pwd);
        send_client_msg(clientFD, pwd_info, strlen(pwd_info));
        break;
      }
      if(pwd[strlen(pwd)-1]=='/') pwd[strlen(pwd)] = '\0';
      char *p = strrchr(pwd,'/');
      while(*p !='\0')
      {
        *p = '\0';
        p ++;
      }
      if(strlen(pwd) == 0)
      {
        pwd[0] = '/';
        pwd[1] = '\0';
      }
      snprintf(pwd_info, MAX_BUF, "200 directory changed to \"%s\".\r\n", pwd);
      send_client_msg(clientFD, pwd_info, strlen(pwd_info));
    }
    else if(login_flag > 0 && strncmp("ABOR ",msg_buf, 5) == 0)
    {
      send_client_msg(clientFD, serverMsg221, strlen(serverMsg221));
      close(clientFD);
      close(serverFD);
      close(connect_data_sock);
      close(ftp_data_sock);
      exit(0);
    }
    else
    {
      send_client_msg(clientFD, serverMsg202, strlen(serverMsg202));
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
    memset(password, 0, sizeof(password));
    for(i = 5; i < length; i++)
    {
      password[i-5] = msg_buf[i];
    }
    password[i-6] = '\0';
    printf("client %d logged in, username:%s password:%s\n",getpid() ,username,password);
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
  struct sockaddr_in addr;
  int len = sizeof(addr);
	if (ret == 0)   //client closed
	{
		printf("client closed");
		return 0;
	}
	else if (ret == -1)
	{
		printf("error read message from client");
		return 0;
	}
  getpeername(clientFD, (struct sockaddr *)&addr, &len);
	printf("client %s send a message:%s",inet_ntoa(addr.sin_addr),msg_buf);
	return 1;
}

void init()
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
    return ;
  }

  srand((int)time(0));
  int port = rand()  + 20000;

  char pasv_msg[MAX_BUF];
  char port_str[8];
  char addr_info_str[30];
  

  memset(&data_transfer_addr, 0, sizeof(struct sockaddr_in));
  data_transfer_addr.sin_family = AF_INET;
  data_transfer_addr.sin_addr.s_addr= INADDR_ANY;
  data_transfer_addr.sin_port = htons(port);  
  int port1 = ntohs(data_transfer_addr.sin_port) / 256;
  int port2 = ntohs(data_transfer_addr.sin_port) % 256;  

  int on = 1;
  if(setsockopt(data_transfer_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
  {
     perror("error setsockopt!\n");
     close(data_transfer_sock);
     return;
  }
  while (bind(data_transfer_sock, (struct sockaddr*)&data_transfer_addr, sizeof(struct sockaddr_in)) < 0)  //if port is in use
  {
    port = rand()  + 20000;
    data_transfer_addr.sin_port = htons(port);
    port1 = ntohs(data_transfer_addr.sin_port) / 256;
    port2 = ntohs(data_transfer_addr.sin_port) % 256;
  }

  if(listen(data_transfer_sock, 10) < 0)
  {
    perror("error listen failed");
    close(data_transfer_sock);
    return;
  }

  long ip = inet_addr(inet_ntoa(client.sin_addr));  //get server's ip                                            
  snprintf(addr_info_str, sizeof(addr_info_str), "%ld,%ld,%ld,%ld,", ip&0xff,ip>>8&0xff,ip>>16&0xff,ip>>24&0xff);
  snprintf(port_str, sizeof(port_str), "%d,%d", port1, port2);
  strcat(addr_info_str, port_str);
  snprintf(pasv_msg, MAX_BUF, "227 Entering Passive Mode (%s).\r\n", addr_info_str);
  send_client_msg(clientFD, pasv_msg, strlen(pasv_msg));
  
  connect_mode = 0;

  if(ftp_data_sock > 0)
  {
    close(connect_data_sock);
    close(ftp_data_sock);
  }
  ftp_data_sock = data_transfer_sock;
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
  char ip[32]={0};
  int  port=0;
  get_ip(msg_buf, ip, &port);

  memset(&data_transfer_addr, 0, sizeof(struct sockaddr_in));
  data_transfer_addr.sin_family = AF_INET;
  data_transfer_addr.sin_port = htons(port);
  if(inet_pton(AF_INET, ip, &data_transfer_addr.sin_addr) == 0)
  {
    printf("second parameter does not contain valid ipaddress");
    return;
  }

  connect_mode = 1;
  if(ftp_data_sock > 0)
  {
    close(ftp_data_sock);
    close(connect_data_sock);
    ftp_data_sock = -1;
    connect_data_sock = -1;
  }
  send_client_msg(clientFD, PORT_SUCCESS, strlen(PORT_SUCCESS));
}


void do_retr(int clientFD)
{
	int i = 0;
	int length = strlen(msg_buf);
	char filename[MAX_BUF] = {0};
  long file_size = 0;
  char size[MAX_BUF];
  char msg[MAX_BUF];
  char path[MAX_BUF];
  struct stat file_info;

	for(i = 5; i < length; i++)
	{
		filename[i-5] = msg_buf[i];
	}
	filename[i-6] = '\0';

  strcpy(path, pwd);
  strcat(path, filename);

  // get file size
  if(!stat(path, &file_info))
  {
    file_size = (int)file_info.st_size;
  }
  snprintf(size, sizeof(size), "%s(%ld bytes).\r\n",filename,file_size);
  strcpy(msg, serverMsgRETR150);
 	strcat(msg, size);
 	send_client_msg(clientFD, msg, strlen(msg));  //send message to client


  //send file
 	int readFD = open(path, O_RDONLY);
  if(readFD < 0)
  {
    send_client_msg(clientFD, serverMsg451, strlen(serverMsg451));
    close(connect_data_sock);
    close(ftp_data_sock);
    return;
  }
 	char data[MAX_BUF];
 	memset(data, 0, sizeof(data));

  if(connect_mode == 0)   //PASV mode
  {
    if(ftp_data_sock < 0)
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(readFD);
      return;
    }
    connect_data_sock = accept(ftp_data_sock, NULL, NULL);
    if(connect_data_sock < 0)
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(readFD);
      return;
    }
    while (read(readFD, data, MAX_BUF) > 0)
  	{
      if (send(connect_data_sock, data, strlen(data), 0) < 0)
      {
    		send_client_msg(clientFD, serverMsg426, strlen(serverMsg426));
        close(connect_data_sock);
        close(ftp_data_sock);
        close(readFD);
        return;
    	}
      memset(data, 0, sizeof(data));  
  	}
  }
  else       //PORT mode
  {
    connect_data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == connect(connect_data_sock, (const struct sockaddr *)&data_transfer_addr, sizeof(struct sockaddr_in)))
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(readFD);
      return;
    }
    while (read(readFD, data, MAX_BUF) > 0)
    {
      if (send(connect_data_sock, data, strlen(data), 0) < 0)
      {
        send_client_msg(clientFD, serverMsg426, strlen(serverMsg426));
        close(connect_data_sock);
        close(ftp_data_sock);
        close(readFD);
        return;
      }
      memset(data, 0, sizeof(data));  
    }
  }

  bytesOfDownload += file_size;
  numOfDownloadFiles ++;
  close(connect_data_sock);
  close(ftp_data_sock);
  close(readFD);
  ftp_data_sock = -1;
  connect_data_sock = -1;
  send_client_msg(clientFD, serverMsg226, strlen(serverMsg226));
}


void do_stor(int clientFD)
{
  int i = 0;
  int length = strlen(msg_buf);
  char filename[MAX_BUF] = {0};
  char msg[MAX_BUF];
  char path[MAX_BUF];
  long filesize = 0;
  struct stat file_info;

  for(i = 5; i < length; i++)
  {
    filename[i-5] = msg_buf[i];
  }
  filename[i-6] = '\0';
  strcpy(path, pwd);
  strcat(path, filename);


  strcpy(msg, serverMsgSTOR150);
  strcat(msg, filename);
  strcat(msg, "\r\n");
  send_client_msg(clientFD, msg, strlen(msg));  //send message to client

  //create file
  mkdir_r(path);
  umask(0);
  int writeFD = open(path, (O_RDWR | O_CREAT | O_TRUNC), S_IRWXU | S_IRWXO | S_IRWXG);
  if(writeFD < 0)
  {
    send_client_msg(clientFD, serverMsgSTOR451, strlen(serverMsgSTOR451));
    close(connect_data_sock);
    close(ftp_data_sock);
    return;
  }
  char data[MAX_BUF];
  memset(data, 0, sizeof(data));


  //transfer file
  if(connect_mode == 0)   //PASV mode
  {
    if(ftp_data_sock < 0)
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(writeFD);
      return;
    }
    connect_data_sock = accept(ftp_data_sock, NULL, NULL);
    if(connect_data_sock < 0)
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(writeFD);
      return;
    }
    while (read(connect_data_sock, data, MAX_BUF) > 0)
    {
      if (write(writeFD, data, strlen(data)) < 0)
      {
        send_client_msg(clientFD, serverMsg426, strlen(serverMsg426));
        close(connect_data_sock);
        close(ftp_data_sock);
        close(writeFD);
        return;
      }
      memset(data, 0, sizeof(data));  
    }
  }
  else       //PORT mode
  {
    connect_data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == connect(connect_data_sock, (const struct sockaddr *)&data_transfer_addr, sizeof(struct sockaddr_in)))
    {
      send_client_msg(clientFD, serverMsg425, strlen(serverMsg425));
      close(connect_data_sock);
      close(ftp_data_sock);
      close(writeFD);
      return;
    }

    while (read(connect_data_sock, data, MAX_BUF) > 0)
    {
      if (write(writeFD, data, strlen(data)) < 0)
      {
        send_client_msg(clientFD, serverMsg426, strlen(serverMsg426));
        close(connect_data_sock);
        close(ftp_data_sock);
        close(writeFD);
        return;
      }
      memset(data, 0, sizeof(data));  
    }
  }
  close(connect_data_sock);
  close(ftp_data_sock);
  close(writeFD);
  ftp_data_sock = -1;
  connect_data_sock = -1;
  if(!stat(path, &file_info))
  {
    filesize = (int)file_info.st_size;
  }
  numOfUploadFiles ++;
  bytesOfUpload += filesize;
  send_client_msg(clientFD, serverMsg226, strlen(serverMsg226));
}

/*
* create multi-level directory
*/
int mkdir_r(const char *path)   
{
  if (path == NULL) 
  {
    return -1;
  }

  char *temp = malloc(MAX_BUF*sizeof(char));

  int i;
  for(i = 0; i<strlen(path); i++)
  {
    *(temp+i) = *(path+i);
  }

  char *pos = temp;


  if (strncmp(temp, "/", 1) == 0) 
  {
    pos += 1;
  } 
  else if (strncmp(temp, "./", 2) == 0) 
  {
    pos += 2;
  }
  
  for ( ; *pos != '\0'; pos++) 
  {
    if (*pos == '/') 
    {
      *pos = '\0';
      umask(0);
      mkdir(temp, S_IRWXU | S_IRWXO | S_IRWXG);
      *pos = '/';
    }
  }
  free(temp);
  return 0;
}
