/* mailproc.c
 */

#include <stdio.h>
#include "common.h"
#include "stdinclude.h"
#include "lock.h"
#include "tally.h"
#include "network.h"

extern char *strstr();

char mail_file[1024], lock_file[1024];
char command[1024];

void PROCESS_A_MESSAGE(char *str)
{
  FILE *fp;

  if (!strstr(str,"Subject")) return;
  fp = popen(command, "w");
  if (!fp) {
    fprintf( stderr, "Couldn't run command >%s<\n", command );
  }
  fprintf( fp, "%s", str );
  pclose(fp);
}

void PROCESS_STRING_MESSAGES(char *str)
{
    char last_char=0;
    int last_ptr;
    int i;
/* Break 'str' into seperate messages. Delimeted by SOF,\n\nFrom, and EOF */
    while (*str) {
        for (i=0;str[i];i++) {
            if ((str[i] == '\n') &&
                (str[i+1] && (str[i+1] == '\n')) &&
                (str[i+2] && (str[i+2] == 'F')) &&
                (str[i+3] && (str[i+3] == 'r')) &&
                (str[i+4] && (str[i+4] == 'o')) &&
                (str[i+5] && (str[i+5] == 'm'))) {
                i+=2;
                break;
            }
        }
        if (str[i]) {
            last_ptr = i;
            last_char = str[i];
            str[i] = '\0';
        } else
            last_ptr = 0;
        PROCESS_A_MESSAGE(str);
        if (last_ptr) {
            str[last_ptr] = last_char;
        }
        str = &(str[i]);
    }
}

void process_messages()
{
    int fd;
    FILE *fp;
    char *str;
    int len;

    fd = mylock(lock_file,60);
    fp = fopen( mail_file, "r" );
    if (!fp) {
        fprintf( stderr, "Can't read mail file >%s<\n", mail_file );
        myunlock(lock_file,fd);
    } else {
        fseek( fp, 0, SEEK_END );
        len = ftell(fp);
        if (len != 0) {
            str = (char *)malloc( sizeof(char) * len+1 );
            fseek( fp, 0, SEEK_SET );
            fread(str, sizeof(char), len, fp);
            str[len] = '\0';
            fclose(fp);
            fp = fopen( mail_file, "w" ); /* Empty the file */
            if (!fp) {
                fprintf( stderr, "WARNING, Can't empty the mail file!\n" );
            } else fclose(fp);
            myunlock(lock_file,fd);
            PROCESS_STRING_MESSAGES( str );
            free(str);
        } else {
            fclose(fp);
            myunlock(lock_file,fd);
        }
    }
}

void main_event_loop()
{
  char *inStr=NULL;
  int inSize=0, inLen=0;
  int connected_socket = -1;
  int current_socket, com_len;
  int status;
  int done=0;
  char command_string[1024];

  while (!done) {
    current_socket = net_get_command( command_string, 5, &status );
    switch (status) {
      case NET_NEW:
        if (connected_socket >= 0) {
          fprintf( stderr, "mailproc: Can only handle one connection at a time.\n" );
          net_close_connection(current_socket);
        } else {
          connected_socket = current_socket;
          if (inStr)
            inStr[0] = '\0';
          inLen = 0;
          net_send_string( connected_socket, "FICS_CONNECTED\n", 0 );
        }
        break;
      case NET_DISCONNECT:
disconnect:
        if (connected_socket >= 0) {
          if (inLen) { /* Read something */
            PROCESS_A_MESSAGE(inStr);
          }
        } else {
          fprintf( stderr, "mailproc: A connection closed that I didn't know about.\n" );
        }
        connected_socket = -1;
        break;
      case NET_NOTCOMPLETE:
      case NET_READLINE:
        com_len = strlen(command_string) + 1;
        if (com_len + inLen > 100000) { /* Too many bytes to accept! */
          fprintf( stderr, "Too many bytes on connection.\n" );
          net_send_string( connected_socket, "Too many bytes\n", 0 );
          net_close_connection(current_socket);
          goto disconnect;
        }
        if (!strncmp( command_string, "FICS_LOGOUT", 11)) {
          net_send_string( connected_socket, "Logout\n", 0 );
          net_close_connection(current_socket);
          goto disconnect;
        }
        while (com_len + inLen > inSize) {
          inSize += 1024;
          if (inStr) {
            inStr = (char *)realloc(inStr, inSize );
          } else {
            inStr = (char *)malloc( inSize );
            inStr[0] = '\0';
          }
        }
        strcat( inStr, command_string );
        strcat( inStr, "\n" );
        inLen += com_len;
        break;
      case NET_TIMEOUT: /* Heart beat */
        break;
      case NET_NETERROR:
        fprintf( stderr, "mailproc: Unrecoverable network error.\n" );
        done = 1;
        break;
    }
  }
}

void process_network(int port)
{
  if (net_init(port)) {
    fprintf( stderr, "mailproc: Network initialize failed on port %d.\n", port );
    exit(1);
  }
  fprintf( stderr, "mailproc: Intialized on port %d\n", port );
  main_event_loop();
  fprintf( stderr, "mailproc: Closing down\n" );
}

/* If started with a portnumber it becomes a daemon just waiting for
   input from a port. If a mailbox, it is a one shot mail pickup */
int main(int argc, char *argv[])
{
  int port;
  if (argc != 3) {
    fprintf( stderr, "Usage %s: [mailbox,portnum] command\n", argv[0] );
    exit(1);
  }
  strcpy( command, argv[2] );
  if (sscanf( argv[1], "%d", &port) != 1) port=0;
  if (!port) {
    sprintf( mail_file, "%s", argv[1] );
    sprintf( lock_file, "%s.lock", argv[1] );
    process_messages();
  } else {
    tally_init();
    process_network(port);
  }
  return 0;
}
