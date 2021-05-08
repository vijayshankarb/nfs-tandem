/*
 *    server.c: Server CLIM
 *
 *    Authored By VijayShankar. B
 *    e-Mail : vijayshankarb@gmail.com
 *
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <linux/wait.h>
#include <setjmp.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#include <pthread.h>

#ifdef linux
#define _XOPEN_SOURCE 500
#endif

typedef struct sockaddr SA;

const int header_size = sizeof (onf_fs_proto_header)
		                      + sizeof(onf_reqext_header);

/* Global Variables */
int ind = 0;
int mountindex = 0;
char dir[FILENAMESIZE];
struct table
{
   char pathName[FILENAMESIZE];
   fileHandle Fh;
}
Table[1024];
char mntpath[255];

void computeFH(fileHandle fh, char sys[FILENAMESIZE])
{
   struct stat u;
   unsigned long d0, d1;
   lstat(sys, &u);
   d0 = ((unsigned long*)&u.st_dev)[0];
   d1 = ((unsigned long*)&u.st_dev)[1];
   fh[1] |= d1;
   fh[0] |= d0;
   fh[0] <<= 32;
   fh[0] |= u.st_ino;
}

int cmp(fileHandle Fh1, fileHandle Fh2)
{
   return Fh1[0] == Fh2[0] && Fh1[1] == Fh2[1];
}

void assign(fileHandle Fh1, fileHandle Fh2)
{
   Fh1[0] = Fh2[0] ;
   Fh1[1] = Fh2[1];
}

