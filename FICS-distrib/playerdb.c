/* playerdb.c
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
#include "command.h"
#include "playerdb.h"
#include "rmalloc.h"
#include "utils.h"
#include "network.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "channel.h"
#include "gamedb.h"
#include "hostinfo.h"
#include "ratings.h"

PUBLIC player *parray=NULL;
PUBLIC int p_num = 0;

PUBLIC int sort_blitz[256];
PUBLIC int sort_stand[256];
PUBLIC int sort_alpha[256];

PUBLIC char *pend_strings[7] = {"match", "draw", "abort", "take back", "adjourn", "switch", "simul"};

PRIVATE int get_empty_slot()
{
  int i;

  for (i=0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) return i;
  }
  p_num++;
  fprintf( stderr, "FICS: Increasing player array to %d.\n", p_num );
  if (!parray) {
    parray = (player *)rmalloc( sizeof(player) * p_num );
  } else {
    parray = (player *)rrealloc( parray, sizeof(player) * p_num );
  }
  parray[p_num-1].status = PLAYER_EMPTY;
  return p_num-1;
}

PUBLIC void player_init( int startConsole )
{
  int i, p;

  for (i=0;i<max_connections;i++) {
    sort_blitz[i] = sort_stand[i] = sort_alpha[i] = i;
  }
  if (startConsole) {
    net_addConnection( 0, 0 );
    p = player_new();
    parray[p].login = strdup("console");
    parray[p].name = strdup("console");
    parray[p].passwd = strdup("*");
    parray[p].fullName = strdup("The Operator");
    parray[p].emailAddress = NULL;
    parray[p].prompt = strdup("FICS # ");
    parray[p].adminLevel = ADMIN_GOD;
    parray[p].socket = 0;
    pprintf_prompt( p, "\nLogged in on console.\n" );
    player_resort( );
  }
}

PUBLIC int player_cmp( int p1, int p2, int sorttype )
{
  if (parray[p1].status != PLAYER_PROMPT) {
    if (parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (parray[p2].status != PLAYER_PROMPT) return 1;
  switch (sorttype) {
    case SORT_BLITZ:
      if (parray[p1].b_stats.rating > parray[p2].b_stats.rating)
        return -1;
      if (parray[p1].b_stats.rating < parray[p2].b_stats.rating)
        return 1;
      return 0;
      break;
    case SORT_STAND:
      if (parray[p1].s_stats.rating > parray[p2].s_stats.rating)
        return -1;
      if (parray[p1].s_stats.rating < parray[p2].s_stats.rating)
        return 1;
      return 0;
      break;
    case SORT_ALPHA:
      return strcmp( parray[p1].login, parray[p2].login );
      break;
    default:
      break;
  }
  return 0;
}

PRIVATE void player_swap( int p1, int p2, int *sort_array )
{
  int tmp;

  tmp = sort_array[p1];
  sort_array[p1] = sort_array[p2];
  sort_array[p2] = tmp;
}

/* Yuck bubble sort! */
PUBLIC void player_resort( )
{
  int i, j;
  int anychange=1;
  int count=0;

  for (i=0;i<p_num;i++) {
    sort_blitz[i] = sort_stand[i] = sort_alpha[i] = i;
    count++;
  }
  for (i=0; (i < count-1) && anychange; i++) {
    anychange = 0;
    for (j=0; j < count-1; j++) {
      if (player_cmp(sort_blitz[j], sort_blitz[j+1],SORT_BLITZ) > 0) {
        player_swap(j, j+1, sort_blitz);
        anychange = 1;
      }
      if (player_cmp(sort_stand[j], sort_stand[j+1],SORT_STAND) > 0) {
        player_swap(j, j+1, sort_stand);
        anychange = 1;
      }
      if (player_cmp(sort_alpha[j], sort_alpha[j+1],SORT_ALPHA) > 0) {
        player_swap(j, j+1, sort_alpha);
        anychange = 1;
      }
    }
  }
}

PUBLIC int player_new( )
{
  int new = get_empty_slot();
  player_zero( new );
  return new;
}

