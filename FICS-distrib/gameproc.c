/* gameproc.c
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
#include "ficsmain.h"
#include "fics_config.h"
#include "playerdb.h"
#include "gamedb.h"
#include "gameproc.h"
#include "movecheck.h"
#include "utils.h"
#include "ratings.h"
#include "rmalloc.h"
#include "comproc.h"
#include "hostinfo.h"

PUBLIC void game_ended( int g, int winner, int why )
{
  char outstr[512];
  char tmp[512];
  int p;
  int rate_change=0;
  int isDraw=0;
  int whiteResult;

  sprintf( outstr, "\n{Game %d (%s vs. %s) ", g+1,
                                              parray[garray[g].white].name,
                                              parray[garray[g].black].name );
  garray[g].result = why;
  garray[g].winner = winner;
  if (winner == WHITE)
    whiteResult = RESULT_WIN;
  else
    whiteResult = RESULT_LOSS;
  switch (why) {
    case END_CHECKMATE:
      sprintf( tmp, "%s checkmated.}\n",
        (winner == WHITE) ? parray[garray[g].black].name :
                            parray[garray[g].white].name );
      rate_change = 1;
      break;
    case END_RESIGN:
      sprintf( tmp, "%s resigns.}\n",
        (winner == WHITE) ? parray[garray[g].black].name :
                            parray[garray[g].white].name );
      rate_change = 1;
      break;
    case END_FLAG:
      sprintf( tmp, "%s forfeits on time.}\n",
        (winner == WHITE) ? parray[garray[g].black].name :
                            parray[garray[g].white].name );
      rate_change = 1;
      break;
    case END_STALEMATE:
      sprintf( tmp, "Stalemate.}\n" );
      rate_change = 1;
      whiteResult = RESULT_DRAW;
      break;
    case END_AGREEDDRAW:
      sprintf( tmp, "Game drawn by mutual agreement.}\n" );
      isDraw = 1;
      rate_change = 1;
      whiteResult = RESULT_DRAW;
      break;
    case END_BOTHFLAG:
      sprintf( tmp, "Both players ran out of time.}\n" );
      isDraw = 1;
      rate_change = 1;
      whiteResult = RESULT_DRAW;
      break;
    case END_REPITITION:
      sprintf( tmp, "Was drawn by position repitition.}\n" );
      isDraw = 1;
      rate_change = 1;
      whiteResult = RESULT_DRAW;
      break;
    case END_50MOVERULE:
      sprintf( tmp, "Was drawn by the 50 move rule.}\n" );
      isDraw = 1;
      rate_change = 1;
      whiteResult = RESULT_DRAW;
      break;
    case END_ADJOURN:
      sprintf( tmp, "Was adjourned.}\n" );
      game_save(g);
      break;
    case END_LOSTCONNECTION:
      sprintf( tmp, "%s lost connection. ",
        (winner == WHITE) ? parray[garray[g].white].name :
                            parray[garray[g].black].name );
      if ( !game_save(g) )
          strcat( tmp, "Game adjourned.}\n");
      else
          strcat( tmp, "Game aborted.}\n");
      whiteResult = RESULT_ABORT;
      break;
    case END_ABORT:
      sprintf( tmp, "Was aborted by agreement.}\n" );
      whiteResult = RESULT_ABORT;
      break;
    case END_COURTESY:
      sprintf( tmp, "Was courtesy-aborted by %s.}\n",
              (winner == WHITE) ? parray[garray[g].white].name :
                                  parray[garray[g].black].name  );
      whiteResult = RESULT_ABORT;
      break;
    default:
      sprintf( tmp, "Hmm, the game ended and I don't know why.}\n" );
      break;
  }
  strcat( outstr, tmp );
  pprintf( garray[g].white, "%s", outstr );
  pprintf( garray[g].black, "%s", outstr );
  for (p=0; p < p_num; p++) {
    if ((p == garray[g].white) || (p == garray[g].black)) continue;
    if (parray[p].status != PLAYER_PROMPT) continue;
    if (!parray[p].i_game && !player_is_observe(p,g)) continue;
    pprintf_prompt( p, "%s", outstr );
  }
  if (garray[g].rated && rate_change) {
    /* Adjust ratings */
    rating_update( g );
    game_write_complete( g );
    if (parray[garray[g].white].network_player &&
        parray[garray[g].black].network_player ) {
      if (MailGameResult) { /* Send to ratings server */
        if (isDraw) {
          hostinfo_mailresults( garray[g].type == TYPE_BLITZ ? "blitz":"stand",
                              parray[garray[g].white].name,
                              parray[garray[g].black].name,
                              "draw" );
        } else {
          hostinfo_mailresults( garray[g].type == TYPE_BLITZ ? "blitz":"stand",
                              parray[garray[g].white].name,
                              parray[garray[g].black].name,
                              (winner == WHITE) ? parray[garray[g].white].name :
                                    parray[garray[g].black].name );
        }
      }
    }
  } else {
    pprintf( garray[g].white, "No ratings adjustment done.\n" );
    pprintf( garray[g].black, "No ratings adjustment done.\n" );
  }
  /* Mail off the moves */
  if (parray[garray[g].white].automail) {
     pcommand( garray[g].white, "mailmoves" );
  }
  if (parray[garray[g].black].automail) {
     pcommand( garray[g].black, "mailmoves" );
  }
  if (garray[g].white != commanding_player)
    pprintf_prompt( garray[g].white, "" );
  if (garray[g].black != commanding_player)
    pprintf_prompt( garray[g].black, "" );
  parray[garray[g].white].num_white++;
  parray[garray[g].white].lastColor = WHITE;
  parray[garray[g].black].num_black++;
  parray[garray[g].black].lastColor = BLACK;
  parray[garray[g].white].game = -1;
  parray[garray[g].black].game = -1;
  parray[garray[g].white].opponent = -1;
  parray[garray[g].white].last_opponent = garray[g].black;
  parray[garray[g].black].opponent = -1;
  parray[garray[g].black].last_opponent = garray[g].white;
  if (parray[garray[g].white].simul_info.numBoards)
    player_simul_over( garray[g].white, g, whiteResult );
  game_finish(g);
}

