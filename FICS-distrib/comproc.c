/* comproc.c
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

#include <math.h>

#include "common.h"
#include "comproc.h"
#include "command.h"
#include "utils.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "playerdb.h"
#include "network.h"
#include "rmalloc.h"
#include "channel.h"
#include "variable.h"
#include "gamedb.h"
#include "board.h"
#include "tally.h"
#include "hostinfo.h"
#include "multicol.h"

PUBLIC int com_quit( int p, param_list param )
{
  if (parray[p].game >= 0) {
    pprintf( p, "You can't quit while you are playing a game.\nType 'resign' to resign the game, or you can request an abort with 'abort'.\n" );
    return COM_OK;
  }
  psend_file( p, mess_dir, MESS_LOGOUT );
  return COM_LOGOUT;
}

PUBLIC int com_help( int p, param_list param )
{
  char command[MAX_FILENAME_SIZE];

  in_push(IN_HELP);
  if (param[0].type == TYPE_NULL) {
    sprintf( command, "ls -C %s", help_dir );
    psend_command( p, command, NULL );
    in_pop();
    return COM_OK;
  }
  if (safestring( param[0].val.word )) {
    if (psend_file( p, help_dir, param[0].val.word )) {
      pprintf( p, "No help available on %s.\n", param[0].val.word );
    }
  } else {
    pprintf( p, "Illegal character in command %s.\n", param[0].val.word );
  }
  in_pop();
  return COM_OK;
}

PUBLIC int com_query( int p, param_list param )
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf( p, "Only registered players can use the query command.\n" );
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf( p, "Your message contains some unprintable character(s).\n" );
    return COM_OK;
  }
  if (!parray[p].query_log) {
    parray[p].query_log = tl_new(5);
  } else {
    if (tl_numinlast(parray[p].query_log,60*60) >= 2) {
      pprintf( p, "Your can only query twice per hour.\n" );
      return COM_OK;
    }
  }
  in_push(IN_SHOUT);
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (player_censored( p1, p )) continue;
    count++;
    if (parray[p1].highlight) {
      pprintf_prompt( p1, "\n\033[7m%s queries:\033[0m %s\n", parray[p].name,
                    param[0].val.string  );
    } else {
      pprintf_prompt( p1, "\n%s queries: %s\n", parray[p].name,
                    param[0].val.string  );
    }
  }
  pprintf( p, "Query heard by %d player(s).\n", count );
  tl_logevent(parray[p].query_log, 1);
  in_pop();
  return COM_OK;
}

PUBLIC int com_shout( int p, param_list param )
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf( p, "Only registered players can use the shout command.\n" );
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf( p, "Your message contains some unprintable character(s).\n" );
    return COM_OK;
  }
  in_push(IN_SHOUT);
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (!parray[p1].i_shout) continue;
    if (player_censored( p1, p )) continue;
    count++;
    pprintf_prompt( p1, "\n%s shouts: %s\n", parray[p].name,
                  param[0].val.string  );
  }
  pprintf( p, "Shout heard by %d player(s).\n", count );
  in_pop();
  return COM_OK;
}

PUBLIC int com_it( int p, param_list param )
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf( p, "Only registered players can use the it command.\n" );
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf( p, "Your message contains some unprintable character(s).\n" );
    return COM_OK;
  }
  in_push(IN_SHOUT);
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (!parray[p1].i_shout) continue;
    if (player_censored( p1, p )) continue;
    count++;
    pprintf_prompt( p1, "\n--> %s %s\n", parray[p].name,
                  param[0].val.string  );
  }
  pprintf( p, "(%d) %s %s\n", count, parray[p].name, param[0].val.string );
  in_pop();
  return COM_OK;
}

#define TELL_TELL 0
#define TELL_SAY 1
#define TELL_WHISPER 2
#define TELL_KIBITZ 3
#define TELL_CHANNEL 4
PRIVATE int tell(p, p1, msg, why, ch)
  int p;
  int p1;
  char *msg;
  int why;
  int ch;
{
  if (!printablestring(msg)) {
    pprintf( p, "Your message contains some unprintable character(s).\n" );
    return COM_OK;
  }
  if (p1 == p) {
    pprintf( p, "Quit talking to yourself! It's embarrassing.\n" );
    return COM_OK;
  }
  if (!parray[p1].i_tell) {
    pprintf( p, "Player \"%s\" isn't listening to tells.\n", parray[p1].name );
    return COM_OK;
  }
  if (player_censored( p1, p )) {
    pprintf( p, "Player \"%s\" is censoring you.\n", parray[p1].name );
    return COM_OK;
  }
  in_push(IN_TELL);
  if (parray[p1].highlight) {
    pprintf( p1, "\n\033[7m%s\033[0m", parray[p].name );
  } else {
    pprintf( p1, "\n%s", parray[p].name );
  }
  switch (why) {
    case TELL_SAY:
      pprintf_prompt( p1, " says: %s\n", msg );
      break;
    case TELL_WHISPER:
      pprintf_prompt( p1, " whispers: %s\n", msg );
      break;
    case TELL_KIBITZ:
      pprintf_prompt( p1, " kibitzes: %s\n", msg );
      break;
    case TELL_CHANNEL:
      pprintf_prompt( p1, "(%d): %s\n", ch, msg );
      break;
    case TELL_TELL:
    default:
      pprintf_prompt( p1, " tells you: %s\n", msg );
      break;
  }
  if ((why == TELL_SAY) || (why == TELL_TELL)) {
    pprintf( p, "(told %s)\n", parray[p1].name );
    parray[p].last_tell = p1;
    parray[p].last_channel = -1;
  }
  in_pop();
  return COM_OK;
}

PRIVATE int chtell( p, ch, msg )
  int p;
  int ch;
  char *msg;
{
  int p1;
  int i, count=0;

  if (ch < 0) {
    pprintf( p, "The lowest channel number is 0.\n" );
    return COM_OK;
  }
  if (ch >= MAX_CHANNELS) {
    pprintf( p, "The maximum channel number is %d.\n", MAX_CHANNELS-1);
    return COM_OK;
  }
  in_push(IN_TELL);
  for (i=0; i < numOn[ch]; i++) {
    p1 = channels[ch][i];
    if (p1 == p) continue;
    if (player_censored( p1, p )) continue;
    if (!parray[p1].i_tell) continue;
    tell(p, p1, msg, TELL_CHANNEL, ch);
    count++;
  }
  if (count) {
    parray[p].last_tell = -1;
    parray[p].last_channel = ch;
  }
  pprintf( p, "(%d->(%d))\n", ch, count );
  in_pop();
  return COM_OK;
}

PUBLIC int com_whisper( int p, param_list param )
{
  int g;
  int p1;
  int count=0;

  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (!parray[p].num_observe) {
    pprintf( p, "You are not observing a game.\n" );
    return COM_OK;
  }
  g = parray[p].observe_list[0];
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (player_is_observe(p1,g)) {
      tell( p, p1, param[0].val.string, TELL_WHISPER, 0);
      count++;
    }
  }
  pprintf( p, "whispered to %d.\n", count );
  return COM_OK;
}

PUBLIC int com_kibitz( int p, param_list param )
{
  int g;
  int p1;
  int count=0;

  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (!parray[p].num_observe && parray[p].game < 0) {
    pprintf( p, "You are not playing or observing a game.\n" );
    return COM_OK;
  }
  if (parray[p].game >= 0)
    g = parray[p].game;
  else
    g = parray[p].observe_list[0];
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (player_is_observe(p1,g) || parray[p1].game == g) {
      tell( p, p1, param[0].val.string, TELL_KIBITZ, 0);
      count++;
    }
  }
  pprintf( p, "kibitzed to %d.\n", count );
  return COM_OK;
}

PUBLIC int com_tell( int p, param_list param )
{
  int p1;

  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (param[0].type == TYPE_WORD) {
    stolower(param[0].val.word);
    if (!strcmp(param[0].val.word, ".")) {
      if (parray[p].last_tell < 0) {
        pprintf( p, "No one to tell anything to.\n" );
        return COM_OK;
      } else {
        return tell( p, parray[p].last_tell, param[1].val.string, TELL_TELL, 0);
      }
    }
    if (!strcmp(param[0].val.word, ",")) {
      if (parray[p].last_channel < 0) {
        pprintf( p, "No previous channel.\n" );
        return COM_OK;
      } else {
        return chtell(p, parray[p].last_channel, param[1].val.string );
      }
    }
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    return tell( p, p1, param[1].val.string, TELL_TELL, 0);
  } else { /* Channel */
    return chtell(p, param[0].val.integer, param[1].val.string );
  }
}

