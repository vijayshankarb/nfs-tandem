/*
 * 
*/
#define MAXBUFFERSIZE 8192
#define FUSE_DEBUG
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <sys/dir.h>
#include <dirent.h>
#include <unistd.h>
#include "common.h"
#include "f2freq.h"

char pathName[255];
char bufString[8192];
int req, res = 0, n = 0;
typedef struct Node Node;

typedef struct sockaddr SA;

char remPath[255], sys[255];

static int chkUMP(const char *path, char *remPath, char *mountAtFS)
{
   FILE *fp;
   char mp[255];
   char temp1[255], temp2[255], fullPath[255], t[255];
	int retVal = 0;
	t[0] = 0;
   fp = fopen(MOUNT_TABLE, "r");
   while (fscanf(fp, "%s%s%s", mp,temp1,temp2) != EOF)
   {
      strcpy(fullPath, path);
      strcat(fullPath, "/");
      if (strncmp(fullPath, mp, strlen(mp)) == 0)
      {
			if (strlen(mp) >= strlen(t))
			{
				strcpy(t, mp);
         	strcpy(remPath, fullPath + strlen(mp));
         	strcpy(mountAtFS, temp1);
         	if (remPath[strlen(remPath) - 1] == '/')
            remPath[strlen(remPath) - 1] = 0;
				retVal = 1;
			}
      }
   }
   return retVal;
}

static int hello_readdir(const char *path, void *buf,
                         fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
   (void) offset;
   (void) fi;
   int i = 0, res = 0;
   int op = Readdir;
   char *p;
   DIR *dp;
   struct dirent *de;


   if ((n = chkUMP(path, remPath, sys)) == 0)
   {
      sprintf(pathName, HOME"%s", path);
      dp = opendir(pathName);
      if(dp == NULL)
         return -errno;

      while((de = readdir(dp)) != NULL)
      {
         struct stat st;
         memset(&st, 0, sizeof(st));
         st.st_ino = de->d_ino;
         st.st_mode= (de->d_type << 12);
         if (filler(buf, de->d_name, &st, 0))
            break;
      }

      closedir(dp);
   }
   else
   {
      strcpy(pathName, remPath);
      write(req, (char*)&op, sizeof(int));
      write(req, sys, 255);
      write(req, pathName, 255);

      read(req, (char*)&res, sizeof(int));
      if (res != 0)
         return res;
      read(req, bufString, 8192);
      p = bufString;
      while (*p != '\0' && (p - bufString) < 8192)
      {
         struct stat st;
         memset((char*)&st, 0, sizeof(stat));
         filler(buf, strdup(p), &st, 0);
         p += strlen(p) + 1;
      }
   }

   return 0;
}

static int hello_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;int mode;
    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
		  stbuf->st_size = 8;
		  return 0;
    }
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
			sprintf(pathName, HOME"%s", path);
    		res = lstat(pathName, stbuf);
	//		if (stbuf->st_mode & S_IFDIR)
	//				  stbuf->st_mode |= 0777;
			if (res == -1) res = -errno;
	 		return res;
	 }
    else
	 {
    		int op  = Getattr;
    		memset(stbuf, 0, sizeof(struct stat));
	 		strcpy(pathName, remPath);
	 		write(req, (char*)&op, sizeof(int));
			write(req, (char*)sys, 255);
	 		write(req, (char*)pathName, 255);
			read(req, (char*)&res, sizeof(int));
			if (res != 0) return res;
	 		read(req, (char*)stbuf, sizeof(struct stat));
	//		if (stbuf->st_mode & S_IFDIR)
	//				  stbuf->st_mode |= 0777;
	 		stbuf->st_size = (unsigned int)stbuf->st_size;
	 		return 0;
	 }
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	 int op = Open;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 sprintf(pathName, HOME"%s", path);
    fi->fh = res = open(pathName, fi->flags);
	 if (res == -1) return -errno;
	 return 0;
	 }
	 strcpy(pathName, remPath);
	 write(req, (char*)&op, sizeof(int));
	 write(req, sys, 255);
	 write(req, pathName, 255);
	 write(req, (char *)&fi->flags, sizeof(int));
	 read(req, (char*)&res, sizeof(int));
	 if (res != 0) return res;
	 read(req, (char*)&fi->fh, sizeof(unsigned long));
	 return 0;
}

