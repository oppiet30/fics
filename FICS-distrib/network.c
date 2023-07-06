/* network.c
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include "common.h"
#include "network.h"
#include "tally.h"

extern int errno;

PRIVATE int sockfd=0; /* The socket */
PRIVATE int numConnections = 0;
/* Sparse array */
PRIVATE connection con[256];

PUBLIC int no_file;
PUBLIC int max_connections;

/* Index == fd, for sparse array, quick lookups! wasted memory :( */
PRIVATE int findConnection( int fd )
{
  if (con[fd].status != NETSTAT_CONNECTED)
    return -1;
  else
    return fd;
}

PUBLIC int net_addConnection( int fd, unsigned int fromHost )
{
  int noblock=1;

  if (findConnection(fd) >= 0) {
    fprintf( stderr, "FICS: FD already in connection table!\n" );
    return -1;
  }
  if (numConnections == max_connections) return -1;
  if (ioctl( fd, FIONBIO, &noblock ) == -1) {
    fprintf( stderr, "Error setting nonblocking mode errno=%d\n",errno );
  }

  con[fd].fd = fd;
  if (fd != 0)
    con[fd].outFd = fd;
  else
    con[fd].outFd = 1;
  con[fd].fromHost = fromHost;
  con[fd].status = NETSTAT_CONNECTED;
  con[fd].numPending = 0;
  con[fd].outPos = 0;
  con[fd].bufpos = 0;
  numConnections++;
  return 0;
}

PRIVATE int remConnection( int fd )
{
  int which;
  if ((which = findConnection(fd)) < 0) {
    return -1;
  }
  numConnections--;
  con[fd].status = NETSTAT_EMPTY;
  return 0;
}

PRIVATE int iswordbreak( int c)
{
  return ((c == ' ') || (c == '\t') || (c == '\n'));
}

PRIVATE int numtillspace(char *str)
{
  int num=0;

  while (*str && iswordbreak(*str)) {
    if (*str == '\n') return num;
    str++;
    num++;
  }
  while (*str && !iswordbreak(*str)) {
    str++;
    num++;
  }
  return num;
}

PUBLIC void net_flush_all_connections()
{
  int which, sent, j;

  for (which=0;which<max_connections;which++) {
    if (con[which].status == NETSTAT_CONNECTED &&
        con[which].bufpos > 0 ) {
/* printf( "Flushing %d bytes\n", con[which].bufpos ); */
      sent = send( con[which].outFd, con[which].outBuf, con[which].bufpos, 0 );
/*      sent = write( con[which].outFd, con[which].outBuf, con[which].bufpos ); */
      if (sent == con[which].bufpos) {
        con[which].bufpos = 0;
        continue;
      }
      if (sent >= 0) { /* No error, but not all data sent */
        fprintf( stderr, "Not all data sent on socket %d\n", con[which].outFd );
        for (j=sent;j<con[which].bufpos;j++)
          con[which].outBuf[j-sent] = con[which].outBuf[j];
        con[which].bufpos -= sent;
        continue;
      } else { /* Error */
        fprintf( stderr, "Error on socket %d - %d\n", con[which].outFd, errno );
        con[which].bufpos = 0;
        continue;
      }
    }
  }
}

