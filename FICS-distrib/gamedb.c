/* gamedb.c
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
#include "gamedb.h"
#include "playerdb.h"
#include "gameproc.h"
#include "command.h"
#include "utils.h"
#include "rmalloc.h"
#include "tally.h"

PUBLIC game *garray=NULL;
PUBLIC int g_num = 0;

PRIVATE int get_empty_slot()
{
  int i;

  for (i=0; i < g_num; i++) {
    if (garray[i].status == GAME_EMPTY) return i;
  }
  g_num++;
  if (!garray) {
    garray = (game *)rmalloc( sizeof(game) * g_num );
  } else {
    garray = (game *)rrealloc( garray, sizeof(game) * g_num );
  }
  garray[g_num-1].status = GAME_EMPTY;
  return g_num-1;
}

PUBLIC int game_new( )
{
  int new = get_empty_slot();
  game_zero( new );
  return new;
}

PUBLIC int game_zero( int g )
{
  garray[g].white = -1;
  garray[g].black = -1;
  garray[g].old_white = -1;
  garray[g].old_black = -1;
  garray[g].status = GAME_NEW;
  garray[g].rated = 0;
  garray[g].private = 0;
  garray[g].result = END_NOTENDED;
  garray[g].type = TYPE_UNTIMED;
  garray[g].passes = 0;
  board_init( &garray[g].game_state, NULL, NULL );
  garray[g].game_state.gameNum = g;
  garray[g].numHalfMoves = 0;
  garray[g].moveListSize = 0;
  garray[g].moveList = NULL;
  garray[g].wInitTime = 300; /* 5 minutes */
  garray[g].wIncrement = 0;
  garray[g].bInitTime = 300; /* 5 minutes */
  garray[g].bIncrement = 0;
  return 0;
}

PUBLIC int game_free( int g )
{
  if (garray[g].moveListSize) rfree(garray[g].moveList);
  garray[g].moveListSize = 0;
  return 0;
}

PUBLIC int game_clear( int g )
{
  game_free( g );
  game_zero( g );
  return 0;
}

PUBLIC int game_remove( int g )
{
  /* Should remove game from players observation list */
  game_clear( g );
  garray[g].status = GAME_EMPTY;
  return 0;
}

/* This may eventually save old games for the 'oldmoves' command */
PUBLIC int game_finish( int g )
{
  player_game_ended(g); /* Alert playerdb that game ended */
  NewOldGame(g);
  return 0;
}

PUBLIC char *game_time_str( int wt, int winc, int bt, int binc )
{
  static char tstr[50];

  if (!wt) { /* Untimed */
    strcpy( tstr, "" );
    return tstr;
  }
  if (!bt) {
    if (!winc)
      sprintf( tstr, " %d", wt );
    else
      sprintf( tstr, " %d %d", wt, winc );
  } else {
    if (!winc && !binc)
      sprintf( tstr, " %d : %d", wt, bt );
    else if (!winc && binc)
      sprintf( tstr, " %d : %d %d", wt, bt, binc );
    else if (winc && !binc )
      sprintf( tstr, " %d %d : %d", wt, winc, bt );
    else
      sprintf( tstr, " %d %d : %d %d", wt, winc, bt, binc );
  }
  return tstr;
}

PUBLIC char *game_str( int rated, int wt, int winc, int bt, int binc,
                       char *cat, char *board )
{
  static char tstr[200];
  static char *bstr[]={"untimed", "blitz", "standard", "non-standard"};
  static char *rstr[]={"Unrated", "Rated"};

  if (cat && cat[0] && board && board[0] &&
      (strcmp(cat,"standard") || strcmp(board, "standard"))) {
    sprintf( tstr, "%s %s match%s Loaded from %s/%s",
                      rstr[rated],
                      bstr[game_isblitz(wt/60,winc,bt/60,binc,cat,board)],
                      game_time_str(wt/60,winc,bt/60,binc),
                      cat, board);
  } else {
    sprintf( tstr, "%s %s match%s",
                      rstr[rated],
                      bstr[game_isblitz(wt/60,winc,bt/60,binc,cat,board)],
                      game_time_str(wt/60,winc,bt/60,binc));
  }
  return tstr;
}