PUBLIC int com_say( int p, param_list param )
{
  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (parray[p].opponent < 0) {
    if (parray[p].last_opponent < 0) {
      pprintf( p, "No one to say anything to, try tell.\n" );
      return COM_OK;
    } else {
      return tell( p, parray[p].last_opponent, param[0].val.string, TELL_SAY, 0);
    }
  }
  return tell( p, parray[p].opponent, param[0].val.string, TELL_SAY, 0);
}

PUBLIC int com_set( int p, param_list param )
{
  int result;
  int which;
  char *val;

  if (param[1].type == TYPE_NULL)
    val = NULL;
  else
    val = param[1].val.string;
  result = var_set( p, param[0].val.word, val, &which );
  switch (result) {
    case VAR_OK:
      break;
    case VAR_BADVAL:
      pprintf( p, "Bad value given for variable %s.\n", variables );
      break;
    case VAR_NOSUCH:
      pprintf( p, "No such variable name %s.\n", param[0].val.word );
      break;
    case VAR_AMBIGUOUS:
      pprintf( p, "Ambiguous variable name %s.\n", param[0].val.word );
      break;
  }
  return COM_OK;
}

PUBLIC int com_stats( int p, param_list param )
{
  int p1, connected;
  char line[80], tmp[80];
  int i, t;

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
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
  } else {
    p1 = p;
    connected = 1;
  }
  sprintf( line, "\nStatistics for %s (", parray[p1].name );
  if (connected) {
    sprintf( tmp, "On for: %s", hms( player_ontime(p1), 0, 0, 0 ) );
    strcat( line, tmp );
    sprintf( tmp, "   Idle for: %s ):\n\n", hms( player_idle(p1), 0, 0, 0 ) );
  } else {
    if ((t = player_lastdisconnect(p1)))
      sprintf( tmp, "Last disconnected %s):\n\n", strltime(&t) );
    else
      sprintf( tmp, "Never connected.)\n\n" );
  }
  strcat( line, tmp );
  pprintf( p, "%s", line );
  if (!parray[p1].registered) {
    pprintf( p, "%s is UNREGISTERED.\n\n", parray[p1].name );
  } else {
    if (!parray[p1].network_player) {
      pprintf( p, "%s is LOCAL ONLY.\n\n", parray[p1].name );
    }
  }
  pprintf( p, "         rating    win   loss   draw  total\n" );
  pprintf( p, "Blitz     %4s    %4d   %4d   %4d   %4d\n",
               ratstr(parray[p1].b_stats.rating),
               parray[p1].b_stats.win,
               parray[p1].b_stats.los,
               parray[p1].b_stats.dra,
               parray[p1].b_stats.num );
  pprintf( p, "Standard  %4s    %4d   %4d   %4d   %4d\n",
               ratstr(parray[p1].s_stats.rating),
               parray[p1].s_stats.win,
               parray[p1].s_stats.los,
               parray[p1].s_stats.dra,
               parray[p1].s_stats.num );

  pprintf( p, "\nAdmin Level: " );
  switch (parray[p1].adminLevel) {
    case 0:
      pprintf( p, "Normal User\n" );
      break;
    case 10:
      pprintf( p, "Administrator\n" );
      break;
    case 20:
      pprintf( p, "Master Administrator\n" );
      break;
    case 100:
      pprintf( p, "Super User, Top Dog, Head Honcho, Boss, ...\n" );
      break;
    default:
      pprintf( p, "%d\n", parray[p1].adminLevel );
      break;
  }
    if (((p1 == p) || (parray[p].adminLevel >= 10)) && (parray[p1].emailAddress))
    pprintf( p, "Email: %s\n", parray[p1].emailAddress );

  if (parray[p1].num_plan) {
    pprintf( p, "\n" );
    for (i=0;i<parray[p1].num_plan;i++)
      pprintf( p, "%d: %s\n", i+1, parray[p1].planLines[i] );
  }
  if (!connected)
    player_remove( p1 );
  return COM_OK;
}

PUBLIC int com_variables( int p, param_list param )
{
  int p1, connected;
  int i;

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
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
  } else {
    p1 = p;
    connected = 1;
  }
  pprintf( p, "Variable settings of %s:\n", parray[p1].name );
  if (parray[p1].fullName)
    pprintf( p, "   Realname: %s\n", parray[p1].fullName );
  if (parray[p1].uscfRating)
    pprintf( p, "   USCF: %d\n", parray[p1].uscfRating );
  pprintf( p, "   rated=%d\n   open=%d simopen=%d\n   shout=%d kib=%d tell=%d pin=%d gin=%d\n   style=%d highlight=%d bell=%d\n   auto=%d private=%d\n",
                 parray[p1].rated, parray[p1].open, parray[p1].sopen,
                 parray[p1].i_shout, parray[p1].i_kibitz,
                 parray[p1].i_tell, parray[p1].i_login, parray[p1].i_game,
                 parray[p1].style, parray[p1].highlight,
                 parray[p1].bell, parray[p1].automail, parray[p1].private );
  if (parray[p1].prompt && parray[p1].prompt != def_prompt)
    pprintf( p, "   Prompt: %s\n", parray[p1].prompt );
  if (parray[p1].numAlias) {
    pprintf( p, "   Aliases:\n" );
    for (i=0;i<parray[p1].numAlias;i++) {
      pprintf( p, "      %s %s\n", parray[p1].alias_list[i].comm_name,
                                     parray[p1].alias_list[i].alias);
    }
  }
  if (!connected)
    player_remove( p1 );
  return COM_OK;
}

PUBLIC int com_password( int p, param_list param )
{
  char *oldpassword = param[0].val.word;
  char *newpassword = param[1].val.word;
  char salt[3];

  if (!parray[p].registered) {
    pprintf( p, "Setting a password is only for registered players.\n" );
    return COM_OK;
  }
  if (parray[p].passwd) {
    salt[0] = parray[p].passwd[0];
    salt[1] = parray[p].passwd[1];
    salt[2] = '\0';
    if (strcmp(crypt(oldpassword, salt), parray[p].passwd)) {
      pprintf( p, "Incorrect password, password not changed!\n" );
      return COM_OK;
    }
    rfree(parray[p].passwd);
    parray[p].passwd = NULL;
  }
  salt[0] = 'a' + rand() % 26;
  salt[1] = 'a' + rand() % 26;
  salt[2] = '\0';
  parray[p].passwd = strdup( crypt(newpassword, salt) );
  pprintf( p, "Password changed to \"%s\".\n", newpassword );
  return COM_OK;
}

