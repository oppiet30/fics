/* stdinclude.h
 *
 */

/*
    fics - An internet chess server.
    Copyright (C) 1993  Richard V. Nash

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/* Revision history:
   name         email           yy/mm/dd        Change
   Richard Nash nash@visus.com  93/10/22        Created
*/

#include <sys/types.h>

/* Set up system specific defines */
#if defined(SYSTEM_NEXT)

#define HASMALLOCSIZE
/* Next has already thoughtfully defined this */
#ifdef  __LITTLE_ENDIAN__
#define LITTLE_ENDIAN
#endif
#include <sys/vfs.h>

#elif defined(SYSTEM_ULTRIX)

#define LITTLE_ENDIAN
#include <sys/param.h>
#include <sys/mount.h>
extern int strcasecmp();
extern int setsockopt();
extern int send();
extern int recv();
extern int socket();
extern int bzero();
extern int setsockop();
extern int bind();
extern int listen();
extern int accept();
extern int wait3();
extern int gettimeofday();
extern char *rindex();
extern char *index();
#endif

/* These are included into every .c file */
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/errno.h>
#include <signal.h>
#include <strings.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/* Forward declare the functions that aren't defined above */
extern int rand(void);
extern void srand(unsigned int seed);
extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int close(int);
extern int atoi (char *nptr);
extern long atol (char *nptr);
extern char *crypt (char *key, char *salt);
extern int sethostname(char *, int), gethostname(char *, int);
extern int truncate(char *, off_t), ftruncate(int, off_t);
extern size_t malloc_size(void *ptr);
  extern fcntl();
  extern open();
  extern int ioctl();
extern void *malloc(unsigned int size);
extern void *calloc(unsigned int num, unsigned int size);
extern void *realloc(void *addr, unsigned int size);
extern void free(void *data);
extern void malloc_good_size(unsigned int size);
extern int link();
extern int unlink();
extern int getpid();
extern int kill();
extern int fork();
extern int access();
extern int getdtablesize();
extern int write();
#if !defined(SYSTEM_ULTRIX)
extern int sleep();
extern int getuid();
extern int statfs();
#endif
