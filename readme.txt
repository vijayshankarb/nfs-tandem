Instructions for compiling and running the project
==================================================

    Once the files have been copied to a folder do the following, 

1.) FUSE latest version has to be installed in the system, FUSE gives libraries for linking and utilities to execute or unmount directories
    FUSE can be loaded by downloading the installable file from sourceforge.net.Though only a part of FUSE is needed for compilation a large         	number of utilities for unmounting and misc execution requires that the FUSE installation is complete. 

2.) libfuse.so (a shared library) and libfuse.la (a standard library) have to be present in the same directory as the executable as 

    a requirement of my Makefile. These files are found in the /lib directory of FUSE installation
    
3.) In order to compile and link the files just type, make 

4.) Go to root mode using, su and type  > /dev/fuse , this is the main device file from where system calls are read. Give this file 

	permissions 666			    ===========
	
5.)Create a directory /fuse under the present directory 

6.)A fuse file system needs a home directory from which mounts, directory creation occurs. This directory has been defined as HOME in "common.h"

and should be changed accordingly before compilation to the actual directory say, eg. /home/nsk/start/

7.)A hard code mount point is created by FS_SERV to do the actual NFS mountings. The directory under which these mount points are created is 

defined as MP_ADDRESS and should be changed to actual directory .

8.)IP Address of FS_SERV system for FS_REQ to connect is defined as SERVER_ADDRESS and can be changed accordingly. 

9.)In root mode, type 
				./server & 
                               --> this gets the FS_SERV running
                               
10.)In user mode type 
				./req &   
                               --> this gets the FS_REQ running 
                               
11.) ./nfsmnt 127.0.0.1:/home/nsk/earth/ /a

This assumes two things that a directory /home/nsk/earth has been exported to the same machine

The loopback 127.0.0.1 can replaced by another IP address as per requirements.

And there is a directory "/a" under HOME(/home/nsk/start/)

Now mount has been achieved

12.)Now type, ./hello fuse 
     sometimes due to a bug in FUSE it give an error invalid argument and can be corrected by appending a / 
    					 type ./hello fuse/ 

  fuse is the directory which will host the entire FUSE file system
  
13.) type,
     cd fuse
     ls
     
	you will see the files that are under the HOME directory
	Now type
		 cd a,
		 ls
		 
             You will see the files under (/home/nsk/start) the mounted directory.
             
14.) You can now experiment with various system calls
     cat --> open read close
     cat > --> write
     mkdir 
     ls -l --> getattr
     rm --> unlink
     rmdir    