PUBLIC void process_move( int p, char *command )
{
  int g;
  move_t move;
  int result;
  unsigned now;
  int inc;

  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return;
  }
  player_decline_offers( p, -1, -1 );
  g = parray[p].game;
  if (parray[p].side != garray[g].game_state.onMove) {
    pprintf( p, "It is not your move.\n" );
    return;
  }
  if (garray[g].clockStopped) {
    pprintf( p, "Game clock is paused, use \"unpause\" to resume.\n" );
    return;
  }
  switch (parse_move( command, &garray[g].game_state, &move, parray[p].promote)) {
    case MOVE_ILLEGAL:
      pprintf( p, "Illegal move.\n" );
      return;
      break;
    case MOVE_AMBIGUOUS:
      pprintf( p, "Ambiguous move.\n" );
      return;
      break;
    default:
      break;
  }
  /* Do the move */
  game_update_time(g);
  if ((garray[g].game_state.onMove == BLACK) && (garray[g].bIncrement))
    inc = garray[g].bIncrement;
  else
    inc = garray[g].wIncrement;
  if (inc) { /* Do Fischer clock (if time == 0 your flag is down, no more time */
    if ((garray[g].game_state.onMove == BLACK) && (garray[g].bTime > 0))
      garray[g].bTime += inc;
    if ((garray[g].game_state.onMove == WHITE) && (garray[g].wTime > 0))
      garray[g].wTime += inc;
  }
  garray[g].numHalfMoves++;
  if (garray[g].numHalfMoves > garray[g].moveListSize) {
    garray[g].moveListSize += 20; /* Allocate 20 moves at a time */
    if (!garray[g].moveList) {
      garray[g].moveList = (move_t *)rmalloc( sizeof(move_t) * garray[g].moveListSize );
    } else {
      garray[g].moveList = (move_t *)rrealloc( garray[g].moveList, sizeof(move_t) * garray[g].moveListSize );
    }
  }
  result = execute_move( &garray[g].game_state, &move, 1 );
  now = tenth_secs();
  move.atTime = now;
  if (garray[g].numHalfMoves > 1) {
    move.tookTime = move.atTime - garray[g].lastMoveTime;
  } else {
    move.tookTime = move.atTime - garray[g].startTime;
  }
  garray[g].lastMoveTime = now;
  garray[g].lastDecTime = now;
  garray[g].moveList[garray[g].numHalfMoves-1] = move;
  send_boards(g);
  if (result == MOVE_ILLEGAL) {
    pprintf( p, "Internal error, illegal move accepted!\n" );
  }
  if (result == MOVE_CHECKMATE) {
    /* Do checkmate */
    game_ended( g, CToggle(garray[g].game_state.onMove), END_CHECKMATE );
  }
  if (result == MOVE_STALEMATE) {
    /* Do checkmate */
    game_ended( g, CToggle(garray[g].game_state.onMove), END_STALEMATE );
  }
}

PUBLIC int com_resign( int p, param_list param )
{
  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  player_decline_offers( p, -1, -1 );
  game_ended( parray[p].game, (garray[parray[p].game].white == p) ? BLACK : WHITE, END_RESIGN );
  return COM_OK;
}

PUBLIC int com_draw( int p, param_list param )
{
  int p1;
  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  p1 = parray[p].opponent;
  if (parray[p1].simul_info.numBoards &&
      parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] !=
      parray[p].game) {
    pprintf( p, "You can only make requests when the simul player is at your board.\n" );
    return COM_OK;
  }
  if (player_find_pendfrom( p, parray[p].opponent, PEND_DRAW) >= 0) {
    player_remove_request( parray[p].opponent, p, PEND_DRAW);
    player_decline_offers( p, -1, -1 );
    game_ended( parray[p].game, (garray[parray[p].game].white == p) ? BLACK : WHITE, END_AGREEDDRAW );
  } else {
    pprintf_prompt( parray[p].opponent, "\n%s offers you a draw.\n",
                                         parray[p].name );
    pprintf( p, "Draw request sent.\n" );
    player_add_request( p, parray[p].opponent, PEND_DRAW, 0);
  }
  return COM_OK;
}

PUBLIC int com_pause( int p, param_list param )
{
  int g;
  int now;

  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (garray[g].wTime == 0) {
    pprintf( p, "You can't pause untimed games.\n" );
    return COM_OK;
  }
  if (garray[g].clockStopped) {
    pprintf( p, "Game is already paused, use \"unpause\" to resume.\n" );
    return COM_OK;
  }
  if (player_find_pendfrom( p, parray[p].opponent, PEND_PAUSE) >= 0) {
    player_remove_request( parray[p].opponent, p, PEND_PAUSE);
    garray[g].clockStopped = 1;
    /* Roll back the time */
    if (garray[g].game_state.onMove == WHITE) {
      garray[g].wTime += (garray[g].lastDecTime - garray[g].lastMoveTime);
    } else {
      garray[g].bTime += (garray[g].lastDecTime - garray[g].lastMoveTime);
    }
    now = tenth_secs();
    if (garray[g].numHalfMoves == 0)
      garray[g].timeOfStart = now;
    garray[g].lastMoveTime = now;
    garray[g].lastDecTime = now;
    send_boards(g);
    pprintf_prompt( parray[p].opponent, "\n%s accepted pause. Game clock paused.\n",
                                         parray[p].name );
    pprintf( p, "Game clock paused.\n" );
  } else {
    pprintf_prompt( parray[p].opponent, "\n%s requests to pause the game.\n",
                                         parray[p].name );
    pprintf( p, "Pause request sent.\n" );
    player_add_request( p, parray[p].opponent, PEND_PAUSE, 0);
  }
  return COM_OK;
}