/*
 * -1 for an error other than EWOULDBLOCK.
 * Put <lf> after every <cr> and put \ at the end of overlength lines.
 * Doesn't send anything unless the buffer fills, output waits until
 * net_flush_all_connections()
*/
PUBLIC int net_send_string( int fd, char *str, int format )
{
  int bstart;
  int bcount=0;
  int len = strlen(str);
  int sent;
  int which;
  int i, j;

  if ((which = findConnection(fd)) < 0) {
    return -1;
  }
  bstart = con[which].bufpos;
  for (i=0;i<len;i++) {
    if (format) {
      if (str[i] == '\033') {
        if (str[i+1] == '[') {
          con[which].outPos -= 3;
        } else {
          con[which].outPos = 0;/* Oh oh, an ESC sequence, all bets are off */
        }
      } else if (str[i] == '\t') {
        con[which].outPos += 8;
        con[which].outPos = 8 * (con[which].outPos / 8);
      } else if (isprint(str[i])) {
        con[which].outPos++;
      }
    } else {
      con[which].outPos = 0;
    }
    if (iswordbreak(str[i]) || con[which].outPos >= LINE_WIDTH) {
      if (((con[which].outPos >= LINE_WIDTH) ||
           ((con[which].outPos + numtillspace(&str[i]) >= LINE_WIDTH) &&
           (numtillspace(&str[i]) < LINE_WIDTH))
          )
          && (str[i] != '\n')) {
        con[which].outBuf[con[which].bufpos++] = '\n';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        con[which].outBuf[con[which].bufpos++] = '\r';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        con[which].outBuf[con[which].bufpos++] = '\\';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        con[which].outBuf[con[which].bufpos++] = ' ';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        con[which].outBuf[con[which].bufpos++] = ' ';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        con[which].outBuf[con[which].bufpos++] = ' ';
        if (con[which].bufpos == MAX_STRING_LENGTH)
          return -1;
        if (str[i] == '\t') {
          con[which].outPos = 8;
        } else if (isprint(str[i])) {
          con[which].outPos = 5;
        } else
          con[which].outPos = 4;
        if (str[i] != ' ')
          con[which].outBuf[con[which].bufpos++] = str[i];
      } else {
        con[which].outBuf[con[which].bufpos++] = str[i];
      }
    } else {
      con[which].outBuf[con[which].bufpos++] = str[i];
    }
    if (con[which].bufpos == MAX_STRING_LENGTH)
      return -1;
    if (str[i] == '\n') {
      con[which].outBuf[con[which].bufpos++] = '\r';
      if (con[which].bufpos == MAX_STRING_LENGTH)
        return -1;
      con[which].outPos = 0;
    }
    if (con[which].bufpos > MAX_STRING_LENGTH-10) {
/* printf( "Sending %d bytes\n", con[which].bufpos ); */
      sent = send( con[which].outFd, con[which].outBuf, con[which].bufpos, 0 );
/*      sent = write( con[which].outFd, con[which].outBuf, con[which].bufpos ); */
      if (sent == con[which].bufpos) {
        bcount += con[which].bufpos - bstart;
        bstart = 0;
        con[which].bufpos = 0;
        continue;
      }
      if (sent >= 0) { /* No error, but not all data sent */
        fprintf( stderr, "Not all data sent on socket %d\n", con[which].outFd );
        for (j=sent;j<con[which].bufpos;j++)
          con[which].outBuf[j-sent] = con[which].outBuf[j];
        bcount += sent;
        con[which].bufpos -= sent;
        bstart = con[which].bufpos;
        tally_sent(bcount);
        return bcount;
      } else { /* Error */
        fprintf( stderr, "Error on socket %d - %d\n", con[which].outFd, errno );
        bcount += con[which].bufpos - bstart;
        bstart = 0;
        con[which].bufpos = 0;
        tally_sent(bcount);
        return bcount;
      }
    }
  }
  bcount += con[which].bufpos - bstart;
  tally_sent(bcount);
  return bcount;
}

/* Returns 1 if a line was read, otherwise 0. */
/* -1 for an error other than EWOULDBLOCK.    */
PRIVATE int readline( int who )
{
  int fd = con[who].fd;
  char *t = con[who].inBuf;
  int recvCount;
  int totalCount=0;
  unsigned char c;
  char answer[10];

  t += con[who].numPending;

/* This could be changed to recieve bigger chunks at a time for efficiency */
  while ((recvCount = recv(fd, &c, 1, 0)) == 1) {
    totalCount += recvCount;
    if (c == IAC) { /* This is some kind of telnet control */
fprintf( stderr, "FICS: Got telnet control %d\n", c );
      recvCount = recv(fd, &c, 1, 0);
      if (recvCount == 1) {
fprintf( stderr, "FICS: T - %d\n", c );
        totalCount += recvCount;
        switch (c) {
          case DO:
            recvCount = recv(fd, &c, 1, 0);
            if (recvCount == 1) {
fprintf( stderr, "FICS: T - %d\n", c );
              totalCount += recvCount;
              if (c == TELOPT_TM) { /* Must answer timing mark */
                answer[0] = IAC; answer[1] = WILL; answer[2] = TELOPT_TM;
                answer[3] = '\0';
                send( fd, answer, 3, 0 );
              }
            }
            c = '\0';
            break;
          case IP: /* Interrupt process */
            c = '\3'; /* Insert ctrl-c in stream */
            break;
          default:
            c = '\0';
            break;
        }
      }
    }
    if (c != '\r' && c > 2) {
      *t++ = c;
      *t = '\0';
      con[who].numPending++;
    }
    if (c == '\n' || con[who].numPending >= MAX_STRING_LENGTH - 1) {
      t--;
      *t = '\0'; /* Nail CR */
      con[who].numPending = 0;
      con[fd].outPos = 0; /* New line */
      in_push(IN_INPUT);
      tally_sent(totalCount);
      in_pop();
      return 1;
    }
  }
  if (recvCount == 0) {
    in_push(IN_INPUT);
    tally_sent(totalCount);
    in_pop();
    return 0;
  }

  if (errno == EWOULDBLOCK) {
    in_push(IN_INPUT);
    tally_sent(totalCount);
    in_pop();
    return 0;
  }
  in_push(IN_INPUT);
  tally_sent(totalCount);
  in_pop();
  return -1;                    /* error handling maybe should go here */
}

PUBLIC int net_init( int port )
{
  int i;
  int opt = 1;
  struct sockaddr_in serv_addr;

  no_file = getdtablesize();
  if (no_file > 256) no_file = 256;
  max_connections = no_file - 10;
  for (i = 0; i< no_file; i++)
    con[i].status = NETSTAT_EMPTY;
  /*
   * Open a TCP socket (an Internet stream socket).
   */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf( stderr, "FICS: can't open stream socket\n");
    return -1;
  }
  /*
   * Bind our local address so that the client can send to us
   */
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  /** added in an attempt to allow rebinding to the port **/

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));