PUBLIC int com_uptime( int p, param_list param )
{
  unsigned long uptime;
  unsigned long total, last1, last5;
  unsigned long ctotal, clast1, clast5;
  unsigned long btotal, blast1, blast5, broken[IN_MAX+1];

  uptime = time(0) - startuptime;
  tally_usage(); /* Get latest usage numbers */
  tally_getusage( &total, &last1, &last5 );
  tall_getcommand( &ctotal, &clast1, &clast5 );
  tally_getsent( &btotal, &blast1, &blast5, broken );
  pprintf( p, "The server has been up since %s.\n", strltime(&startuptime) );
  pprintf( p, "Up for: %s.\n", hms(uptime, 1, 1, 0) );
  pprintf( p, "Allocs: %u  Frees: %u  Allocs In Use: %u\n",
              malloc_count, free_count, malloc_count - free_count );
  pprintf( p, "CPU Usage:\tTotal:\t\t%.2f%%\n\t\tLast minute:\t%.2f%%\n\t\tLast 5 minutes:\t%.2f%%\n",
           (float)total / ((float)uptime * 100.0),
           (float)last1 / ((float)uptime * 100.0),
           (float)last5 / ((float)uptime * 100.0) );
  pprintf( p, "\n\t\t\t\tCount\tPer Minute\n" );
  pprintf( p, "Commands:\tTotal:\t\t%u\t%.2f\n\t\tLast minute:\t%u\t%.2f\n\t\tLast 5 Minutes:\t%u\t%.2f\n",
         ctotal, (float)ctotal/((float)uptime / 60.0),
         clast1, (float)clast1,
         clast5, (float)clast5/5.0 );
  pprintf( p, "Network Usage:\tTotal:\t\t%.1f\t%.2f\n\t\tLast minute:\t%.1f\t%.2f\n\t\tLast 5 Minutes:\t%.1f\t%.2f\n",
         (float)btotal/1024.0, ((float)btotal/1024.0)/((float)uptime / 60.0),
         (float)blast1/1024.0, ((float)blast1/1024.0),
         (float)blast1/1024.0, ((float)blast5/1024.0)/5.0 );
  pprintf( p, "Distribution:\tBoards\tShout\tTell\tHelp\tInput\tOther\n" );
  pprintf( p, "\t\t%3.0f%%\t%3.0f%%\t%3.0f%%\t%3.0f%%\t%3.0f%%\t%3.0f%%\n",
            100.0*(float)broken[IN_BOARD]/btotal,
            100.0*(float)broken[IN_SHOUT]/btotal,
            100.0*(float)broken[IN_TELL]/btotal,
            100.0*(float)broken[IN_HELP]/btotal,
            100.0*(float)broken[IN_INPUT]/btotal,
            100.0*(float)broken[IN_OTHER]/btotal
             );
  pprintf( p, "Player limit: %d\n", max_connections );
  pprintf( p, "There are currently %d player(s).\n", player_count() );
  pprintf( p, "There are currently %d games(s).\n", game_count() );
  return COM_OK;
}

PUBLIC int com_date( int p, param_list param )
{
  int t=time(0);
  pprintf( p, "Local time     - %s\n", strltime(&t) );
  pprintf( p, "Greenwich time - %s\n", strgtime(&t) );
  return COM_OK;
}

char *inout_string[]= {
  "Connected", "Disconnected"
};

PRIVATE int plogins( p, fname )
  int p;
  char *fname;
{
  FILE *fp;
  int inout, thetime, registered;
  char loginName[MAX_LOGIN_NAME+1];
  fp = fopen( fname, "r" );
  if (!fp) {
      pprintf( p, "Sorry, no login information available.\n" );
      return COM_OK;
  }
  while (!feof(fp)) {
    if (fscanf(fp, "%d %s %d %d\n", &inout, loginName, &thetime, &registered) != 4) {
      fprintf( stderr, "FICS: Error in login info format. %s\n", fname );
      fclose(fp);
      return COM_OK;
    }
    pprintf( p, "%s: %s %s\n", strltime(&thetime), loginName, inout_string[inout] );
  }
  fclose(fp);
  return COM_OK;
}

PUBLIC int com_llogons( int p, param_list param )
{
  char fname[MAX_FILENAME_SIZE];

  sprintf( fname, "%s/%s", stats_dir, STATS_LOGONS );
  return plogins( p, fname );
}

PUBLIC int com_logons( int p, param_list param )
{
  char fname[MAX_FILENAME_SIZE];

  if (param[0].type == TYPE_WORD) {
    sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, param[0].val.word[0], param[0].val.word, STATS_LOGONS );
  } else {
    sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS );
  }
  return plogins( p, fname );
}

#define WHO_OPEN 0x01
#define WHO_CLOSED 0x02
#define WHO_RATED 0x04
#define WHO_UNRATED 0x08
#define WHO_FREE 0x10
#define WHO_PLAYING 0x20
#define WHO_REGISTERED 0x40
#define WHO_UNREGISTERED 0x80

PRIVATE void who_terse( p, num, plist )
  int p;
  int num;
  int plist[];
{
  char ptmp[80];
  multicol *m=multicol_start( 256 );
  int i;
  int p1;

  for (i=0;i<num;i++) {
    p1 = plist[i];
    if (parray[p1].game >= 0) {
      sprintf( ptmp, "%-4s^%s", ratstr(parray[p1].b_stats.rating),
                                parray[p1].name );
    } else if (!parray[p1].open) {
      sprintf( ptmp, "%-4s:%s", ratstr(parray[p1].b_stats.rating),
                                parray[p1].name );
    } else if (player_idle(p1) > 300) {
      sprintf( ptmp, "%-4s.%s", ratstr(parray[p1].b_stats.rating),
                                parray[p1].name );
    } else {
      sprintf( ptmp, "%-4s %s", ratstr(parray[p1].b_stats.rating),
                                parray[p1].name );
    }
    if (parray[p1].adminLevel > 10)
      strcat( ptmp, "(*)" );
    multicol_store( m, ptmp );
  }
  multicol_pprint( m, p, 80, 2 );
  multicol_end( m );
  pprintf( p, "\n %d Players displayed (of %d). (*) indicates system administrator.\n", num, player_count() );
}

PRIVATE void who_verbose( p, num, plist )
  int p;
  int num;
  int plist[];
{
  int i, p1;
  char playerLine[80], tmp[80];

  pprintf( p,
" +---------------------------------------------------------------+\n"
  );
  pprintf( p,
" |      User              Standard    Blitz        On for   Idle |\n"
  );
  pprintf( p,
" +---------------------------------------------------------------+\n"
  );

  for (i=0;i<num;i++) {
    p1 = plist[i];

    strcpy( playerLine, " |" );

    if (parray[p1].game >= 0)
      sprintf( tmp, "%3d", parray[p1].game+1);
    else
      sprintf( tmp, "   " );
    strcat( playerLine, tmp );

    if (!parray[p1].open)
      sprintf( tmp, "X"  );
    else
      sprintf( tmp, " " );
    strcat( playerLine, tmp );

    if (parray[p1].registered)
      if (parray[p1].rated) {
        sprintf( tmp, " "  );
      } else {
        sprintf( tmp, "u"  );
      }
    else
      sprintf( tmp, "U" );
    strcat( playerLine, tmp );

    sprintf( tmp, " %-18s%4s        %-4s        %5s  ",
              parray[p1].name,
              ratstr(parray[p1].s_stats.rating),
              ratstr(parray[p1].b_stats.rating),
              hms( player_ontime(p1), 0, 0, 0 ) );
    strcat( playerLine, tmp );

    if (player_idle(p1) >= 60) {
      sprintf( tmp, "%5s   |\n", hms( player_idle(p1), 0, 0, 0 ) );
    } else {
      sprintf( tmp, "        |\n" );
    }
    strcat( playerLine, tmp );
    pprintf( p, "%s", playerLine );
  }

  pprintf( p,
" |                                                               |\n"
  );
  pprintf( p,
" |    %3d Players Displayed                                      |\n",
  num
  );
  pprintf( p,
" +---------------------------------------------------------------+\n"
  );
}

