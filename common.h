/*
 *    common.h: Commonly used macros and structures
 *
 *    Authored By VijayShankar. B
 *    e-Mail : vijayshankarb@gmail.com
 *
 */

#ifndef _COMMON_H
#define _COMMON_H

#define FILENAMESIZE 255
#define LINES_PER_PAGE  25
#define LINE_INPUT 1025
#define SERVER_WELLKNOWN_PORT 9877
#define SEG_SIZE 32766
#define LISTENQ 10
#define MAXSIZE 1024

#define OPENREQUEST 1
#define OPENRESPONSE 2
#define READREQUEST 3
#define READRESPONSE 4
#define CLOSEREQUEST 5
#define CLOSERESPONSE 6
#define READDIR_REQUEST 7
#define READDIR_RESPONSE 8
#define GETATTR_REQUEST 9
#define GETATTR_RESPONSE 10
#define MKDIRREQUEST 11
#define MKDIRRESPONSE 12
#define UNLINKREQUEST 13 
#define UNLINKRESPONSE 14
#define RMDIRREQUEST 15
#define RMDIRRESPONSE 16
#define UMOUNTREQUEST 17
#define UMOUNTRESPONSE 18
#define WRITEREQUEST 27
#define WRITERESPONSE 28
#define MOUNTREQUEST 33
#define MOUNTRESPONSE 34
#define MKNOD_REQUEST 35
#define MKNOD_RESPONSE 36
#define OK 101
#define EACCESS 102

#define MP_ADDRESS  "/home/vijay/project"
#define SERVER_ADDRESS "127.0.0.1"
#define HOME "/home/vijay/nsk"

#ifndef NDEBUG
#define DEBUG(module, str) { \
	FILE *fp = NULL; \
	if ( (fp = fopen("err1.txt", "a")) != NULL) \
   { \
      fprintf(fp, "%s:%d - %s : %s\n", __FILE__, __LINE__, module, str); \
      fclose(fp); \
   } \
}
#else
#define DEBUG(str)
#endif

#include <utime.h>

//FIXME:
typedef enum bool_tag { false , true } bool;
typedef unsigned long long fileHandle[2];

typedef struct _onf_fsproto_header
{
   unsigned int magic;
   unsigned short function;
   unsigned short length;
   unsigned char flags;
   unsigned char flow_control;
   unsigned short  return_code;
   unsigned int handle;
}
onf_fs_proto_header;

typedef struct _onf_reqext_header
{
   unsigned int uid;
   unsigned int gid;
}
onf_reqext_header;

typedef struct _onf_fileattr
{
   unsigned int filetype;
   unsigned int mode;
   unsigned int nlink;
   unsigned int uid;
   unsigned int gid;
   unsigned long long size;
   time_t atime, mtime, ctime;
   unsigned int spec1;
   unsigned int spec2;
}
onf_fileattr;

typedef struct _onf_openrequest
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   fileHandle Fh;
   unsigned int flags;
}
onf_openrequest;

typedef struct _onf_openresponse
{
   onf_fs_proto_header fshdr;
   unsigned int fd;
}
onf_openresponse;

typedef struct _onf_closerequest
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   int fd ;
}
onf_closerequest;

typedef struct _onf_closeresponse
{
   onf_fs_proto_header fshdr;
   int fd;
}
onf_closeresponse;

typedef struct _onf_mount_request
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
}
onf_mount_request;

typedef struct _onf_mount_response
{
   onf_fs_proto_header fshdr;
   fileHandle Fh;
}
onf_mount_response;

typedef struct _onf_umount_request
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
}
onf_umount_request;

typedef struct _onf_umount_response
{
   onf_fs_proto_header fshdr;
}
onf_umount_response;
typedef struct _onf_readrequest
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   int fd;
   unsigned int nbytes;
   unsigned int offset;
}
onf_readrequest;

typedef struct _onf_writerequest
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   int fd;
   unsigned int size;
}
onf_writerequest;

typedef struct _onf_readresponse
{
   onf_fs_proto_header fshdr;
   unsigned int nread;
}
onf_readresponse;

typedef struct _onf_writeresponse
{
   onf_fs_proto_header fshdr;
   unsigned int nwritten;
}
onf_writeresponse;

typedef struct _onf_readdir_request
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   char sys[FILENAMESIZE];
   char pathName[FILENAMESIZE];
}
onf_readdir_request;

typedef struct _onf_readdir_response
{
   onf_fs_proto_header fshdr;
   unsigned int nentries;
}
onf_readdir_response;

typedef struct _onf_getattr_request
{
   onf_fs_proto_header fshdr;
   onf_reqext_header ext;
   char sys[FILENAMESIZE];
   char pathName[FILENAMESIZE];
	int update;
}
onf_getattr_request;

typedef struct _onf_getattr_response
{
   onf_fs_proto_header fshdr;
   struct stat attr;
   fileHandle Fh;
}onf_getattr_response;
typedef struct _onf_mknod_request
{
		  onf_fs_proto_header fshdr;
   	  onf_reqext_header ext;
   	  char sys[FILENAMESIZE];
   	  char pathName[FILENAMESIZE];
		  mode_t mode;
		  dev_t rdev;
} onf_mknod_request;
typedef struct _onf_mknod_response
{
		  onf_fs_proto_header fshdr;
} onf_mknod_response;

typedef struct _onf_mkdir_request
{
		  onf_fs_proto_header fshdr;
   	  onf_reqext_header ext;
   	  char sys[FILENAMESIZE];
   	  char pathName[FILENAMESIZE];
		  mode_t mode;
} onf_mkdirrequest;
typedef struct _onf_mkdir_response
{
		  onf_fs_proto_header fshdr;
} onf_mkdirresponse;

typedef struct _onf_rmdir_request
{
		  onf_fs_proto_header fshdr;
   	  onf_reqext_header ext;
   	  char sys[FILENAMESIZE];
   	  char pathName[FILENAMESIZE];
} onf_rmdirrequest;
typedef struct _onf_rmdir_response
{
		  onf_fs_proto_header fshdr;
} onf_rmdirresponse;
typedef struct _onf_unlink_request
{
		  onf_fs_proto_header fshdr;
   	  onf_reqext_header ext;
   	  char sys[FILENAMESIZE];
   	  char pathName[FILENAMESIZE];
} onf_unlinkrequest;
typedef struct _onf_unlink_response
{
		  onf_fs_proto_header fshdr;
} onf_unlinkresponse;

extern int do_mount(char*, char*);

#endif // _COMMON_H