PUBLIC int com_unpause( int p, param_list param )
{
  int now;
  int g;

  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (!garray[g].clockStopped) {
    pprintf( p, "Game is not paused.\n" );
    return COM_OK;
  }
  garray[g].clockStopped = 0;
  now = tenth_secs();
  if (garray[g].numHalfMoves == 0)
    garray[g].timeOfStart = now;
  garray[g].lastMoveTime = now;
  garray[g].lastDecTime = now;
  send_boards(g);
  pprintf( p, "Game clock resumed.\n" );
  pprintf_prompt( parray[p].opponent, "\nGame clock resumed.\n" );
  return COM_OK;
}

PUBLIC int com_abort( int p, param_list param )
{
  int p1;
  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  p1 = parray[p].opponent;
  if (parray[p1].simul_info.numBoards &&
      parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] !=
      parray[p].game) {
    pprintf( p, "You can only make requests when the simul player is at your board.\n" );
    return COM_OK;
  }
  if (player_find_pendfrom( p, parray[p].opponent, PEND_ABORT) >= 0) {
    player_remove_request( parray[p].opponent, p, PEND_ABORT);
    player_decline_offers( p, -1, -1 );
    game_ended( parray[p].game, (garray[parray[p].game].white == p) ? BLACK : WHITE, END_ABORT );
  } else {
    pprintf_prompt( parray[p].opponent, "\n%s would like to abort the game.\n",
                                         parray[p].name );
    pprintf( p, "Abort request sent.\n" );
    player_add_request( p, parray[p].opponent, PEND_ABORT, 0);
  }
  return COM_OK;
}

PUBLIC int com_courtesyabort( int p, param_list param )
{
  int g;

  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (garray[g].wInitTime == 0) {
    pprintf( p, "You can't courtesy an untimed game, just resign.\n" );
    return COM_OK;
  }
  game_update_time(g);
  if ( ( (garray[g].white == p) && (garray[g].bTime <= 0) && (garray[g].wTime > 0)) ||
       ( (garray[g].black == p) && (garray[g].wTime <= 0) && (garray[g].bTime > 0)) ) {
    player_decline_offers( p, -1, -1 );
    game_ended( parray[p].game, (garray[g].white == p) ? WHITE : BLACK,
                END_COURTESY );
  } else {
    pprintf( p, "Game not aborted.\nEither your opponent still has time, or you are out of time also.\n" );
  }
  return COM_OK;
}

PUBLIC int com_flag( int p, param_list param )
{
  int g;

  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (garray[g].wInitTime == 0) {
    pprintf( p, "You can't flag an untimed game.\n" );
    return COM_OK;
  }
  game_update_time(g);
  if ((garray[g].wTime <= 0) && (garray[g].bTime <= 0)) {
    player_decline_offers( p, -1, -1 );
    game_ended( g, (garray[g].white == p) ? WHITE : BLACK, END_BOTHFLAG );
    return COM_OK;
  }
  if (garray[g].white == p) {
    if (garray[g].bTime > 0) {
      pprintf( p, "Your opponent is not out of time!\n" );
      return COM_OK;
    }
  }
  if (garray[g].black == p) {
    if (garray[g].wTime > 0) {
      pprintf( p, "Your opponent is not out of time!\n" );
      return COM_OK;
    }
  }
  player_decline_offers( p, -1, -1 );
  game_ended( g, (garray[g].white == p) ? WHITE : BLACK, END_FLAG );
  return COM_OK;
}

PUBLIC int com_adjourn( int p, param_list param )
{
  ASSERT(param[0].type == TYPE_NULL);
  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  if (!garray[parray[p].game].rated) {
    pprintf( p, "You cannot adjourn an unrated game. Use \"abort\".\n" );
    return COM_OK;
  }
  if (player_find_pendfrom( p, parray[p].opponent, PEND_ADJOURN) >= 0) {
    player_remove_request( parray[p].opponent, p, PEND_ADJOURN);
    player_decline_offers( p, -1, -1 );
    game_ended( parray[p].game, (garray[parray[p].game].white == p) ? BLACK : WHITE, END_ADJOURN );
  } else {
    pprintf_prompt( parray[p].opponent, "\n%s would like to adjourn the game.\n",
                                         parray[p].name );
    pprintf( p, "Adjourn request sent.\n" );
    player_add_request( p, parray[p].opponent, PEND_ADJOURN, 0);
  }
  return COM_OK;
}

PUBLIC int com_games( int p, param_list param )
{
  int i;
  int wp, bp;
  int wr, br;
  int ws, bs;

  ASSERT(param[0].type == TYPE_NULL);
  for (i=0;i<g_num;i++) {
    if (garray[i].status != GAME_ACTIVE) continue;
    board_calc_strength( &garray[i].game_state, &ws, &bs );
    pprintf( p, "%2d ", i+1 );
    wp = garray[i].white;
    bp = garray[i].black;
    if (garray[i].type == TYPE_BLITZ) {
      wr = parray[wp].b_stats.rating;
      br = parray[bp].b_stats.rating;
    } else {
      wr = parray[wp].s_stats.rating;
      br = parray[bp].s_stats.rating;
    }
    if (wr > 0)
      pprintf( p, "%4s %-12s", ratstr(wr), parray[wp].name );
    else
      pprintf( p, "     %-12s", parray[wp].name );
    if (br > 0)
      pprintf( p, "%4s %-11s", ratstr(br), parray[bp].name );
    else
      pprintf( p, "     %-11s", parray[bp].name );
    pprintf( p, "[%c%c%3d %3d] ",
                 (garray[i].type == TYPE_BLITZ) ? 'b' :
                 (garray[i].type == TYPE_STAND) ? 's' : 'u',
                 (garray[i].rated) ? 'r': 'u',
                 garray[i].wInitTime/600, garray[i].wIncrement/10 );
    game_update_time(i);
    pprintf_noformat( p, "%6s", tenth_str( garray[i].wTime, 0 ) );
    pprintf_noformat( p, "-%6s (%2d-%2d) %c: %2d\n",
                         tenth_str( garray[i].bTime, 0 ),
                         ws, bs,
                         (garray[i].game_state.onMove == WHITE) ? 'W' : 'B',
                         garray[i].game_state.moveNum );
  }
  return COM_OK;
}

PRIVATE int do_observe( int p, int obgame )
{
  if (garray[obgame].private) {
    pprintf( p, "Sorry, game %d is a private game.\n", obgame+1 );
    return COM_OK;
  }
  if ((garray[obgame].white == p) || (garray[obgame].black == p)) {
    pprintf( p, "You cannot observe a game that you are playing.\n" );
    return COM_OK;
  }
  if (player_is_observe( p, obgame )) {
      pprintf( p, "Removing game %d from observation list.\n", obgame + 1 );
      player_remove_observe( p, obgame );
  } else {
    if (!player_add_observe( p, obgame )) {
      pprintf( p, "Adding game %d to observation list.\n", obgame + 1 );
    } else {
      pprintf( p, "You are already observing the maximum number of games.\n" );
    }
  }
  return COM_OK;
}

PUBLIC int com_observe( int p, param_list param )
{
  int i;
  int p1= -1, obgame;

  if (param[0].type == TYPE_NULL) {
    for (i=0;i<parray[p].num_observe;i++) {
      pprintf( p, "Removing game %d from observation list.\n", parray[p].observe_list[i] + 1 );
    }
    parray[p].num_observe = 0; return COM_OK;
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    obgame = parray[p1].game;
  } else { /* Must be an integer */
    obgame = param[0].val.integer-1;
  }
  if ((obgame < 0) || (obgame >= g_num) || (garray[obgame].status != GAME_ACTIVE)) {
    pprintf( p, "There is no such game.\n" );
    return COM_OK;
  }
  if ((p1 >= 0) && parray[p1].simul_info.numBoards) {
    for (i=0;i<parray[p1].simul_info.numBoards;i++)
      if (parray[p1].simul_info.boards[i] >= 0)
        do_observe( p, parray[p1].simul_info.boards[i] );
  } else {
    do_observe( p, obgame );
  }
  return COM_OK;
}

PUBLIC int com_allobservers( int p, param_list param )
{
  int obgame;
  int p1;
  int start, end;
  int g;
  int first;

  if (param[0].type == TYPE_NULL) {
    obgame = -1;
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    obgame = parray[p1].game;
  } else { /* Must be an integer */
    obgame = param[0].val.integer-1;
  }
  if (obgame >= g_num) return COM_OK;
  if (obgame < 0) {
    start = 0;
    end = g_num;
  } else {
    start = obgame;
    end = obgame+1;
  }
  for (g=start;g<end;g++) {
    first = 1;
    for (p1=0; p1<p_num; p1++) {
      if (parray[p1].status == PLAYER_EMPTY) continue;
      if (player_is_observe(p1,g)) {
        if (first) {
          pprintf( p, "Observing %2d [%s vs. %s]:",
                        g+1,
                        parray[garray[g].white].name,
                        parray[garray[g].black].name );
          first = 0;
        }
        pprintf( p, " %s", parray[p1].name );
      }
    }
    if (!first) pprintf( p, "\n" );
  }
  return COM_OK;
}

PUBLIC int com_moves( int p, param_list param )
{
  int g;
  int p1;

  if (param[0].type == TYPE_NULL) {
    if (parray[p].game >= 0) {
      g = parray[p].game;
    } else {
      pprintf( p, "You are neither playing nor observing a game.\n" );
      return COM_OK;
    }
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    g = parray[p1].game;
  } else { /* Must be an integer */
    g = param[0].val.integer-1;
  }
  if ((g < 0) || (g >= g_num) || (garray[g].status != GAME_ACTIVE)) {
    pprintf( p, "There is no such game.\n" );
    return COM_OK;
  }
  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    return COM_OK;
  }
  pprintf( p, "%s\n", movesToString(g));
  return COM_OK;
}