PUBLIC int player_zero( int p )
{
  parray[p].name = NULL;
  parray[p].login = NULL;
  parray[p].fullName = NULL;
  parray[p].emailAddress = NULL;
  parray[p].prompt = def_prompt;
  parray[p].passwd = NULL;
  parray[p].socket = -1;
  parray[p].registered = 0;
  parray[p].status = PLAYER_NEW;
  parray[p].s_stats.num = 0;
  parray[p].s_stats.win = 0;
  parray[p].s_stats.los = 0;
  parray[p].s_stats.dra = 0;
  parray[p].s_stats.rating = 0;
  parray[p].b_stats.num = 0;
  parray[p].b_stats.win = 0;
  parray[p].b_stats.los = 0;
  parray[p].b_stats.dra = 0;
  parray[p].b_stats.rating = 0;
  parray[p].open = 1;
  parray[p].rated = 0;
  parray[p].bell = 0;
  parray[p].i_login = 1;
  parray[p].i_game = 1;
  parray[p].i_shout = 1;
  parray[p].i_tell = 1;
  parray[p].i_kibitz = 1;
  parray[p].private = 0;
  parray[p].automail = 0;
  parray[p].style = 0;
  parray[p].promote = QUEEN;
  parray[p].game = -1;
  parray[p].last_tell = -1;
  parray[p].last_channel = -1;
  parray[p].logon_time = 0;
  parray[p].last_command_time = time(0);
  parray[p].num_from = 0;
  parray[p].num_to = 0;
  parray[p].adminLevel = 0;
  parray[p].num_plan = 0;
  parray[p].num_censor = 0;
  parray[p].num_white = 0;
  parray[p].num_black = 0;
  parray[p].num_observe = 0;
  parray[p].uscfRating = 0;
  parray[p].muzzled = 0;
  parray[p].network_player = 0;
  parray[p].thisHost = 0;
  parray[p].lastHost = 0;
  parray[p].lastColor = WHITE;
  parray[p].numAlias = 0;
  parray[p].alias_list[0].comm_name = NULL;
  parray[p].opponent = -1;
  parray[p].last_opponent = -1;
  parray[p].highlight = 0;
  parray[p].query_log = NULL;
  parray[p].sopen = 0;
  parray[p].simul_info.numBoards = 0;
  return 0;
}

PUBLIC int player_free( int p )
{
  int i;

  if (parray[p].login) rfree(parray[p].login);
  if (parray[p].name) rfree(parray[p].name);
  if (parray[p].passwd) rfree(parray[p].passwd);
  if (parray[p].fullName) rfree(parray[p].fullName);
  if (parray[p].emailAddress) rfree(parray[p].emailAddress);
  if (parray[p].prompt && (parray[p].prompt != def_prompt))
    rfree(parray[p].prompt);
  for (i=0;i<parray[p].num_plan;i++)
    if (parray[p].planLines[i][0] != ' ')
      rfree(parray[p].planLines[i]);
  for (i=0;i<parray[p].num_censor;i++)
    rfree(parray[p].censorList[i]);
  for (i=0;i<parray[p].numAlias;i++) {
    if (parray[p].alias_list[i].comm_name)
      rfree( parray[p].alias_list[i].comm_name);
    if (parray[p].alias_list[i].alias)
      rfree( parray[p].alias_list[i].alias);
  }
  if (parray[p].query_log)
    tl_free(parray[p].query_log);
  return 0;
}

PUBLIC int player_clear( int p )
{
  player_free( p );
  player_zero( p );
  return 0;
}

PUBLIC int player_remove( int p )
{
  int i;

  player_decline_offers( p, -1, -1 );
  player_withdraw_offers( p, -1, -1 );
  if (parray[p].simul_info.numBoards) { /* Player disconnected in middle of simul */
    for (i=0;i<parray[p].simul_info.numBoards;i++) {
      if (parray[p].simul_info.boards[i] >= 0) {
        game_disconnect( parray[p].simul_info.boards[i], p );
      }
    }
  }
  if (parray[p].game >= 0) { /* Player disconnected in the middle of a game! */
    game_disconnect( parray[p].game, p );
  }
  ReallyRemoveOldGamesForPlayer(p);
  for (i=0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) continue;
    if (parray[i].last_tell == p)
      parray[i].last_tell = -1;
    if (parray[i].last_opponent == p)
      parray[i].last_opponent = -1;
  }
  for (i=0;i<MAX_CHANNELS;i++) channel_remove(i,p);
  player_clear( p );
  parray[p].status = PLAYER_EMPTY;
  player_resort( );
  return 0;
}

