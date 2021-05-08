#
#  Makefile for NFS Client
#  Author : VijayShankar. B
#
.SUFFIXES: .c .o

.c.o:
	echo Compiling $<
	gcc -c -D_FILE_OFFSET_BITS=64 -g -Wall $< 2>$(<:.c=.err)

all: server req nfsmnt hello nfsumnt

server: server.o nfsmount.o
	echo Linking $@
	gcc -g -o $@ -pthread server.o nfsmount.o

req: fsreq.o
	gcc -Wall -D_FILE_OFFSET_BITS=64 -g -o $@ $< -pthread
	echo Linking $@
	gcc -g -o $@ -pthread $<

nfsmnt: mount.o
	echo Linking $@
	gcc -g -o $@ -pthread $<
	
nfsumnt: umount.o
	echo Linking $@
	gcc -g -o $@ -pthread $<

hello: hello.c
	gcc -D_REENTRANT -DFUSE_USE_VERSION=22 -D_FILE_OFFSET_BITS=64 -o hello.o  -c hello.c
	/bin/sh  ./libtool --mode=link gcc -Wall -W -g -O2 -o $@ hello.o ./libfuse.la -lpthread
	gcc -Wall -W -g -O2 -o hello hello.o ./libfuse.so -lpthread -Wl,--rpath -Wl,/usr/local/lib

clean:
	rm -f *.o
	rm -f server req mnt hello

runvalgrind:

runstrace:

run:


