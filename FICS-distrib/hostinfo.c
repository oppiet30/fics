/* hostinfo.c
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
   Richard Nash nash@visus.com  94/03/08        Created
*/


#include "stdinclude.h"
#include "common.h"
#include "command.h"
#include "hostinfo.h"
#include "utils.h"
#include "lock.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "playerdb.h"
#include "ratings.h"
#include "gamedb.h"
#include "network.h"
#include "rmalloc.h"
#include "get_tcp_conn.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

PUBLIC int numHosts=0;
PUBLIC hostinfo hArray[MAXNUMHOSTS];
PUBLIC int iamserver;
PUBLIC FILE *mail_log_file=NULL;
PRIVATE char hostinfofile[1024];
PRIVATE char myemailaddress[1024];

PUBLIC int numServers=0;
PUBLIC char *serverNames[MAXNUMSERVERS];
PUBLIC int serverPorts[MAXNUMSERVERS];

extern int read();

PUBLIC int hostinfo_write()
{
  int i;
  FILE *fp;

  if (!iamserver) {
    fprintf( stderr, "Shouldn't be writing hostinfo as client!\n" );
    fprintf( mail_log_file, "Shouldn't be writing hostinfo as client!\n" );
  }
  fp = fopen( hostinfofile, "w" );
  if (!fp) {
    fprintf( stderr, "Can't write hostinfo\n" );
    fprintf( mail_log_file, "Can't write hostinfo\n" );
    return -1;
  }
  fprintf( fp, "%s %s\n", iamserver ? "server" : "client", myemailaddress );
  for (i=0;i<numHosts;i++) {
    fprintf( fp, "%s %s %s %d %d %d\n", hArray[i].email_address,
                                  hArray[i].secret_code,
                                  hArray[i].telnet_address,
                                  hArray[i].port,
                                  hArray[i].update_interval,
                                  hArray[i].last_update );
  }
  fclose(fp);
  return 0;
}

PUBLIC int hostinfo_init()
{
  FILE *fp;
  char email[1024], secret[1024], telnet[1024];
  int update, last, port;

  sprintf( hostinfofile, "%s/mail_log", stats_dir );
  mail_log_file = fopen( hostinfofile, "a" );
  if (!mail_log_file) {
    fprintf( stderr, "Can't open log file!\n" );
    return -1;
  }
  if (iamserver)
    sprintf( hostinfofile, "%s/hostinfo.server", config_dir );
  else
    sprintf( hostinfofile, "%s/hostinfo.client", config_dir );
  fp = fopen( hostinfofile, "r" );
  if (!fp) {
    if (iamserver) {
      fprintf( stderr, "Can't open host info file %s, aborting mail.\n", hostinfofile );
      fprintf( mail_log_file, "Can't open host info file %s, aborting mail.\n", hostinfofile );
    }
    return -1;
  }
  fscanf( fp, "%s %s", secret, myemailaddress );
  if (!strcmp(secret, "server")) {
    if (iamserver != 1) {
      fprintf( stderr, "Server claimed, not true.\n" );
      fprintf( mail_log_file, "Server claimed, not true.\n" );
    }
  } else if (!strcmp(secret, "client")) {
    if (iamserver != 0) {
      fprintf( stderr, "Client claimed, not true.\n" );
      fprintf( mail_log_file, "Client claimed, not true.\n" );
    }
  } else {
    fprintf( stderr, "First line of hostinfo file must be \"client\" or \"server\".\n" );
    fprintf( mail_log_file, "First line of hostinfo file must be \"client\" or \"server\".\n" );
    return -1;
  }
  if (iamserver) {
    while (!feof(fp)) {
      if (fscanf( fp, "%s %s %s %d %d %d",
                  email, secret, telnet, &port, &update, &last ) == 6) {
        hArray[numHosts].email_address = strdup( email );
        hArray[numHosts].secret_code = strdup( secret );
        hArray[numHosts].telnet_address = strdup( telnet );
        hArray[numHosts].port = port;
        hArray[numHosts].update_interval = update;
        hArray[numHosts].last_update = last;
        numHosts++;
      }
    }
  } else {
    if (fscanf( fp, "%s %s", email, secret ) != 2) {
      fprintf( stderr, "Badly formatted hostinfo.client file\n" );
      fprintf( mail_log_file, "Badly formatted hostinfo.client file\n" );
    } else { /* Just read one line */
      hArray[numHosts].email_address = strdup( email );
      hArray[numHosts].secret_code = strdup( secret );
      numHosts++;
    }
  }
  fclose(fp);
  return 0;
}