PRIVATE void who_winloss( p, num, plist )
  int p;
  int num;
  int plist[];
{
  int i, p1;
  char playerLine[80], tmp[80];

  pprintf( p,
"Name               Stand     win loss draw   Blitz    win loss draw    idle\n"
  );
  pprintf( p,
"----------------   -----     -------------   -----    -------------    ----\n"
  );

  for (i=0;i<num;i++) {
    p1 = plist[i];

    sprintf( playerLine, "%-19s", parray[p1].name );

    if (parray[p1].s_stats.rating != 0) {
      sprintf( tmp, "%4s     %4d %4d %4d   ",
                 ratstr(parray[p1].s_stats.rating),
                 (int)parray[p1].s_stats.win,
                 (int)parray[p1].s_stats.los,
                 (int)parray[p1].s_stats.dra );
    } else {
      sprintf( tmp, "----        0    0    0   " );
    }
    strcat( playerLine, tmp );

    if (parray[p1].b_stats.rating != 0) {
      sprintf( tmp, "%4s    %4d %4d %4d   ",
                 ratstr(parray[p1].b_stats.rating),
                 (int)parray[p1].b_stats.win,
                 (int)parray[p1].b_stats.los,
                 (int)parray[p1].b_stats.dra );
    } else {
      sprintf( tmp, "----       0    0    0   " );
    }
    strcat( playerLine, tmp );

    if (player_idle(p1) >= 60) {
      sprintf( tmp, "%5s\n", hms( player_idle(p1), 0, 0, 0 ) );
    } else {
      sprintf( tmp, "     \n" );
    }
    strcat( playerLine, tmp );

    pprintf( p, "%s", playerLine );
  }
  pprintf( p,"    %3d Players Displayed.\n", num );
}

PRIVATE int who_ok( p, sel_bits )
  int p;
  unsigned int sel_bits;
{
  if (parray[p].status != PLAYER_PROMPT) return 0;
  if (sel_bits == 0xff) return 1;
  if (sel_bits & WHO_OPEN)
    if (!parray[p].open) return 0;
  if (sel_bits & WHO_CLOSED)
    if (parray[p].open) return 0;
  if (sel_bits & WHO_RATED)
    if (!parray[p].rated) return 0;
  if (sel_bits & WHO_UNRATED)
    if (parray[p].rated) return 0;
  if (sel_bits & WHO_FREE)
    if (parray[p].game >= 0) return 0;
  if (sel_bits & WHO_PLAYING)
    if (parray[p].game < 0) return 0;
  if (sel_bits & WHO_REGISTERED)
    if (!parray[p].registered) return 0;
  if (sel_bits & WHO_UNREGISTERED)
    if (parray[p].registered) return 0;
  return 1;
}

/* This is the of the most compliclicated commands in terms of parameters */
PUBLIC int com_who( int p, param_list param )
{
  int style=0;
  int *sortarray=sort_blitz;
  float stop_perc=1.0;
  float start_perc=0;
  unsigned int sel_bits=0xff;

  int plist[256];
  int startpoint;
  int stoppoint;
  int i, len;
  int tmpI, tmpJ;
  char c;
  int p1, count, num_who;

  if (param[0].type == TYPE_WORD) {
    len = strlen(param[0].val.word);
    for (i=0;i<len;i++) {
      c = param[0].val.word[i];
      if (isdigit(c)) {
        if (i==0 || !isdigit(param[0].val.word[i-1])) {
          tmpI = c - '0';
          if (tmpI == 1) {
            start_perc = 0.0;
            stop_perc = 0.333333;
          } else if (tmpI == 2) {
            start_perc = 0.333333;
            stop_perc = 0.6666667;
          } else if (tmpI == 3) {
            start_perc = 0.6666667;
            stop_perc = 1.0;
          } else
            if ((i == len-1) || (!isdigit(param[0].val.word[i+1])))
             return COM_BADPARAMETERS;
        } else {
          tmpI = c - '0';
          tmpJ = param[0].val.word[i-1] - '0';
          if (tmpI == 0) return COM_BADPARAMETERS;
          if (tmpJ > tmpI) return COM_BADPARAMETERS;
          start_perc = (float)tmpJ-1.0 / (float)tmpI;
          stop_perc = (float)tmpJ / (float)tmpI;
        }
      } else {
        switch (c) {
          case 'o':
            if (sel_bits == 0xff)
              sel_bits = WHO_OPEN;
            else
              sel_bits |= WHO_OPEN;
            break;
          case 'r':
            if (sel_bits == 0xff)
              sel_bits = WHO_RATED;
            else
              sel_bits |= WHO_RATED;
            break;
          case 'f':
            if (sel_bits == 0xff)
              sel_bits = WHO_FREE;
            else
              sel_bits |= WHO_FREE;
            break;
          case 'a':
            if (sel_bits == 0xff)
              sel_bits = WHO_FREE | WHO_OPEN;
            else
              sel_bits |= (WHO_FREE | WHO_OPEN);
            break;
          case 'g':
            if (sel_bits == 0xff)
              sel_bits = WHO_REGISTERED;
            else
              sel_bits |= WHO_REGISTERED;
            break;
          case 's': /* Sort order */
              sortarray = sort_stand;
              break;
          case 'b': /* Sort order */
              sortarray = sort_blitz;
              break;
          case 't': /* format */
            style = 0;
            break;
          case 'v': /* format */
            style = 1;
            break;
          case 'n': /* format */
            style = 2;
            break;
          default:
            return COM_BADPARAMETERS;
            break;
        }
      }
    }
  }
  count = 0;
  for (p1=0; p1 < p_num; p1++) {
    if (!who_ok( sortarray[p1], sel_bits )) continue;
    count++;
  }
  startpoint = floor((float)count * start_perc);
  stoppoint = ceil((float)count * stop_perc) - 1;
  num_who = 0;
  count = 0;
  for (p1=0; p1 < p_num; p1++) {
    if (!who_ok( sortarray[p1], sel_bits )) continue;
    if ((count >= startpoint) && (count <= stoppoint)) {
      plist[num_who++] = sortarray[p1];
    }
    count++;
  }
  if (num_who == 0) {
    pprintf( p, "There are no players logged in that match your flag set.\n" );
    return COM_OK;
  }
  switch (style) {
    case 0: /* terse */
      who_terse( p, num_who, plist );
      break;
    case 1: /* verbose */
      who_verbose( p, num_who, plist );
      break;
    case 2: /* win-loss */
      who_winloss( p, num_who, plist );
      break;
    default:
      return COM_BADPARAMETERS;
      break;
  }
  return COM_OK;
}

PUBLIC int com_censor( int p, param_list param )
{
  int i;
  int p1;

  if (param[0].type != TYPE_WORD) {
    if (!parray[p].num_censor) {
      pprintf( p, "You have no one censored.\n" );
      return COM_OK;
    } else
      pprintf( p, "You have censored:" );
    for (i=0; i < parray[p].num_censor; i++ ) {
      pprintf( p, " %s", parray[p].censorList[i] );
    }
    pprintf( p, ".\n" );
    return COM_OK;
  }
  if (parray[p].num_censor >= MAX_CENSOR) {
    pprintf( p, "Sorry, you are already censoring the maximum number of players.\n" );
    return COM_OK;
  }
  if ((p1 = player_find_part_login( param[0].val.word )) >= 0) {
    if (player_censored( p, p1 )) {
      pprintf( p, "You are already censoring %s.\n", parray[p1].name );
      return COM_OK;
    }
    if (p1 == p ) {
      pprintf( p, "You can't censor yourself.\n" );
      return COM_OK;
    }
    parray[p].censorList[parray[p].num_censor++] = strdup( parray[p1].name );
    pprintf( p, "%s censored.\n", parray[p1].name );
  } else {
    pprintf( p, "No player named %s is logged in.\n", param[0].val.word );
  }
  return COM_OK;
}

PUBLIC int com_uncensor( int p, param_list param )
{
  char *pname=NULL;
  int i;
  int unc=0;

  if (param[0].type == TYPE_WORD) {
    pname = param[0].val.word;
  }
  for (i=0; i < parray[p].num_censor; i++) {
    if (!pname || !strcasecmp( pname, parray[p].censorList[i] ) ) {
      pprintf( p, "%s uncensored.\n", parray[p].censorList[i] );
      rfree( parray[p].censorList[i] );
      parray[p].censorList[i] = NULL;
      unc++;
    }
  }
  if (unc) {
    for (i=0; i < parray[p].num_censor; i++) {
      if (!parray[p].censorList[i] ) {
        parray[p].censorList[i] = parray[p].censorList[i+1];
        i = i-1;
        parray[p].num_censor = parray[p].num_censor - 1;
      }
    }
  } else {
    pprintf( p, "No one was uncensored.\n" );
  }
  return COM_OK;
}

