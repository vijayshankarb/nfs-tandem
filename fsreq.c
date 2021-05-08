/*
 *    fsreq.c: Request from NSK to CLIM
 *
 *    By Shruthi
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <assert.h>

#include <fuse.h>
#include "common.h"
#include "f2freq.h"

#define MAXBUFFERSIZE 8192

typedef struct sockaddr SA;

int length = 0;
int ind = 0;

struct table
{
   char fileName[FILENAMESIZE];
   fileHandle Fh;
}
Table[1024];

struct offTable
{
   unsigned long fileid;
   int fd;
   int offset;
}
OffTable[1024];

void assign(fileHandle Fh1, fileHandle Fh2)
{
   Fh1[0] = Fh2[0];
   Fh1[1] = Fh2[1];
}

int mountindex = 0;

static int _getattr(int clSockFd, const char *path, const char *sys,
                    struct stat *stbuf, int update)
{
   int res = 0;
   int mode = 0;

   onf_getattr_request o;
   onf_getattr_response or;

   	 o.ext.uid = geteuid();
   	 o.ext.gid = getegid();

   //  memset(stbuf, 0, sizeof(struct stat));
   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = GETATTR_REQUEST;
   o.fshdr.length = sizeof(o);
   strcpy(o.sys, sys);
   strcpy(o.pathName, path);
	o.update = update;
   write(clSockFd, (char*)&o, sizeof(o));
   read(clSockFd, (char*)&or, sizeof(or));
   if (update == 1)
	{
   	strcpy(Table[length].fileName, path);
   	assign(Table[length].Fh, or.Fh);
   	length++;
	}

   if ( or.fshdr.function == GETATTR_RESPONSE && or.fshdr.return_code == OK)
   {
      *stbuf = or.attr;
      stbuf->st_size = (unsigned int)stbuf->st_size;

      return 0;
   }

   return -or.fshdr.return_code;
}

static int _readdir(int clSockFd, const char *path, char *sys,
                    void *buf)
{
   int i = 0;
   onf_readdir_request o;
   onf_readdir_response or;
   char buffer[11], *p = NULL;

   	 o.ext.uid = geteuid();
   	 o.ext.gid = getegid();

   memset(buffer, 0, 11);
   bzero(&o, sizeof(onf_readdir_request));
   bzero(&or, sizeof(onf_readdir_response));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = READDIR_REQUEST;
   o.fshdr.length = sizeof(o);
   strcpy(o.sys, sys);
   strcpy(o.pathName, path);

   write(clSockFd, (char*)&o, sizeof(o));

   read(clSockFd, (char*)&or, sizeof(or));

   p = (char*)buf;
   p[0] = 0;
   if (or.fshdr.function == READDIR_RESPONSE)
   {
      if (or.fshdr.return_code == OK)
      {
         for (i = 0; i < or.nentries; i++)
         {
            struct stat st;
            memset((char*)&st, 0, sizeof(stat));

            read(clSockFd, buffer, 11);

            strcpy((char*)p, buffer);
            p += strlen(p) + 1;
         }

         *p = 0;
         return 0;
      }
   }

   return -or.fshdr.return_code;
}

int search(char *str)
{
   int i = 0;

   for (i = 0; i < length; i++)
      if (strcmp(Table[i].fileName, str) == 0)
		{
		   DEBUG("FSREQ SEARCH", str)
         return i;
      }
   return -1;
}

static int _open(int clSockFd, char *sysAtServ, const char *path,
                 int flags, unsigned long *pFileid)
{
   onf_openrequest o;
   onf_openresponse or;
   struct stat statbuf;

   char fullpath[255], str[255];
   char *p = NULL;

   o.ext.uid = geteuid();
   o.ext.gid = getegid();
   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));
   bzero(&statbuf, sizeof(statbuf));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = OPENREQUEST;
   o.fshdr.length = sizeof(o);
   strcpy(str, sysAtServ);
   strcpy(fullpath, path);
   p = strtok(fullpath, "/");

   do
   {
      strcat(str, p);
      DEBUG("FSREQ", str)
      if (search(str) == -1)
         break;
   }
   while (((p = strtok(NULL, "/")) != NULL) && strcat(str, "/"));

   do
   {
      _getattr(clSockFd, str, "",  &statbuf, 1);
      DEBUG("FSREQ", str)
   }
   while (((p = strtok(NULL, "/")) != NULL) && strcat(str, "/")
          && strcat(str, p) );

   assign(o.Fh, Table[search(str)].Fh);
   o.flags = flags;
   write(clSockFd, (char*)&o, sizeof(o));

   read(clSockFd, (char*)&or, sizeof(or));
	DEBUG("FSREQ ", "OPEN RETURNS" )
   *pFileid = OffTable[ind].fileid = ind;
   OffTable[ind].fd = or.fd;
   OffTable[ind].offset = 0;
   ind++;
   if (or.fshdr.return_code == OK)
      return 0;
   else
      return -or.fshdr.return_code;
}

static int _close(int clSockFd, unsigned long fileid)
{
   int i = 0, fd = -1;
   onf_closeresponse or;
   onf_closerequest o;

   o.ext.uid = geteuid();
   o.ext.gid = getegid();

   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = CLOSEREQUEST;
   o.fshdr.length = sizeof(o);
   for (i = 0; i < ind; i++)
   {
      if (OffTable[i].fileid == fileid)
      {
         fd = OffTable[i].fd;
         break;
      }
   }
	o.fd = fd;
   write(clSockFd, (char*)&o, sizeof(o));
   read(clSockFd, (char*)&or, sizeof(or));
   if (or.fshdr.function == CLOSERESPONSE)
   {
      if (or.fshdr.return_code == OK)
      {
				  for (i = 0; i < ind; i++)
				  {
							 if (OffTable[i].fd == or.fd)
							 {
										OffTable[i].fd = OffTable[i].fileid = -1;
										OffTable[i].offset = 0;
										break;
							 }
							 return 0;
				  }
      }
      return -or.fshdr.return_code;
   }
	return -1;
}
static int _read(int clSockFd, char *buf, size_t size
                 ,unsigned long fileid)

{
   size_t len = 0;
   int nread = 0, off = 0, i = 0, fd = -1;
   char buffer[MAXSIZE];
   onf_readresponse or;
   onf_readrequest o;
   	 o.ext.uid = geteuid();
   	 o.ext.gid = getegid();
   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = READREQUEST;
   o.fshdr.length = sizeof(o);

   for (i = 0; i < ind; i++)
   {
      if (OffTable[i].fileid == fileid)
      {
         fd = OffTable[i].fd;
         off = OffTable[i].offset;
         break;
      }
   }


   o.nbytes = size > 1024 ? 1024 :size;
   o.offset = off;
   o.fd = fd;
   sleep(2);
   write(clSockFd, (char*)&o, sizeof(o));
   read(clSockFd, (char*)&or, sizeof(or));
   if (or.fshdr.function == READRESPONSE)
   {
      if (or.fshdr.return_code == OK)
      {
         read(clSockFd, (char*)buf, 1024);
			strncpy(buffer, buf, 5)[5] = 0;
			DEBUG("FSREQ", buffer)
         OffTable[i].offset += or.nread;
         return or.nread;
      }
      return -or.fshdr.return_code;
   }

   return -or.fshdr.return_code;
}

static int _umount(int clSockFd, const char *from)
{
   int res = 0;
   char sys[255];
   onf_umount_request o;
   onf_umount_response or;
   o.ext.uid = geteuid();
   o.ext.gid = getegid();
   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = UMOUNTREQUEST;
   o.fshdr.length = sizeof(o);
   strcpy(sys, from);
	DEBUG("_UMOUNT", sys)
   write(clSockFd, (char*)&o, sizeof(o));
   write(clSockFd, (char*)sys, 255);
   read(clSockFd, (char*)&or, sizeof(or));
  if (or.fshdr.function == UMOUNTRESPONSE && or.fshdr.return_code == OK)
   {
      return 0;
   }
   return 0;
}
static int _mount(int clSockFd, const char *from, char *mptAtServ)
{
   int res = 0;
   char sys[255];
   onf_mount_request o;
   onf_mount_response or;
   	 o.ext.uid = geteuid();
   	 o.ext.gid = getegid();
   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = MOUNTREQUEST;
   o.fshdr.length = sizeof(o);
   strcpy(sys, from);
   //  o.index = ++mountindex;
   write(clSockFd, (char*)&o, sizeof(o));
   write(clSockFd, (char*)sys, 255);
   read(clSockFd, (char*)&or, sizeof(or));
   read(clSockFd, mptAtServ, 255);
   strcpy(Table[length].fileName, mptAtServ);
   assign(Table[length].Fh, or.Fh);
   length++;

   if (or.fshdr.function == MOUNTRESPONSE && or.fshdr.return_code == OK)
   {
      return 0;
   }

   return 0;
}
static int _mknod(int clSockFd, const char *sys, const char *path,
					 mode_t mode, dev_t rdev)
{
		  onf_mknod_request o;
		  onf_mknod_response or;
   	  bzero(&o, sizeof(o));
        bzero(&or, sizeof(or));
   	  o.ext.uid = geteuid();
   	  o.ext.gid = getegid();
   	  o.fshdr.magic = 0x0fbeda0f;
		  o.fshdr.function = MKNOD_REQUEST;
        o.fshdr.length = sizeof(o);
		  strcpy(o.sys, sys);
		  strcpy(o.pathName, path);
		  DEBUG("MKNOD",sys)
		  DEBUG("MKNOD",path)
		  o.mode = mode;
		  o.rdev = rdev;
		  write(clSockFd, (char*)&o, sizeof(o));
		  read(clSockFd, (char*)&or, sizeof(or));
		  if (or.fshdr.function == MKNOD_RESPONSE)
		  {
					 if(or.fshdr.return_code == OK)
								return 0;
					 else
								return -or.fshdr.return_code;
		  }
}

static int _write(int clSockFd, const char *buf, int size,
                  unsigned long fileid)
{
   int fd = -1, i = 0;
   char sendBuf[1024];
   onf_writerequest o;
   onf_writeresponse or;
   	 o.ext.uid = geteuid();
   	 o.ext.gid = getegid();

   bzero(&o, sizeof(o));
   bzero(&or, sizeof(or));
   memset(sendBuf, '\0', sizeof(sendBuf));

   o.fshdr.magic = 0x0fbeda0f;
   o.fshdr.function = WRITEREQUEST;
   o.fshdr.length = sizeof(o);

   for (i = 0; i < ind; i++)
   {
      if (OffTable[i].fileid == fileid)
      {
         fd = OffTable[i].fd;
         break;
      }
   }

   strncpy(sendBuf, buf, size);
   o.size = size;
	o.fd = fd;
   write(clSockFd, (char*)&o, sizeof(o));
   write(clSockFd, (char*)sendBuf, size);
   sleep(2);
   read(clSockFd, (char*)&or, sizeof(or));
   if (or.fshdr.function == WRITERESPONSE)
   {
      if (or.fshdr.return_code == OK)
         return or.nwritten;
   }
   return -or.fshdr.return_code;
}
static int _mkdir(int clSockFd, const char *sys, const char *path, mode_t mode)
{
    int res;
	 char name[1024];
	 onf_mkdirrequest o;
	 onf_mkdirresponse or;
    o.fshdr.magic = 0x0fbeda0f;
    o.fshdr.function = MKDIRREQUEST;
	 o.fshdr.length = sizeof(o);
	 strcpy(o.sys, sys);
	 strcpy(o.pathName, path);
	 o.mode = mode;
	 write(clSockFd, (char*)&o, sizeof(o));
	 read(clSockFd, (char*)&or, sizeof(or));
	 if (or.fshdr.function == MKDIRRESPONSE)
	 {
	 	if (or.fshdr.return_code == OK)
				  return 0;
	 }
    return -or.fshdr.return_code;
}

static int _unlink(int clSockFd, const char *sys, const char *path)
{
    int res;

	 onf_unlinkrequest o;
	 onf_unlinkresponse or;
    o.fshdr.magic = 0x0fbeda0f;
    o.fshdr.function = UNLINKREQUEST;
	 o.fshdr.length = sizeof(o);
	 write(clSockFd, (char*)&o, sizeof(o));
	 read(clSockFd, (char*)&or, sizeof(or));
	 if (or.fshdr.function == UNLINKRESPONSE)
	 {
	 	if (or.fshdr.return_code == OK)
				  return 0;
	 }

    return -1;
}

static int _rmdir(int clSockFd, const char *sys, const char *path)
{
    int res;

	 onf_rmdirrequest o;
	 onf_rmdirresponse or;
    o.fshdr.magic = 0x0fbeda0f;
    o.fshdr.function = RMDIRREQUEST;
	 o.fshdr.length = sizeof(o);
	 write(clSockFd, (char*)&o, sizeof(o));
	 read(clSockFd, (char*)&or, sizeof(or));
	 if (or.fshdr.function == RMDIRRESPONSE)
	 {
	 	if (or.fshdr.return_code == OK)
				  return 0;
	 }

    return -1;
}

void* callServer(void*);
struct args
{
   int connFd;
   int clSockFd;
};

int main(int argc, char *argv[])
{
   struct args arg;
   int retVal = 0;
   pid_t childPid = 0;
   size_t cliLen = 0;
   int listenFd = -1;
   int runFd = -1;

   struct sockaddr_in servAddr1, cliAddr;
   struct sockaddr_in servaddr;

   /* Initialize Globals */
   bzero(&Table, sizeof(Table));
   bzero(&OffTable, sizeof(OffTable));

   pthread_t tid = 0;
   bzero(&arg, sizeof(arg));

   bzero(&cliAddr, sizeof(cliAddr));
   bzero(&servaddr, sizeof(servaddr));

   arg.clSockFd = socket(AF_INET, SOCK_STREAM, 0);
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(SERVER_WELLKNOWN_PORT);

     inet_pton(AF_INET, SERVER_ADDRESS, &servaddr.sin_addr);

   if ( connect(arg.clSockFd, (SA*)&servaddr, sizeof(servaddr)) < 0 )
   {
      DEBUG("FSREQ", "Failed to Connect to Server");
      DEBUG("FSREQ", strerror(errno));
      exit(1);
   }

   if( (listenFd = socket (AF_INET, SOCK_STREAM, 0)) < 0 )
   {
      DEBUG("FSREQ", "No Listen File Descriptor");
      DEBUG("FSREQ", strerror(errno));
      exit(1);
   }

   bzero(&servAddr1, sizeof(servAddr1));

   servAddr1.sin_family = AF_INET;
   servAddr1.sin_addr.s_addr = INADDR_ANY;
   servAddr1.sin_port = htons (SERVER_WELLKNOWN_PORT + 1);

   if( bind (listenFd, (SA *) & servAddr1, sizeof (servAddr1)) < 0)
   {
      DEBUG("FSREQ", "Failed to Bind");
      DEBUG("FSREQ", strerror(errno));
      exit(1);
   }

   if( listen (listenFd, LISTENQ) < 0 )
   {
      DEBUG("FSREQ", "Failed to Listen");
      DEBUG("FSREQ", strerror(errno));
      exit(1);
   }

   for(;;)
   {
      cliLen = sizeof (cliAddr);
      arg.connFd = accept (listenFd, (SA *) &cliAddr, &cliLen);

      if( arg.connFd > 0 )
      {
         pthread_create(&tid, NULL, callServer, (void *)&arg);
      }
      else
      {
         sleep(1);
      }
   }
}