/*
#ifdef DEBUG
  opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, (char *)&opt, sizeof(opt));
#endif
*/

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf( stderr, "FICS: can't bind local address\n");
    return -1;
  }

  fcntl(sockfd, F_SETFL, FNDELAY);
  listen(sockfd, 5);
  return 0;
}

PUBLIC void net_close()
{
  net_flush_all_connections();
}

PUBLIC void net_close_connection( int fd )
{
  if (!remConnection(fd)) {
    if (fd > 2)
      close(fd);
  }
}

PUBLIC void net_flushinputbuffer( int fd )
{
  int which;
  if ((which = findConnection(fd)) < 0) {
    return;
  }
  con[which].numPending = 0;
  return;
}

PUBLIC void turn_echo_on( int fd )
{
  char answer[10];

  answer[0] = IAC; answer[1] = WONT; answer[2] = TELOPT_ECHO;
  answer[3] = '\0';
  send( fd, answer, 3, 0 );
}

PUBLIC void turn_echo_off( int fd )
{
  char answer[10];

  answer[0] = IAC; answer[1] = WILL; answer[2] = TELOPT_ECHO;
  answer[3] = '\0';
  send( fd, answer, 3, 0 );
}

PUBLIC unsigned int net_connected_host( int fd )
{
  int which;

  if ((which = findConnection(fd)) < 0) {
    fprintf( stderr, "FICS: FD not in connection table!\n" );
    return -1;
  }
  return con[which].fromHost;
}

PUBLIC int net_get_command( char *com, int timeout, int *status )
{
  struct sockaddr_in cli_addr;
  int cli_len=sizeof(struct sockaddr_in);
  int tmp_fd;
  int loop, pending;
  fd_set readfds, exceptfds;
  struct timeval to;
  int nfound;
  int lineComplete;

  net_flush_all_connections();
  tmp_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
  if (tmp_fd > 0) {
    if (net_addConnection( tmp_fd, cli_addr.sin_addr.s_addr )) {
      net_send_string( tmp_fd, "\n\rSorry, this server has reached a hard limit on the number of connections.\n\rTry again later.\n\r\n\r", 1 );
      close(tmp_fd);
    } else {
      *status = NET_NEW;
      return tmp_fd;
    }
  } else {
    if (errno != EWOULDBLOCK) {
      fprintf( stderr, "FICS: Problem accepting on socket. errno=%d\n",errno );
    }
  }
  FD_ZERO (&readfds);
  for (loop = 0; loop < no_file; loop++)
    if (con[loop].status == NETSTAT_CONNECTED)
      FD_SET (con[loop].fd, &readfds);
  FD_ZERO (&exceptfds);
  for (loop = 0; loop < no_file; loop++)
    if (con[loop].status == NETSTAT_CONNECTED)
      FD_SET (con[loop].fd, &exceptfds);
  to.tv_usec = 0;
  to.tv_sec = timeout;

  nfound = select(no_file, &readfds, 0, &exceptfds, &to);
  if (nfound == -1)
  {
    fprintf (stderr, "FICS: I/O error reading connections.\n\r");
    *status = NET_TIMEOUT;
    return -1;
  }
  if (nfound == 0) {
    *status = NET_TIMEOUT;
    return -1;
  }

  for (loop = 0; loop < no_file; loop++) {
    if (con[loop].status != NETSTAT_CONNECTED)
      continue;
    tmp_fd = con[loop].fd;
    if (FD_ISSET (con[loop].fd, &exceptfds)) {
fprintf( stderr, "FICS: Exception on %d, closing it.\n", con[loop].fd );
      net_close_connection( tmp_fd );
      *status = NET_DISCONNECT;
      return tmp_fd;
    }
    if (FD_ISSET (con[loop].fd, &readfds)) {
      if (ioctl(con[loop].fd, FIONREAD, &pending) == -1) {
        /*
          * This is a dropped connection
          */
        fprintf (stderr, "FICS: Can't ioct socket.\n");
        net_close_connection( tmp_fd );
        *status = NET_DISCONNECT;
        return tmp_fd;
      } else {
        if (!pending) { /* Connection closed */
          net_close_connection( tmp_fd );
          *status = NET_DISCONNECT;
          return tmp_fd;
        } else { /* Finally try to read the connection */
          lineComplete = readline(tmp_fd);
          if (lineComplete < 0) {
            net_close_connection( tmp_fd );
            *status = NET_DISCONNECT;
            return tmp_fd;
          } else if (lineComplete == 0) {
fprintf( stderr, "FICS: Incomplete read on %d.\n", con[loop].fd );
            strcpy( com, con[loop].inBuf );
            *status = NET_NOTCOMPLETE;
            return con[loop].fd;
          } else { /* Got a whole line of input! Yea! */
            strcpy( com, con[loop].inBuf );
            *status = NET_READLINE;
            return con[loop].fd;
          }
        }
      }
    }
  }
  fprintf( stderr, "FICS: Select found action, but no one owned up to it\n" );
  *status = NET_TIMEOUT;
  return -1;
}
