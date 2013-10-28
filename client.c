#include "client.h"
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
#include <termios.h>

static struct termios stored_settings;
void echo_off(void)
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= (~ECHO);
    tcsetattr(0,TCSANOW,&new_settings);
    return;
}
void echo_on(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}

int main(int argc, char * argv[])
{
	if(argc == 1)
	{
		printf("Hello!Welcome to Vince's FTP!\n");
		start_ftp_client();
	}
	else if(argc == 2)
	{
		connect_to_server(argv[1], DEFAULT_SERVER_PORT);
	}
	else if(argc == 3)
	{
		connect_to_server(argv[1], atol(argv[2]));
	}
	else 
	{
		showUsage(argv[0]);
	}
}

int get_ip_port(char* ip, int *port)
{
	char *p = strchr(cmd,' ');
	if(p == NULL)
	{
		return 1;
	}
	else
	{
		while(*p == ' ')  p++;
	}
	if(p == NULL || p == '\0')
	{
		return 1;
	}
	else 
	{
		char *start = p;
		char *p = strchr(start,' ');
		if(p != NULL) 
		{
			*p = '\0';
			p++;
			while(*p == ' ')  p++;
		}
		if(p == NULL || *p == '\0')
		{
			strcpy(ip, start);
			*port = DEFAULT_SERVER_PORT;
		}
		else
		{
			strcpy(ip, start);
			*port = atoi(p);
			if(*port == 0)
			{
				*port = DEFAULT_SERVER_PORT;
			}
		}
	}
    return 0;
}


int login()
{
	char user[MAX_BUF];
	char buf[MAX_BUF];
	printf("User(Press Enter for anonymous): ");
	fgets(buf, MAX_BUF, stdin);
	if(buf[0]=='\n')
	{
		strncpy(user, "anonymous", 9);
	}
	else
	{
		strncpy(user, buf, strlen(buf)-1);
	}

	memset(buf, 0, sizeof(buf));
	strcpy(buf, "USER ");
	strcat(buf, user);
	strcat(buf,"\r\n");
	send_server_msg(buf,strlen(buf));
	int res = receive_server_msg();
	printf("%s",msg_buf);

	if(res == 331)
	{
		char pass[MAX_BUF];
		printf("Password(Press Enter for anonymous): ");
		memset(buf, 0, sizeof(buf));
		echo_off();
		fgets(buf, sizeof(buf), stdin);
		if(buf[0]=='\n')
		{
			strncpy(pass, "anonymous", 9);
		}
		else
		{
			strncpy(pass, buf, strlen(buf)-1);
		}
		echo_on();
		printf("\n");

		memset(buf, 0, sizeof(buf));
		strcpy(buf, "PASS ");
		strcat(buf, pass);
		strcat(buf,"\r\n");
		send_server_msg(buf,strlen(buf));
		int ret = receive_server_msg();
		printf("%s",msg_buf);

		if(ret == 230)
		{
			return 0;
		}
		else
		{
			printf("Login failed!\n");
			return 1;
		}
	}
	return 0;
}