PRIVATE int got_attr_value( int p, char *attr, char *value, FILE *fp, char *file )
{
  int i, len;
  char tmp[MAX_LINE_SIZE], *tmp1;

  if (!strcmp( attr, "name:" )) {
    parray[p].name = strdup(value);
  } else if (!strcmp( attr, "password:" )) {
    parray[p].passwd = strdup(value);
  } else if (!strcmp( attr, "fullname:" )) {
    parray[p].fullName = strdup(value);
  } else if (!strcmp( attr, "email:" )) {
    parray[p].emailAddress = strdup(value);
  } else if (!strcmp( attr, "prompt:" )) {
    parray[p].prompt = strdup(value);
  } else if (!strcmp( attr, "s_num:" )) {
    parray[p].s_stats.num = atoi(value);
  } else if (!strcmp( attr, "s_win:" )) {
    parray[p].s_stats.win = atoi(value);
  } else if (!strcmp( attr, "s_loss:" )) {
    parray[p].s_stats.los = atoi(value);
  } else if (!strcmp( attr, "s_draw:" )) {
    parray[p].s_stats.dra = atoi(value);
  } else if (!strcmp( attr, "s_rating:" )) {
    parray[p].s_stats.rating = atoi(value);
  } else if (!strcmp( attr, "b_num:" )) {
    parray[p].b_stats.num = atoi(value);
  } else if (!strcmp( attr, "b_win:" )) {
    parray[p].b_stats.win = atoi(value);
  } else if (!strcmp( attr, "b_loss:" )) {
    parray[p].b_stats.los = atoi(value);
  } else if (!strcmp( attr, "b_draw:" )) {
    parray[p].b_stats.dra = atoi(value);
  } else if (!strcmp( attr, "b_rating:" )) {
    parray[p].b_stats.rating = atoi(value);
  } else if (!strcmp( attr, "open:" )) {
    parray[p].open = atoi(value);
  } else if (!strcmp( attr, "rated:" )) {
    parray[p].rated = atoi(value);
  } else if (!strcmp( attr, "bell:" )) {
    parray[p].bell = atoi(value);
  } else if (!strcmp( attr, "i_login:" )) {
    parray[p].i_login = atoi(value);
  } else if (!strcmp( attr, "i_game:" )) {
    parray[p].i_game = atoi(value);
  } else if (!strcmp( attr, "i_shout:" )) {
    parray[p].i_shout = atoi(value);
  } else if (!strcmp( attr, "i_tell:" )) {
    parray[p].i_tell = atoi(value);
  } else if (!strcmp( attr, "i_kibitz:" )) {
    parray[p].i_kibitz = atoi(value);
  } else if (!strcmp( attr, "private:" )) {
    parray[p].private = atoi(value);
  } else if (!strcmp( attr, "automail:" )) {
    parray[p].automail = atoi(value);
  } else if (!strcmp( attr, "style:" )) {
    parray[p].style = atoi(value);
  } else if (!strcmp( attr, "admin_level:" )) {
    parray[p].adminLevel = atoi(value);
  } else if (!strcmp( attr, "black_games:" )) {
    parray[p].num_black = atoi(value);
  } else if (!strcmp( attr, "white_games:" )) {
    parray[p].num_white = atoi(value);
  } else if (!strcmp( attr, "uscf:" )) {
    parray[p].uscfRating = atoi(value);
  } else if (!strcmp( attr, "muzzled:" )) {
    parray[p].muzzled = atoi(value);
  } else if (!strcmp( attr, "highlight:" )) {
    parray[p].highlight = atoi(value);
  } else if (!strcmp( attr, "network:" )) {
    parray[p].network_player = atoi(value);
  } else if (!strcmp( attr, "lasthost:" )) {
    parray[p].lastHost = atoi(value);
  } else if (!strcmp( attr, "num_plan:" )) {
    parray[p].num_plan = atoi(value);
    if (parray[p].num_plan > 0) {
      for (i=0;i<parray[p].num_plan;i++) {
        fgets( tmp, MAX_LINE_SIZE, fp );
        if (!(len = strlen(tmp))) {
          fprintf( stderr, "FICS: Error bad plan in file %s\n", file );
        } else {
          tmp[len-1] = '\0'; /* Get rid of '\n' */
          parray[p].planLines[i] = strdup(tmp);
        }
      }
    }
  } else if (!strcmp( attr, "num_alias:" )) {
    parray[p].numAlias = atoi(value);
    if (parray[p].numAlias > 0) {
      for (i=0;i<parray[p].numAlias;i++) {
        fgets( tmp, MAX_LINE_SIZE, fp );
        if (!(len = strlen(tmp))) {
          fprintf( stderr, "FICS: Error bad alias in file %s\n", file );
        } else {
          tmp[len-1] = '\0'; /* Get rid of '\n' */
          tmp1=tmp;
          tmp1=eatword(tmp1);
          *tmp1 = '\0';
          tmp1++;
          tmp1 = eatwhite(tmp1);
          parray[p].alias_list[i].comm_name = strdup( tmp );
          parray[p].alias_list[i].alias = strdup( tmp1 );
        }
      }
    }
    parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
  } else if (!strcmp( attr, "num_censor:" )) {
    parray[p].num_censor = atoi(value);
    if (parray[p].num_censor > 0) {
      for (i=0;i<parray[p].num_censor;i++) {
        fgets( tmp, MAX_LINE_SIZE, fp );
        if (!(len = strlen(tmp))) {
          fprintf( stderr, "FICS: Error bad censor in file %s\n", file );
        } else {
          tmp[len-1] = '\0'; /* Get rid of '\n' */
          parray[p].censorList[i] = strdup(tmp);
        }
      }
    }
  } else {
    fprintf( stderr, "FICS: Error bad attribute >%s< from file %s\n", attr, file );
  }
  return 0;
}

