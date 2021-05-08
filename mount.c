#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "f2freq.h"
#define SERVER_WELLKNOWN_PORT 9877
typedef struct sockaddr SA;
static struct sockaddr_in servaddr;

int main(int argc, char *argv[])
{
		int clSockFd, req;
		FILE *fp = NULL;
		char mptAtServ[255], sys[255];
		fp = fopen(MOUNT_TABLE, "a");

	   clSockFd = socket(AF_INET, SOCK_STREAM, 0);
		memset((char*)&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERVER_WELLKNOWN_PORT + 1);
		inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
		if (connect(clSockFd, (SA*)&servaddr, sizeof(servaddr)) < 0)
		{
			exit(1);
		}
		req = Mount;
	   write(clSockFd, (char*)&req, sizeof(int));
		strcpy(sys, argv[1]);
		write(clSockFd, (char*)sys, sizeof(sys));
		read(clSockFd, (char*)mptAtServ, sizeof(mptAtServ));
		fprintf(fp, "%s/ %s %s\n", argv[2], mptAtServ, sys);
		fclose(fp);
		close(clSockFd);

		return 0;
}
