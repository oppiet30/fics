/* lock.c
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

#include "stdinclude.h"

#include "common.h"
#include "lock.h"

PUBLIC int mylock( char *file, int timeo)
{
    int fd;
    int retry_count;
    int retry_time;
    struct stat buf;

    retry_time = 0;
    retry_count = 0;
retry:
    fd = open(file, O_CREAT | O_EXCL, 0644);


    if (fd < 0) {       /* Something didn't work */
            if (errno != EEXIST) {      /* Real error occurred */
                fprintf(stderr, "FICS: Lock file open failed!\n");
                retry_count++;
                fprintf( stderr, "FICS: Retry #%d in %d seconds...\n", retry_count, timeo );
                sleep(timeo);
                goto retry;
            }
            else {              /* Lock file is there   */
                int timeout = timeo;
                if (retry_count == 1) {
                    retry_time = time(0);
                    stat(file, &buf);
                    if ((retry_time > buf.st_mtime + 18) &&
                        (retry_time < buf.st_mtime + 22) &&
                        (buf.st_uid == getuid())) {
                        fprintf( stderr, "FICS: Caught NFS problem.\n FICS: I created this lock file and I will delete it.\n" );
                        unlink(file);
                        goto retry;
                    }
                }
                while (!access(file,F_OK) && timeout) {
                    timeout--;
                    sleep(1);
                }           /* await lock release */
                if (timeout == 0) {
                    fprintf( stderr, "FICS: WARNING: Lock file timeout!\n" );
                    fprintf( stderr, "FICS: WARNING: Lock removed!\n" );
                    unlink(file);
                    return 0;
                }
                goto retry;
            }
    }
    else /* Things are OK */ {
        if (retry_count != 0) {
            fprintf( stderr, "FICS: RETRY Succeeded!\n" );
        }
        return fd;
    }
}

PUBLIC int myunlock(char *file,int fd)
{
  int retry=0;

  close(fd);
  while (unlink(file) && !access(file, F_OK)) {
    retry++;
    fprintf( stderr, "FICS: Could not remove lock file. Retry #%d in 5 seconds...\n", retry );
    sleep(5);
  }
  return 0;
}
