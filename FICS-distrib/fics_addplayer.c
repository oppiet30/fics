/* fics_addplayer.c
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
#include "utils.h"
#include "playerdb.h"
#include "hostinfo.h"
#include "command.h"

PRIVATE void usage(char *progname)
{
  fprintf( stderr, "Usage: %s [-l] [-n] UserName FullName EmailAddress\n", progname );
  exit(1);
}

/* Parameters */
int local=0;
char *uname=NULL, *fname=NULL, *email=NULL;

#define PASSLEN 4
PUBLIC int main( int argc, char *argv[] )
{
  int i;
  int p;
  char password[PASSLEN+1];
  char salt[3];
  char text[2048];

  for (i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'l':
          local = 1;
          break;
        case 'n':
          local = 0;
          break;
        default:
          usage(argv[0]);
          break;
      }
    } else {
      if (!uname)
        uname = argv[i];
      else if (!fname)
        fname = argv[i];
      else if (!email)
        email = argv[i];
      else
        usage(argv[0]);
    }
  }
  if (!uname || !fname || !email)
    usage(argv[0]);

  /* Add the player here */
  if (strlen(uname) >= MAX_LOGIN_NAME) {
    fprintf( stderr, "Player name is too long\n" );
    exit(0);
  }
  if (strlen(uname) < 3) {
    fprintf( stderr, "Player name is too short\n" );
    exit(0);
  }
  if (!alphastring(uname)) {
    fprintf( stderr, "Illegal characters in player name. Only A-Za-z allowed.\n" );
    exit(0);
  }
  if (local)
    iamserver = 0;
  else
    iamserver = 1;

  if (hostinfo_init()) {
    if (iamserver) {
      fprintf( stderr, "Can't read from hostinfo file.\n" );
      fprintf( stderr, "Remember you need -l for local.\n" );
      exit(1);
    } else {
    }
  }
  player_init(0);
  srand(time(0));
  p = player_new();
  if (!player_read( p, uname )) {
    fprintf( stderr, "%s already exists.\n", uname );
    exit(0);
  }
  parray[p].name = strdup( uname );
  parray[p].login = strdup( uname );
  stolower(parray[p].login);
  parray[p].fullName = strdup( fname );
  parray[p].emailAddress = strdup( email );
  for (i=0;i<PASSLEN;i++) {
    password[i] = 'a' + rand() % 26;
  }
  password[i] = '\0';
  salt[0] = 'a' + rand() % 26;
  salt[1] = 'a' + rand() % 26;
  salt[2] = '\0';
  parray[p].passwd = strdup( crypt( password, salt ) );
  parray[p].registered = 1;
  parray[p].network_player = !local;
  player_save(p);
  printf( "Added %s player >%s< >%s< >%s< >%s<\n", local ? "local" : "network",
                                    uname, fname, email, password );
  sprintf( text, "\nYou have been added as a %s player.\nIf this is a network account it may take a while to show up on all of the\nclients.\n\nLogin Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\nIf any of this information is incorrect, please contact the administrator\nto get it corrected.\nPlease write down your password, as it will be your initial passoword\non all of the servers.\n", local ? "local" : "network", uname, fname, email, password );
  mail_string_to_address( email, "FICS Account Created", text );
  exit(0);
}
