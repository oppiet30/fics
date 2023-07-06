/* ficsmain.c
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
#include "ficsmain.h"
#include "fics_config.h"
#include "network.h"
#include "command.h"
#include "channel.h"
#include "playerdb.h"
#include "ratings.h"
#include "utils.h"
#include "hostinfo.h"
#include "tally.h"
#include "board.h"
#include "stdlib.h"

/* Arguments */
PUBLIC int port;
PUBLIC int withConsole;

PRIVATE void usage(char *progname)
{
  fprintf( stderr, "Usage: %s [-p port] [-C] [-h]\n", progname );
  fprintf( stderr, "\t\t-p port\t\tSpecify port. (Default=5000)\n" );
  fprintf( stderr, "\t\t-C\t\tStart with console player connected to stdin.\n" );
  fprintf( stderr, "\t\t-h\t\tDisplay this information.\n" );
  exit(1);
}

PRIVATE void GetArgs(int argc, char *argv[])
{
  int i;

  port = DEFAULT_PORT;
  withConsole = 0;

  for (i=1;i<argc;i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
        case 'p':
          if (i == argc-1)
            usage(argv[0]);
          i++;
          if (sscanf( argv[i], "%d", &port) != 1)
            usage(argv[0]);
          break;
        case 'C':
fprintf( stderr, "-C Not implemented!\n" );
exit(1);
          withConsole = 1;
          break;
        case 'h':
          usage(argv[0]);
          break;
      }
    } else {
      usage(argv[0]);
    }
  }
}

PRIVATE void main_event_loop()
{
  int current_socket;
  char command_string[MAX_STRING_LENGTH];
  int status;
  int done;
  int ret;

  done = 0;
  while (!done) {
    in_reset();
    current_socket = net_get_command( command_string, HEARTBEATTIME, &status );
/* printf( "Got status = %d on socket %d\n", status, current_socket ); */
    switch (status) {
      case NET_NOTCOMPLETE:
        tally_gotcommand();
        if ((ret = process_incomplete( current_socket, command_string )) == COM_LOGOUT) {
          process_disconnection( current_socket );
          net_flush_all_connections();
          net_close_connection(current_socket);
        } else if (ret == COM_FLUSHINPUT) {
          net_flushinputbuffer(current_socket);
        }
        break;
      case NET_TIMEOUT: /* Heart beat */
        if (process_heartbeat(&current_socket) == COM_LOGOUT) {
          process_disconnection( current_socket );
          net_flush_all_connections();
          net_close_connection(current_socket);
        }
        break;
      case NET_NETERROR:
        fprintf( stderr, "FICS: Unrecoverable network error.\n" );
        done = 1;
        break;
      case NET_NEW:
        process_new_connection( current_socket,
                                net_connected_host(current_socket) );
        break;
      case NET_DISCONNECT:
        process_disconnection( current_socket );
        break;
      case NET_READLINE:
        tally_gotcommand();
        if (process_input( current_socket, command_string ) == COM_LOGOUT) {
          process_disconnection( current_socket );
          net_flush_all_connections();
          net_close_connection(current_socket);
        }
        break;
    }
  }
}

void TerminateServer(int sig)
{
  fprintf( stderr, "FICS: Got signal %d\n", sig );
  TerminateCleanup();
  net_close();
  exit(1);
}

void BrokenPipe(int sig)
{
  fprintf( stderr, "FICS: Got Broken Pipe\n" );
}

PUBLIC int main( int argc, char *argv[] )
{
#ifdef SYSTEM_NEXT
  #ifdef DEBUG
    malloc_debug(31);
  #endif
#endif
  GetArgs(argc,argv);
  signal(SIGTERM, TerminateServer);
  signal(SIGINT, TerminateServer);
  signal(SIGPIPE, BrokenPipe);
  if (net_init(port)) {
    fprintf( stderr, "FICS: Network initialize failed on port %d.\n", port );
    exit(1);
  }
  startuptime = time(0);
  srand(startuptime);
  fprintf( stderr, "FICS: Initialized on port %d at %s.\n", port, strltime(&startuptime) );
  iamserver = 0;
  if (hostinfo_init()) {
    MailGameResult = 0;
    fprintf( stderr, "FICS: No ratings server connection.\n" );
  } else {
    if (iamserver) {
      fprintf( stderr, "FICS: I don't know how to be a ratings server, refusing.\n" );
      MailGameResult = 0;
    } else {
      fprintf( stderr, "FICS: Ratings server set to: %s\n", hArray[0].email_address );
      MailGameResult = 1;
    }
  }
  if (mail_log_file) fclose(mail_log_file);
  commands_init();
  channel_init();
  rating_init();
  tally_init();
  wild_init();
  player_init(withConsole);
  main_event_loop();
  fprintf( stderr, "FICS: Closing down.\n" );
  net_close();
  return 0;
}