PUBLIC int game_isblitz( int wt, int winc, int bt, int binc,
                         char *cat, char *board)
{
  int totalw, totalb, total;

  if (cat && cat[0] && board && board[0] &&
      (strcmp(cat,"standard") || strcmp(board, "standard")))
    return TYPE_NONSTANDARD;
  if (wt == 0) return TYPE_UNTIMED;
  if (bt && ((wt != bt) || (winc != binc)))
    return TYPE_NONSTANDARD;
  totalw = wt*60 + winc * 40;
  totalb = bt*60 + binc * 40;
  if (totalw > totalb)
    total = totalw;
  else
    total = totalb;
  if (total < 180) /* 3 minutes */
    return TYPE_NONSTANDARD;
  if (total >= 900) /* 15 minutes */
    return TYPE_STAND;
  else
    return TYPE_BLITZ;
}

PUBLIC void send_board_to( int g, int p )
{
  char *b;
  int side;
  int observing=0;

  if (parray[p].game == g) {
    side = parray[p].side;
  } else {
    observing = 1;
    side = WHITE;
  }
  game_update_time(g);
  b = board_to_string( parray[garray[g].white].name,
                       parray[garray[g].black].name,
                       garray[g].wTime,
                       garray[g].bTime,
                       &garray[g].game_state, garray[g].moveList,
                       parray[p].style,
                       side, observing, p );
  in_push(IN_BOARD);
  if (parray[p].bell) {
    pprintf_noformat( p, "\007%s", b );
  } else {
    pprintf_noformat( p, "%s", b );
  }
  if (p != commanding_player) {
    pprintf( p, "%s", parray[p].prompt );
  }
  in_pop();
}

PUBLIC void send_boards( int g )
{
  int p;

  if (!parray[garray[g].white].simul_info.numBoards ||
      parray[garray[g].white].simul_info.boards[
         parray[garray[g].white].simul_info.onBoard] == g)
    send_board_to( g, garray[g].white );
  send_board_to( g, garray[g].black );
  for (p=0; p<p_num; p++) {
    if (parray[p].status == PLAYER_EMPTY) continue;
    if (player_is_observe(p,g)) {
      send_board_to( g, p );
    }
  }
}

PUBLIC void game_update_time( int g )
{
  unsigned now, timesince;

  if (garray[g].clockStopped) return;
  if (garray[g].type == TYPE_UNTIMED) return;
  now = tenth_secs();
  timesince = now - garray[g].lastDecTime;
  if (garray[g].game_state.onMove == WHITE) {
    if (garray[g].wTime < timesince)
      garray[g].wTime = 0;
    else
      garray[g].wTime = garray[g].wTime - timesince;
  } else {
    if (garray[g].bTime < timesince)
      garray[g].bTime = 0;
    else
      garray[g].bTime = garray[g].bTime - timesince;
  }
  garray[g].lastDecTime = now;
}

PUBLIC void game_update_times()
{
  int g;

  for (g=0; g < g_num; g++) {
    if (garray[g].status != GAME_ACTIVE) continue;
    if (garray[g].clockStopped) continue;
    game_update_time(g);
  }
}


PRIVATE int oldGameArray[MAXOLDGAMES];
PRIVATE int numOldGames=0;

PRIVATE int RemoveOldGame( int g )
{
  int i;

  for (i=0; i < numOldGames; i++) {
    if (oldGameArray[i] == g) break;
  }
  if (i == numOldGames) return -1; /* Not found! */
  for (;i<numOldGames-1;i++)
    oldGameArray[i] = oldGameArray[i+1];
  numOldGames--;
  game_remove(g);
  return 0;
}

PRIVATE int AddOldGame(int g)
{
  if (numOldGames == MAXOLDGAMES) /* Remove the oldest */
    RemoveOldGame(oldGameArray[0]);
  oldGameArray[numOldGames] = g;
  numOldGames++;
  return 0;
}

PUBLIC int FindOldGameFor( int p )
{
  int i;

  if (p == -1) return numOldGames-1;
  for (i=numOldGames-1; i >= 0; i--) {
    if (garray[oldGameArray[i]].old_white == p) return oldGameArray[i];
    if (garray[oldGameArray[i]].old_black == p) return oldGameArray[i];
  }
  return -1;
}

/* This just removes the game if both players have new-old games */
PUBLIC int RemoveOldGamesForPlayer( int p )
{
  int g;

  g = FindOldGameFor(p);
  if (g < 0) return 0;
  if (garray[g].old_white == p) garray[g].old_white = -1;
  if (garray[g].old_black == p) garray[g].old_black = -1;
  if ((garray[g].old_white == -1) && (garray[g].old_black == -1)) {
    RemoveOldGame( g );
  }
  return 0;
}