PUBLIC int com_channel(p, param)
  int p;
  param_list param;
{
  int i;

  if (param[0].type == TYPE_NULL) { /* Turn off all channels */
    for (i=0;i<MAX_CHANNELS;i++) {
      if (!channel_remove( i, p ))
        pprintf( p, "Channel %d turned off.\n", i );
    }
  } else {
    i = param[0].val.integer;
    if (i < 0) {
      pprintf( p, "The lowest channel number is 0.\n" );
      return COM_OK;
    }
    if (i >= MAX_CHANNELS) {
      pprintf( p, "The maximum channel number is %d.\n", MAX_CHANNELS-1);
      return COM_OK;
    }
    if (on_channel( i, p )) {
      if (!channel_remove( i, p ))
        pprintf( p, "Channel %d turned off.\n", i );
    } else {
      if (!channel_add( i, p ))
        pprintf( p, "Channel %d turned on.\n", i );
      else
        pprintf( p, "Channel %d is already full.\n", i );
    }
  }
  return COM_OK;
}

PUBLIC int com_inchannel(p, param)
  int p;
  param_list param;
{
  int c1, c2;
  int i, j, count=0;

  if (param[0].type == TYPE_NULL) { /* List everyone on every channel */
    c1 = -1;
    c2 = -1;
  } else if (param[1].type == TYPE_NULL) { /* One parameter */
    c1 = param[0].val.integer;
    if (c1 < 0) {
      pprintf( p, "The lowest channel number is 0.\n" );
      return COM_OK;
    }
    c2 = -1;
  } else { /* Two parameters */
    c1 = param[0].val.integer;
    c2 = param[2].val.integer;
    if ((c1 < 0) || (c2 < 0)) {
      pprintf( p, "The lowest channel number is 0.\n" );
      return COM_OK;
    }
    pprintf( p, "Two parameter inchannel is not implemented.\n" );
    return COM_OK;
  }
  if ((c1 >= MAX_CHANNELS) || (c2 >= MAX_CHANNELS)) {
    pprintf( p, "The maximum channel number is %d.\n", MAX_CHANNELS-1);
    return COM_OK;
  }
  for (i=0;i < MAX_CHANNELS; i++ ) {
    if (numOn[i] && ((c1 < 0) || (i == c1))) {
      pprintf( p, "Channel %d:", i );
      for (j=0; j < numOn[i]; j++) {
        pprintf( p, " %s", parray[channels[i][j]].name );
      }
      count++;
      pprintf( p, "\n" );
    }
  }
  if (!count) {
    if (c1 < 0)
      pprintf( p, "No channels in use.\n" );
    else
      pprintf( p, "Channel not in use.\n" );
  }
  return COM_OK;
}

PUBLIC int create_new_match( int white_player, int black_player,
                          int wt, int winc,
                          int bt, int binc,
                          int rated,
                          char *catagory, char *board )
{
  int g=game_new(), p;
  char outStr[1024];
  int reverse=0;
  int tmp;
  int w_whiteness;
  int b_whiteness;
  char *typestr="";

  if (g < 0) return COM_FAILED;
  if (!bt) {
    if (parray[white_player].lastColor == parray[black_player].lastColor) {
      w_whiteness = parray[white_player].num_white - parray[white_player].num_black;
      b_whiteness = parray[black_player].num_white - parray[black_player].num_black;
      if (w_whiteness > b_whiteness) reverse = 1;
    } else {
      if (parray[white_player].lastColor == WHITE) {
        reverse = 1;
      }
    }
  } else {
    reverse = 1; /* Chalanger is always white in unbalanced match */
  }
  if (reverse) {
    tmp = white_player;
    white_player = black_player;
    black_player = tmp;
  }
  player_remove_request( white_player, black_player, PEND_MATCH);
  player_remove_request( black_player, white_player, PEND_MATCH);
  player_remove_request( white_player, black_player, PEND_SIMUL);
  player_remove_request( black_player, white_player, PEND_SIMUL);
  player_decline_offers( white_player, -1, PEND_MATCH );
  player_withdraw_offers( white_player, -1, PEND_MATCH );
  player_decline_offers( black_player, -1, PEND_MATCH );
  player_withdraw_offers( black_player, -1, PEND_MATCH );

  wt = wt * 60; /* To Seconds */
  bt = bt * 60;
  garray[g].white = white_player;
  garray[g].black = black_player;
  garray[g].status = GAME_ACTIVE;
  garray[g].type = game_isblitz( wt/60, winc, bt/60, binc, catagory, board);
  if (garray[g].type == TYPE_UNTIMED)
    typestr = "untimed";
  else
  if (garray[g].type == TYPE_NONSTANDARD)
    typestr = "non-standard";
  else
  if (garray[g].type == TYPE_BLITZ)
    typestr = "blitz";
  else
  if (garray[g].type == TYPE_STAND)
    typestr = "standard";
  if ((garray[g].type == TYPE_UNTIMED) || (garray[g].type == TYPE_NONSTANDARD))
    garray[g].rated = 0;
  else
    garray[g].rated = rated;
  garray[g].private = parray[white_player].private ||
                      parray[black_player].private;
  garray[g].white = white_player;
  if (board_init( &garray[g].game_state, catagory, board )) {
    pprintf( white_player, "PROBLEM LOADING BOARD. Game Aborted.\n" );
    pprintf( black_player, "PROBLEM LOADING BOARD. Game Aborted.\n" );
    fprintf( stderr, "FICS: PROBLEM LOADING BOARD %s %s. Game Aborted.\n", catagory, board );
  }
  garray[g].game_state.gameNum = g;
  garray[g].wTime = wt * 10;
  garray[g].wInitTime = wt*10;
  garray[g].wIncrement = winc*10;
  if (bt == 0) {
    garray[g].bTime = wt * 10;
  } else {
    garray[g].bTime = bt * 10;
  }
  garray[g].bInitTime = bt*10;
  garray[g].bIncrement = binc*10;
  if (garray[g].game_state.onMove == BLACK) { /* Start with black */
    garray[g].numHalfMoves = 1;
    garray[g].moveListSize = 1;
    garray[g].moveList = (move_t *)rmalloc(sizeof(move_t));
    garray[g].moveList[0].fromFile = -1;
    garray[g].moveList[0].fromRank = -1;
    garray[g].moveList[0].toFile = -1;
    garray[g].moveList[0].toRank = -1;
    garray[g].moveList[0].color = WHITE;
    strcpy(garray[g].moveList[0].moveString, "NONE");
    strcpy(garray[g].moveList[0].algString, "NONE");
  } else {
    garray[g].numHalfMoves = 0;
    garray[g].moveListSize = 0;
    garray[g].moveList = NULL;
  }
  garray[g].timeOfStart = time(0);
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;
  garray[g].clockStopped = 0;
  sprintf( outStr, "\n{Game %d (%s vs. %s) Creating %s %s match.}\n",
                   g+1, parray[white_player].name,
                   parray[black_player].name,
                   garray[g].rated ? "rated" : "unrated",
                    typestr );
  pprintf( white_player, "%s", outStr );
  pprintf( black_player, "%s", outStr );
  for (p=0; p < p_num; p++) {
    if ((p == white_player) || (p == black_player)) continue;
    if (parray[p].status != PLAYER_PROMPT) continue;
    if (!parray[p].i_game) continue;
    pprintf_prompt( p, "%s", outStr );
  }
  parray[white_player].game = g;
  parray[white_player].opponent = black_player;
  parray[white_player].side = WHITE;
  parray[white_player].promote = QUEEN;
  parray[black_player].game = g;
  parray[black_player].opponent = white_player;
  parray[black_player].side = BLACK;
  parray[black_player].promote = QUEEN;
  send_boards(g);
  return COM_OK;
}