PUBLIC int com_mailmoves( int p, param_list param )
{
  int g;
  int p1;

  if (param[0].type == TYPE_NULL) {
    if (parray[p].game >= 0) {
      g = parray[p].game;
    } else {
      pprintf( p, "You are neither playing nor observing a game.\n" );
      return COM_OK;
    }
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    g = parray[p1].game;
  } else { /* Must be an integer */
    g = param[0].val.integer-1;
  }
  if ((g < 0) || (g >= g_num) || (garray[g].status != GAME_ACTIVE)) {
    pprintf( p, "There is no such game.\n" );
    return COM_OK;
  }
  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    return COM_OK;
  }
  if (mail_string_to_user(p, movesToString(g) )) {
    pprintf( p, "Moves NOT mailed, perhaps your address is incorrect.\n" );
  } else {
    pprintf( p, "Moves mailed.\n" );
  }
  return COM_OK;
}

PUBLIC int com_oldmoves( int p, param_list param )
{
  int g;
  int p1=p;

  if (param[0].type == TYPE_NULL) {
    p1 = p;
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
  }
  g = FindOldGameFor(p1);
  if (g < 0) {
    pprintf( p, "There is not old game for %s.\n", parray[p1].name );
    return COM_OK;
  }
  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    return COM_OK;
  }
  pprintf( p, "%s\n", movesToString(g));
  return COM_OK;
}

PUBLIC int com_mailoldmoves( int p, param_list param )
{
  int g;
  int p1=p;

  if (param[0].type == TYPE_NULL) {
    p1 = p;
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
  }
  g = FindOldGameFor(p1);
  if (g < 0) {
    pprintf( p, "There is not old game for %s.\n", parray[p1].name );
    return COM_OK;
  }
  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    return COM_OK;
  }
  if (mail_string_to_user(p, movesToString(g) )) {
    pprintf( p, "Moves NOT mailed, perhaps your address is incorrect.\n" );
  } else {
    pprintf( p, "Moves mailed.\n" );
  }
  return COM_OK;
}

PUBLIC int com_load( int p, param_list param )
{
  int wp, bp, p3;
  int g;
  char outStr[1024];

  if ((wp = player_find_part_login(param[0].val.word)) < 0) {
    pprintf( p, "No player named %s is logged in.\n", param[0].val.word );
    return COM_OK;
  }
  if (parray[wp].game >= 0) {
    pprintf( p, "%s is currently playing a game.\n", parray[wp].name );
    return COM_OK;
  }
  if (!parray[wp].open) {
    pprintf( p, "%s is not open to playing a game.\n", parray[wp].name );
    return COM_OK;
  }
  if ((bp = player_find_part_login(param[1].val.word)) < 0) {
    pprintf( p, "No player named %s is logged in.\n", param[1].val.word );
    return COM_OK;
  }
  if (parray[bp].game >= 0) {
    pprintf( p, "%s is currently playing a game.\n", parray[bp].name );
    return COM_OK;
  }
  if (!parray[bp].open) {
    pprintf( p, "%s is not open to playing a game.\n", parray[bp].name );
    return COM_OK;
  }
  if ((bp != p) && (wp != p)) {
    pprintf( p, "You cannot load someone else's game.\nTry \"smoves\" or \"sposition\" to see the game.\n" );
    return COM_OK;
  }
  g = game_new();
  if (game_read( g, wp, bp ) < 0) {
    pprintf( p, "There is no stored game %s vs. %s\n", parray[wp].name, parray[bp].name );
    game_remove(g);
    return COM_OK;
  }
  game_delete( wp, bp );
  for (p3=0; p3 < p_num; p3++) {
    if ((p3 == wp) || (p3 == bp)) continue;
    if (parray[p3].status != PLAYER_PROMPT) continue;
    if (player_find_pendfrom( p3, wp, PEND_MATCH) >= 0) {
      pprintf_prompt( p3, "\n%s is continuing a game with %s\n",
                   parray[wp].name, parray[bp].name );
      player_remove_pendfrom( p3, wp, PEND_MATCH);
      player_remove_pendto( wp, p3, PEND_MATCH);
    }
    if (player_find_pendfrom( p3, bp, PEND_MATCH) >= 0) {
      pprintf_prompt( p3, "\n%s is continuing a game with %s\n",
                   parray[bp].name, parray[wp].name );
      player_remove_pendfrom( p3, bp, PEND_MATCH);
      player_remove_pendto( bp, p3, PEND_MATCH);
    }
    if (player_find_pendto( p3, wp, PEND_MATCH) >= 0) {
      pprintf_prompt( p3, "\n%s is continuing a game with %s\n",
                   parray[wp].name, parray[bp].name );
      player_remove_pendto( p3, wp, PEND_MATCH);
      player_remove_pendfrom( wp, p3, PEND_MATCH);
    }
    if (player_find_pendto( p3, bp, PEND_MATCH) >= 0) {
      pprintf_prompt( p3, "\n%s is continuing a game with %s\n",
                   parray[bp].name, parray[wp].name );
      player_remove_pendto( p3, bp, PEND_MATCH);
      player_remove_pendfrom( bp, p3, PEND_MATCH);
    }
  }
  player_remove_request( wp, bp, PEND_MATCH);
  player_remove_request( bp, wp, PEND_MATCH);
  garray[g].white = wp;
  garray[g].black = bp;
  garray[g].status = GAME_ACTIVE;
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;
  sprintf( outStr, "Continuing %s, between %s and %s.\n\n",
                   game_str(garray[g].rated, garray[g].wInitTime/10,
                            garray[g].wIncrement/10,
                            garray[g].bInitTime/10,
                            garray[g].bIncrement/10, NULL, NULL),
                   parray[wp].name, parray[bp].name );
  pprintf( wp, "%s", outStr );
  pprintf( bp, "%s", outStr );
  for (p3=0; p3 < p_num; p3++) {
    if ((p3 == wp) || (p3 == bp)) continue;
    if (parray[p3].status != PLAYER_PROMPT) continue;
    if (!parray[p3].i_game) continue;
    pprintf_prompt( p3, "%s", outStr );
  }
  parray[wp].game = g;
  parray[wp].opponent = bp;
  parray[wp].side = WHITE;
  parray[bp].game = g;
  parray[bp].opponent = wp;
  parray[bp].side = BLACK;
  send_boards(g);
  return COM_OK;
}