void get_ip(char *buffer,char *ip_address,int *port)
{
  char *p, *q, *r;
  int i;
  char port1[6] = {0};
  char port2[6] = {0};
  r = strchr(buffer, '(') + 1;
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

void get_cmd_filename(const char *cmd, char *filename)
{
	char *p = strchr(cmd,' ');
	while(*p == ' ') p++;
	if(p != NULL && *p != '\0')
	{
		strcpy(filename, p);
	}
}
int connect_server_data()
{
	if(mode == 0)
	{
		struct sockaddr_in local;
		int addr_len =  sizeof(struct sockaddr);
		int data_transfer_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (data_transfer_sock < 0)
	    {
		    perror("error create data socket!\n");
		    exit(EXIT_FAILURE);
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
			exit(EXIT_FAILURE);
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
			exit(EXIT_FAILURE);
		}

		if(getsockname(client_sock,(struct sockaddr*)&local,(socklen_t *)&addr_len) < 0)
				return -1;
		long ip = inet_addr(inet_ntoa(local.sin_addr));  //get client's ip                                            
		snprintf(addr_info_str, sizeof(addr_info_str), "PORT %ld,%ld,%ld,%ld,", ip&0xff,ip>>8&0xff,ip>>16&0xff,ip>>24&0xff);
		snprintf(port_str, sizeof(port_str), "%d,%d\r\n", port1, port2);
		strcat(addr_info_str, port_str);
		strcpy(pasv_msg, addr_info_str);
		send_server_msg(pasv_msg, strlen(pasv_msg));
		if(receive_server_msg() != 200)
		{
			printf("Can not use PORT mode!Please use \"mode\" change to PASV mode.\n");
			return -1;
		}
		else
		{
			return data_transfer_sock;
		}
	}
	else
	{
		char ip[32] = {0};
		int  port = 0;

		send_server_msg("PASV\r\n",strlen("PASV\r\n"));
		if(receive_server_msg() != 227)
		{
			printf("Can not use PASV mode!Please use \"mode\" change to PORT mode.\n");
			return -1;
		}

		get_ip(msg_buf, ip, &port);

		memset(&data_transfer_addr, 0, sizeof(struct sockaddr_in));
		data_transfer_addr.sin_family = AF_INET;
		data_transfer_addr.sin_port = htons(port);
		if(inet_pton(AF_INET, ip, &data_transfer_addr.sin_addr) == 0)
		{
			perror("second parameter does not contain valid ipaddress");
			exit(EXIT_FAILURE);
		}

		int data_transfer_sock = socket(AF_INET, SOCK_STREAM, 0);

		if (connect(data_transfer_sock,(struct sockaddr *)&data_transfer_addr,sizeof(struct sockaddr_in)) < 0)
		{
			printf("Can't connect to server %s, port %d\n", inet_ntoa(data_transfer_addr.sin_addr),ntohs(data_transfer_addr.sin_port));
			exit(-1);
		}
		return data_transfer_sock;
	}
}

/*download file user RETR*/
void download_file()
{
	int connect_data_sock = -1, i = 0;
	char filename[MAX_BUF] = {0};
	char local_file[MAX_BUF] = {0};
	char buf[MAX_BUF] = {0};
	char data[MAX_BUF] = {0};
	char cover_flag[3];
	struct stat file_info;
	int writeFD;

	get_cmd_filename(cmd, filename);
	strcpy(local_file,"/tmp/a"); 
	char *p = strrchr(filename,'/');
	if(p == NULL)
	{
		p = filename;
		strcat(local_file, "/");
	}
	strcat(local_file, p);
	if(!stat(local_file, &file_info))
	{
		printf("local file %s exists: %d bytes\n", local_file, (int)file_info.st_size);
		printf("Do you want to cover it? [y/n]");
		fgets(cover_flag, sizeof(cover_flag), stdin);
		fflush(stdin);
		if(cover_flag[0] == 'n')
		{
			printf("get file %s aborted.\n", filename);
			return;
		}
	}
	umask(0);
	writeFD = open(local_file, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU | S_IRWXO | S_IRWXG);
	if(writeFD < 0)
	{
		printf("create local file %s failed!\n", local_file);
		return;
	}

	data_sock = connect_server_data();
	if(data_sock < 0)
	{
		printf("socket error!\n");
		return;
	}


  	memset(data, 0, sizeof(data));
	
	send_server_msg("TYPE I\r\n",strlen("TYPE I\r\n"));
	receive_server_msg();
	printf("%s",msg_buf);

	memset(buf, 0, sizeof(buf));
	strcpy(buf, "RETR ");
	strcat(buf, filename);
	strcat(buf,"\r\n");
	send_server_msg(buf,strlen(buf));
	receive_server_msg();
	printf("%s", msg_buf);
	if(mode == 0)   //PORT mode
	{
		while(i < 3)
		{
			connect_data_sock = accept(data_sock, NULL, NULL);
			if(connect_data_sock == -1)
			{
				i++;
				continue;
			}
			else break;
		}
		if(connect_data_sock == -1)
		{
			printf("error in connect, can not use PORT mode\n");
			return;
		}
		while(read(connect_data_sock, data, MAX_BUF)>0)
		{
			write(writeFD, data, strlen(data));
		}
		close(writeFD);
		close(connect_data_sock);
		close(data_sock);
		receive_server_msg();
		printf("%s", msg_buf);
	}
	else   //PASV mode
	{
		while(read(connect_data_sock, data, MAX_BUF)>0)
		{
			printf("loop \n");
			write(writeFD, data, strlen(data));
		}
		close(writeFD);
		close(data_sock);
		receive_server_msg();
		printf("%s", msg_buf); 
	}
}

/*upload file user STOR*/
void upload_file()
{
	int connect_data_sock = -1, i = 0, count = 0;
	char filename[MAX_BUF] = {0};
	char buf[MAX_BUF] = {0};
	char data[MAX_BUF] = {0};
	struct stat file_info;
	int readFD;

	get_cmd_filename(cmd, filename);
	if(stat(filename, &file_info) < 0)
	{
		printf("local file %s doesn't exist!\n", filename);
		return;
	}

	readFD = open(filename, O_RDONLY);
	if(readFD < 0)
	{
		printf("open local file %s error!\n", filename);
		return;
	}

	data_sock = connect_server_data();
	if(data_sock < 0)
	{	
		printf("creat data sock error!\n");
		return;
	}

	send_server_msg("TYPE I\r\n",strlen("TYPE I\r\n"));
	receive_server_msg();
	printf("%s",msg_buf);

	memset(buf, 0, sizeof(buf));
	strcpy(buf, "STOR ");
	strcat(buf, filename);
	strcat(buf, "\r\n");
	send_server_msg(buf,strlen(buf));
	receive_server_msg();
	printf("%s",msg_buf);

	if(!mode)
	{
		while(i < 3)
		{
			connect_data_sock = accept(data_sock,NULL, NULL);
			if(connect_data_sock == -1)
			{
				continue;
			}
			else break;
		}
		if(connect_data_sock == -1)
		{
			printf("error in connect, can not use PORT mode\n");
			return;
		}
		while(1)
		{
			count = read(readFD, data, sizeof(data));
			if(count <= 0)
			{
				break;
			}
			else
			{
				write(connect_data_sock, data, sizeof(data));
			}
		}
		close(readFD);
		close(data_sock);
		close(connect_data_sock);
	}
	else
	{
		while(1)
		{
			count = read(readFD, data, sizeof(data));
			if(count <= 0)
			{
				break;
			}
			else
			{
				write(data_sock, data, sizeof(data));
			}
		}
		close(readFD);
		close(data_sock);
	}
	receive_server_msg();
	printf("%s",msg_buf);
}


void start_ftp_client()
{
	int login_flag = 1;
	while(1)
	{
		printf("ftp>");
		memset(cmd, '\0', sizeof(cmd));
		fgets(cmd, MAX_BUF, stdin);
		fflush(stdin);
		if(cmd[0] == '\n')continue;
		cmd[strlen(cmd) - 1] = '\0';
		if(strncmp(cmd, "open ", 5) == 0)
		{
			char ip[MAX_BUF]={0};
			int port = -1;
			if(get_ip_port(ip, &port) == 1)
			{
				printf("Invalid command!\n");
				continue;
			}
			if(connect_to_server(ip, port) == 0)
			{
				receive_server_msg();
				printf("%s",msg_buf);
				login_flag = login();
			}
			else
			{
				printf("Connect error!\n");
				continue;
			}
		}
		else if(strncmp(cmd,"mode",4) == 0)
		{
			if(login_flag == 0)
			{
				if(mode)
				{
					mode = 0;
					printf("change mode to PORT\n");
				}
				else
				{
					mode = 1;
					printf("change mode to PASV\n");
				}
			}
			else
			{
				printf("Not connected!\n");
			}
		}
		else if(strncmp(cmd,"put ",4) == 0)
		{
			if(login_flag == 0)
			{
				upload_file();
			}
			else
			{
				printf("Not connected!\n");
			}
		}
		else if(strncmp(cmd,"get ",4) == 0)
		{
			if(login_flag == 0)
			{
				download_file();
			}
			else
			{
				printf("Not connected!\n");
			}
		}
		else if(strncmp(cmd,"put ",4) == 0)
		{
			if(login_flag == 0)
			{
				upload_file();
			}
			else
			{
				printf("Not connected!\n");
			}
		}
		else if(strncmp(cmd, "close", 5) == 0)
		{
			send_server_msg("QUIT\r\n",strlen("QUIT\r\n"));
			receive_server_msg();
			printf("%s",msg_buf);
			close(client_sock);
			printf("Connection closed.\n");
		}
		else if(strncmp(cmd, "quit", 4) == 0)
		{
			send_server_msg("QUIT\r\n",strlen("QUIT\r\n"));
			receive_server_msg();
			printf("%s",msg_buf);
			close(client_sock);
			exit(0);
		}
		else if(strncmp(cmd, "bye", 3) == 0)
		{
			send_server_msg("QUIT\r\n",strlen("QUIT\r\n"));
			receive_server_msg();
			printf("%s",msg_buf);
			close(client_sock);
			exit(0);
		}
		else if(strncmp(cmd, "help", 4) == 0)
		{
			show_help();
		}
		else
		{
			printf("Invalid command!\n");
			show_help();
		}
	}		
		
}
int connect_to_server(char *ip, int port)
{
	struct sockaddr_in client_sock_addr;
    int res;
    client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (client_sock < 0)
    {
      perror("cannot create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&client_sock_addr, 0, sizeof(struct sockaddr_in));
 
    client_sock_addr.sin_family = AF_INET;
    client_sock_addr.sin_port = htons(port);
    res = inet_pton(AF_INET, ip, &client_sock_addr.sin_addr);
 
    if (0 == res)
    {
      perror("second parameter does not contain valid ipaddress");
      close(client_sock);
      exit(EXIT_FAILURE);
    }
 
    if (-1 == connect(client_sock, (const struct sockaddr *)&client_sock_addr, sizeof(struct sockaddr_in)))
    {
      perror("connect failed");
      close(client_sock);
      exit(EXIT_FAILURE);
    }
    return 0;
}


int send_server_msg(char* msg,int length)
{
  if(write(client_sock, msg, length)<0)
  {
	  perror("error send message to server\n");
	  return 1;
  }
  return 0;
}

int receive_server_msg()
{
	int res = 0;
	memset(msg_buf, 0, sizeof(msg_buf));
	int ret = read(client_sock, msg_buf, MAX_BUF);
	if(ret > 0)
		res = atoi(msg_buf);
	else
		return 0;
    return res;
}

void showUsage(char *cmd)
{
	printf("Usage:\n");
    printf("%s [host] [port]\n", cmd);
}

void show_help()
{	
	printf("\033[32mhelp\033[0m\t--print this command list\n");
//////////////////////////////////////////
	printf("\033[32mopen\033[0m\t--open the server\n");
	printf("\033[32mclose\033[0m\t--close the connection with the server\n");
	printf("\033[32mmkdir\033[0m\t--make new dir on the ftp server\n");
	printf("\033[32mrmdir\033[0m\t--delete the dir on the ftp server\n");
	printf("\033[32mdele\033[0m\t--delete the file on the ftp server\n");
//////////////////////////////////////////
	printf("\033[32mpwd\033[0m\t--print the current directory of server\n");
	printf("\033[32mls\033[0m\t--list the files and directoris in current directory of server\n");
	printf("\033[32mcd [directory]\033[0m\n\t--enter  of server\n");
	printf("\033[32mmode\033[0m\n\t--change current mode, PORT or PASV\n");
	printf("\033[32mput [local_file] \033[0m\n\t--send [local_file] to server as \n");
	printf("\tif  isn't given, it will be the same with [local_file] \n");
	printf("\tif there is any \' \' in , write like this \'\\ \'\n");
	printf("\033[32mget [remote file] \033[0m\n\t--get [remote file] to local host as\n");
	printf("\tif  isn't given, it will be the same with [remote_file] \n");
	printf("\tif there is any \' \' in , write like this \'\\ \'\n");
	printf("\033[32mlpwd\033[0m\t--print the current directory of local host\n");
	printf("\033[32mlls\033[0m\t--list the files and directoris in current directory of local host\n");
	printf("\033[32mlcd [directory]\033[0m\n\t--enter  of localhost\n");
	printf("\033[32mbye\033[0m\t--quit this ftp client program\n");
	printf("\033[32mquit\033[0m\t--quit this ftp client program\n");
}
