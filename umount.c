#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "f2freq.h"
#define MOUNT_TABLE "/home/vijay/project/mount_table"
#define SERVER_WELLKNOWN_PORT 9877
typedef struct sockaddr SA;
static struct sockaddr_in servaddr;

int main(int argc, char *argv[])
{
		int clSockFd, req;
		int mptFound = 0, retVal = -1, mountIndex = 0, index = 0;
		FILE *fp = NULL, *tp = NULL;
		char mptAtServ[255], sys[255], toServ[255];
		char mp[255], smp[255];


	   clSockFd = socket(AF_INET, SOCK_STREAM, 0);
		memset((char*)&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERVER_WELLKNOWN_PORT + 1);
		inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
		if (connect(clSockFd, (SA*)&servaddr, sizeof(servaddr)) < 0)
		{
			exit(1);
		}
		
		fp = fopen(MOUNT_TABLE, "r");
      tp = fopen(FILES_HOME"/tmp$$", "w");
		
		req = Umount;
		strcpy(mp, argv[1]);
		strcat(mp, "/");
		while (fscanf(fp, "%s%s%s", smp, mptAtServ, sys) != EOF)
		{
			index++;
			if (strncmp(mp, smp, strlen(smp)) == 0)
			{
				strcpy(toServ, mptAtServ);
				mptFound = 1;
			   mountIndex = index;	
			}
				fprintf(tp, "%s %s %s\n", smp, mptAtServ, sys);
		}
		if (mptFound)
		{
	   	write(clSockFd, (char*)&req, sizeof(int));
			write(clSockFd, (char*)toServ, 255);
			read(clSockFd, (char*)&retVal, sizeof(int));
		}
		close(clSockFd);
		
		fclose(fp);
		fclose(tp);
		fp = fopen(MOUNT_TABLE, "w");
      tp = fopen(FILES_HOME"/tmp$$", "r");
		index = 0;

		while (fscanf(tp, "%s%s%s", smp, mptAtServ, sys) != EOF)
		{
				  if (++index != mountIndex)
						fprintf(fp, "%s %s %s\n", smp, mptAtServ, sys);
		}
		unlink(FILES_HOME"/tmp$$");
		return retVal;
}
