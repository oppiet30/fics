/* fics_mailproc.c
 *
 */

/*
    fics - An internet chess server.
    Copyright (C) 1994  Richard V. Nash

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
   Richard Nash nash@visus.com  94/03/07        Created
*/

#include "stdinclude.h"

#include "common.h"
#include "hostinfo.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "utils.h"
#include "lock.h"
#include "command.h"
#include "playerdb.h"
#include "ratings.h"
#include "gamedb.h"
#include "tally.h"

#define MAXLINES 10000
char fics_comfilename[1024];
char fics_comfilelock[1024];
int lock=0;
FILE *fics_comfile=NULL;

PRIVATE void usage(char *progname)
{
  fprintf( stderr, "Usage: %s [update,server,client]\n", progname );
  exit(1);
}

PRIVATE void ParseMail( char **fromemail, char **date, char **encrypt,
                       char *txt[] )
{
  char line[1024];
  char tmpe[1024], tmpd[1024], tmpen[1024];
  int online, len;

  txt[0] = NULL;
  online=0;
  while(!feof(stdin)) {
    if (fgets( line, 1020, stdin ) == NULL) return;
    len = strlen(line);
    line[len-1] = '\0';
    if (index(line, ':')) {
      if (!strncmp(line, "Subject: FICS", strlen("Subject: FICS"))) {
        if (sscanf( line, "Subject: FICS %s %s %s", tmpe, tmpd, tmpen) != 3) {
          fprintf( stderr, "Not a FICS subject line %s\n", line );
          fprintf( mail_log_file, "Not a FICS subject line %s\n", line );
          exit(1);
        }
        *fromemail = strdup(tmpe);
        *date = strdup(tmpd);
        *encrypt = strdup(tmpen);
      } else {
      }
    } else {
      if (online >= MAXLINES-3) {
        fprintf( stderr, "Ignoring lines on mail input\n" );
        fprintf( mail_log_file, "Ignoring lines on mail input\n" );
        return;
      }
      txt[online] = strdup(line);
      online++;
      txt[online] = 0;
    }
  }
}

PRIVATE void CheckValidation( char *fromemail, char *date, char *encrypt )
{
  char tmp[1024];
  int i;
  char *cr, salt[3];

  if (!fromemail || !date || !encrypt) {
    fprintf( stderr, "No validation information!\n" );
    fprintf( mail_log_file, "No validation information!\n" );
   exit(1);
  }
  for (i=0;i<numHosts;i++) {
    if (!strcmp(hArray[i].email_address, fromemail))
      break;
  }
  if (i >= numHosts) {
    fprintf( stderr, "Host not found in host file %s\n", fromemail );
    fprintf( mail_log_file, "Host not found in host file %s\n", fromemail );
    exit(1);
  }
  sprintf( tmp, "%s-%s", date, hArray[i].secret_code );
  salt[0] = encrypt[0];
  salt[1] = encrypt[1];
  salt[2] = 0;
  cr = crypt( tmp, salt );
  if (strcmp(cr,encrypt)) {
    fprintf( stderr, "Password not validated Host: %s Date: %s Encrypt:%s\n", fromemail, date, encrypt );
    fprintf( mail_log_file, "Password not validated Host: %s Date: %s Encrypt:%s\n", fromemail, date, encrypt );
    exit(1);
  }
}