PUBLIC int com_stored( int p, param_list param )
{
  DIR *dirp;
  struct direct *dp;
  int p1, connected;
  char dname[MAX_FILENAME_SIZE];

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
  sprintf( dname, "%s/%c", game_dir, parray[p1].login[0] );
  dirp = opendir( dname );
  if (!dirp) {
    pprintf( p, "Player %s has no games stored.\n", parray[p1].name );
    if (!connected)
      player_remove( p1 );
    return COM_OK;
  }
  pprintf( p, "Stored games for %s:\n", parray[p1].name );
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (file_has_pname(dp->d_name, parray[p1].login)) {
      pprintf( p, "   %s vs. %s\n", file_wplayer(dp->d_name), file_bplayer(dp->d_name) );
    }
  }

  closedir( dirp );
  pprintf( p, "\n" );
  if (!connected)
    player_remove( p1 );
  return COM_OK;
}


PUBLIC int com_smoves( int p, param_list param )
{
  int wp, wconnected, bp, bconnected;
  int g;

  if ((wp = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
    wconnected = 0;
    wp = player_new();
    if (player_read( wp, param[0].val.word )) {
      player_remove( wp );
      pprintf( p, "There is no player named %s.\n", param[0].val.word );
      return COM_OK;
    }
  } else {
    wconnected = 1;
  }
  if ((bp = player_find_part_login(param[1].val.word)) < 0) { /* not logged in */
    bconnected = 0;
    bp = player_new();
    if (player_read( bp, param[1].val.word )) {
      player_remove( bp );
      if (!wconnected)
        player_remove( wp );
      pprintf( p, "There is no player named %s.\n", param[1].val.word );
      return COM_OK;
    }
  } else {
    bconnected = 1;
  }
  g = game_new();
  if (game_read( g, wp, bp ) < 0) {
    pprintf( p, "There is no stored game %s vs. %s\n", parray[wp].name, parray[bp].name );
    game_remove(g);
    if (!wconnected)
      player_remove( wp );
    if (!bconnected)
      player_remove( bp );
    return COM_OK;
  }
  garray[g].white = wp;
  garray[g].black = bp;
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;

  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    game_remove(g);
    if (!wconnected)
      player_remove( wp );
    if (!bconnected)
      player_remove( bp );
    return COM_OK;
  }
  pprintf( p, "%s\n", movesToString(g));

  game_remove(g);
  if (!wconnected)
    player_remove( wp );
  if (!bconnected)
    player_remove( bp );
  return COM_OK;
}