PUBLIC int com_match( int p, param_list param )
{
  int p1;
  int pendfrom, pendto;
  int ppend, p1pend;
  int wt=0, winc=0, bt=0, binc=0, rated=0;
  char catagory[100], board[100];

  if (parray[p].muzzled) {
    pprintf( p, "You are muzzled.\n" );
    return COM_OK;
  }
  if (parray[p].game >= 0) {
    pprintf( p, "You can't challenge while you are playing a game.\n" );
    return COM_OK;
  }
  if (parray[p].open == 0) {
    parray[p].open = 1;
    pprintf( p, "Setting you open for matches.\n" );
  }
  stolower(param[0].val.word);
  p1 = player_find_part_login( param[0].val.word );
  if (p1 < 0) {
    pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (p1 == p) {
    pprintf( p, "You can't offer a match to yourself.\n" );
    return COM_OK;
  }
  if (player_censored( p1, p )) {
    pprintf( p, "Player \"%s\" is censoring you.\n", parray[p1].name );
    return COM_OK;
  }
  if (player_censored( p, p1 )) {
    pprintf( p, "You are censoring \"%s\".\n", parray[p1].name );
    return COM_OK;
  }
  if (!parray[p1].open) {
    pprintf( p, "Player \"%s\" is not open to match requests.\n", parray[p1].name );
    return COM_OK;
  }
  if (parray[p1].game >= 0) {
    pprintf( p, "Player \"%s\" is involved in another game.\n", parray[p1].name );
    return COM_OK;
  }
  catagory[0] = '\0';
  board[0] = '\0';
  if (param[1].type == TYPE_NULL) {
    wt = 2; /* This should be a user variable */
    winc = 12;
    bt = 0;
    binc = 0;
    catagory[0] = '\0';
    board[0] = '\0';
  } else {
    wt = param[1].val.integer;
    if (param[2].type != TYPE_NULL)
      winc = param[2].val.integer;
    if (param[3].type != TYPE_NULL)
      bt = param[3].val.integer;
    if (param[4].type != TYPE_NULL)
      binc = param[4].val.integer;
    if (param[5].type != TYPE_NULL)
      strcpy( catagory, param[5].val.word );
    if (param[6].type != TYPE_NULL)
      strcpy( board, param[6].val.word );
  }
  if (catagory[0] && !board[0]) {
    pprintf( p, "You must specify a board and a catagory.\n" );
    return COM_OK;
  }
  if (catagory[0]) {
    char fname[MAX_FILENAME_SIZE];

    sprintf( fname, "%s/%s/%s", board_dir, catagory, board );
    if (!file_exists(fname)) {
      pprintf( p, "No such catagory/board: %s/%s\n", catagory, board );
      return COM_OK;
    }
  }
  if ((wt < 0) || (winc < 0) || (bt < 0) || (binc < 0)) {
     pprintf( p, "You can't specify negative time controls.\n" );
     return COM_OK;
  }
  if (parray[p].rated && parray[p1].rated &&
      ((game_isblitz(wt,winc,bt,binc,catagory,board) == TYPE_STAND) ||
       (game_isblitz(wt,winc,bt,binc,catagory,board) == TYPE_BLITZ))) {
    if (parray[p].network_player == parray[p1].network_player) {
      rated = 1;
    } else {
      pprintf( p, "Network vs. local player forced to not rated\n" );
      rated = 0;
    }
  } else
    rated = 0;
  /* Ok match offer will be made */

  pendto = player_find_pendto( p, p1, PEND_MATCH );
  if (pendto >= 0) {
    pprintf( p, "Updating offer already made to \"%s\".\n", parray[p1].name );
  }
  pendfrom = player_find_pendfrom( p, p1, PEND_MATCH );
  if (pendfrom >= 0) {
    if (pendto >= 0) {
      pprintf( p, "Internal error\n" );
      fprintf( stderr, "FICS: This shouldn't happen. You can't have a match pending from and to the same person.\n" );
      return COM_OK;
    }
    if ((wt == parray[p].p_from_list[pendfrom].param1) &&
        (winc == parray[p].p_from_list[pendfrom].param2) &&
        (bt == parray[p].p_from_list[pendfrom].param3) &&
        (binc == parray[p].p_from_list[pendfrom].param4) &&
        (rated == parray[p].p_from_list[pendfrom].param5) &&
        (!strcmp(catagory, parray[p].p_from_list[pendfrom].char1)) &&
        (!strcmp(board, parray[p].p_from_list[pendfrom].char2))) {
        /* Identical match, should accept! */
        if (create_new_match( p, p1, wt, winc, bt, binc, rated, catagory, board )) {
          pprintf( p, "There was a problem creating the new match.\n" );
          pprintf_prompt( p1, "There was a problem creating the new match.\n" );
        }
        return COM_OK;
    } else {
      player_remove_pendfrom( p, p1, PEND_MATCH );
      player_remove_pendto( p1, p, PEND_MATCH );
    }
  }
  if (pendto < 0) {
    ppend = player_new_pendto( p );
    if (ppend < 0) {
      pprintf( p, "Sorry you can't have any more pending matches.\n" );
      return COM_OK;
    }
    p1pend = player_new_pendfrom( p1 );
    if (p1pend < 0) {
      pprintf( p, "Sorry %s can't have any more pending matches.\n", parray[p1].name );
      parray[p].num_to = parray[p].num_to - 1;
      return COM_OK;
    }
  } else {
    ppend = pendto;
    p1pend = player_find_pendfrom( p1, p, PEND_MATCH );
  }
  parray[p].p_to_list[ppend].param1 =  wt;
  parray[p].p_to_list[ppend].param2 =  winc;
  parray[p].p_to_list[ppend].param3 =  bt;
  parray[p].p_to_list[ppend].param4 =  binc;
  parray[p].p_to_list[ppend].param5 =  rated;
  strcpy(parray[p].p_to_list[ppend].char1, catagory);
  strcpy(parray[p].p_to_list[ppend].char2, board);
  parray[p].p_to_list[ppend].type =  PEND_MATCH;
  parray[p].p_to_list[ppend].whoto =  p1;
  parray[p].p_to_list[ppend].whofrom =  p;

  parray[p1].p_from_list[p1pend].param1 =  wt;
  parray[p1].p_from_list[p1pend].param2 =  winc;
  parray[p1].p_from_list[p1pend].param3 =  bt;
  parray[p1].p_from_list[p1pend].param4 =  binc;
  parray[p1].p_from_list[p1pend].param5 =  rated;
  strcpy(parray[p1].p_from_list[p1pend].char1, catagory);
  strcpy(parray[p1].p_from_list[p1pend].char2, board);
  parray[p1].p_from_list[p1pend].type =  PEND_MATCH;
  parray[p1].p_from_list[p1pend].whoto =  p1;
  parray[p1].p_from_list[p1pend].whofrom =  p;

  if (pendfrom >= 0) {
    pprintf( p, "Declining offer from %s and offering new match parameters.\n", parray[p1].name );
    pprintf( p1, "\n%s declines your match offer a match with these parameters:", parray[p].name );
  }
  if (pendto >= 0) {
    pprintf( p, "Updating match request to: " );
    pprintf( p1, "\n%s updates the match request.\n", parray[p].name );
  } else {
    pprintf( p, "Requesting " );
    pprintf( p1, "\n", parray[p].name );
  }
  pprintf( p, "%s with %s.\n", game_str( rated, wt*60, winc, bt*60, binc, catagory, board ), parray[p1].name );
  if (parray[p1].highlight) {
    pprintf( p1, "%s requested with \033[7m%s\033[0m (%d).\n",
                  game_str( rated, wt*60, winc, bt*60, binc, catagory, board ),
                  parray[p].name,
                  game_isblitz(wt,winc,bt,binc,catagory,board) == TYPE_STAND ?
                                               parray[p].s_stats.rating :
                                               parray[p].b_stats.rating);
  } else {
    pprintf( p1, "%s requested with %s (%d).\n",
                  game_str( rated, wt*60, winc, bt*60, binc, catagory, board ),
                  parray[p].name,
                  game_isblitz(wt,winc,bt,binc,catagory,board) == TYPE_STAND ?
                                               parray[p].s_stats.rating :
                                               parray[p].b_stats.rating);
  }
  if (parray[p1].bell)
    pprintf_noformat( p1, "\007" );
  pprintf_prompt( p1, "You may accept this with \"accept %s\", decline it with\n\"decline %s\", or propose different parameters.\n",
                       parray[p].name, parray[p].name );
  return COM_OK;
}

PUBLIC int com_accept( int p, param_list param )
{
  int acceptNum= -1;
  int type= -1;
  int p1;

  if (parray[p].num_from == 0) {
    pprintf( p, "You have no offers to accept.\n" );
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_from != 1) {
      pprintf( p, "You have more than one offer to accept.\nUse \"pending\" to see them and \"accept n\" to choose which one.\n" );
      return COM_OK;
    }
    acceptNum = 0;
  } else if (param[0].type == TYPE_INT) {
    if ((param[0].val.integer < 1) || (param[0].val.integer > parray[p].num_from)) {
      pprintf( p, "Out of range. Use \"pending\" to see the list of offers.\n" );
      return COM_OK;
    }
    acceptNum = param[0].val.integer-1;
  } else if (param[0].type == TYPE_WORD) {
    if (!strcmp(param[0].val.word, "draw")) {
      type = PEND_DRAW;
    } else
    if (!strcmp(param[0].val.word, "pause")) {
      type = PEND_PAUSE;
    } else
    if (!strcmp(param[0].val.word, "adjourn")) {
      type = PEND_ADJOURN;
    } else
    if (!strcmp(param[0].val.word, "abort")) {
      type = PEND_ABORT;
    } else
    if (!strcmp(param[0].val.word, "takeback")) {
      type = PEND_TAKEBACK;
    }
    if (!strcmp(param[0].val.word, "simmatch")) {
      type = PEND_SIMUL;
    }
    if (!strcmp(param[0].val.word, "switch")) {
      type = PEND_SWITCH;
    }
    if (!strcmp(param[0].val.word, "all")) {
      while (parray[p].num_from != 0) {
        pcommand( p, "accept 1" );
      }
      return COM_OK;
    }
    if (type > 0) {
      if ((acceptNum = player_find_pendfrom( p, -1, type )) < 0) {
        pprintf( p, "There are no pending %s offers.\n", param[0].val.word );
        return COM_OK;
      }
    } else { /* Word must be a name */
      p1 = player_find_part_login( param[0].val.word );
      if (p1 < 0) {
        pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
        return COM_OK;
      }
      if ((acceptNum = player_find_pendfrom( p, p1, -1 )) < 0) {
        pprintf( p, "There are no pending offers from %s.\n", parray[p1].name );
        return COM_OK;
      }
    }
  }
  switch (parray[p].p_from_list[acceptNum].type) {
    case PEND_MATCH:
      if (parray[p].p_from_list[acceptNum].char1[0] &&
          parray[p].p_from_list[acceptNum].char2[0] ) {
        pcommand( p, "match %s %d %d %d %d %s %s",
                  parray[parray[p].p_from_list[acceptNum].whofrom].name,
                  parray[p].p_from_list[acceptNum].param1,
                  parray[p].p_from_list[acceptNum].param2,
                  parray[p].p_from_list[acceptNum].param3,
                  parray[p].p_from_list[acceptNum].param4,
                  parray[p].p_from_list[acceptNum].char1,
                  parray[p].p_from_list[acceptNum].char2 );
      } else {
        pcommand( p, "match %s %d %d %d %d",
                  parray[parray[p].p_from_list[acceptNum].whofrom].name,
                  parray[p].p_from_list[acceptNum].param1,
                  parray[p].p_from_list[acceptNum].param2,
                  parray[p].p_from_list[acceptNum].param3,
                  parray[p].p_from_list[acceptNum].param4 );
      }
      break;
    case PEND_DRAW:
      pcommand( p, "draw" );
      break;
    case PEND_PAUSE:
      pcommand( p, "pause" );
      break;
    case PEND_ABORT:
      pcommand( p, "abort" );
      break;
    case PEND_TAKEBACK:
      pcommand( p, "takeback %d", parray[p].p_from_list[acceptNum].param1 );
      break;
    case PEND_SIMUL:
      pcommand( p, "simmatch %s",
                  parray[parray[p].p_from_list[acceptNum].whofrom].name );
      break;
    case PEND_SWITCH:
      pcommand( p, "switch" );
      break;
    case PEND_ADJOURN:
      pcommand( p, "adjourn" );
      break;
  }
  return COM_OK_NOPROMPT;
}