/* Server Commands:
gameover type w b winner
ex.
gameover blitz Red Len Red
gameover stand Foobar BazBaz BazBaz
gameover stand Foobar BazBaz draw

Client Commands:
newrec who email_address s_Rating s_w s_l s_d b_Rating b_w b_l b_d
ex.
newrec Red nash@visus.com 1024 10 23 5 1090 129 150 20

host hostaddress port
ex
host hostaddress port
*/
PRIVATE void RunCommand(char *txt)
{
  char type[1024], w[1024], b[1024], winner[1024], email[1024];
  char fname[1024], passwd[1024];
  int s_Rating, s_w, s_l, s_d, b_Rating, b_w, b_l, b_d;
  int pw, pb;
  int g;
  char telnet_addr[1024];
  int port;

  if (!txt || !txt[0]) return;
  if (sscanf( txt, "gameover %s %s %s %s", type, w, b, winner ) == 4) {
    if (!iamserver) {
      fprintf( stderr, "Got gameover as a client, ignoring: >%s<\n", txt );
      fprintf( mail_log_file, "Got gameover as a client, ignoring: >%s<\n", txt );
      return;
    }
    pw = player_new();
    pb = player_new();
    stolower(w);
    stolower(b);
    stolower(winner);
    if (player_read( pw, w ) || player_read( pb, b )) {
      fprintf( stderr, "Player %s or %s doesn't exist on server.\n", w, b );
      fprintf( mail_log_file, "Player %s or %s doesn't exist on server.\n", w, b );
      player_remove(pw);
      player_remove(pb);
      return;
    }
    g = game_new();
    garray[g].white = pw;
    garray[g].black = pb;
    garray[g].status = GAME_ACTIVE;
    if (!strcmp("blitz", type)) {
      garray[g].type = TYPE_BLITZ;
    } else if (!strcmp("stand", type)) {
      garray[g].type = TYPE_STAND;
    } else {
      fprintf( stderr, "Unknown game type %s.\n", type );
      fprintf( mail_log_file, "Unknown game type %s.\n", type );
      player_remove(pw);
      player_remove(pb);
      game_remove(g);
    }
    garray[g].result = END_CHECKMATE; /* It doesn't matter */
    if (!strcmp(winner,w)) {
      garray[g].winner = WHITE;
    } else if (!strcmp(winner,b)) {
      garray[g].winner = BLACK;
    } else if (!strcmp(winner,"draw")) {
      garray[g].result = END_AGREEDDRAW; /* It doesn't matter */
    } else {
      fprintf( stderr, "Unknown winner %s.\n", winner );
      fprintf( mail_log_file, "Unknown winner %s.\n", winner );
      player_remove(pw);
      player_remove(pb);
      game_remove(g);
    }
    /* Update ratings based on the game */
    rating_update( g );
    /* Write out players files */
    player_save(pw);
    player_save(pb);
    player_remove(pw);
    player_remove(pb);
    game_remove(g);
    return;
  }
  if (sscanf( txt, "host %s %d", telnet_addr, &port) == 2) {
    if (iamserver) {
      fprintf( stderr, "Got host as a server, ignoring: >%s<\n", txt );
      fprintf( mail_log_file, "Got host as a server, ignoring: >%s<\n", txt );
      return;
    }
    /* Write host information to file to be picked up by fics */
    if (!fics_comfile) {
      fics_comfile = fopen( fics_comfilename, "a" );
      lock = mylock( fics_comfilelock, 600);
      if (!lock) {
        fprintf( stderr, "Couldn't unlock lock file!\n" );
        return;
      }
      if (!fics_comfile) {
        fprintf( stderr, "Can't open comfile with fics >%s<\n", fics_comfilename );
        fprintf( mail_log_file, "Can't open comfile with fics >%s<\n", fics_comfilename );
        exit(1);
      }
    }
    fprintf( fics_comfile, "host %s %d\n",
                        telnet_addr, port );

    return;
  }
  if (sscanf( txt, "delrec %s", w) == 1) {
    if (iamserver) {
      fprintf( stderr, "Got delrec as a server, ignoring: >%s<\n", txt );
      fprintf( mail_log_file, "Got delrec as a server, ignoring: >%s<\n", txt );
      return;
    }
    /* Write newrec information to file to be picked up by fics */
    if (!fics_comfile) {
      fics_comfile = fopen( fics_comfilename, "a" );
      lock = mylock( fics_comfilelock, 600);
      if (!lock) {
        fprintf( stderr, "Couldn't unlock lock file!\n" );
        return;
      }
      if (!fics_comfile) {
        fprintf( stderr, "Can't open comfile with fics >%s<\n", fics_comfilename );
        fprintf( mail_log_file, "Can't open comfile with fics >%s<\n", fics_comfilename );
        exit(1);
      }
    }
    fprintf( fics_comfile, "delrec %s\n", w );
    return;
  }
  if (sscanf( txt, "newrec %s %s %d %d %d %d %d %d %d %d %s \"%[^\"]\"",
                    w, email,
                    &s_Rating, &s_w, &s_l, &s_d,
                    &b_Rating, &b_w, &b_l, &b_d,
                    passwd, fname ) == 12) {
    if (iamserver) {
      fprintf( stderr, "Got newrec as a server, ignoring: >%s<\n", txt );
      fprintf( mail_log_file, "Got newrec as a server, ignoring: >%s<\n", txt );
      return;
    }
    /* Write newrec information to file to be picked up by fics */
    if (!fics_comfile) {
      fics_comfile = fopen( fics_comfilename, "a" );
      lock = mylock( fics_comfilelock, 600);
      if (!lock) {
        fprintf( stderr, "Couldn't unlock lock file!\n" );
        return;
      }
      if (!fics_comfile) {
        fprintf( stderr, "Can't open comfile with fics >%s<\n", fics_comfilename );
        fprintf( mail_log_file, "Can't open comfile with fics >%s<\n", fics_comfilename );
        exit(1);
      }
    }
    fprintf( fics_comfile, "newrec %s %s %d %d %d %d %d %d %d %d %s \"%s\"\n",
                        w, email, s_Rating, s_w, s_l, s_d,
                                  b_Rating, b_w, b_l, b_d,
                                  passwd, fname );
    return;
  }
  fprintf( stderr, "Ignoring command >%s<\n", txt );
  fprintf( mail_log_file, "Ignoring command >%s<\n", txt );
}