static int hello_release(const char *path, struct fuse_file_info *fi)
{
	 int op = Close, res = 0;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 res = close(fi->fh);
	 if (res == -1) return -errno;
	 return 0;
	 }
	 write(req, (char*)&op, sizeof(int));
	 write(req, (char*)&fi->fh, sizeof(unsigned long));
	 read(req, (char*)&res, sizeof(int));
	 return res;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) fi;
	 int op = Read;

	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 int fd, count;
	 sprintf(pathName, HOME"%s", path);
    res = read(fi->fh, bufString, size);
	 if (res < 0)
				res = -errno;
	 else
	 {
	 	bufString[res] = 0;
	 	strcpy(buf, bufString);
	 }
	 return res;
	 }
	 write(req, (char*)&op, sizeof(int));
	 write(req, (char*)&size, sizeof(int));
	 write(req, (char*)&fi->fh, sizeof(unsigned long));
	 read(req, (char*)&res, sizeof(int));
	 read(req, (char*)buf, MAXSIZE);
	 buf[res] = 0;
	 return res;
}
static int hello_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
	 int op = Write;int res = 0;int count;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 int fd;
	 sprintf(pathName, HOME"%s", path);
	 fd = fi->fh;
    res = write(fd, buf, size);
	 	if (res == -1) res = -errno;
	 return res;
	 }
	 count = size < 1024 ? size : 1024;
	 write(req, (char*)&op, sizeof(int));
	 write(req, buf, 1024);
	 write(req, (char*)&count, sizeof(int));
	 write(req, (char*)&fi->fh, sizeof(unsigned long));
	 read(req, (char*)&res, sizeof(int));
    return res;
}
static int hello_mknod(const char *path, mode_t mode , dev_t rdev)
{
		  int res;
		  int op =Mknod;
		  if ((n = chkUMP(path, remPath, sys)) == 0)
		  {
	 	  sprintf(pathName, HOME"%s", path);

		  res = mknod(pathName, mode, rdev);
		  if (res == -1) res = -errno;
					 return res;
		  }
	 	  strcpy(pathName, remPath);
	 	  write(req, (char*)&op, sizeof(int));
		  write(req, (char*)sys, 255);
	 	  write(req, (char*)pathName, 255);
		  write(req, (char*)&mode ,sizeof(mode_t));
		  write(req, (char*)&rdev ,sizeof(dev_t));
		  read(req, (char*)&res, sizeof(int));
		  return res;
}
static int hello_mkdir(const char *path, mode_t mode)
{
	 int op = Mkdir;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 sprintf(pathName, HOME"%s", path);
    res = mkdir(pathName, mode);
	 if (res == -1) res =  -errno;
	 return res;
	 }
	 strcpy(pathName, remPath);
	 write(req, (char*)&op, sizeof(int));
	 write(req, (char*)sys, 255);
	 write(req, pathName, 255);
	 write(req, (char*)&mode, sizeof(mode));
	 read(req, (char*)&res, sizeof(int));
    return res;
}
static int hello_unlink(const char *path)
{
	 int op = Unlink;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 sprintf(pathName, HOME"%s", path);
    res = unlink(pathName);
	 if (res == -1) res = -errno;
	 return res;
	 }
	 strcpy(pathName, remPath);
	 write(req, (char*)&op, sizeof(int));
	 write(req, (char*)sys, 255);
	 write(req, pathName, 255);
	 read(req, (char*)&res, sizeof(int));
    return res;
}
static int hello_rmdir(const char *path)
{
	 int op = Rmdir;
	 if ((n = chkUMP(path, remPath, sys)) == 0)
	 {
	 sprintf(pathName, HOME"%s", path);
    res = rmdir(pathName);
	 if (res == -1) res = -errno;
	 return res;
	 }
	 strcpy(pathName, remPath);
	 write(req, (char*)&op, sizeof(int));
	 write(req, (char*)sys, 255);
	 write(req, pathName, 255);
	 read(req, (char*)&res, sizeof(int));
    return res;
}

static struct fuse_operations hello_oper = {
    .getattr	= hello_getattr,
    .readdir	= hello_readdir,
    .open	= hello_open,
    .read	= hello_read,
    .write	= hello_write,
	 .mknod = hello_mknod,
    .release = hello_release,
    .rmdir = hello_rmdir,
    .mkdir = hello_mkdir,
    .unlink = hello_unlink,
};

int main(int argc, char *argv[])
{
		struct sockaddr_in servaddr;
	   req = socket(AF_INET, SOCK_STREAM, 0);
		memset((char*)&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERVER_WELLKNOWN_PORT + 1);
		inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
		if (connect(req, (SA*)&servaddr, sizeof(servaddr)) < 0)
		{
			exit(1);
		}
		fuse_main(argc, argv, &hello_oper);
		return 0;
}
