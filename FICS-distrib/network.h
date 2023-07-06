/* network.h
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

#ifndef _NETWORK_H
#define _NETWORK_H

#include "command.h"   /* For MAX_STRING_LENGTH */

#define NET_NETERROR 0
#define NET_NEW 1
#define NET_DISCONNECT 2
#define NET_READLINE 3
#define NET_TIMEOUT 4
#define NET_NOTCOMPLETE 5

#define LINE_WIDTH 80

#ifndef O_NONBLOCK
#define O_NONBLOCK      00004
#endif

#define NETSTAT_EMPTY 0
#define NETSTAT_CONNECTED 1

typedef struct _connection {
  int fd;
  int outFd;
  unsigned int fromHost;
  int status;
  /* Input buffering */
  int numPending;
  char inBuf[MAX_STRING_LENGTH];
  /* Output buffering */
  int bufpos;
  char outBuf[MAX_STRING_LENGTH];
  /* Column count */
  int outPos;
} connection;

extern int no_file;
extern int max_connections;

extern int net_init( );
extern void net_close( );
extern int net_get_command( );
extern void net_close_connection( );
extern int net_send_string( );
extern void net_flushinputbuffer( );
extern void turn_echo_on( );
extern void turn_echo_off( );
extern unsigned int net_connected_host();
extern void net_flush_all_connections();
extern int net_addConnection();
extern char *dotQuad();

#endif /* _NETWORK_H */