PRIVATE void RunCommands( char *txt[] )
{
  int i;

  for (i=0;txt[i];i++) {
    if (txt[i][0] == '-' && txt[i][1] == '-') { /* Signature! */
      return;
    }
    RunCommand(txt[i]);
  }
}

PUBLIC int main( int argc, char *argv[] )
{
  char *fromemail=NULL;
  char *date=NULL, *encrypt=NULL, *txt[MAXLINES];
  int doupdate=0;

  srand(time(0));
  if (argc != 2) usage(argv[0]);
  if (!strcmp( argv[1], "update")) {
    iamserver = 1;
    doupdate = 1;
  } else
  if (!strcmp( argv[1], "server")) {
    iamserver = 1;
  } else
  if (!strcmp( argv[1], "client")) {
    iamserver = 0;
  } else {
    fprintf( stderr, "Unknown option %s\n", argv[1] );
  }
  if (hostinfo_init()) {
    fprintf( stderr, "Can't read from hostinfo file.\n" );
    fprintf( mail_log_file, "Can't read from hostinfo file.\n" );
    exit(1);
  }
fprintf( mail_log_file, "fics_mailproc %s: Running as uid: %d.\n", argv[1],  getuid() );
  if (doupdate) {
    if (iamserver)
      hostinfo_updatehosts();
    exit(0);
  }
  if (!iamserver) {
    sprintf( fics_comfilename, "%s/comfile", stats_dir );
    sprintf( fics_comfilelock, "%s/comfile.lock", stats_dir );
  }
  player_init(0);
  rating_init();
  tally_init();
  ParseMail( &fromemail, &date, &encrypt, txt );
  CheckValidation( fromemail, date, encrypt );
  RunCommands( txt );
  if (fics_comfile) fclose( fics_comfile );
  if (lock) {
    myunlock(fics_comfilelock, lock );
  }
  exit(0);
}
