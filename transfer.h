#ifndef _TRANSFER_H_
#define _TRANSFER_H_

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>



void do_retr(int clientFD)
{
	int i = 0;
	int length = strlen(msg_buf);
	char filename[MAX_BUF] = {0};
	for(i = 5; i < length; i++)
	{
		filename[i-5] = msg_buf[i];
	}
	filename[i-6] = '\0';

 	long file_size = 0;
 	char size[MAX_BUF];
 	char msg[MAX_BUF];
    FILE *pFile = fopen(filename,"r");
    if(pFile == NULL)
    {
       perror("error PETR file not exit");
    }
    else
    {
      fseek(pFile, 0, SEEK_END);//移向END
      file_size = ftell(pFile);
    }
    fclose(pFile);
    sprintf(size, "%ld", file_size);
    strcat(size, "\r\n");
    strcpy(msg, serverMsg150);
   	strcat(msg, size);
   	send_client_msg(clientFD, msg, strlen(msg));
}

#endif