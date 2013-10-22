#ifndef __SERVER_H_
#define __SERVER_H_

#define MAX_BUF 1024
#define SERVER_PORT 21
#define MAX_NUM_USER 10

char msg_buf[MAX_BUF];
char default_user[MAX_NUM_USER][MAX_BUF];
char default_pwd[MAX_NUM_USER][MAX_BUF];
int numOfUser = 0;

char serverMsg220[]="220 Vince's FTP Server ready...\r\n";
char serverMsg230[]="230 User logged in, proceed.\r\n";
char serverMsgAnms331[]="331 User name okay, send your complete e-mail address as password.\r\n";
char serverMsg331[]="331 User name okay, User name okay, need password.\r\n";
char serverMsg221[]="221 Goodbye!\r\n";
char serverMsg150[]="150 File status okay; about to open data connection.\r\n";
char serverMsg226[]="226 Closing data connection.\r\n";
char serverMsg200[]="200 Command okay.\r\n";
char serverMsg215[]="215 Unix Type FC5.\r\n";
char serverMsg213[]="213 File status.\r\n";
char serverMsg211[]="211 System status, or system help reply.\r\n";
char serverMsg350[]="350 Requested file action pending further information.\r\n";
char serverMsg530[]="530 Not logged in.\r\n";
char serverMsg202[]="202 Command not implemented, superfluous at this site.\r\n";


void handle_client_request(int clientFD);
int send_client_msg(int clientFD,char* msg,int length);
int receive_client_msg(int clientFD);
int login(int clientFD);
void add_default_user();

#endif