PUBLIC int com_decline( int p, param_list param )
{
  int declineNum;
  int p1= -1, type= -1;
  int count;

  if (parray[p].num_from == 0) {
    pprintf( p, "You have no pending offers from other players.\n" );
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_from == 1) {
      p1 = parray[p].p_from_list[0].whofrom;
      type = parray[p].p_from_list[0].type;
    } else {
      pprintf( p, "You have more than one pending offer. Please specify which one\nyou wish to decline.\n'Pending' will give you the list.\n" );
      return COM_OK;
    }
  } else {
    if (param[0].type == TYPE_WORD) {
      /* Draw adjourn match takeback abort or <name> */
      if (!strcmp( param[0].val.word, "match" )) {
        type = PEND_MATCH;
      } else if (!strcmp( param[0].val.word, "draw" )) {
        type = PEND_DRAW;
      } else if (!strcmp( param[0].val.word, "pause" )) {
        type = PEND_PAUSE;
      } else if (!strcmp( param[0].val.word, "abort" )) {
        type = PEND_ABORT;
      } else if (!strcmp( param[0].val.word, "takeback" )) {
        type = PEND_TAKEBACK;
      } else if (!strcmp( param[0].val.word, "adjourn" )) {
        type = PEND_ADJOURN;
      } else if (!strcmp( param[0].val.word, "switch" )) {
        type = PEND_SWITCH;
      } else if (!strcmp( param[0].val.word, "simul" )) {
        type = PEND_SIMUL;
      } else if (!strcmp( param[0].val.word, "all" )) {
      } else {
        p1 = player_find_part_login( param[0].val.word );
        if (p1 < 0) {
          pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
          return COM_OK;
        }
      }
    } else { /* Must be an integer */
      declineNum = param[0].val.integer-1;
      if (declineNum >= parray[p].num_from || declineNum < 0) {
        pprintf( p, "Invalid offer number. Must be between 1 and %d.\n", parray[p].num_from );
        return COM_OK;
      }
      p1 = parray[p].p_from_list[declineNum].whofrom;
      type = parray[p].p_from_list[declineNum].type;
    }
  }
  count = player_decline_offers( p, p1, type );
  if (count != 1)
    pprintf( p, "%d offers declined\n", count );
  return COM_OK;
}

PUBLIC int com_withdraw( int p, param_list param )
{
  int withdrawNum;
  int p1= -1, type= -1;
  int count;

  if (parray[p].num_to == 0) {
    pprintf( p, "You have no pending offers to other players.\n" );
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_to == 1) {
      p1 = parray[p].p_to_list[0].whoto;
      type = parray[p].p_to_list[0].type;
    } else {
      pprintf( p, "You have more than one pending offer. Please specify which one\nyou wish to withdraw.\n'Pending' will give you the list.\n" );
      return COM_OK;
    }
  } else {
    if (param[0].type == TYPE_WORD) {
      /* Draw adjourn match takeback abort or <name> */
      if (!strcmp( param[0].val.word, "match" )) {
        type = PEND_MATCH;
      } else if (!strcmp( param[0].val.word, "draw" )) {
        type = PEND_DRAW;
      } else if (!strcmp( param[0].val.word, "pause" )) {
        type = PEND_PAUSE;
      } else if (!strcmp( param[0].val.word, "abort" )) {
        type = PEND_ABORT;
      } else if (!strcmp( param[0].val.word, "takeback" )) {
        type = PEND_TAKEBACK;
      } else if (!strcmp( param[0].val.word, "adjourn" )) {
        type = PEND_ADJOURN;
      } else if (!strcmp( param[0].val.word, "switch" )) {
        type = PEND_SWITCH;
      } else if (!strcmp( param[0].val.word, "simul" )) {
        type = PEND_SIMUL;
      } else if (!strcmp( param[0].val.word, "all" )) {
      } else {
        p1 = player_find_part_login( param[0].val.word );
        if (p1 < 0) {
          pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
          return COM_OK;
        }
      }
    } else { /* Must be an integer */
      withdrawNum = param[0].val.integer-1;
      if (withdrawNum >= parray[p].num_to || withdrawNum < 0) {
        pprintf( p, "Invalid offer number. Must be between 1 and %d.\n", parray[p].num_to );
        return COM_OK;
      }
      p1 = parray[p].p_to_list[withdrawNum].whoto;
      type = parray[p].p_to_list[withdrawNum].type;
    }
  }
  count = player_withdraw_offers( p, p1, type );
  if (count != 1)
    pprintf( p, "%d offers withdrawn\n", count );
  return COM_OK;
}