PRIVATE int send_string_to_address( char *addr, char *subject, char *text )
{
  int port;
  char address[100];
  int fd;
  char instr[81];
  char subjstring[1024];
  int rc;
  int sent;

  if (sscanf( addr, "%d:%s", &port, address) != 2) {
    fprintf( stderr, "ERROR: Bad port:address in hostinfo.server\n" );
    return -1;
  }
  if ((fd = get_tcp_conn( address, port )) < 0) {
    fprintf( stderr, "Couldn't make connection to %s at port %d.\n", address, port );
    return -1;
  }
  if ( (rc = read (fd, instr, 1020)) < 0) {
    fprintf( stderr, "Error reading socket. %d\n", errno );
    close(fd);
    return -1;
  }
  instr[rc] = '\0';
  if (strncmp( instr, "FICS_CONNECTED", 14)) {
    fprintf( stderr, "Not FICS connection >%s<\n", instr );
    close(fd);
    return -1;
  }
  sprintf( subjstring, "Subject: %s\n\n", subject );
  sent = send( fd, subjstring, strlen(subjstring), 0 );
  if (sent != strlen(subjstring)) {
    fprintf( stderr, "Whole subject not sent!\n" );
  }
  sent = send( fd, text, strlen(text), 0 );
  if (sent != strlen(text)) {
    fprintf( stderr, "Whole text not sent!\n" );
  }
  close(fd);
  return 0;
}

PUBLIC int hostinfo_updatehosts()
{
  int onHost;
  char salt[3], thecrypt[200], thetime[40];
  char subject[400], text[10100], tmp[400], fname[1024];
  char dname[MAX_FILENAME_SIZE];
  int need_to_write=0;
  int now = time(0);
  int p, c;
  DIR *dirp;
  struct direct *dp;
  struct stat sbuf;
  int error;
  int oh;

  for (onHost=0;onHost<numHosts;onHost++) {
    error = 0;
    text[0]='\0';
    if (hArray[onHost].last_update + hArray[onHost].update_interval >= now) continue;
    salt[0] = 'a' + rand() % 26;
    salt[1] = 'a' + rand() % 26;
    salt[2] = '\0';
    sprintf( thetime, "%d", time(0) );
    sprintf( thecrypt, "%s-%s", thetime, hArray[onHost].secret_code );
    sprintf( subject, "FICS %s %s %s", myemailaddress, thetime, crypt( thecrypt, salt ) );
    for (oh=0;oh<numHosts;oh++) {
      sprintf( tmp, "host %s %d\n", hArray[oh].telnet_address,
                                    hArray[oh].port );
      strcat( text, tmp );
    }
    for (c='a'; c <= 'z'; c++) {
      sprintf( dname, "%s.server/%c", player_dir, c );
      dirp = opendir( dname );
      if (!dirp) continue;
      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        if (!strcmp(dp->d_name, ".")) continue;
        if (!strcmp(dp->d_name, "..")) continue;
        sprintf( fname, "%s/%s", dname, dp->d_name );
        stat(fname, &sbuf);
        if (sbuf.st_mtime > hArray[onHost].last_update) {
          if (index(dp->d_name,'.') &&
             !strcmp(index(dp->d_name,'.'),".delete")) {
            *(index(dp->d_name,'.')) = '\0';
            sprintf( tmp, "delrec %s\n", dp->d_name );
            strcat(text, tmp);
          } else {
            p = player_new();
            if (player_read( p, dp->d_name )) {
              player_remove( p );
              fprintf( stderr, "FICS: Problem reading player %s.\n", dp->d_name );
              continue;
            }
            if (!parray[p].emailAddress) parray[p].emailAddress=strdup("NONE");
            sprintf( tmp, "newrec %s %s %d %d %d %d %d %d %d %d %s \"%s\"\n",
                            parray[p].name, parray[p].emailAddress,
                            parray[p].s_stats.rating,
                            parray[p].s_stats.win, parray[p].s_stats.los,
                            parray[p].s_stats.dra,
                            parray[p].b_stats.rating,
                            parray[p].b_stats.win, parray[p].b_stats.los,
                            parray[p].b_stats.dra,
                            parray[p].passwd, parray[p].fullName );
            strcat(text, tmp);
            player_remove( p );
          }
          if (strlen(text)>=10000) {
            if (index(hArray[onHost].email_address, '@')) {
              mail_string_to_address( hArray[onHost].email_address, subject, text );
            } else {
              if (send_string_to_address( hArray[onHost].email_address, subject, text )) {
                error = 1;
              }
            }
            text[0]='\0';
          }
        }
      }
      closedir(dirp);
    }
    if (strlen(text)>0) {
      if (index(hArray[onHost].email_address, '@')) {
        mail_string_to_address( hArray[onHost].email_address, subject, text );
      } else {
        if (send_string_to_address( hArray[onHost].email_address, subject, text )) {
          error = 1;
        }
      }
      text[0]='\0';
    }
    if (!error) {
      hArray[onHost].last_update = now;
      need_to_write = 1;
    }
  }
  if (need_to_write) {
    hostinfo_write();
  }
  return 0;
}