void*
callServer (void *s)
{
   onf_fs_proto_header o;
   onf_reqext_header r;

	bzero(&o, sizeof(o));
   bzero(&r, sizeof(r));

   int sockFd = *((int*)s);

   for (;;)
   {
      read (sockFd, (char*)&o, sizeof (o));

      if (o.magic != 0x0fbeda0f)
         return NULL;
      read (sockFd, (char*)&r, sizeof (r));

      DEBUG("MAIN", "SWITCH")

      switch (o.function)
		{
		case MOUNTREQUEST:
		   {
		      char sys[255];

		      onf_mount_request op;
		      onf_mount_response or;
		      seteuid(0);
		      setegid(0);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				memset(sys, '\0', sizeof(sys));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_mount_request) - header_size);
		      read(sockFd, (char*)sys, 255);


		      sprintf(mntpath, MP_ADDRESS"/MP_%d/",++mountindex);

		      if (do_mount(sys, mntpath) != 0)
				{
						  exit(1);
				}

		      computeFH(or.Fh, mntpath);
		      assign(Table[ind].Fh , or.Fh);
		      strcpy(Table[ind].pathName, mntpath);
		      ind++;
		      or.fshdr.function = MOUNTRESPONSE;
		      or.fshdr.return_code = OK;
		      seteuid(-2);
		    	setegid(-2);
		      write (sockFd, (char *)&or, sizeof (onf_mount_response));
		      write(sockFd, (char*)mntpath, 255);
		      break;
		   }
		case UMOUNTREQUEST:
		{
		      char sys[255];
		      onf_umount_request op;
		      onf_umount_response or;
		      seteuid(0);
		      setegid(0);
				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				memset(sys, '\0', sizeof(sys));
		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_umount_request) - header_size);
		      read(sockFd, (char*)sys, 255);
		      or.fshdr.return_code = OK;
				DEBUG("UMOUNT", sys)
				if (umount(sys) == -1) or.fshdr.return_code = errno;
		      or.fshdr.function = UMOUNTRESPONSE;
		      seteuid(-2);
		    	setegid(-2);
		      write (sockFd, (char *)&or, sizeof (onf_umount_response));
		      break;
		   }
		case OPENREQUEST:
		   {
		      onf_openrequest op;
		      onf_openresponse or;

		      int i = 0, fd = -1;
		      char buffer[255];
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				memset(buffer, '\0', sizeof(buffer));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_openrequest) - header_size);
		      for (i = 0; i < ind; i++)
		      {
		         if (cmp(Table[i].Fh, op.Fh))
		            break;
		      }

		      if (i < ind)
		         strcpy(buffer, Table[i].pathName);
		      or.fshdr.function = OPENRESPONSE;

				DEBUG("SERVER OPEN", buffer)
		      if ((fd = open(buffer, op.flags)) == -1)
		         or.fshdr.return_code = errno;
		      else
		      {
		         or.fshdr.return_code = OK;
		         or.fd = fd;
					DEBUG("OPEN", "SUCCESS")
		      }
		      		seteuid(-2);
		      	   setegid(-2);
		      write (sockFd, (char *) &or, sizeof (onf_openresponse));
		      break;
		   }
		case CLOSEREQUEST:
		   {
		      int err, fd;
		      onf_closeresponse or;
		      onf_closerequest op;
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_closerequest) - header_size);
		      err  = close(op.fd);
		      or.fshdr.function = CLOSERESPONSE;
		      if (err == -1)
		         or.fshdr.return_code = EACCES;
		      else
		      {
		         or.fshdr.return_code = OK;
		         or.fd = fd;
					DEBUG("CLOSE", "I AM HERE")
		      }
		      		seteuid(-2);
		      		setegid(-2);
		      write (sockFd, (char *) &or, sizeof (onf_closeresponse));
		      break;
		   }
		case READREQUEST:
		   {
		      int err = 0, nread = 0, fd = -1;
		      char buffer[1025];
		      char name[1024];
		      onf_readresponse or;
		      onf_readrequest op;
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				memset(buffer, '\0', sizeof(buffer));
				memset(name, '\0', sizeof(name));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_readrequest) - header_size);
		      or.fshdr.function = READRESPONSE;
				lseek(op.fd, op.offset, 0);
		      if ((nread = read(op.fd, buffer, 1024)) == -1)
		      {
		         or.fshdr.return_code = errno;
		         		seteuid(-2);
		         		setegid(-2);
		         write (sockFd, (char *) &or, sizeof (onf_readresponse));
		      }
		      else
		      {
		         or.fshdr.return_code = OK;
		         or.nread = nread;
		         		seteuid(-2);
		         		setegid(-2);
		         write (sockFd, (char *) &or, sizeof (onf_readresponse));
		         write(sockFd, buffer, 1024);
		      }
		      break;
		   }
		case READDIR_REQUEST:
		   {
		      int err = 0, nread = 0, fd = -1, i = 0;
		      struct direct dirbuf;
		      const int fileNameSize = 10;
		      onf_readdir_response or;
		      onf_readdir_request op;
		      char name[1024];
		      char dirname[1024];
		      char array[1024][11];
		      DIR *dp = NULL;
		      struct dirent *de = NULL;
		      struct stat st[1024];
		      int index = 0;
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				bzero(&dirbuf, sizeof(dirbuf));
				bzero(&st, sizeof(st));

				memset(dirname, '\0', sizeof(dirname));
				memset(name, '\0', sizeof(name));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_readdir_request) - header_size);
		      sprintf(dirname, "%s%s", op.sys, op.pathName);
		      or.fshdr.function = READDIR_RESPONSE;
		      dp = opendir (dirname);
		      if (dp == NULL)
		      {
		         or.fshdr.return_code = errno;
		         write (sockFd, (char *) &or, sizeof (onf_readdir_response));
		         break;
		      }
		      or.fshdr.return_code = OK;
		      i = 0;
		      while ((de = readdir (dp)) != NULL)
		      {
		         strcpy (array[i], de->d_name);
		         i++;
		      }
		      or.nentries = i;
		      	 seteuid(-2);
		      	 setegid(-2);
		      write (sockFd, (char *) &or, sizeof (onf_readdir_response));
		      for (index = 0; index < i; index++)
		      {
		         write (sockFd, array[index], fileNameSize + 1);
		         //write (sockFd, (char*)&st[index],sizeof(struct stat));
		      }
		      closedir (dp);
		      break;
		   }
		case MKNOD_REQUEST:
			{
			  onf_mknod_response or;
			  onf_mknod_request op;
			  char name[1024];
		   seteuid(0);
		   setegid(0);
			  memset(name, '\0', sizeof(name));
			  bzero(&op, sizeof(op));
			  bzero(&or, sizeof(or));
			DEBUG("MKNOD", ": HERE")
		      read (sockFd, (char *) ((char *) &op + header_size),
	            sizeof (onf_mknod_request) - header_size);
	      sprintf(name, "%s%s", op.sys, op.pathName);
			DEBUG("MKNOD", name)
			or.fshdr.function = MKNOD_RESPONSE;
			if (mknod(name, op.mode, op.rdev) != -1)
				or.fshdr.return_code = OK;
			else
			  or.fshdr.return_code = errno;
		      		seteuid(-2);
		      		setegid(-2);
	      write (sockFd, (char *) &or, sizeof(or));
		   break;
			}
	case UNLINKREQUEST:
	  {
	 char name[1024];
	    onf_unlinkresponse or;
	    onf_unlinkrequest op;
		   seteuid(r.uid);
		   setegid(r.gid);
	    read (sockFd, (char *) ((char *) &op + header_size),
		  sizeof (onf_unlinkrequest) - header_size);
	      sprintf(name, "%s%s", op.sys, op.pathName);

		 or.fshdr.function = UNLINKRESPONSE;
		 if (unlink(name) == -1)
		 or.fshdr.return_code = errno;
		 or.fshdr.return_code = OK;
		   seteuid(-2);
		   setegid(-2);
	    write (sockFd, (char *) &or, sizeof(or));
				 break;
	  }
	case MKDIRREQUEST:
	  {
		 char name[1024];
		 int fd;
	    onf_mkdirresponse or;
	    onf_mkdirrequest op;
		   seteuid(r.uid);
		   setegid(r.gid);

	    read (sockFd, (char *) ((char *) &op + header_size),
		  sizeof (onf_mkdirrequest) - header_size);
		 sprintf(name, "%s%s", op.sys, op.pathName);
		 DEBUG("MKDIR", name)
		 or.fshdr.return_code = OK;
		 if(mkdir(name, op.mode) == -1)
					or.fshdr.return_code = errno;
		 or.fshdr.function = MKDIRRESPONSE;
	    write (sockFd, (char *) &or, sizeof(or));
		   seteuid(-2);
		   setegid(-2);
		 break;
	  }
	case RMDIRREQUEST:
	  {
		 char name[1024];
	    onf_rmdirresponse or;
	    onf_rmdirrequest op;
		 seteuid(r.uid);
		 setegid(r.gid);
	    read (sockFd, (char *) ((char *) &op + header_size),
		  sizeof (onf_rmdirrequest) - header_size);
		 sprintf(name, "%s%s", op.sys, op.pathName);
		 or.fshdr.function = RMDIRRESPONSE;
		 or.fshdr.return_code = OK;
		 if(rmdir(name) == -1) or.fshdr.return_code = errno;
 		 seteuid(-2);
		 setegid(-2);
	    write (sockFd, (char *) &or, sizeof(or));
				 break;
	  }
		case GETATTR_REQUEST:
		   {
		      onf_getattr_response or;
		      onf_getattr_request op;
		      struct stat attr;
		      char name[1024];
		      char buffer[255];
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				bzero(&attr, sizeof(attr));
				memset(name, '\0', sizeof(name));
				memset(buffer, '\0', sizeof(buffer));

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_getattr_request) - header_size);
		      sprintf(mntpath, "%s%s", op.sys, op.pathName);
		      DEBUG("GETATTR", mntpath)
			   if (op.update)
				{
		      	computeFH(or.Fh, mntpath);
		      	assign(Table[ind].Fh , or.Fh);
		      	strcpy(Table[ind].pathName, mntpath);
		      	ind++;
				}
		      or.fshdr.return_code = OK;
		      if (lstat(mntpath, &or.attr) == -1)
		         or.fshdr.return_code = errno;

		      or.fshdr.function = GETATTR_RESPONSE;
		      	seteuid(-2);
		      	setegid(-2);
		      write (sockFd, (char *) &or, sizeof(or));
		      break;
		   }
		case WRITEREQUEST:
		   {
		      char buf[8193];
		      int fd = -1, n = 8192;
		      char tmp[10];
		      char name[255];
		      onf_writeresponse or;
		      onf_writerequest op;
		      seteuid(r.uid);
		      setegid(r.gid);

				bzero(&op, sizeof(op));
				bzero(&op, sizeof(or));
				memset(tmp, '\0', sizeof(tmp));
				memset(name, '\0', sizeof(name));
				memset(buf, '\0', sizeof(buf));

		      sleep(2);

		      read (sockFd, (char *) ((char *) &op + header_size),
		            sizeof (onf_writerequest) - header_size);
		      read (sockFd, (char*)buf, op.size);
		      buf[op.size] = 0;
		      n = op.size;
				DEBUG("WRITE", buf)
		      if( (or.nwritten = write(op.fd, buf, n)) == -1 )
		         or.fshdr.return_code = errno;
		      else
		         or.fshdr.return_code = OK;

		      or.fshdr.function = WRITERESPONSE;
		      		seteuid(-2);
		      		setegid(-2);
		      write (sockFd, (char *)&or, sizeof(or));
		      break;
		   }
		}
   }

   return 0;
}