void *callServer(void* data)
{
   struct args arg = *((struct args *)data);
   int serv = arg.connFd;
   int clSockFd = arg.clSockFd;

   for(;;)
   {
      int req = 0, res = 0, n = 0, l = 0, flags = 0;
		unsigned long fileid = 0;
      char path[255], sys[255];
      char buf[8192];
      int size = 0;
      struct fuse_file_info fi;
      struct stat stbuf;
      char mptAtServ[255];
      char from[255], to[255];
      int ret = 0;
		mode_t mode;
		dev_t rdev;

      bzero(&fi, sizeof(fi));
      bzero(&stbuf, sizeof(stbuf));
      bzero(&path, sizeof(path));
      bzero(&sys, sizeof(sys));
      bzero(&buf, sizeof(buf));
      bzero(&mptAtServ, sizeof(mptAtServ));
      bzero(&from, sizeof(from));
      bzero(&to, sizeof(to));

      ret = read(serv, (char*)&req, sizeof(int));

      if( ret > 0 )
      {
         switch(req)
         {
         case Getattr:
            read(serv, sys, 255);
            read(serv, path, 255);
            res = _getattr(clSockFd, path, sys, &stbuf, 0);
            write(serv, (char*)&res, sizeof(int));
            if (res == 0)
               write(serv, (char*)&stbuf, sizeof(struct stat));
            break;
         case Readdir:
            read(serv, sys, 255);
            read(serv, path, 255);
            res = _readdir(clSockFd, path, sys, buf);
            write(serv, (char*)&res, sizeof(int));
            if (res == 0)
               write(serv, (char*)buf, 8192);
            break;
	 case Mkdir:
	   read(serv, sys, 255);
      read(serv, path, 255);
 	   read(serv, (char*)&mode, sizeof(mode));
	   res = _mkdir(clSockFd, sys, path, mode);
	   write(serv, (char*)&res, sizeof(int));
	   break;
   case Unlink:
	  read(serv, sys, 255);
     read(serv, path, 255);
	  res = _unlink(clSockFd, sys, path);
	  write(serv, (char*)&res, sizeof(int));
	  break;
	case Rmdir:
	  read(serv, sys, 255);
     read(serv, path, 255);
	  res = _rmdir(clSockFd, sys, path);
	  write(serv, (char*)&res, sizeof(int));
	  break;

         case Mount:
            read(serv, from, 255);
            res = _mount(clSockFd, from, mptAtServ);
            write(serv, mptAtServ, 255);
            close(serv);
            return NULL;
            break;
         case Umount:
            read(serv, from, 255);
            res = _umount(clSockFd, from);
            write(serv, (char*)&res, sizeof(int));
            close(serv);
            return NULL;
            break;
         case Open:
            read(serv, sys, 255);
            read(serv, path, 255);
            read(serv, (char*)&flags, sizeof(int));
            res = _open(clSockFd, sys, path, flags, &fileid);
            write(serv, (char*)&res, sizeof(int));
				if (res == 0)
            write(serv, (char*)&fileid, sizeof(unsigned long));
				DEBUG("FSREQ OPEN RETURNS", " TO FUSE 3")
            break;
			case Close:
				read(serv, (char*)&fileid, sizeof(unsigned long));
				res = _close(clSockFd, fileid);
				write(serv, (char*)&res, sizeof(int));
				break;
         case Read:
            read(serv, (char*)&size, sizeof(size_t));
            read(serv, (char*)&fileid, sizeof(unsigned long));
            res = _read(clSockFd, buf, size, fileid);
            write(serv, (char*)&res, sizeof(int));
            write(serv, buf, 1024);
            break;
         case Write:
            read(serv, buf, 1024);
            read(serv, (char*)&size, sizeof(int));
            read(serv, (char*)&fileid, sizeof(unsigned long));
            res =_write(clSockFd, buf, size, fileid);
            write(serv, (char*)&res, sizeof(int));
            break;
	case Mknod:
            read(serv, sys, 255);
            read(serv, path, 255);
				read(serv, (char*)&mode, sizeof(mode_t));
				read(serv, (char*)&rdev, sizeof(dev_t));
				DEBUG("MKNOD", "HERE1")
            res = _mknod(clSockFd, sys, path, mode, rdev);
            write(serv, (char*)&res, sizeof(int));
				DEBUG("MKNOD", "HERE2")
         }
      }
      else
      {
         DEBUG("FSREQ", strerror(errno));
         return NULL;
      }
   }

   return NULL;
}