PUBLIC int player_read( int p, char *name )
{
  char fname[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  char *attr, *value;
  FILE *fp;
  int len;

  parray[p].login = stolower( strdup( name ) );

  if (iamserver)
    sprintf( fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  else
    sprintf( fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  fp = fopen( fname, "r" );
  if (!fp) {
    parray[p].name = strdup( name );
    parray[p].registered = 0;
    return -1;
  }
  parray[p].registered = 1;
  while (!feof(fp)) {
    fgets(line, MAX_LINE_SIZE, fp);
    if (feof(fp)) break;
    if ((len = strlen(line)) <= 1) continue;
    line[len-1] = '\0';
    attr = eatwhite(line);
    if (attr[0] == '#') continue; /* Comment */
    value = eatword(attr);
    if (!*value) {
      fprintf( stderr, "FICS: Error reading file %s\n", fname );
      continue;
    }
    *value = '\0';
    value++;
    value = eatwhite(value);
    if (!*value) {
      fprintf( stderr, "FICS: Error reading file %s\n", fname );
      continue;
    }
    stolower(attr);
    got_attr_value( p, attr, value, fp, fname );
  }
  fclose( fp );
  return 0;
}

PUBLIC int player_delete( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) { /* Player must not be registered */
    return -1;
  }
  if (iamserver)
    sprintf( fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  else
    sprintf( fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  unlink(fname);
  return 0;
}

PUBLIC int player_markdeleted( int p )
{
  FILE *fp;
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  if (!parray[p].registered) { /* Player must not be registered */
    return -1;
  }
  if (iamserver) {
    sprintf( fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
    sprintf( fname2, "%s.server/%c/%s.delete", player_dir, parray[p].login[0], parray[p].login);
  } else {
    sprintf( fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
    sprintf( fname2, "%s/%c/%s.delete", player_dir, parray[p].login[0], parray[p].login);
  }
  rename(fname, fname2);
  fp = fopen( fname2, "a" ); /* Touch the file */
  if (fp) {fprintf( fp, "\n" ); fclose(fp); }
  return 0;
}

PUBLIC int player_save( int p )
{
  FILE *fp;
  int i;
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) { /* Player must not be registered */
    return -1;
  }
  if (iamserver)
    sprintf( fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  else
    sprintf( fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  fp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Problem openning file %s for write\n", fname );
    return -1;
  }
  if (parray[p].name)
    fprintf( fp, "Name: %s\n", parray[p].name );
  if (parray[p].fullName)
    fprintf( fp, "Fullname: %s\n", parray[p].fullName );
  if (parray[p].passwd)
    fprintf( fp, "Password: %s\n", parray[p].passwd );
  if (parray[p].emailAddress)
    fprintf( fp, "Email: %s\n", parray[p].emailAddress );
  fprintf( fp, "S_NUM: %u\n", parray[p].s_stats.num );
  fprintf( fp, "S_WIN: %u\n", parray[p].s_stats.win );
  fprintf( fp, "S_LOSS: %u\n", parray[p].s_stats.los );
  fprintf( fp, "S_DRAW: %u\n", parray[p].s_stats.dra );
  fprintf( fp, "S_RATING: %u\n", parray[p].s_stats.rating );
  fprintf( fp, "B_NUM: %u\n", parray[p].b_stats.num );
  fprintf( fp, "B_WIN: %u\n", parray[p].b_stats.win );
  fprintf( fp, "B_LOSS: %u\n", parray[p].b_stats.los );
  fprintf( fp, "B_DRAW: %u\n", parray[p].b_stats.dra );
  fprintf( fp, "B_RATING: %u\n", parray[p].b_stats.rating );
  fprintf( fp, "Network: %d\n", parray[p].network_player );
  if (iamserver) return 0; /* No need to write any more */
  fprintf( fp, "LastHost: %d\n", parray[p].lastHost );
  if (parray[p].prompt != def_prompt)
    fprintf( fp, "Prompt: %s\n", parray[p].prompt );
  fprintf( fp, "Open: %d\n", parray[p].open );
  fprintf( fp, "Rated: %d\n", parray[p].rated );
  fprintf( fp, "Bell: %d\n", parray[p].bell );
  fprintf( fp, "I_Login: %d\n", parray[p].i_login );
  fprintf( fp, "I_Game: %d\n", parray[p].i_game );
  fprintf( fp, "I_Shout: %d\n", parray[p].i_shout );
  fprintf( fp, "I_Tell: %d\n", parray[p].i_tell );
  fprintf( fp, "I_Kibitz: %d\n", parray[p].i_kibitz );
  fprintf( fp, "Private: %d\n", parray[p].private );
  fprintf( fp, "Automail: %d\n", parray[p].automail );
  fprintf( fp, "Style: %d\n", parray[p].style );
  fprintf( fp, "Admin_level: %d\n", parray[p].adminLevel );
  fprintf( fp, "White_Games: %d\n", parray[p].num_white );
  fprintf( fp, "Black_Games: %d\n", parray[p].num_black );
  fprintf( fp, "USCF: %d\n", parray[p].uscfRating );
  fprintf( fp, "Muzzled: %d\n", parray[p].muzzled );
  fprintf( fp, "Highlight: %d\n", parray[p].highlight );
  fprintf( fp, "Num_plan: %d\n", parray[p].num_plan );
  for (i=0; i < parray[p].num_plan; i++)
    fprintf( fp, "%s\n", parray[p].planLines[i] );
  fprintf( fp, "Num_censor: %d\n", parray[p].num_censor );
  for (i=0; i < parray[p].num_censor; i++)
    fprintf( fp, "%s\n", parray[p].censorList[i] );
  fprintf( fp, "Num_alias: %d\n", parray[p].numAlias );
  for (i=0; i<parray[p].numAlias; i++)
    fprintf( fp, "%s %s\n", parray[p].alias_list[i].comm_name,
                            parray[p].alias_list[i].alias );
  fclose(fp);
  return 0;
}

PUBLIC int player_find( int fd )
{
  int i;

  for (i=0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) continue;
    if (parray[i].socket == fd) return i;
  }
  return -1;
}

PUBLIC int player_find_bylogin( char *name )
{
  int i;

  for (i=0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) continue;
    if (!parray[i].login) continue;
    if (!strcmp(parray[i].login, name)) return i;
  }
  return -1;
}

PUBLIC int player_find_part_login( char *name )
{
  int i;
  int found = -1;

  i = player_find_bylogin( name );
  if (i >= 0) return i;
  for (i=0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) continue;
    if (!parray[i].login) continue;
    if (!strncmp(parray[i].login, name, strlen(name))) {
      if (found >= 0) { /* Ambiguous */
        return -2;
      }
      found = i;
    }
  }
  return found;
}

PUBLIC int player_censored( int p, int p1 )
{
  int i;

  for (i=0; i < parray[p].num_censor; i++) {
    if (!strcasecmp(parray[p].censorList[i], parray[p1].login))
      return 1;
  }
  return 0;
}

PUBLIC int player_count()
{
  int count=0;
  int i;

  for (i=0; i < p_num; i++) {
    if (parray[i].status != PLAYER_PROMPT) continue;
    count++;
  }
  return count;
}

PUBLIC int player_idle( int p )
{
  if (parray[p].status != PLAYER_PROMPT)
  return time(0) - parray[p].logon_time;
  else
  return time(0) - parray[p].last_command_time;
}

PUBLIC int player_ontime( int p )
{
  return time(0) - parray[p].logon_time;
}

PRIVATE void write_p_inout( int inout, int p, char *file, int maxlines )
{
  FILE *fp;

  fp = fopen( file, "a" );
  if (!fp) return;
  fprintf( fp, "%d %s %d %d\n", inout, parray[p].name, (int)time(0), parray[p].registered );
  fclose(fp);
  truncate_file( file, maxlines );
}

PUBLIC void player_write_login( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (parray[p].registered) {
    sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS );
    write_p_inout( P_LOGIN, p, fname, 8 );
  }
  sprintf( fname, "%s/%s", stats_dir, STATS_LOGONS );
  write_p_inout( P_LOGIN, p, fname, 30 );
}

PUBLIC void player_write_logout( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (parray[p].registered) {
    sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS );
    write_p_inout( P_LOGOUT, p, fname, 8 );
  }
  sprintf( fname, "%s/%s", stats_dir, STATS_LOGONS );
  write_p_inout( P_LOGOUT, p, fname, 30 );
}

PUBLIC int player_lastdisconnect( int p )
{
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  int inout, thetime, registered;
  int last=0;
  char loginName[MAX_LOGIN_NAME];

    sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS );
  fp = fopen( fname, "r" );
  if (!fp) return 0;
  while (!feof(fp)) {
    if (fscanf(fp, "%d %s %d %d\n", &inout, loginName, &thetime, &registered) != 4) {
      fprintf( stderr, "FICS: Error in login info format. %s\n", fname );
      fclose(fp);
      return 0;
    }
    if (inout == P_LOGOUT) last = thetime;
  }
  fclose(fp);
  return last;
}