/* This recycles any old games for players who disconnect */
PUBLIC int ReallyRemoveOldGamesForPlayer( int p )
{
  int g;

  g = FindOldGameFor(p);
  if (g < 0) return 0;
  RemoveOldGame( g );
  return 0;
}

PUBLIC int NewOldGame( int g )
{
  RemoveOldGamesForPlayer( garray[g].white );
  RemoveOldGamesForPlayer( garray[g].black );
  garray[g].old_white = garray[g].white;
  garray[g].old_black = garray[g].black;
  garray[g].status = GAME_STORED;
  AddOldGame( g );
  return 0;
}

/* This should be enough to hold any game up to at least 250 moves
 * If we overwrite this, the server will crash.
 */
#define GAME_STRING_LEN 16000
PRIVATE char gameString[GAME_STRING_LEN];
PUBLIC char *movesToString( int g )
{
  char tmp[160];
  int wp, bp;
  int wr, br;
  int i;
  wp = garray[g].white;
  bp = garray[g].black;
  if (garray[g].type == TYPE_BLITZ) {
    wr = parray[wp].b_stats.rating;
    br = parray[bp].b_stats.rating;
  } else {
    wr = parray[wp].s_stats.rating;
    br = parray[bp].s_stats.rating;
  }
  sprintf( gameString, "\n%s ", parray[wp].name );
  if (wr > 0) {
    sprintf( tmp, "(%d) ", wr );
  } else {
    sprintf( tmp, "(UNR) " );
  }
  strcat( gameString, tmp );
  sprintf( tmp, "vs. %s ", parray[bp].name );
  strcat( gameString, tmp );
  if (br > 0) {
    sprintf( tmp, "(%d) ", br );
  } else {
    sprintf( tmp, "(UNR) " );
  }
  strcat( gameString, tmp );
  strcat( gameString, "--- " );
  strcat( gameString, strgtime( &garray[g].timeOfStart ) );
  if (garray[g].rated) {
    strcat( gameString, "\nRated " );
  } else {
    strcat( gameString, "\nUnrated " );
  }
  if (garray[g].type == TYPE_BLITZ) {
    strcat( gameString, "Blitz " );
  } else if (garray[g].type == TYPE_STAND) {
    strcat( gameString, "Standard " );
  } else {
    strcat( gameString, "Untimed " );
  }
  strcat( gameString, "match, initial time: " );
  if (garray[g].bInitTime) {
    sprintf( tmp, "%d minutes, increment: %d seconds AND %d minutes, increment: %d seconds.\n\n", garray[g].wInitTime/600, garray[g].wIncrement/10, garray[g].bInitTime/600, garray[g].bIncrement/10 );
  } else {
    sprintf( tmp, "%d minutes, increment: %d seconds.\n\n", garray[g].wInitTime/600, garray[g].wIncrement/10 );
  }
  strcat( gameString, tmp );
  sprintf( tmp, "Move  %-19s%-19s\n", parray[wp].name, parray[bp].name );
  strcat( gameString, tmp );
  strcat (gameString, "----  ----------------   ----------------\n" );
  for (i=0;i<garray[g].numHalfMoves;i+=2) {
    if (i+1 < garray[g].numHalfMoves) {
      sprintf( tmp, "%4d  %-16s   ", i/2+1,
             move_and_time(&garray[g].moveList[i]) );
      strcat( gameString, tmp );
      sprintf( tmp, "%-16s\n", move_and_time(&garray[g].moveList[i+1]) );
    } else {
      sprintf( tmp, "%4d  %-16s\n", i/2+1,
              move_and_time(&garray[g].moveList[i]) );
    }
    strcat( gameString, tmp );
    if (strlen( gameString ) > GAME_STRING_LEN-100) { /* Bug out if getting close to filling this string */
      return gameString;
    }
  }
  switch (garray[g].result) {
    case END_CHECKMATE:
      strcat( gameString, "      Checkmate.\n" );
      break;
    case END_RESIGN:
      sprintf( tmp, "      %s resigned.\n",
              garray[g].winner == WHITE ? "Black" : "White" );
      strcat( gameString, tmp );
      break;
    case END_FLAG:
      sprintf( tmp, "      %s forfeits on time.\n",
              garray[g].winner == WHITE ? "Black" : "White" );
      strcat( gameString, tmp );
      break;
    case END_AGREEDDRAW:
      strcat( gameString, "      Game drawn by mutual agreement.\n" );
      break;
    case END_BOTHFLAG:
      strcat( gameString, "      Both players ran out of time.\n" );
      break;
    case END_REPITITION:
      strcat( gameString, "      Draw by move repetition.\n" );
      break;
    case END_50MOVERULE:
      strcat( gameString, "      Draw by the 50 move rule.\n" );
      break;
    case END_ADJOURN:
      strcat( gameString, "      The game was adjourned and saved.\n" );
      break;
    case END_LOSTCONNECTION:
      sprintf( tmp, "      %s lost connection, game adjourned.\n",
              garray[g].winner == WHITE ? "White" : "Black" );
      strcat( gameString, tmp );
      break;
    case END_ABORT:
      strcat( gameString, "      The game was aborted.\n" );
      break;
    case END_STALEMATE:
      strcat( gameString, "      Stalemate.\n" );
      break;
    case END_NOTENDED:
      strcat( gameString, "      {Still in progress} *\n" );
      break;
    case END_COURTESY:
      sprintf( tmp, "      %s courtesy abort.\n",
              garray[g].winner == WHITE ? "White" : "Black" );
      strcat( gameString, tmp );
      break;
    default:
      strcat( gameString, "      ???????\n" );
      break;
  }
  return gameString;
}

PUBLIC void game_disconnect( int g, int p )
{
  game_ended( g, (garray[g].white == p) ? WHITE : BLACK, END_LOSTCONNECTION );
}

/* One line has everything on it */
PRIVATE void WriteMoves( FILE *fp, move_t *m )
{
  fprintf( fp, "%d %d %d %d %d %d %d %d %d \"%s\" \"%s\" %u %u\n",
           m->color, m->fromFile, m->fromRank, m->toFile, m->toRank,
           m->pieceCaptured, m->piecePromotionTo, m->enPassant, m->doublePawn,
           m->moveString, m->algString, m->atTime, m->tookTime );
}

PRIVATE int ReadMoves( FILE *fp, move_t *m )
{
  if (fscanf( fp, "%d %d %d %d %d %d %d %d %d \"%[^\"]\" \"%[^\"]\" %u %u\n",
           &m->color, &m->fromFile, &m->fromRank, &m->toFile, &m->toRank,
      &m->pieceCaptured, &m->piecePromotionTo, &m->enPassant, &m->doublePawn,
           m->moveString, m->algString, &m->atTime, &m->tookTime ) != 13)
    return -1;
  return 0;
}

PRIVATE void WriteGameState( FILE *fp, game_state_t *gs )
{
  int i, j;

  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      fprintf( fp, "%d ", gs->board[i][j] );
  fprintf( fp, "%d %d %d %d %d %d",
     gs->wkmoved, gs->wqrmoved, gs->wkrmoved,
     gs->bkmoved, gs->bqrmoved, gs->bkrmoved );
  for (i=0;i<8;i++)
    fprintf( fp, " %d %d", gs->ep_possible[0][i], gs->ep_possible[1][i] );
  fprintf( fp, " %d %d %d\n", gs->lastIrreversable, gs->onMove, gs->moveNum );
}

PRIVATE int ReadGameState( FILE *fp, game_state_t *gs )
{
  int i, j;
  int wkmoved, wqrmoved, wkrmoved, bkmoved, bqrmoved,  bkrmoved;

  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      if (fscanf( fp, "%d", &gs->board[i][j]) != 1) return -1;
  if (fscanf( fp, "%d %d %d %d %d %d",
     &wkmoved, &wqrmoved, &wkrmoved,
     &bkmoved, &bqrmoved, &bkrmoved ) != 6) return -1;
  gs->wkmoved = wkmoved;
  gs->wqrmoved = wqrmoved;
  gs->wkrmoved = wkrmoved;
  gs->bkmoved = bkmoved;
  gs->bqrmoved = bqrmoved;
  gs->bkrmoved = bkrmoved;
  for (i=0;i<8;i++)
    if (fscanf( fp, " %d %d", &gs->ep_possible[0][i], &gs->ep_possible[1][i] ) != 2) return -1;
  if (fscanf( fp, " %d %d %d\n", &gs->lastIrreversable, &gs->onMove, &gs->moveNum ) != 3) return -1;
  return 0;
}