PUBLIC int com_sposition( int p, param_list param )
{
  int wp, wconnected, bp, bconnected;
  int g;

  if ((wp = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
    wconnected = 0;
    wp = player_new();
    if (player_read( wp, param[0].val.word )) {
      player_remove( wp );
      pprintf( p, "There is no player named %s.\n", param[0].val.word );
      return COM_OK;
    }
  } else {
    wconnected = 1;
  }
  if ((bp = player_find_part_login(param[1].val.word)) < 0) { /* not logged in */
    bconnected = 0;
    bp = player_new();
    if (player_read( bp, param[1].val.word )) {
      player_remove( bp );
      if (!wconnected)
        player_remove( wp );
      pprintf( p, "There is no player named %s.\n", param[1].val.word );
      return COM_OK;
    }
  } else {
    bconnected = 1;
  }
  g = game_new();
  if (game_read( g, wp, bp ) < 0) {
    pprintf( p, "There is no stored game %s vs. %s\n", parray[wp].name, parray[bp].name );
    game_remove(g);
    if (!wconnected)
      player_remove( wp );
    if (!bconnected)
      player_remove( bp );
    return COM_OK;
  }
  garray[g].white = wp;
  garray[g].black = bp;
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;
  if ((garray[g].white != p) && (garray[g].black != p) && garray[g].private) {
    pprintf( p, "Sorry, that is a private game.\n" );
    game_remove(g);
    if (!wconnected)
      player_remove( wp );
    if (!bconnected)
      player_remove( bp );
    return COM_OK;
  }
  pprintf( p, "Position of stored game %s vs. %s\n", parray[wp].name, parray[bp].name );
  send_board_to( g, p );
  game_remove(g);
  if (!wconnected)
    player_remove( wp );
  if (!bconnected)
    player_remove( bp );
  return COM_OK;
}

PUBLIC int com_takeback( int p, param_list param )
{
  int nHalfMoves=1;
  int from;
  int g, i;
  int p1;

  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  p1 = parray[p].opponent;
  if (parray[p1].simul_info.numBoards &&
      parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] !=
      parray[p].game) {
    pprintf( p, "You can only make requests when the simul player is at your board.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (param[0].type == TYPE_INT) {
    nHalfMoves = param[0].val.integer;
  }
  if ((from=player_find_pendfrom( p, parray[p].opponent, PEND_TAKEBACK))>= 0) {
    player_remove_request( parray[p].opponent, p, PEND_TAKEBACK);
    if (parray[p].p_from_list[from].param1 == nHalfMoves) {
    /* Doing the takeback */
      player_decline_offers( p, -1, -1 );
      for (i=0;i<nHalfMoves;i++) {
        if (backup_move( g ) != MOVE_OK) {
          pprintf( garray[g].white, "Can only backup %d moves\n", i );
          pprintf( garray[g].black, "Can only backup %d moves\n", i );
          break;
        }
      }
      send_boards(g);
    } else {
      if (garray[g].numHalfMoves < nHalfMoves) {
        pprintf( p, "There are only %d half moves in your game.\n", garray[g].numHalfMoves );
        pprintf_prompt( parray[p].opponent, "\n%s has declined the takeback request.\n", parray[p].name, nHalfMoves );
        return COM_OK;
      }
      pprintf( p, "You disagree on the number of half-moves to takeback.\n" );
      pprintf( p, "Alternate takeback request sent.\n" );
      pprintf_prompt( parray[p].opponent, "\n%s proposes a different number (%d) of half-move(s).\n", parray[p].name, nHalfMoves );
      player_add_request( p, parray[p].opponent, PEND_TAKEBACK, nHalfMoves);
    }
  } else {
    if (garray[g].numHalfMoves < nHalfMoves) {
      pprintf( p, "There are only %d half moves in your game.\n", garray[g].numHalfMoves );
      return COM_OK;
    }
    pprintf_prompt( parray[p].opponent, "\n%s would like to take back %d half move(s).\n",
                              parray[p].name, nHalfMoves );
    pprintf( p, "Takeback request sent.\n" );
    player_add_request( p, parray[p].opponent, PEND_TAKEBACK, nHalfMoves);
  }
  return COM_OK;
}


PUBLIC int com_switch( int p, param_list param )
{
  int g;
  int tmp, now;
  int p1;

  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  p1 = parray[p].opponent;
  if (parray[p1].simul_info.numBoards &&
      parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] !=
      parray[p].game) {
    pprintf( p, "You can only make requests when the simul player is at your board.\n" );
    return COM_OK;
  }
  g = parray[p].game;
  if (player_find_pendfrom( p, parray[p].opponent, PEND_SWITCH)>= 0) {
    player_remove_request( parray[p].opponent, p, PEND_SWITCH);
    /* Doing the switch */
    player_decline_offers( p, -1, -1 );
    tmp = garray[g].white;
    garray[g].white = garray[g].black;
    garray[g].black = tmp;
    parray[p].side = (parray[p].side == WHITE) ? BLACK : WHITE;
    parray[parray[p].opponent].side =
                  (parray[parray[p].opponent].side == WHITE) ? BLACK : WHITE;
    /* Roll back the time */
    if (garray[g].game_state.onMove == WHITE) {
      garray[g].wTime += (garray[g].lastDecTime - garray[g].lastMoveTime);
    } else {
      garray[g].bTime += (garray[g].lastDecTime - garray[g].lastMoveTime);
    }
    now = tenth_secs();
    if (garray[g].numHalfMoves == 0)
      garray[g].timeOfStart = now;
    garray[g].lastMoveTime = now;
    garray[g].lastDecTime = now;
    send_boards( g );
    return COM_OK;
  }
  if (garray[g].rated && garray[g].numHalfMoves > 0) {
    pprintf( p, "You cannot switch sides once a rated game is underway.\n" );
    return COM_OK;
  }
  pprintf_prompt( parray[p].opponent, "\n%s would like to switch sides.\nType \"accept\" to switch sides, or \"decline\" to refuse.\n", parray[p].name );
  pprintf( p, "Swtich request sent.\n" );
  player_add_request( p, parray[p].opponent, PEND_SWITCH, 0);
  return COM_OK;
}


PUBLIC int com_history( int p, param_list param )
{
  int p1=p, connected=1;
  char fname[MAX_FILENAME_SIZE];

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_part_login(param[0].val.word)) < 0) { /* not logged in */
      connected = 0;
      p1 = player_new();
      if (player_read( p1, param[0].val.word )) {
        player_remove( p1 );
        pprintf( p, "There is no player by that name.\n" );
        return COM_OK;
      }
    }
  }
  sprintf( fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p1].login[0], parray[p1].login, STATS_GAMES );
  pgames( p, fname );
  if (!connected)
    player_remove( p1 );
  return COM_OK;
}

PUBLIC int com_time( int p, param_list param )
{
  int p1, g;

  if (param[0].type == TYPE_NULL) {
    if (parray[p].game < 0) {
      pprintf( p, "You are not playing a game.\n" );
      return COM_OK;
    }
    g = parray[p].game;
  } else if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login( param[0].val.word );
    if (p1 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    g = parray[p1].game;
  } else { /* Must be an integer */
    g = param[0].val.integer-1;
  }
  if ((g < 0) || (g >= g_num) || (garray[g].status != GAME_ACTIVE)) {
    pprintf( p, "There is no such game.\n" );
    return COM_OK;
  }
  game_update_time(g);
  pprintf( p, "White (%s) : %d mins, %d secs\n",
                            parray[garray[g].white].name,
                            garray[g].wTime / 600,
              (garray[g].wTime - ((garray[g].wTime / 600) * 600)) / 10 );
  pprintf( p, "Black (%s) : %d mins, %d secs\n",
                            parray[garray[g].black].name,
                            garray[g].bTime / 600,
              (garray[g].bTime - ((garray[g].bTime / 600) * 600)) / 10 );
  return COM_OK;
}