PUBLIC void player_pend_print(int p, pending *pend)
{
  char outstr[200];
  char tmp[200];

  if (p == pend->whofrom) {
    sprintf( outstr, "You are offering " );
  } else {
    sprintf( outstr, "%s is offering ", parray[pend->whofrom].name );
  }
  if (p == pend->whoto) {
    strcpy( tmp, "" );
  } else {
    sprintf( tmp, "%s ", parray[pend->whoto].name );
  }
  strcat( outstr, tmp );
  switch (pend->type) {
    case PEND_MATCH:
      sprintf( tmp, "%s.", game_str(pend->param5, pend->param1*60, pend->param2, pend->param3*60, pend->param4, pend->char1, pend->char2) );
      break;
    case PEND_DRAW:
      sprintf( tmp, "a draw.\n" );
      break;
    case PEND_PAUSE:
      sprintf( tmp, "to pause the clock.\n" );
      break;
    case PEND_ABORT:
      sprintf( tmp, "to abort the game.\n" );
      break;
    case PEND_TAKEBACK:
      sprintf( tmp, "to takeback the last %d half moves.\n", pend->param1 );
      break;
    case PEND_SIMUL:
      sprintf( tmp, "to play a simul match.\n" );
      break;
    case PEND_SWITCH:
      sprintf( tmp, "to switch sides.\n" );
      break;
    case PEND_ADJOURN:
      sprintf( tmp, "an adjournment.\n" );
      break;
  }
  strcat( outstr, tmp );
  pprintf( p, "%s\n", outstr );
}

