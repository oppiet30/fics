/* adminproc.c
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
#include "network.h"
#include "adminproc.h"
#include "command.h"
#include "playerdb.h"
#include "utils.h"
#include "rmalloc.h"

PUBLIC int com_addplayer( int p, param_list param )
{
  char *newplayer = param[0].val.word;
  char newplayerlower[MAX_LOGIN_NAME];
  char *newpassword;
  char salt[3];
  int p1, lookup;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (strlen(newplayer) >= MAX_LOGIN_NAME) {
    pprintf( p, "Player name is too long\n" );
    return COM_OK;
  }
  if (strlen(newplayer) < 3) {
    pprintf( p, "Player name is too short\n" );
    return COM_OK;
  }
  if (!alphastring(newplayer)) {
    pprintf( p, "Illegal characters in player name. Only A-Za-z allowed.\n" );
    return COM_OK;
  }
  strcpy( newplayerlower, newplayer );
  stolower( newplayerlower );
  if (player_find_bylogin( newplayerlower ) >= 0) {
    pprintf( p, "A player by that name is logged in.\nYou can't register a player who is logged in.\n" );
    return COM_OK;
  }
  p1 = player_new();
  lookup = player_read( p1, newplayerlower );
  player_remove( p1 );
  if (!lookup) {
    pprintf( p, "A player by the name %s is already registered.\n", newplayerlower );
    return COM_OK;
  }
  if (param[1].type == TYPE_NULL) {
    newpassword = NULL;
  } else {
    newpassword = param[1].val.word;
  }
  p1 = player_new();
  parray[p1].name = strdup( newplayer );
  parray[p1].login = strdup( newplayerlower );
  if (newpassword) {
    salt[0] = 'a' + rand() % 26;
    salt[1] = 'a' + rand() % 26;
    salt[2] = '\0';
    parray[p1].passwd = strdup( crypt(newpassword, salt) );
  } else {
    parray[p1].passwd = NULL;
  }
  parray[p1].registered = 1;
  player_save(p1);
  player_remove(p1);
  pprintf( p, "Player %s added.\n", newplayer );
  return COM_OK;
}

PRIVATE int shutdownTime=0;
PRIVATE int shutdownStartTime;
PRIVATE char downer[1024];

PUBLIC void ShutDown()
{
  int p1;
  int shuttime=time(0);

  for (p1=0; p1 < p_num; p1++) {
    if (parray[p1].status == PLAYER_EMPTY) continue;
    pprintf(p1, "\n\n    **** Server shutdown ordered by %s. ****\n", downer );
  }
  TerminateCleanup();
  fprintf( stderr, "FICS: Shut down orderly at %s by %s.\n", strltime(&shuttime), downer );
  net_close();
  exit(0);
}

PUBLIC void ShutHeartBeat()
{
  int t=time(0);
  int p1;
  static int lastTimeLeft=0;
  int timeLeft;
  int crossing=0;

  if (!shutdownTime) return;
  if (!lastTimeLeft)
    lastTimeLeft=shutdownTime;
  timeLeft = shutdownTime - (t - shutdownStartTime);
  if ((lastTimeLeft > 600) && (timeLeft <= 600)) crossing = 1;
  if ((lastTimeLeft > 300) && (timeLeft <= 300)) crossing = 1;
  if ((lastTimeLeft > 60) && (timeLeft <= 60)) crossing = 1;
  if ((lastTimeLeft > 30) && (timeLeft <= 30)) crossing = 1;
  if ((lastTimeLeft > 10) && (timeLeft <= 10)) crossing = 1;
  if ((lastTimeLeft > 5) && (timeLeft <= 5)) crossing = 1;
  if (crossing) {
    fprintf(stderr,
    "FICS:   **** Server going down in %d minutes and %d seconds. ****\n\n",
                timeLeft/60,
                timeLeft - ((timeLeft/60) * 60) );
    for (p1=0; p1 < p_num; p1++) {
      if (parray[p1].status == PLAYER_EMPTY) continue;
      pprintf_prompt(p1,
      "\n\n    **** Server going down in %d minutes and %d seconds. ****\n\n",
                  timeLeft/60,
                  timeLeft - ((timeLeft/60) * 60) );
    }
  }
  lastTimeLeft=timeLeft;
  if (timeLeft <= 0) {
    ShutDown();
  }
}

PUBLIC int com_shutdown( int p, param_list param )
{
  int p1;
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  strcpy( downer, parray[p].name );
  shutdownStartTime = time(0);
  if (shutdownTime) {
    for (p1=0; p1 < p_num; p1++) {
      if (parray[p1].status == PLAYER_EMPTY) continue;
      pprintf(p1, "\n\n    **** Server shutdown canceled by %s. ****\n", downer );
    }
  }
  shutdownTime = 0; /* Cancel any pending shutdowns */
  if (param[0].type == TYPE_NULL) {
    ShutDown();
    return COM_OK;
  }
  if (param[0].type == TYPE_WORD) {
    if (!strcmp( param[0].val.word, "now" )) {
      ShutDown();
      return COM_OK;
    } if (!strcmp( param[0].val.word, "cancel" )) {
      return COM_OK;
    } else {
      pprintf( p, "I don't know what you mean by %s\n", param[0].val.word );
      return COM_OK;
    }
  }
  if (param[0].val.integer <= 0) {
    ShutDown();
    return COM_OK;
  } else {
    shutdownTime = param[0].val.integer;
    for (p1=0; p1 < p_num; p1++) {
      if (parray[p1].status == PLAYER_EMPTY) continue;
      pprintf(p1, "\n\n    **** Server shutdown ordered by %s. ****\n", downer );
      pprintf_prompt(p1,
        "    **** Server going down in %d minutes and %d seconds. ****\n\n",
                    shutdownTime/60,
                    shutdownTime - ((shutdownTime/60) * 60) );
    }
    return COM_OK;
  }
}