PUBLIC int com_boards( int p, param_list param )
{
  char *catagory=NULL;
  char dname[MAX_FILENAME_SIZE];
  DIR *dirp;
  struct direct *dp;

  if (param[0].type == TYPE_WORD)
    catagory = param[0].val.word;
  if (catagory) {
    pprintf( p, "Boards Available For Catagory %s:\n", catagory );
    sprintf( dname, "%s/%s", board_dir, catagory );
  } else {
    pprintf( p, "Catagories Available:\n" );
    sprintf( dname, "%s", board_dir );
  }
  dirp = opendir( dname );
  if (!dirp) {
    pprintf( p, "No such catagory %s, try \"boards\".\n", catagory );
    return COM_OK;
  }
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (!strcmp(dp->d_name, ".")) continue;
    if (!strcmp(dp->d_name, "..")) continue;
    pprintf( p, "%s\n", dp->d_name );
  }
  closedir( dirp );
  return COM_OK;
}

PUBLIC int com_simmatch( int p, param_list param )
{
  int p1;
  int num;

  p1 = player_find_part_login( param[0].val.word );
  if (p1 < 0) {
    pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (player_find_pendfrom( p, p1, PEND_SIMUL) >= 0) {
    player_remove_request( p1, p, PEND_SIMUL);
    /* Accepting Simul ! */
    if (parray[p].simul_info.numBoards >= MAX_SIMUL) {
      pprintf( p, "You are already playing the maximum of %d boards.\n", MAX_SIMUL );
      pprintf( p1, "Simul request removed, boards filled.\n" );
      return COM_OK;
    }
    if (create_new_match( p, p1, 0, 0, 0, 0, 0, "standard", "standard" )) {
      pprintf( p, "There was a problem creating the new match.\n" );
      pprintf_prompt( p1, "There was a problem creating the new match.\n" );
      return COM_OK;
    }
    num = parray[p].simul_info.numBoards;
    parray[p].simul_info.results[num] = -1;
    parray[p].simul_info.boards[num] = parray[p].game;
    parray[p].simul_info.numBoards++;
    if (parray[p].simul_info.numBoards > 1 &&
        parray[p].simul_info.onBoard >= 0)
      player_goto_board( p, parray[p].simul_info.onBoard );
    else
      parray[p].simul_info.onBoard = 0;
    return COM_OK;
  }
  if (player_find_pendfrom( p, -1, PEND_SIMUL)) {
    pprintf( p, "You cannot be the simul giver and request to join another simul.\nThat would just be too confusing for me and you.\n" );
    return COM_OK;
  }
  if (parray[p].simul_info.numBoards) {
    pprintf( p, "You cannot be the simul giver and request to join another simul.\nThat would just be too confusing for me and you.\n" );
    return COM_OK;
  }
  if (parray[p].game >= 0) {
    pprintf( p, "You are already playing a game.\n" );
    return COM_OK;
  }
  if (!parray[p1].sopen) {
    pprintf( p, "%s is not open to receiving simul requests.\n", parray[p1].name );
    return COM_OK;
  }
  if (parray[p1].simul_info.numBoards >= MAX_SIMUL) {
    pprintf( p, "%s is already playing the maximum of %d boards.\n", parray[p1].name, MAX_SIMUL );
    return COM_OK;
  }
  if (player_add_request( p, p1, PEND_SIMUL )) {
    pprintf( p, "Maximum number of pending actions reached. Your request was not sent.\nTry again later.\n" );
    return COM_OK;
  } else {
    pprintf( p, "Simul match request sent.\n" );
    pprintf_prompt( p1, "\n%s requests to join a simul match with you.\n", parray[p].name );
  }
  return COM_OK;
}

PUBLIC int com_simnext( int p, param_list param )
{
  int on, g;

  if (!parray[p].simul_info.numBoards) {
    pprintf( p, "You are not playing a simul.\n" );
    return COM_OK;
  }
  player_decline_offers( p, -1, -1 );
  on = parray[p].simul_info.onBoard;
  g = parray[p].simul_info.boards[on];
  if (g >= 0) {
    pprintf_prompt( garray[g].black, "\n%s is moving off of your board.\n", parray[p].name );
  }
  player_goto_next_board( p );
  return COM_OK;
}

PUBLIC int com_simgames( int p, param_list param )
{
  int p1=p;

  if (param[0].type == TYPE_WORD) {
    if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
      pprintf( p, "No player named %s is logged in.\n", param[0].val.word );
      return COM_OK;
    }
  }
  if (p1 == p)
    pprintf( p, "You are playing %d simultaneous games.\n",
          player_num_active_boards(p1) );
  else
    pprintf( p, "%s is playing %d simultaneous games.\n", parray[p1].name,
          player_num_active_boards(p1) );
  return COM_OK;
}

PUBLIC int com_simpass( int p, param_list param )
{
  int g;

  if (parray[p].game < 0) {
    pprintf( p, "You are not playing a game.\n" );
    return COM_OK;
  }
  player_decline_offers( p, -1, -1 );
  g = parray[p].game;
  if (!parray[garray[g].white].simul_info.numBoards) {
    pprintf( p, "You are not participating in a simul.\n" );
    return COM_OK;
  }
  if (garray[g].passes >= 3) {
    pprintf( p, "You have reached your maximum of 3 passes.\n" );
    pprintf( p, "Please move IMMEDIATELY!\n" );
    return COM_OK;
  }
  garray[g].passes++;
  player_goto_next_board(garray[g].white);
  return COM_OK;
}

PUBLIC int com_simabort( int p, param_list param )
{
  if (!parray[p].simul_info.numBoards) {
    pprintf( p, "You are not playing a simul.\n" );
    return COM_OK;
  }
  player_decline_offers( p, -1, -1 );
  game_ended( parray[p].simul_info.boards[parray[p].simul_info.onBoard],
              WHITE, END_ABORT );
  return COM_OK;
}