PRIVATE int got_attr_value( int g, char *attr, char *value, FILE *fp, char *file )
{
  int i;

  if (!strcmp( attr, "w_init:" )) {
    garray[g].wInitTime = atoi(value);
  } else if (!strcmp( attr, "w_inc:" )) {
    garray[g].wIncrement = atoi(value);
  } else if (!strcmp( attr, "b_init:" )) {
    garray[g].bInitTime = atoi(value);
  } else if (!strcmp( attr, "b_inc:" )) {
    garray[g].bIncrement = atoi(value);
  } else if (!strcmp( attr, "timestart:" )) {
    garray[g].timeOfStart = atoi(value);
  } else if (!strcmp( attr, "w_time:" )) {
    garray[g].wTime = atoi(value);
  } else if (!strcmp( attr, "b_time:" )) {
    garray[g].bTime = atoi(value);
  } else if (!strcmp( attr, "clockstopped:" )) {
    garray[g].clockStopped = atoi(value);
  } else if (!strcmp( attr, "rated:" )) {
    garray[g].rated = atoi(value);
  } else if (!strcmp( attr, "private:" )) {
    garray[g].private = atoi(value);
  } else if (!strcmp( attr, "type:" )) {
    garray[g].type = atoi(value);
  } else if (!strcmp( attr, "halfmoves:" )) {
    garray[g].numHalfMoves = atoi(value);
    if (garray[g].numHalfMoves == 0) return 0;
    garray[g].moveListSize = garray[g].numHalfMoves;
    garray[g].moveList = (move_t *)rmalloc( sizeof(move_t) * garray[g].moveListSize );
    for (i=0;i<garray[g].numHalfMoves;i++) {
      if (ReadMoves( fp, &garray[g].moveList[i] )) {
        fprintf( stderr, "FICS: Trouble reading moves from %s.\n", file );
        return -1;
      }
    }
  } else if (!strcmp( attr, "gamestate:" )) { /* Value meaningless */
    if (ReadGameState( fp, &garray[g].game_state)) {
      fprintf( stderr, "FICS: Trouble reading game state from %s.\n", file );
      return -1;
    }
  } else {
    fprintf( stderr, "FICS: Error bad attribute >%s< from file %s\n", attr, file );
  }
  return 0;
}


#define MAX_GLINE_SIZE 1024
PUBLIC int game_read( int g, int wp, int bp )
{
  FILE *fp;
  int len;
  char * attr, *value;
  char fname[MAX_FILENAME_SIZE];
  char line[MAX_GLINE_SIZE];

  garray[g].white = wp;
  garray[g].black = bp;
  garray[g].old_white = -1;
  garray[g].old_black = -1;
  garray[g].moveListSize = 0;
  garray[g].game_state.gameNum = g;

  sprintf( fname, "%s/%c/%s-%s", game_dir, parray[wp].login[0],
                                parray[wp].login, parray[bp].login);
  fp = fopen( fname, "r" );
  if (!fp) {
    return -1;
  }

  /* Read the game file here */
  while (!feof(fp)) {
    fgets(line, MAX_GLINE_SIZE, fp);
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
    if (got_attr_value( g, attr, value, fp, fname )) return -1;
  }

  fclose(fp);
  garray[g].status = GAME_ACTIVE;
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;
  /* Need to do notification and pending cleanup */
  return 0;
}

PUBLIC int game_delete( int wp, int bp )
{
  char fname[MAX_FILENAME_SIZE];
  char lname[MAX_FILENAME_SIZE];

  sprintf( fname, "%s/%c/%s-%s", game_dir, parray[wp].login[0],
                                parray[wp].login, parray[bp].login);
  sprintf( lname, "%s/%c/%s-%s", game_dir, parray[bp].login[0],
                                parray[wp].login, parray[bp].login);
  unlink(fname);
  unlink(lname);
  return 0;
}