PUBLIC int server_shutdown( int secs, char *why )
{
  int p1;

  if (shutdownTime && (shutdownTime <= secs)) {
   /* Server is already shutting down, I'll let it go */
    return 0;
  }
  strcpy( downer, "Automatic" );
  shutdownTime = secs;
  shutdownStartTime = time(0);
  for (p1=0; p1 < p_num; p1++) {
    if (parray[p1].status == PLAYER_EMPTY) continue;
    pprintf(p1, "\n\n    **** Automatic Server shutdown. ****\n" );
    pprintf(p1, "%s\n", why );
    pprintf_prompt(p1,
      "    **** Server going down in %d minutes and %d seconds. ****\n\n",
                  shutdownTime/60,
                  shutdownTime - ((shutdownTime/60) * 60) );
  }
  fprintf(stderr, "FICS:    **** Automatic Server shutdown. ****\n" );
  fprintf(stderr, "FICS: %s\n", why );
  return 0;
}

PUBLIC int com_pose( int p, param_list param )
{
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if ((p1 = player_find_part_login( param[0].val.word )) < 0) {
    pprintf( p, "No player named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (parray[p1].adminLevel >= parray[p].adminLevel) {
    pprintf( p, "You can only pose as players below your administration level.\n" );
  }
  pprintf_prompt( p1, "\n%s is issuing the following command posing as you: %s\n", parray[p].name, param[1].val.string );
  pprintf( p, "Command issued as %s\n", parray[p1].name );
  pcommand( p1, param[1].val.string );
  return COM_OK;
}

/* Like shout, but can't censor */
PUBLIC int com_announce( int p, param_list param )
{
  int p1;
  int count = 0;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (!printablestring(param[0].val.string)) {
    pprintf( p, "Your message contains some unprintable character(s).\n" );
    return COM_OK;
  }
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    count++;
    pprintf_prompt( p1, "\n*** %s announces: %s\n", parray[p].name,
                  param[0].val.string  );
  }
  pprintf( p, "Announcement heard by %d player(s).\n", count );
  return COM_OK;
}

