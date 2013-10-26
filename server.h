#ifndef __SERVER_H_
#define __SERVER_H_

#define MAX_BUF 1024
#define SERVER_PORT 21
#define MAX_NUM_USER 10
#define PORT_SUCCESS         "200 PORT command successful.\r\n"
#define TYPE_I_SUCCESS         "200 Type set to I.\r\n"
#

char msg_buf[MAX_BUF]={0};
char ftp_path[MAX_BUF]={0};
char pwd[MAX_BUF] = {0};
char default_user[MAX_NUM_USER][MAX_BUF];
char default_pwd[MAX_NUM_USER][MAX_BUF];
int numOfUser = 0;

struct sockaddr_in server;
struct sockaddr_in data_transfer_addr;

int ftp_data_sock = -1;
int connect_data_sock = -1;
int connect_mode = 0;   // 0 means PASV while 1 means PORT


char serverMsg220[]="220 Vince's FTP Server ready...\r\n";
char serverMsg230[]="230-\r\n230-Welcome to Vince's FTP\r\n230-\r\n230-This FTP program is made by Wen Qingfu.\r\n230-Use in violation of any applicable laws is strictly\r\n230-prohibited. We make no guarantees, explicit or implicit, about the\r\n230-contents of this site. Use at your own risk.\r\n230-\r\n230 User logged in, proceed.\r\n";
char serverMsgAnms331[]="331 User name okay, send your complete e-mail address as password.\r\n";
char serverMsg331[]="331 User name okay, need password.\r\n";
char serverMsg221[]="221-Thank you for using Vince's FTP\r\n221 Goodbye!\r\n";
char serverMsgRETR150[]="150 Opening BINARY mode data connection for ";
char serverMsgSTOR150[]="150 Opening BINARY mode data connection for ";
char serverMsg226[]="226 Transfer complete.\r\n";
char serverMsg215[]="215 Unix Type: L8.\r\n";
char serverMsg213[]="213 File status.\r\n";
char serverMsg211[]="211 System status, or system help reply.\r\n";
char serverMsg350[]="350 Requested file action pending further information.\r\n";
char serverMsg530[]="530 Not logged in.\r\n";
char serverMsg202[]="202 Command not implemented, superfluous at this site.\r\n";
char serverMsg425[]="425 Can't open data connection.\r\n";
char serverMsg426[]="426 Connection closed; transfer aborted.\r\n";
char serverMsg451[]="451 Requested action aborted, error in reading file from disk.\r\n";
char serverMsgSTOR451[]="451 Requested action aborted, error in writing file.\r\n";

void handle_client_request(int clientFD, struct sockaddr_in client);
int send_client_msg(int clientFD,char* msg,int length);
int receive_client_msg(int clientFD);
int login(int clientFD);
void init();
void get_ip(char *buffer,char *ip_address,int *port);
void do_pasv(int clientFD, struct sockaddr_in client);
void do_port(int clientFD);
void do_retr(int clientFD);
void do_stor(int clientFD);
int mkdir_r(const char *path);
#endif