PUBLIC int hostinfo_mailresults( char *type, char *w, char *b, char *winner)
{
  char salt[3], thecrypt[200], thetime[40];
  char subject[300], text[1024];

  salt[0] = 'a' + rand() % 26;
  salt[1] = 'a' + rand() % 26;
  salt[2] = '\0';
  sprintf( thetime, "%d", time(0) );
  sprintf( thecrypt, "%s-%s", thetime, hArray[0].secret_code );
  sprintf( subject, "FICS %s %s %s", myemailaddress, thetime, crypt( thecrypt, salt ) );
  sprintf( text, "gameover %s %s %s %s\n", type, w, b, winner );
  mail_string_to_address( hArray[0].email_address, subject, text );
  return 0;
}

PUBLIC int hostinfo_checkcomfile()
{
  char fics_comfilename[1024];
  char fics_comfilelock[1024];
  int lock;
  FILE *fp;
  char w[1024], email[1024], txt[1024], fname[1024], passwd[1024];
  int s_Rating, s_w, s_l, s_d, b_Rating, b_w, b_l, b_d;
  int p, connected=0;
  int t=time(0);
  char telnet_addr[1024];
  int port;
  int firstHost;
  int prevRegistered;

  sprintf( fics_comfilename, "%s/comfile", stats_dir );
  sprintf( fics_comfilelock, "%s/comfile.lock", stats_dir );
  lock = mylock( fics_comfilelock, 2);
  if (!lock) return 0;
  fp = fopen( fics_comfilename, "r" );
  if (!fp) {
    myunlock(fics_comfilelock, lock );
    return 0;
  }
  fprintf( stderr, "FICS: Got update file from server at %s.\n", strltime(&t)  );
  firstHost = 1;
  while (!feof(fp)) {
    if (fgets( txt, 1020, fp ) == NULL) continue;
    if (sscanf( txt, "host %s %d", telnet_addr, &port ) == 2) {
      if (firstHost) { /* Remove all host entries */
        while (numServers) {
          numServers--;
          rfree(serverNames[numServers]);
        }
      }
      if (numServers >= MAXNUMSERVERS) {
        numServers = MAXNUMSERVERS - 1;
      }
      serverNames[numServers] = strdup(telnet_addr);
      serverPorts[numServers] = port;
      numServers++;
      firstHost = 0;
      continue;
    }
    if (sscanf( txt, "delrec %s", w ) == 1) {
      p = player_find_bylogin( w );
      if (p < 0) {
        p = player_new();
        if (player_read( p, w )) {
          fprintf( stderr, "FICS: No such player to delete >%s<\n", w );
          continue;
        }
        connected = 0;
      } else {
        connected = 1;
        pprintf_prompt( p, "\n **********   Your registration has been revoked.\n ********* You are account is being deleted.\n" );
        player_write_logout( p );
        net_close_connection( parray[p].socket );
      }
      fprintf( stderr, "FICS: Deleteing account >%s<\n", w );
      player_delete(p);
      if (!connected)
        player_remove(p);
      continue;
    }
    if (sscanf( txt, "newrec %s %s %d %d %d %d %d %d %d %d %s \"%[^\"]\"",
                      w, email,
                      &s_Rating, &s_w, &s_l, &s_d,
                      &b_Rating, &b_w, &b_l, &b_d,
                      passwd, fname ) == 12) {
#ifdef DEBUG
fprintf( stderr, "Updating user %s SR=%d %d %d %d BR=%d %d %d %d\n",
                  w, s_Rating, s_w, s_l, s_d,
                     b_Rating, b_w, b_l, b_d );
#endif
      p = player_find_bylogin( w );
      if (p < 0) {
        p = player_new();
        if (player_read( p, w )) {
          fprintf( stderr, "FICS: New network player >%s<\n", w );
          player_zero( p );
          parray[p].name = strdup( w );
          parray[p].login = strdup( w );
          stolower(parray[p].login);
          parray[p].emailAddress = strdup( email );
          parray[p].passwd = strdup( passwd );
          parray[p].fullName = strdup( fname );
          parray[p].s_stats.rating = s_Rating;
          parray[p].s_stats.win = s_w;
          parray[p].s_stats.los = s_l;
          parray[p].s_stats.dra = s_d;
          parray[p].s_stats.num = s_w + s_l + s_d;
          parray[p].b_stats.rating = b_Rating;
          parray[p].b_stats.win = b_w;
          parray[p].b_stats.los = b_l;
          parray[p].b_stats.dra = b_d;
          parray[p].b_stats.num = b_w + b_l + b_d;
          rating_add( parray[p].s_stats.rating, TYPE_STAND );
          rating_add( parray[p].b_stats.rating, TYPE_BLITZ );
          BestUpdate(p);
          parray[p].registered = 1;
          parray[p].network_player = 1;
          player_save(p);
          player_remove(p);
          continue;
        }
        connected = 0;
      } else {
        connected = 1;
        parray[p].network_player = 1;
      }
      if (!parray[p].network_player) {
        fprintf( stderr, "FICS: Can't update non-network player! >%s<\n", w );
        if (!connected) {
          player_remove( p );
        }
        continue;
      }
      prevRegistered = parray[p].registered;
      parray[p].registered = 1;
      if (parray[p].emailAddress) rfree(parray[p].emailAddress);
      parray[p].emailAddress = strdup( email );
      rating_remove( parray[p].s_stats.rating, TYPE_STAND );
      rating_remove( parray[p].b_stats.rating, TYPE_BLITZ );
      parray[p].s_stats.rating = s_Rating;
      parray[p].s_stats.win = s_w;
      parray[p].s_stats.los = s_l;
      parray[p].s_stats.dra = s_d;
      parray[p].s_stats.num = s_w + s_l + s_d;
      parray[p].b_stats.rating = b_Rating;
      parray[p].b_stats.win = b_w;
      parray[p].b_stats.los = b_l;
      parray[p].b_stats.dra = b_d;
      parray[p].b_stats.num = b_w + b_l + b_d;
      rating_add( parray[p].s_stats.rating, TYPE_STAND );
      rating_add( parray[p].b_stats.rating, TYPE_BLITZ );
      BestUpdate(p);
      if (!connected) {
        player_save( p );
        player_remove( p );
      } else {
        if (!prevRegistered)
          pprintf_prompt( p, "\n **********   Your registration has occurred.\n ********* You are now a registered player!\n" );
      }
      continue;
    }
  }
  fclose(fp);
  unlink(fics_comfilename);
  save_ratings();
  myunlock(fics_comfilelock, lock );
  player_resort( );
  return 0;
}