PUBLIC int com_muzzle( int p, param_list param )
{
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (param[0].type == TYPE_NULL) {
    pprintf( p, "Muzzled players:\n" );
    for (p1=0;p1<p_num;p1++) {
      if (parray[p1].status != PLAYER_PROMPT) continue;
      if (parray[p1].muzzled)
        pprintf( p, "%s\n", parray[p1].name );
    }
    return COM_OK;
  }
  if ((p1 = player_find_part_login( param[0].val.word )) < 0) {
    pprintf( p, "No player named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (parray[p1].adminLevel >= parray[p].adminLevel) {
    pprintf( p, "You can only muzzle players below your administration level.\n" );
  }
  if (parray[p1].muzzled) {
    pprintf_prompt( p1, "\n*** %s has unmuzzled you.***\n", parray[p].name );
    pprintf( p, "%s unmuzzled.\n", parray[p1].name );
    parray[p1].muzzled = 0;
  } else {
    pprintf_prompt( p1, "\n*** %s has muzzled you.***\n", parray[p].name );
    pprintf( p, "%s muzzled.\n", parray[p1].name );
    parray[p1].muzzled = 1;
  }
  return COM_OK;
}

PUBLIC int com_asetpasswd( int p, param_list param )
{
  int p1, connected;
  char salt[3];

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if ((p1 = player_find_part_login( param[0].val.word )) < 0) {
    connected = 0;
    p1 = player_new();
    if (player_read( p1, param[0].val.word )) {
      player_remove( p1 );
      pprintf( p, "There is no player by that name.\n" );
      return COM_OK;
    }
  } else {
    connected = 1;
  }
  if (parray[p1].passwd)
    rfree(parray[p1].passwd);
  if (param[1].val.word[0] == '*') {
    parray[p1].passwd = strdup( param[1].val.word );
    pprintf( p, "Account %s locked!\n", parray[p1].name );
  } else {
    salt[0] = 'a' + rand() % 26;
    salt[1] = 'a' + rand() % 26;
    salt[2] = '\0';
    parray[p1].passwd = strdup( crypt(param[1].val.word, salt) );
    pprintf( p, "Password of %s changed to \"%s\".\n", parray[p1].name, param[1].val.word );
  }
  player_save( p1 );
  if (connected) {
    if (param[1].val.word[0] == '*') {
      pprintf_prompt( p1, "\n%s has changed locked your account.\n", parray[p].name );
    } else {
      pprintf_prompt( p1, "\n%s has changed your password.\n", parray[p].name );
    }
  } else {
    player_remove( p1 );
  }
  return COM_OK;
}

PUBLIC int com_asetemail( int p, param_list param )
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if ((p1 = player_find_part_login( param[0].val.word )) < 0) {
    connected = 0;
    p1 = player_new();
    if (player_read( p1, param[0].val.word )) {
      player_remove( p1 );
      pprintf( p, "There is no player by that name.\n" );
      return COM_OK;
    }
  } else {
    connected = 1;
  }
  if (parray[p1].emailAddress)
    rfree(parray[p1].emailAddress);
  if (param[1].type == TYPE_NULL) {
    parray[p1].emailAddress = NULL;
    pprintf( p, "Email address for %s removed\n", parray[p1].name );
  } else {
    parray[p1].emailAddress = strdup( param[1].val.word );
    pprintf( p, "Email address of %s changed to \"%s\".\n", parray[p1].name, param[1].val.word );
  }
  player_save( p1 );
  if (connected) {
    if (param[1].type == TYPE_NULL) {
      pprintf_prompt( p1, "\n%s has removed your email address.\n", parray[p].name );
    } else {
      pprintf_prompt( p1, "\n%s has changed your email address.\n", parray[p].name );
    }
  } else {
    player_remove( p1 );
  }
  return COM_OK;
}

PUBLIC int com_asetadmin( int p, param_list param )
{
  int p1, connected;

  if ((p1 = player_find_part_login( param[0].val.word )) < 0) {
    connected = 0;
    p1 = player_new();
    if (player_read( p1, param[0].val.word )) {
      player_remove( p1 );
      pprintf( p, "There is no player by that name.\n" );
      return COM_OK;
    }
  } else {
    connected = 1;
  }
  if (parray[p].adminLevel <= parray[p1].adminLevel) {
    pprintf( p, "You can't change the admin level of those higher than or equal to yourself.\n(%s's Admin level: %d Your Admin Level: %d).\n", parray[p1].name, parray[p1].adminLevel, parray[p].adminLevel );
    return COM_OK;
  }
  if (param[1].val.integer >= parray[p].adminLevel) {
    pprintf( p, "You can't promote someone to or above your level of %d.\n", parray[p].adminLevel );
    return COM_OK;
  }
  parray[p1].adminLevel = param[1].val.integer;
  pprintf( p, "Admin level of %s set to %d.\n", parray[p1].name, parray[p1].adminLevel );
  player_save( p1 );
  if (connected) {
    pprintf_prompt( p1, "\n%s has set your admin level to %d.\n", parray[p].name, parray[p1].adminLevel );
  } else {
    player_remove( p1 );
  }
  return COM_OK;
}