PUBLIC int game_save( int g )
{
  FILE *fp;
  int wp, bp;
  int i;
  char fname[MAX_FILENAME_SIZE];
  char lname[MAX_FILENAME_SIZE];

  wp = garray[g].white;
  bp = garray[g].black;
  sprintf( fname, "%s/%c/%s-%s", game_dir, parray[wp].login[0],
                                parray[wp].login, parray[bp].login);
  sprintf( lname, "%s/%c/%s-%s", game_dir, parray[bp].login[0],
                                parray[wp].login, parray[bp].login);
  fp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Problem openning file %s for write\n", fname );
    return -1;
  }
  fprintf( fp, "W_Init: %d\n", garray[g].wInitTime );
  fprintf( fp, "W_Inc: %d\n", garray[g].wIncrement );
  fprintf( fp, "B_Init: %d\n", garray[g].bInitTime );
  fprintf( fp, "B_Inc: %d\n", garray[g].bIncrement );
  fprintf( fp, "TimeStart: %d\n", garray[g].timeOfStart );
  fprintf( fp, "W_Time: %d\n", garray[g].wTime );
  fprintf( fp, "B_Time: %d\n", garray[g].bTime );
  fprintf( fp, "ClockStopped: %d\n", garray[g].clockStopped );
  fprintf( fp, "Rated: %d\n", garray[g].rated );
  fprintf( fp, "Private: %d\n", garray[g].private );
  fprintf( fp, "Type: %d\n", garray[g].type );
  fprintf( fp, "HalfMoves: %d\n", garray[g].numHalfMoves );
  for (i=0;i<garray[g].numHalfMoves;i++) {
    WriteMoves( fp, &garray[g].moveList[i] );
  }
  fprintf( fp, "GameState: IsNext\n" );
  WriteGameState( fp, &garray[g].game_state );
  fclose( fp );
  /* Create link for easier stored game finding */
  if (parray[bp].login[0] != parray[wp].login[0])
    link( fname, lname );
  return 0;
}

PRIVATE void write_g_out( int g, char *file, int maxlines )
{
  FILE *fp;
  int wp, bp;
  int wr, br;
  char *type;

  wp = garray[g].white;
  bp = garray[g].black;
  if (garray[g].type == TYPE_BLITZ) {
    wr = parray[wp].b_stats.rating;
    br = parray[bp].b_stats.rating;
    type = "b";
  } else {
    wr = parray[wp].s_stats.rating;
    br = parray[bp].s_stats.rating;
    type = "s";
  }
  fp = fopen( file, "a" );
  if (!fp) return;
/* 1433 red        1311 Clueless   red       [br  2  12] Fri Feb 11 94 11:40 */
  fprintf( fp, "%s %d %s %d %s %s %d %d %d %d %d\n",
                parray[wp].name, wr,
                parray[bp].name, br,
                (garray[g].winner==WHITE) ? parray[wp].name : parray[bp].name,
                type,
                garray[g].wInitTime, garray[g].wIncrement,
                garray[g].bInitTime, garray[g].bIncrement,
                (int)time(0) );
  fclose(fp);
  truncate_file( file, maxlines );
}


PUBLIC int pgames( p, fname )
  int p;
  char *fname;
{
  FILE *fp;
  int wr, br, wt, wi, bt, bi, t;
  char wName[MAX_LOGIN_NAME+1];
  char bName[MAX_LOGIN_NAME+1];
  char winName[MAX_LOGIN_NAME+1];
  char type[512];

  fp = fopen( fname, "r" );
  if (!fp) {
      pprintf( p, "Sorry, no game information available.\n" );
      return COM_OK;
  }
  pprintf( p,"     White          Black     Winner    Type        Date\n" );
  while (!feof(fp)) {
    if (fscanf( fp, "%s %d %s %d %s %s %d %d %d %d %d\n",
                  wName, &wr,
                  bName, &br,
                  winName,
                  type,
                  &wt, &wi,
                  &bt, &bi,
                  &t ) != 11) {
      fprintf( stderr, "FICS: Error in games info format. %s\n", fname );
      fclose(fp);
      return COM_OK;
    }
    pprintf( p, "%4d %-9s %4d %-9s %-9s [%sr%3d%4d] %s",
                 wr, wName, br, bName, winName,
                 type, wt/600, wi/10, ctime(&t) );
  }
  fclose(fp);
  return COM_OK;
}

PUBLIC void game_write_complete( int g )
{
  char fname[MAX_FILENAME_SIZE];

  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[garray[g].white].login[0], parray[garray[g].white].login, STATS_GAMES );
  write_g_out( g, fname, 10 );
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[garray[g].black].login[0], parray[garray[g].black].login, STATS_GAMES );
  write_g_out( g, fname, 10 );
  sprintf( fname, "%s/%s", stats_dir, STATS_GAMES );
  write_g_out( g, fname, 10 );
}

PUBLIC int game_count()
{
  int g, count=0;

  for (g=0;g<g_num;g++)
    if (garray[g].status == GAME_ACTIVE) count++;
  return count;
}