int
main (int argc, char **argv)
{
   int listenFd = -1;
	int retVal = 0;
   pid_t childPid;
   size_t cliLen = 0;
   struct sockaddr_in servAddr, cliAddr;
   int connFd;
   pthread_t tid;

	bzero(&cliAddr, sizeof(cliAddr));
   bzero(&servAddr, sizeof(servAddr));

   listenFd = socket (AF_INET, SOCK_STREAM, 0);

   if( listenFd > 0 )
   {
      memset ((char *) &servAddr, 0, sizeof (servAddr));

      servAddr.sin_family = AF_INET;
      servAddr.sin_addr.s_addr = INADDR_ANY;
      servAddr.sin_port = htons (SERVER_WELLKNOWN_PORT);

      if ( bind (listenFd, (SA *) & servAddr, sizeof (servAddr)) < 0)
		{
			fprintf(stderr, "Bind failed for Port : %d", servAddr.sin_port);
			fprintf(stderr, "Error : %s", strerror(errno));
			fprintf(stderr, "Server Process Exiting Now . . .");
			exit(1);
		}

      if( listen (listenFd, LISTENQ) < 0 )
		{
			fprintf(stderr, "Listen failed for Port : %d", servAddr.sin_port);
			fprintf(stderr, "Error : %s", strerror(errno));
			fprintf(stderr, "Server Process Exiting Now . . .");
			exit(1);
		}

      for(;;)
		{
		   int *connFd = (int *)malloc(sizeof(int));
		   cliLen = sizeof (cliAddr);
		   *connFd = accept (listenFd, (SA *) & cliAddr, &cliLen);

		   if( *connFd > 0)
		   {
		      DEBUG("ACCEPT", "ME")
		      pthread_create(&tid, NULL, callServer,(void*)connFd);
		   }
		   else
		   {
		      sleep(1);
		   }
		}
   }

   return 0;
}
