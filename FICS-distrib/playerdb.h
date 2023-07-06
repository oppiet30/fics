/* playerdb.h
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

#include "command.h"
#include "timelog.h"

#ifndef _PLAYERDB_H
#define _PLAYERDB_H

#define MAX_PENDING 10
#define MAX_OBSERVE 50
#define MAX_PLAN 9
#define MAX_CENSOR 10
#define MAX_ALIASES 10
#define MAX_SIMUL 50
#define MAX_MESSAGES 20

#define PLAYER_EMPTY 0
#define PLAYER_NEW 1
#define PLAYER_INQUEUE 2
#define PLAYER_LOGIN 3
#define PLAYER_PASSWORD 4
#define PLAYER_PROMPT 5

#define P_LOGIN 0
#define P_LOGOUT 1

#define SORT_BLITZ 0
#define SORT_STAND 1
#define SORT_ALPHA 2

typedef struct _statistics {
  int num, win, los, dra, rating;
} statistics;

#define PEND_MATCH 0  /* Params 1=wt 2=winc 3=bt 4=binc 5=registered */
#define PEND_DRAW 1
#define PEND_ABORT 2
#define PEND_TAKEBACK 3
#define PEND_ADJOURN 4
#define PEND_SWITCH 5
#define PEND_SIMUL 6
#define PEND_PAUSE 7
extern char *pend_strings[7];

#define PEND_TO 0
#define PEND_FROM 1
typedef struct _pending {
  int type;
  int whoto;
  int whofrom;
  int param1, param2, param3, param4, param5;
  char char1[50], char2[50];
} pending;

typedef struct _simul_info_t {
  int numBoards;
  int onBoard;
  int results[MAX_SIMUL];
  int boards[MAX_SIMUL];
} simul_info_t;

typedef struct _player {
/* This first block is not saved between logins */
  char *login;
  int registered;
  int socket;
  int status;
  int game;
  int opponent; /* Only valid if game is >= 0 */
  int side;     /* Only valid if game is >= 0 */
  int last_tell;
  int last_channel;
  int last_opponent;
  int logon_time;
  int last_command_time;
  int num_from;
  pending p_from_list[MAX_PENDING];
  int num_to;
  pending p_to_list[MAX_PENDING];
  int num_observe;
  int observe_list[MAX_OBSERVE];
  int lastColor;
  unsigned int thisHost;
  timelog *query_log;
  int sopen;
  simul_info_t simul_info;

/* All of this is saved between logins */
  char *name;
  char *passwd;
  char *fullName;
  char *emailAddress;
  char *prompt;
  statistics s_stats;
  statistics b_stats;
  int open;
  int rated;
  int bell;
  int i_login;
  int i_game;
  int i_shout;
  int i_tell;
  int i_kibitz;
  int private;
  int automail;
  int style;
  int promote;
  int adminLevel;
  int num_plan;
  char *planLines[MAX_PLAN];
  int num_censor;
  char *censorList[MAX_CENSOR];
  int num_white;
  int num_black;
  int uscfRating;
  int muzzled;
  int network_player;
  unsigned int lastHost;
  int numAlias;
  alias_type alias_list[MAX_ALIASES];
  int highlight;
} player;

extern player *parray;
extern int p_num;

extern void player_init();

extern int player_new( );
extern int player_remove( );
extern int player_read( );
extern int player_delete();
extern int player_markdeleted();
extern int player_save( );
extern int player_find( );
extern int player_find_bylogin( );
extern int player_find_part_login( );
extern int player_censored( );
extern int player_count();
extern int player_idle();
extern int player_ontime();

extern int player_zero( );
extern int player_free( );
extern int player_clear( );

extern void player_write_login( );
extern void player_write_logout( );
extern int player_lastdisconnect();

extern int player_cmp();
extern void player_resort();

extern void player_pend_print();

extern int player_find_pendto();
extern int player_new_pendto();
extern int player_remove_pendto();

extern int player_find_pendfrom();
extern int player_new_pendfrom();
extern int player_remove_pendfrom();
extern int player_add_request();
extern int player_remove_request();
extern int player_decline_offers();
extern int player_withdraw_offers();

extern int player_is_observe();
extern int player_add_observe();
extern int player_remove_observe();
extern int player_game_ended();

extern int player_goto_board();
extern int player_goto_next_board();
extern int player_num_active_boards();
extern int player_num_results();
extern int player_simul_over();

extern int player_num_messages();
extern int player_add_message();
extern int player_show_messages();
extern int player_clear_messages();

extern int sort_blitz[];
extern int sort_stand[];
extern int sort_alpha[];

#endif /* _PLAYERDB_H */