PUBLIC int com_pending( int p, param_list param )
{
  int i;

  if (!parray[p].num_to) {
    pprintf( p, "There are no offers pending TO other players.\n" );
  } else {
    pprintf( p, "Offers TO other players:\n" );
    for (i=0;i<parray[p].num_to;i++) {
      pprintf( p, "   " );
      player_pend_print( p, &parray[p].p_to_list[i] );
    }
  }
  if (!parray[p].num_from) {
    pprintf( p, "\nThere are no offers pending FROM other players.\n" );
  } else {
    pprintf( p, "\nOffers FROM other players:\n" );
    for (i=0;i<parray[p].num_from;i++) {
      pprintf( p, " %d: ", i+1 );
      player_pend_print( p, &parray[p].p_from_list[i] );
    }
    pprintf( p, "\nIf you wish to accept any of these offers type 'accept n'\nor just 'accept' if there is only one offer.\n" );
  }
  return COM_OK;
}

PUBLIC int com_refresh( int p, param_list param )
{
  int g;
  if (param[0].type == TYPE_NULL) {
    if (parray[p].game >= 0) {
      send_board_to( parray[p].game, p );
    } else { /* Do observing in here */
      if (parray[p].num_observe) {
        for (g=0;g<parray[p].num_observe;g++) {
          send_board_to( parray[p].observe_list[g], p );
        }
      } else
        pprintf( p, "You are neither playing nor observing a game.\n" );
    }
  } else if (param[0].type == TYPE_INT) {
    g = param[0].val.integer-1;
    if (g < 0) {
      pprintf( p, "Game numbers must be >= 1.\n" );
    } else if ((g >= g_num) || (garray[g].status != GAME_ACTIVE)) {
      pprintf( p, "No such game.\n" );
    } else {
      send_board_to( g, p );
    }
  } else {
    return COM_BADPARAMETERS;
  }
  return COM_OK;
}

PUBLIC int com_open( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand( p, "set open" )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_simopen( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand( p, "set simopen" )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_bell( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand( p, "set bell" )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_highlight( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand( p, "set highlight" )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_style( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_INT);
  if ((retval = pcommand( p, "set style %d", param[0].val.integer )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_promote( int p, param_list param )
{
  int retval;
  ASSERT(param[0].type == TYPE_WORD);
  if ((retval = pcommand( p, "set promote %s", param[0].val.word )) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_alias( int p, param_list param )
{
  int al;

  if (param[0].type == TYPE_NULL) {
    for (al=0; al<parray[p].numAlias; al++) {
      pprintf( p, "%s -> %s\n", parray[p].alias_list[al].comm_name,
                                parray[p].alias_list[al].alias );
    }
    return COM_OK;
  }
  al = alias_lookup( param[0].val.word, parray[p].alias_list );
  if (param[1].type == TYPE_NULL) {
    if (al < 0) {
      pprintf( p, "You have no alias named '%s'.\n", param[0].val.word );
    } else {
      pprintf( p, "%s -> %s\n", parray[p].alias_list[al].comm_name,
                                parray[p].alias_list[al].alias );
    }
  } else {
    if (al < 0) {
      if (parray[p].numAlias >= MAX_ALIASES-1) {
        pprintf( p, "You have your maximum of %d aliases.\n", MAX_ALIASES-1);
      } else {
        parray[p].alias_list[parray[p].numAlias].comm_name =
                                              strdup(param[0].val.word);
        parray[p].alias_list[parray[p].numAlias].alias =
                                              strdup(param[1].val.string);
        parray[p].numAlias++;
        pprintf( p, "Alias set.\n" );
      }
    } else {
      rfree(parray[p].alias_list[al].alias);
      parray[p].alias_list[al].alias = strdup(param[1].val.string);
      pprintf( p, "Alias replaced.\n" );
    }
    parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
  }
  return COM_OK;
}

PUBLIC int com_unalias( int p, param_list param )
{
  int al;
  int i;

  ASSERT(param[0].type == TYPE_WORD);
  al = alias_lookup( param[0].val.word, parray[p].alias_list );
  if (al < 0) {
    pprintf( p, "You have no alias named '%s'.\n", param[0].val.word );
  } else {
    rfree(parray[p].alias_list[al].comm_name);
    rfree(parray[p].alias_list[al].alias);
    for (i=al;i<parray[p].numAlias;i++) {
      parray[p].alias_list[i].comm_name = parray[p].alias_list[i+1].comm_name;
      parray[p].alias_list[i].alias = parray[p].alias_list[i+1].alias;
    }
    parray[p].numAlias--;
    parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
    pprintf( p, "Alias removed.\n" );
  }
  return COM_OK;
}

PUBLIC int com_servers( int p, param_list param )
{
  int i;

  ASSERT(param[0].type == TYPE_NULL);
  if (numServers == 0) {
    pprintf( p, "There are no other servers known to this server.\n" );
    return COM_OK;
  }
  pprintf( p, "There are %d known servers.\n", numServers );
  pprintf( p, "(Not all of these may be active)\n" );
  pprintf( p, "%-30s%-7s\n", "HOST", "PORT" );
  for (i=0;i<numServers;i++)
    pprintf( p, "%-30s%-7d\n", serverNames[i], serverPorts[i] );
  return COM_OK;
}

PUBLIC int com_sendmessage( int p, param_list param )
{
  int p1, connected=1;

  if (!parray[p].registered) {
    pprintf( p, "You are not registered and cannot send messages.\n" );
    return COM_OK;
  }
  if ((param[0].type == TYPE_NULL) || (param[1].type == TYPE_NULL)) {
    pprintf( p, "No message sent.\n" );
    return COM_OK;
  }
  if ((p1 = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
    connected = 0;
    p1 = player_new();
    if (player_read( p1, param[0].val.word )) {
      player_remove( p1 );
      pprintf( p, "There is no player by that name.\n" );
      return COM_OK;
    }
  }
  if (player_add_message( p1, p, param[1].val.string )) {
    pprintf( p, "Couldn't send message to %s. Message buffer full.\n", parray[p1].name );
  } else {
    pprintf( p, "Message sent to %s.\n", parray[p1].name );
    if (connected)
      pprintf_prompt( p1, "\n%s just sent you a message.\n", parray[p].name );
  }
  if (!connected)
    player_remove( p1 );
  return COM_OK;
}

PUBLIC int com_messages( int p, param_list param )
{
  if (param[0].type != TYPE_NULL) {
    return com_sendmessage( p, param );
  }
  if (player_num_messages(p) <= 0) {
    pprintf( p, "You have no messages.\n" );
    return COM_OK;
  }
  pprintf( p, "Messages:\n" );
  player_show_messages(p);
  return COM_OK;
}

PUBLIC int com_clearmessages( int p, param_list param )
{
  if (player_num_messages(p) <= 0) {
    pprintf( p, "You have no messages.\n" );
    return COM_OK;
  }
  pprintf( p, "Messages cleared.\n" );
  player_clear_messages(p);
  return COM_OK;
}