PUBLIC int player_find_pendto( int p, int p1, int type)
{
  int i;

  for (i=0; i < parray[p].num_to; i++) {
    if (((parray[p].p_to_list[i].whoto == p1) || (p1 == -1)) &&
        ((type < 0) || (parray[p].p_to_list[i].type == type))) {
      return i;
    }
  }
  return -1;
}

PUBLIC int player_new_pendto( int p )
{
  if (parray[p].num_to >= MAX_PENDING) return -1;
  parray[p].num_to++;
  return parray[p].num_to - 1;
}

PUBLIC int player_remove_pendto( int p, int p1, int type)
{
  int w;
  if ((w = player_find_pendto( p, p1, type)) < 0) return -1;
  for (;w < parray[p].num_to - 1;w++)
    parray[p].p_to_list[w] = parray[p].p_to_list[w+1];
  parray[p].num_to = parray[p].num_to - 1;
  return 0;
}

PUBLIC int player_find_pendfrom( int p, int p1, int type)
{
  int i;

  for (i=0; i < parray[p].num_from; i++) {
    if (((parray[p].p_from_list[i].whofrom == p1) || (p1 == -1)) &&
        ((type < 0) || (parray[p].p_from_list[i].type == type)))
      return i;
  }
  return -1;
}

PUBLIC int player_new_pendfrom( int p )
{
  if (parray[p].num_from >= MAX_PENDING) return -1;
  parray[p].num_from++;
  return parray[p].num_from - 1;
}

PUBLIC int player_remove_pendfrom( int p, int p1, int type)
{
  int w;
  if ((w = player_find_pendfrom( p, p1, type)) < 0) return -1;
  for (;w < parray[p].num_from - 1;w++)
    parray[p].p_from_list[w] = parray[p].p_from_list[w+1];
  parray[p].num_from = parray[p].num_from - 1;
  return 0;
}

PUBLIC int player_add_request( int p, int p1, int type, int param )
{
  int pendt;
  int pendf;

  if (player_find_pendto( p, p1, type) >= 0) return -1; /* Already exists */
  pendt=player_new_pendto(p);
  if (pendt == -1) {
    return -1;
  }
  pendf=player_new_pendfrom(p1);
  if (pendf == -1) {
    parray[p].num_to--; /* Remove the pendto we allocated */
    return -1;
  }
  parray[p].p_to_list[pendt].type = type;
  parray[p].p_to_list[pendt].whoto = p1;
  parray[p].p_to_list[pendt].whofrom = p;
  parray[p].p_to_list[pendt].param1 = param;
  parray[p1].p_from_list[pendf].type = type;
  parray[p1].p_from_list[pendf].whoto = p1;
  parray[p1].p_from_list[pendf].whofrom = p;
  parray[p1].p_from_list[pendf].param1 = param;
  return 0;
}

PUBLIC int player_remove_request( int p, int p1, int type )
{
  int to=0, from=0;

  while (to != -1) {
    to = player_find_pendto( p, p1, type);
    if (to != -1) {
      for (;to < parray[p].num_to - 1;to++)
        parray[p].p_to_list[to] = parray[p].p_to_list[to+1];
      parray[p].num_to = parray[p].num_to - 1;
    }
  }
  while (from != -1) {
    from = player_find_pendfrom( p1, p, type);
    if (from != -1) {
      for (; from < parray[p1].num_from - 1; from++)
        parray[p1].p_from_list[from] = parray[p1].p_from_list[from+1];
      parray[p1].num_from = parray[p1].num_from - 1;
    }
  }
  return 0;
}


PUBLIC int player_decline_offers( int p, int p1, int offerType )
{
  int offer;
  int type, p2;
  int count=0;

  while ((offer = player_find_pendfrom( p, p1, offerType)) >= 0) {
    type = parray[p].p_from_list[offer].type;
    p2 = parray[p].p_from_list[offer].whofrom;
    switch (type) {
      case PEND_MATCH:
        pprintf_prompt( p2, "\n%s declines the match offer.\n", parray[p].name );
      pprintf( p, "You decline the match offer from %s.\n", parray[p2].name );
      break;
      case PEND_DRAW:
        pprintf_prompt( p2, "\n%s declines draw request.\n", parray[p].name );
      pprintf( p, "You decline the draw request from %s.\n", parray[p2].name );
      break;
      case PEND_PAUSE:
        pprintf_prompt( p2, "\n%s declines pause request.\n", parray[p].name );
      pprintf( p, "You decline the pause request from %s.\n", parray[p2].name );
      break;
      case PEND_ABORT:
        pprintf_prompt( p2, "\n%s declines abort request.\n", parray[p].name );
      pprintf( p, "You decline the abort request from %s.\n", parray[p2].name );
      break;
      case PEND_TAKEBACK:
        pprintf_prompt( p2, "\n%s declines the takeback request.\n", parray[p].name );
      pprintf( p, "You decline the takeback request from %s.\n", parray[p2].name );
      break;
      case PEND_ADJOURN:
        pprintf_prompt( p2, "\n%s declines the adjourn request.\n", parray[p].name );
      pprintf( p, "You decline the adjourn request from %s.\n", parray[p2].name );
      break;
      case PEND_SWITCH:
        pprintf_prompt( p2, "\n%s declines the switch sides request.\n", parray[p].name );
      pprintf( p, "You decline the switch sides request from %s.\n", parray[p2].name );
      break;
      case PEND_SIMUL:
        pprintf_prompt( p2, "\n%s declines the simul offer.\n", parray[p].name );
      pprintf( p, "You decline the simul offer from %s.\n", parray[p2].name );
      break;
    }
    player_remove_request( p2, p, type);
    count++;
  }
  return count;
}


