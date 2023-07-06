/* gamedb.h
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

#ifndef _GAMEDB_H
#define _GAMEDB_H

#include "board.h"

#define GAME_EMPTY 0
#define GAME_NEW 1
#define GAME_ACTIVE 2
#define GAME_STORED 3

#define TYPE_UNTIMED 0
#define TYPE_BLITZ 1
#define TYPE_STAND 2
#define TYPE_NONSTANDARD 3

#define END_CHECKMATE 0
#define END_RESIGN 1
#define END_FLAG 2
#define END_AGREEDDRAW 3
#define END_REPITITION 4
#define END_50MOVERULE 5
#define END_ADJOURN 6
#define END_LOSTCONNECTION 7
#define END_ABORT 8
#define END_STALEMATE 9
#define END_NOTENDED 10
#define END_COURTESY 11
#define END_BOTHFLAG 12

typedef struct _game {
  /* Saved in the game file */
  int wInitTime, wIncrement;
  int bInitTime, bIncrement;
  unsigned timeOfStart; /* The absolute time the game started */
  int wTime;
  int bTime;
  int clockStopped;
  int rated;
  int private;
  int type;
  int passes; /* For simul's */
  int numHalfMoves;
  move_t *moveList;
  game_state_t game_state;

  /* Not saved in game file */
  int white;
  int black;
  int old_white; /* Contains the old game player number */
  int old_black; /* Contains the old game player number */
  int status;
  int moveListSize; /* Total allocated in *moveList */

  unsigned startTime;    /* The relative time the game started  */
  unsigned lastMoveTime; /* Last time a move was made */
  unsigned lastDecTime;  /* Last time a players clock was decremented */

  int result;
  int winner;

} game;

extern game *garray;
extern int g_num;

extern int game_new( );
extern int game_zero( );
extern int game_free( );
extern int game_clear( );
extern int game_remove( );
extern int game_finish();

extern char *game_time_str( );
extern char *game_str( );
extern int game_isblitz( );

extern void send_board_to( );
extern void send_boards( );
extern void game_update_time();
extern void game_update_times();

#define MAXOLDGAMES 50

extern int FindOldGameFor();
extern int RemoveOldGamesForPlayer();
extern int ReallyRemoveOldGamesForPlayer();
extern int NewOldGame();

extern char *movesToString();

extern void game_disconnect();

extern int game_read();
extern int game_delete();
extern int game_save();

extern int pgames();
extern void game_write_complete();
extern int game_count();

#endif