PUBLIC int player_withdraw_offers( int p, int p1, int offerType )
{
  int offer;
  int type, p2;
  int count=0;

  while ((offer = player_find_pendto( p, p1, offerType)) >= 0) {
    type = parray[p].p_to_list[offer].type;
    p2 = parray[p].p_to_list[offer].whoto;
    switch (type) {
      case PEND_MATCH:
        pprintf_prompt( p2, "\n%s withdraws the match offer.\n", parray[p].name );
      pprintf( p, "You withdraw the match offer to %s.\n", parray[p2].name );
      break;
      case PEND_DRAW:
        pprintf_prompt( p2, "\n%s withdraws draw request.\n", parray[p].name );
      pprintf( p, "You withdraw the draw request to %s.\n", parray[p2].name );
      break;
      case PEND_PAUSE:
        pprintf_prompt( p2, "\n%s withdraws pause request.\n", parray[p].name );
      pprintf( p, "You withdraw the pause request to %s.\n", parray[p2].name );
      break;
      case PEND_ABORT:
        pprintf_prompt( p2, "\n%s withdraws abort request.\n", parray[p].name );
      pprintf( p, "You withdraw the abort request to %s.\n", parray[p2].name );
      break;
      case PEND_TAKEBACK:
        pprintf_prompt( p2, "\n%s withdraws the takeback request.\n", parray[p].name );
      pprintf( p, "You withdraw the takeback request to %s.\n", parray[p2].name );
      break;
      case PEND_ADJOURN:
        pprintf_prompt( p2, "\n%s withdraws the adjourn request.\n", parray[p].name );
      pprintf( p, "You withdraw the adjourn request to %s.\n", parray[p2].name );
      break;
      case PEND_SWITCH:
        pprintf_prompt( p2, "\n%s withdraws the switch sides request.\n", parray[p].name );
      pprintf( p, "You withdraw the switch sides request to %s.\n", parray[p2].name );
      break;
      case PEND_SIMUL:
        pprintf_prompt( p2, "\n%s withdraws the simul offer.\n", parray[p].name );
      pprintf( p, "You withdraw the simul offer to %s.\n", parray[p2].name );
      break;
    }
    player_remove_request( p, p2, type);
    count++;
  }
  return count;
}


PUBLIC int player_is_observe( int p, int g )
{
  int i;

  for (i=0;i<parray[p].num_observe;i++) {
    if (parray[p].observe_list[i] == g) break;
  }
  if (i==parray[p].num_observe)
    return 0;
  else
    return 1;
}

PUBLIC int player_add_observe( int p, int g )
{
  if (parray[p].num_observe == MAX_OBSERVE) return -1;
  parray[p].observe_list[parray[p].num_observe] = g;
  parray[p].num_observe++;
  return 0;
}

PUBLIC int player_remove_observe( int p, int g )
{
  int i;

  for (i=0;i<parray[p].num_observe;i++) {
    if (parray[p].observe_list[i] == g) break;
  }
  if (i==parray[p].num_observe) return -1; /* Not found! */
  for (;i<parray[p].num_observe-1;i++) {
    parray[p].observe_list[i] = parray[p].observe_list[i+1];
  }
  parray[p].num_observe--;
  return 0;
}

PUBLIC int player_game_ended( int g )
{
  int p;

  for (p=0;p<p_num;p++) {
    if (parray[p].status == PLAYER_EMPTY) continue;
    player_remove_observe( p, g );
  }
  player_remove_request( garray[g].white, garray[g].black, -1);
  player_remove_request( garray[g].black, garray[g].white, -1);
  return 0;
}

PUBLIC int player_goto_board( int p, int board_num )
{
  int start, count=0, on, g;

  if (board_num < 0 || board_num >= parray[p].simul_info.numBoards) return -1;
  if (parray[p].simul_info.boards[board_num] < 0) return -1;
  parray[p].simul_info.onBoard = board_num;
  parray[p].game = parray[p].simul_info.boards[board_num];
  parray[p].opponent = garray[parray[p].game].black;
  if (parray[p].simul_info.numBoards == 1) return 0;
  send_board_to( parray[p].game, p );
  start = parray[p].game;
  on = parray[p].simul_info.onBoard;
  do {
    g = parray[p].simul_info.boards[on];
    if (g >= 0) {
      if (count == 0) {
        if (parray[garray[g].black].bell) {
          pprintf( garray[g].black, "\007" );
        }
        pprintf_prompt( garray[g].black, "\n%s is at your board!\n", parray[p].name );
      } else if (count == 1) {
        if (parray[garray[g].black].bell) {
          pprintf( garray[g].black, "\007" );
        }
        pprintf_prompt( garray[g].black, "\n%s will be at your board NEXT!\n", parray[p].name );
      } else {
        pprintf_prompt( garray[g].black, "\n%s is %d boards away.\n", parray[p].name, count );
      }
      count++;
    }
    on++;
    if (on >= parray[p].simul_info.numBoards) on = 0;
  } while (start != parray[p].simul_info.boards[on]);
  return 0;
}

PUBLIC int player_goto_next_board( int p )
{
  int on;
  int start;
  int g;

  on = parray[p].simul_info.onBoard;
  start = on;
  g = -1;
  do {
    on++;
    if (on >= parray[p].simul_info.numBoards) on = 0;
    g = parray[p].simul_info.boards[on];
    if (g >= 0) break;
  } while (start != on);
  if (g == -1) {
    pprintf_prompt( p, "\nMajor Problem! Can't find your next board.\n" );
    return -1;
  }
  return player_goto_board( p, on );
}

PUBLIC int player_num_active_boards( int p )
{
  int count=0, i;

  if (!parray[p].simul_info.numBoards) return 0;
  for (i=0;i<parray[p].simul_info.numBoards;i++)
    if (parray[p].simul_info.boards[i] >= 0) count++;
  return count;
}

PUBLIC int player_num_results( int p, int result )
{
  int count=0, i;

  if (!parray[p].simul_info.numBoards) return 0;
  for (i=0;i<parray[p].simul_info.numBoards;i++)
    if (parray[p].simul_info.results[i] == result) count++;
  return count;
}

PUBLIC int player_simul_over( int p, int g, int result )
{
  int on, ong, p1, which= -1, won;
  char tmp[1024];

  for (won=0; won<parray[p].simul_info.numBoards; won++) {
    if (parray[p].simul_info.boards[won] == g) {
      which = won;
      break;
    }
  }
  if (which == -1) {
    pprintf( p, "I can't find that game!\n" );
    return -1;
  }
  pprintf( p, "\nBoard %d has completed.\n", won+1 );
  on = parray[p].simul_info.onBoard;
  ong = parray[p].simul_info.boards[on];
  parray[p].simul_info.boards[won] = -1;
  parray[p].simul_info.results[won] = result;
  if (player_num_active_boards(p) == 0) {
    sprintf(tmp, "\n{Simul (%s vs. %d) is over.}\nResults: %d Wins, %d Losses, %d Draws, %d Aborts\n",
             parray[p].name,
             parray[p].simul_info.numBoards,
             player_num_results(p, RESULT_WIN),
             player_num_results(p, RESULT_LOSS),
             player_num_results(p, RESULT_DRAW),
             player_num_results(p, RESULT_ABORT) );
    for (p1=0; p1 < p_num; p1++) {
      if (parray[p].status != PLAYER_PROMPT) continue;
      if (!parray[p1].i_game && !player_is_observe(p1,g) && (p1 != p))
        continue;
      pprintf_prompt( p1, "%s", tmp );
    }
    parray[p].simul_info.numBoards = 0;
    pprintf_prompt( p, "\nThat was the last board, thanks for playing.\n" );
    return 0;
  }
  if (ong == g) { /* This game is over */
    player_goto_next_board( p );
  } else {
    player_goto_board( p, parray[p].simul_info.onBoard );
  }
  pprintf_prompt( p, "\nThere are %d boards left.\n",
                     player_num_active_boards(p) );
  return 0;
}

PUBLIC int player_num_messages( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) return 0;
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_MESSAGES );
  return lines_file(fname);
}

PUBLIC int player_add_message( int top, int fromp, char *message )
{
  char fname[MAX_FILENAME_SIZE];
  FILE *fp;
  int t=time(0);

  if (!parray[top].registered) return -1;
  if (!parray[fromp].registered) return -1;
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[top].login[0], parray[top].login, STATS_MESSAGES );
  if (lines_file(fname) >= MAX_MESSAGES) return -1;
  fp = fopen( fname, "a" );
  if (!fp) return -1;
  fprintf( fp, "%s at %s: %s\n", parray[fromp].name, strgtime(&t), message );
  fclose(fp);
  return 0;
}

PUBLIC int player_show_messages( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) return -1;
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_MESSAGES );
  if (lines_file(fname) <= 0) return -1;
  psend_file( p, NULL, fname );
  return 0;
}

PUBLIC int player_clear_messages( int p )
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) return -1;
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_MESSAGES );
  unlink(fname);
  return 0;
}
