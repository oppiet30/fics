/* board.c
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
#include "board.h"
#include "playerdb.h"
#include "gamedb.h"
#include "utils.h"

extern int style1();
extern int style2();
extern int style3();
extern int style4();
extern int style5();
extern int style6();
extern int style7();
extern int style8();
extern int style9();
extern int style10();
extern int style11();
extern int style12();

PUBLIC char *wpstring[]={" ", "P", "N", "B", "R", "Q", "K" };
PUBLIC char *bpstring[]={" ", "p", "n", "b", "r", "q", "k" };

PUBLIC int pieceValues[7] = {0, 1, 3, 3, 5, 9, 0};
PUBLIC int (*styleFuncs[MAX_STYLES])() = {
  style1,
  style2,
  style3,
  style4,
  style5,
  style6,
  style7,
  style8,
  style9,
  style10,
  style11,
  style12
};

PRIVATE char bstring[MAX_BOARD_STRING_LEGTH];

PUBLIC int board_init( game_state_t *b, char *catagory, char *board )
{
  int retval;
  int wval;

  if (!catagory || !board || !catagory[0] || !board[0])
    retval = board_read_file( "standard", "standard", b );
  else {
    if (!strcmp(catagory, "wild")) {
      sscanf( board, "%d", &wval );
      if (wval >= 1 && wval <= 4)
        wild_update(wval);
    }
    retval = board_read_file( catagory, board, b );
  }
  b->gameNum = -1;
  return retval;
}

PUBLIC void board_calc_strength( game_state_t *b, int *ws, int *bs )
{
  int r, f;
  int *p;

  *ws = *bs = 0;
  for (f=0; f<8; f++) {
    for (r=0; r<8; r++) {
      if (colorval(b->board[r][f]) == WHITE)
        p = ws;
      else
        p = bs;
      *p += pieceValues[piecetype(b->board[r][f])];
    }
  }
}

/* Globals used for each board */
PRIVATE char *wName, *bName;
PRIVATE int wTime, bTime;
PRIVATE int orient;
PRIVATE int forPlayer;
PRIVATE int myTurn; /* 1 = my turn, 0 = observe, -1 = other turn */

PUBLIC char *board_to_string( char *wn, char *bn,
                              int wt, int bt,
                              game_state_t *b, move_t *ml, int style,
                              int orientation, int observing,
                              int p )
{
  wName = wn; bName = bn;
  wTime = wt; bTime = bt;
  orient = orientation;
  if (observing) {
      myTurn = 0;
  } else {
    if (orientation == b->onMove)
      myTurn = 1;
    else
      myTurn = -1;
  }
  forPlayer = p;
  if ((style < 0) || (style >= MAX_STYLES)) return NULL;
  if ( styleFuncs[style](b,ml) ) {
    return NULL;
  } else {
    return bstring;
  }
}

PUBLIC char *move_and_time( move_t *m )
{
  static char tmp[40];
  sprintf( tmp, "%-6s (%s)", m->algString, tenth_str(m->tookTime,0) );
  return tmp;
}

/* The following take the game state and whole move list */

PRIVATE int genstyle( game_state_t *b, move_t *ml, char *wp[], char *bp[],
                    char *wsqr, char *bsqr,
                    char *top, char *mid, char *start, char *end, char *label,
                      char *blabel )
{
  int f, r, count;
  char tmp[512];
  int first, last, inc;
  int ws, bs;

  board_calc_strength( b, &ws, &bs );
  if (myTurn)
    sprintf( bstring, "\nGame %d (%s vs. %s)\n\n",
                       b->gameNum+1,
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name );
  else
    sprintf( bstring, "\n OBSERVATION REPORT : Game %s vs %s [%d]\n",
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name,
                       b->gameNum+1 );
  if (orient == WHITE) {
    first = 7;
    last = 0;
    inc = -1;
  } else {
    first = 0;
    last = 7;
    inc = 1;
  }
  strcat( bstring, top );
  for (f = first, count = 7;f != last+inc; f += inc, count--) {
    sprintf( tmp, "     %d  %s", f+1, start );
    strcat( bstring, tmp );
    for (r = last; r != first-inc; r=r-inc) {
      if (square_color(r,f) == WHITE)
        strcat( bstring, wsqr );
      else
        strcat( bstring, bsqr );
      if (piecetype(b->board[r][f]) == NOPIECE) {
        if (square_color(r,f) == WHITE)
          strcat( bstring, bp[0] );
        else
          strcat( bstring, wp[0] );
      } else {
        if (colorval(b->board[r][f]) == WHITE)
          strcat( bstring, wp[piecetype(b->board[r][f])] );
        else
          strcat( bstring, bp[piecetype(b->board[r][f])] );
      }
    }
    sprintf( tmp, "%s", end );
    strcat( bstring, tmp );
    switch (count) {
      case 7:
        sprintf( tmp, "     Move # : %d (%s)", b->moveNum, CString(b->onMove) );
        strcat( bstring, tmp );
        break;
      case 6:
        if ((b->moveNum > 1) || (b->onMove == BLACK)) {
          int lastMove = MoveToHalfMove( b ) - 1;

          sprintf( tmp, "     %s Moves : '%s'", CString(CToggle(b->onMove)),
                      move_and_time(&ml[lastMove]) );
          strcat( bstring, tmp );
        }
        break;
      case 5:
        break;
      case 4:
          sprintf( tmp, "     Black Clock : %s", tenth_str(bTime,1) );
          strcat( bstring, tmp );
          break;
      case 3:
          sprintf( tmp, "     White Clock : %s", tenth_str(wTime,1) );
          strcat( bstring, tmp );
          break;
      case 2:
          sprintf( tmp, "     Black Strength : %d", bs );
          strcat( bstring, tmp );
          break;
      case 1:
          sprintf( tmp, "     White Strength : %d", ws );
          strcat( bstring, tmp );
        break;
      case 0:
        break;
    }
    strcat( bstring, "\n" );
    if (count != 0)
      strcat( bstring, mid );
    else
      strcat( bstring, top );
  }
  if (orient == WHITE)
    strcat( bstring, label );
  else
    strcat( bstring, blabel );
  return 0;
}

/* Standard ICS */
PUBLIC int style1( game_state_t *b, move_t *ml )
{
  static char *wp[]={"   |", " P |", " N |", " B |", " R |", " Q |", " K |" };
  static char *bp[]={"   |", " *P|", " *N|", " *B|", " *R|", " *Q|", " *K|" };
  static char *wsqr="";
  static char *bsqr="";
  static char *top="\t---------------------------------\n";
  static char *mid="\t|---+---+---+---+---+---+---+---|\n";
  static char *start="|";
  static char *end="";
  static char *label ="\t  a   b   c   d   e   f   g   h\n";
  static char *blabel="\t  h   g   f   e   d   c   b   a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* USA-Today Sports Center-style board */
PUBLIC int style2( game_state_t *b, move_t *ml )
{
  static char *wp[]={"+  ", "P  ", "N  ", "B  ", "R  ", "Q  ", "K  " };
  static char *bp[]={"-  ", "p' ", "n' ", "b' ", "r' ", "q' ", "k' " };
  static char *wsqr="";
  static char *bsqr="";
  static char *top="";
  static char *mid="";
  static char *start="";
  static char *end="";
  static char *label ="\ta  b  c  d  e  f  g  h\n";
  static char *blabel="\th  g  f  e  d  c  b  a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* Experimental vt-100 ANSI board for dark backgrounds */
PUBLIC int style3( game_state_t *b, move_t *ml )

{
  static char *wp[]={"   ", " P ", " N ", " B ", " R ", " Q ", " K " };
  static char *bp[]={"   ", " *P", " *N", " *B", " *R", " *Q", " *K" };
  static char *wsqr="\033[0m";
  static char *bsqr="\033[7m";
  static char *top="\t+------------------------+\n";
  static char *mid="";
  static char *start="|";
  static char *end="\033[0m|";
  static char *label ="\t  a  b  c  d  e  f  g  h\n";
  static char *blabel="\t  h  g  f  e  d  c  b  a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* Experimental vt-100 ANSI board for light backgrounds */
PUBLIC int style4( game_state_t *b, move_t *ml )

{
  static char *wp[]={"   ", " P ", " N ", " B ", " R ", " Q ", " K " };
  static char *bp[]={"   ", " *P", " *N", " *B", " *R", " *Q", " *K" };
  static char *wsqr="\033[7m";
  static char *bsqr="\033[0m";
  static char *top="\t+------------------------+\n";
  static char *mid="";
  static char *start="|";
  static char *end="\033[0m|";
  static char *label ="\t  a  b  c  d  e  f  g  h\n";
  static char *blabel="\t  h  g  f  e  d  c  b  a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* Style suggested by ajpierce@med.unc.edu */
PUBLIC int style5( game_state_t *b, move_t *ml )

{
  static char *wp[]={"    ", "  o ", " :N:", " <B>", " |R|", " {Q}", " =K=" };
  static char *bp[]={"    ", "  p ", " :n:", " <b>", " |r|", " {q}", " =k=" };
  static char *wsqr="";
  static char *bsqr="";
  static char *top="        .   .   .   .   .   .   .   .   .\n";
  static char *mid="        .   .   .   .   .   .   .   .   .\n";
  static char *start="";
  static char *end="";
  static char *label ="\ta  b  c  d  e  f  g  h\n";
  static char *blabel="\th  g  f  e  d  c  b  a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* Email Board suggested by Thomas Fought (tlf@rsch.oclc.org) */
PUBLIC int style6( game_state_t *b, move_t *ml )

{
  static char *wp[]={"    |", " wp |", " WN |", " WB |", " WR |", " WQ |", " WK |" };
  static char *bp[]={"    |", " bp |", " BN |", " BB |", " BR |", " BQ |", " BK |" };
  static char *wsqr="";
  static char *bsqr="";
  static char *top="\t-----------------------------------------\n";
  static char *mid="\t-----------------------------------------\n";
  static char *start="|";
  static char *end="";
  static char *label ="\t  A    B    C    D    E    F    G    H\n";
  static char *blabel="\t  H    G    F    E    D    C    B    A\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* Miniature board */
PUBLIC int style7( game_state_t *b, move_t *ml )

{
  static char *wp[]={"  ", " P", " N", " B", " R", " Q", " K" };
  static char *bp[]={" -", " p", " n", " b", " r", " q", " k" };
  static char *wsqr="";
  static char *bsqr="";
  static char *top="\t:::::::::::::::::::::\n";
  static char *mid="";
  static char *start="..";
  static char *end=" ..";
  static char *label ="\t    a b c d e f g h\n";
  static char *blabel="\t    h g f e d c b a\n";

  return genstyle(b,ml,wp,bp,wsqr,bsqr,top,mid,start,end,label,blabel);
}

/* ICS interface maker board-- raw data dump */
PUBLIC int style8( game_state_t *b, move_t *ml )

{
  char tmp[512];
  int f, r;
  int ws, bs;

  board_calc_strength( b, &ws, &bs );
  if (myTurn)
    sprintf( bstring, "\nGame %d (%s vs. %s)\n",
                       b->gameNum+1,
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name );
  else
    sprintf( bstring, "\n OBSERVATION REPORT : Game %s vs %s [%d]\n",
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name,
                       b->gameNum+1 );
  sprintf( tmp, "#@#%03d%-16s%s%-16s%s",  b->gameNum+1,
              parray[garray[b->gameNum].white].name,
              (garray[b->gameNum].white == forPlayer) ? "*" : ":",
              parray[garray[b->gameNum].black].name,
              (garray[b->gameNum].black == forPlayer) ? "*" : ":" );
  strcat( bstring, tmp );
  for (r=0;r<8;r++) {
    for (f=0;f<8;f++) {
      if (b->board[f][r] == NOPIECE) {
        strcat( bstring, " " );
      } else {
        if (colorval(b->board[f][r]) == WHITE)
          strcat( bstring, wpstring[piecetype(b->board[f][r])] );
        else
          strcat( bstring, bpstring[piecetype(b->board[f][r])] );
      }
    }
  }
  if (garray[b->gameNum].numHalfMoves) {
    sprintf( tmp, "%03d%s%02d%02d%05d%05d%s (%s)@#@\n",
                        garray[b->gameNum].numHalfMoves/2+1,
                        (b->onMove == WHITE) ? "W" : "B",
                        ws,
                        bs,
                        (wTime+5)/10,
                        (bTime+5)/10,
                garray[b->gameNum].numHalfMoves ?
                  ml[garray[b->gameNum].numHalfMoves-1].algString :
                  "       ",
                garray[b->gameNum].numHalfMoves ?
                 tenth_str( ml[garray[b->gameNum].numHalfMoves-1].tookTime,0) :
                  "    " );
  } else {
    sprintf( tmp, "%03d%s%02d%02d%05d%05d@#@\n",
                        garray[b->gameNum].numHalfMoves/2+1,
                        (b->onMove == WHITE) ? "W" : "B",
                        ws,
                        bs,
                        (wTime+5)/10,
                        (bTime+5)/10 );
  }
  strcat( bstring, tmp );
  return 0;
}

/* last 2 moves only (previous non-verbose mode) */
PUBLIC int style9( game_state_t *b, move_t *ml )

{
  int i, count;
  char tmp[512];
  int startmove;

  if (myTurn)
    sprintf( bstring, "\nGame %d (%s vs. %s)\n",
                       b->gameNum+1,
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name );
  else
    sprintf( bstring, "\n OBSERVATION REPORT : Game %s vs %s [%d]\n",
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name,
                       b->gameNum+1 );
  sprintf( tmp, "\nMove     %-23s%s\n",
                   parray[garray[b->gameNum].white].name,
                   parray[garray[b->gameNum].black].name );
  strcat( bstring, tmp );
  sprintf( tmp, "----     --------------         --------------\n" );
  strcat( bstring, tmp );
  startmove = ((garray[b->gameNum].numHalfMoves-3) / 2) * 2;
  if (startmove < 0) startmove = 0;
  for (i=startmove, count=0;
       i < garray[b->gameNum].numHalfMoves && count < 4;
       i++,count++) {
    if (!(i & 0x01)) {
      sprintf( tmp, "  %2d     ", i/2+1 );
      strcat( bstring, tmp );
    }
    sprintf( tmp, "%-23s", move_and_time(& ml[i]) );
    strcat( bstring, tmp );
    if (i & 0x01)
      strcat( bstring, "\n" );
  }
  if (i & 0x01)
    strcat( bstring, "\n" );
  return 0;
}

/* Sleator's new and improved raw dump */
PUBLIC int style10( game_state_t *b, move_t *ml )
{
  int f, r;
  char tmp[512];
  int ws, bs;

  board_calc_strength( b, &ws, &bs );
  if (myTurn)
    sprintf( bstring, "\nGame %d (%s vs. %s)\n",
                       b->gameNum+1,
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name );
  else
    sprintf( bstring, "\n OBSERVATION REPORT : Game %s vs %s [%d]\n",
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name,
                       b->gameNum+1 );
  sprintf( tmp, "<10>\n" );
  strcat( bstring, tmp );
  for (r=7;r>=0;r--) {
    strcat( bstring, "|" );
    for (f=0;f<8;f++) {
      if (b->board[f][r] == NOPIECE) {
        strcat( bstring, " " );
      } else {
        if (colorval(b->board[f][r]) == WHITE)
          strcat( bstring, wpstring[piecetype(b->board[f][r])] );
        else
          strcat( bstring, bpstring[piecetype(b->board[f][r])] );
      }
    }
    strcat( bstring, "|\n" );
  }
  strcat( bstring, (b->onMove == WHITE) ? "W " : "B " );
  if (garray[b->gameNum].numHalfMoves) {
    sprintf( tmp, "%d ",
           ml[garray[b->gameNum].numHalfMoves-1].doublePawn );
  } else {
    sprintf( tmp, "-1 " );
  }
  strcat( bstring, tmp );
  sprintf( tmp, "%d %d %d %d %d\n",
             !(b->wkmoved || b->wkrmoved),
             !(b->wkmoved || b->wqrmoved),
             !(b->bkmoved || b->bkrmoved),
             !(b->bkmoved || b->bqrmoved),
             (garray[b->gameNum].numHalfMoves - b->lastIrreversable) / 2 );
  strcat( bstring, tmp );
  sprintf( tmp, "%d %s %s %d %d %d %d %d %d %d %d %s %s %s\n",
              b->gameNum,
              parray[garray[b->gameNum].white].name,
              parray[garray[b->gameNum].black].name,
              myTurn,
              garray[b->gameNum].wInitTime / 600,
              garray[b->gameNum].wIncrement / 10,
              ws,
              bs,
              (wTime+5)/10,
              (bTime+5)/10,
              garray[b->gameNum].numHalfMoves/2+1,
              garray[b->gameNum].numHalfMoves ?
                ml[garray[b->gameNum].numHalfMoves-1].moveString :
                "none",
              garray[b->gameNum].numHalfMoves ?
                tenth_str( ml[garray[b->gameNum].numHalfMoves-1].tookTime,0) :
                "0:00",
              garray[b->gameNum].numHalfMoves ?
                ml[garray[b->gameNum].numHalfMoves-1].algString :
                "none"
                );
  strcat( bstring, tmp );
  sprintf( tmp, ">10<\n" );
  strcat( bstring, tmp );
  return 0;
}

/* Same as 8, but with verbose moves ("P/e3-e4", instead of "e4") */
PUBLIC int style11( game_state_t *b, move_t *ml )

{
  char tmp[512];
  int f, r;
  int ws, bs;

  board_calc_strength( b, &ws, &bs );
  if (myTurn)
    sprintf( bstring, "\nGame %d (%s vs. %s)\n",
                       b->gameNum+1,
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name );
  else
    sprintf( bstring, "\n OBSERVATION REPORT : Game %s vs %s [%d]\n",
                       parray[garray[b->gameNum].white].name,
                       parray[garray[b->gameNum].black].name,
                       b->gameNum+1 );
  sprintf( tmp, "#@#%03d%-16s%s%-16s%s",  b->gameNum,
              parray[garray[b->gameNum].white].name,
              (garray[b->gameNum].white == forPlayer) ? "*" : ":",
              parray[garray[b->gameNum].black].name,
              (garray[b->gameNum].black == forPlayer) ? "*" : ":" );
  strcat( bstring, tmp );
  for (r=0;r<8;r++) {
    for (f=0;f<8;f++) {
      if (b->board[f][r] == NOPIECE) {
        strcat( bstring, " " );
      } else {
        if (colorval(b->board[f][r]) == WHITE)
          strcat( bstring, wpstring[piecetype(b->board[f][r])] );
        else
          strcat( bstring, bpstring[piecetype(b->board[f][r])] );
      }
    }
  }
  if (garray[b->gameNum].numHalfMoves) {
    sprintf( tmp, "%03d%s%02d%02d%05d%05d%s (%s)@#@\n",
                        garray[b->gameNum].numHalfMoves/2+1,
                        (b->onMove == WHITE) ? "W" : "B",
                        ws,
                        bs,
                        (wTime+5)/10,
                        (bTime+5)/10,
                garray[b->gameNum].numHalfMoves ?
                  ml[garray[b->gameNum].numHalfMoves-1].moveString :
                  "       ",
                garray[b->gameNum].numHalfMoves ?
                tenth_str( ml[garray[b->gameNum].numHalfMoves-1].tookTime,0) :
                  "    " );
  } else {
    sprintf( tmp, "%03d%s%02d%02d%05d%05d@#@\n",
                        garray[b->gameNum].numHalfMoves/2+1,
                        (b->onMove == WHITE) ? "W" : "B",
                        ws,
                        bs,
                        (wTime+5)/10,
                        (bTime+5)/10 );
  }
  strcat( bstring, tmp );
  return 0;
}

/* Similar to style 10.  See the "style12" help file for information */
PUBLIC int style12( game_state_t *b, move_t *ml )
{
  int f, r;
  char tmp[512];
  int ws, bs;

  board_calc_strength( b, &ws, &bs );
  sprintf( bstring, "\n<12> " );
  for (r=7;r>=0;r--) {
    for (f=0;f<8;f++) {
      if (b->board[f][r] == NOPIECE) {
        strcat( bstring, "-" );
      } else {
        if (colorval(b->board[f][r]) == WHITE)
          strcat( bstring, wpstring[piecetype(b->board[f][r])] );
        else
          strcat( bstring, bpstring[piecetype(b->board[f][r])] );
      }
    }
    strcat( bstring, " " );
  }
  strcat( bstring, (b->onMove == WHITE) ? "W " : "B " );
  if (garray[b->gameNum].numHalfMoves) {
    sprintf( tmp, "%d ",
           ml[garray[b->gameNum].numHalfMoves-1].doublePawn );
  } else {
    sprintf( tmp, "-1 " );
  }
  strcat( bstring, tmp );
  sprintf( tmp, "%d %d %d %d %d ",
             !(b->wkmoved || b->wkrmoved),
             !(b->wkmoved || b->wqrmoved),
             !(b->bkmoved || b->bkrmoved),
             !(b->bkmoved || b->bqrmoved),
             (garray[b->gameNum].numHalfMoves - b->lastIrreversable) / 2 );
  strcat( bstring, tmp );
  sprintf( tmp, "%d %s %s %d %d %d %d %d %d %d %d %s (%s) %s\n",
              b->gameNum+1,
              parray[garray[b->gameNum].white].name,
              parray[garray[b->gameNum].black].name,
              myTurn,
              garray[b->gameNum].wInitTime / 600,
              garray[b->gameNum].wIncrement / 10,
              ws,
              bs,
              (wTime+5)/10,
              (bTime+5)/10,
              garray[b->gameNum].numHalfMoves/2+1,
              garray[b->gameNum].numHalfMoves ?
                ml[garray[b->gameNum].numHalfMoves-1].moveString :
                "none",
              garray[b->gameNum].numHalfMoves ?
                tenth_str( ml[garray[b->gameNum].numHalfMoves-1].tookTime,0) :
                "0:00",
              garray[b->gameNum].numHalfMoves ?
                 ml[garray[b->gameNum].numHalfMoves-1].algString :
                "none"
                );
  strcat( bstring, tmp );
  return 0;
}

PUBLIC int board_read_file( char *catagory, char *gname, game_state_t *gs )
{
  int f, r;
  FILE *fp;
  char fname[MAX_FILENAME_SIZE+1];
  int c;
  int onNewLine=1;
  int onColor= -1;
  int onPiece= -1;
  int onFile= -1;
  int onRank= -1;

  sprintf( fname, "%s/%s/%s", board_dir, catagory, gname );
  fp = fopen( fname, "r" );
  if (!fp) return 1;

  for (f=0;f<8;f++) for (r=0;r<8;r++) gs->board[f][r]=NOPIECE;
  for (f=0;f<2;f++) for (r=0;r<8;r++) gs->ep_possible[f][r]=0;
  gs->wkmoved = gs->wqrmoved = gs->wkrmoved = 0;
  gs->bkmoved = gs->bqrmoved = gs->bkrmoved = 0;
  gs->onMove = -1;
  gs->moveNum = 1;
  gs->lastIrreversable = -1;
  while (!feof(fp)) {
    c = fgetc(fp);
    if (onNewLine) {
      if (c == 'W') {
        onColor = WHITE;
        if (gs->onMove < 0) gs->onMove = WHITE;
      } else if (c == 'B') {
        onColor = BLACK;
        if (gs->onMove < 0) gs->onMove = BLACK;
      } else if (c == '#') {
        while (!feof(fp) && c != '\n') c=fgetc(fp); /* Comment line */
        continue;
      } else { /* Skip any line we don't understand */
        while (!feof(fp) && c != '\n') c=fgetc(fp);
        continue;
      }
      onNewLine = 0;
    } else {
      switch (c) {
        case 'P':
          onPiece = PAWN;
          break;
        case 'R':
          onPiece = ROOK;
          break;
        case 'N':
          onPiece = KNIGHT;
          break;
        case 'B':
          onPiece = BISHOP;
          break;
        case 'Q':
          onPiece = QUEEN;
          break;
        case 'K':
          onPiece = KING;
          break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
          onFile=c-'a';
          onRank = -1;
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
          onRank=c-'1';
          if (onFile >= 0 && onColor >= 0 && onPiece >= 0)
            gs->board[onFile][onRank] = onPiece | onColor;
          break;
        case '#':
          while (!feof(fp) && c != '\n') c=fgetc(fp); /* Comment line */
        case '\n':
          onNewLine=1;
          onColor = -1;
          onPiece = -1;
          onFile = -1;
          onRank = -1;
          break;
        default:
          break;
      }
    }
  }
  fclose(fp);
  return 0;
}

PRIVATE void place_piece( board_t b, int piece, int balanceBishop )
{
  int r, f;
  int placed=0;
  int oBish= -1;

  if (iscolor(piece, BLACK))
    r = 7;
  else
    r = 0;
  if (balanceBishop && piecetype(piece) == BISHOP) {
    for (f=0;f<8 && oBish == -1;f++) {
      if (piecetype((b)[f][r]) == BISHOP)
        oBish = f;
    }
    if (oBish == -1)
      balanceBishop = 0;
  } else
    balanceBishop = 0;
  while (!placed) {
    f = rand() % 8;
    if (balanceBishop) {
      if (oBish & 0x1 == f & 0x1) continue;
    }
    if ((b)[f][r] == NOPIECE) {
      (b)[f][r] = piece;
      placed = 1;
    }
  }
}

PUBLIC void wild_update( int style )
{
  int f, r;
  board_t b;

  for (f=0;f<8;f++) for (r=0;r<8;r++) b[f][r]=NOPIECE;
  for (f=0;f<8;f++) {
    b[f][1] = W_PAWN;
    b[f][6] = B_PAWN;
  }
  switch (style) {
    case 1:
      if (rand()&0x01) {
        b[4][0] = W_KING;
        b[3][0] = W_QUEEN;
      } else {
        b[3][0] = W_KING;
        b[4][0] = W_QUEEN;
      }
      if (rand()&0x01) {
        b[4][7] = B_KING;
        b[3][7] = B_QUEEN;
      } else {
        b[3][7] = B_KING;
        b[4][7] = B_QUEEN;
      }
      b[0][0] = b[7][0] = W_ROOK;
      b[0][7] = b[7][7] = B_ROOK;
      place_piece( b, W_BISHOP, 1 );
      place_piece( b, W_BISHOP, 1 );
      place_piece( b, W_KNIGHT, 1 );
      place_piece( b, W_KNIGHT, 1 );
      place_piece( b, B_BISHOP, 1 );
      place_piece( b, B_BISHOP, 1 );
      place_piece( b, B_KNIGHT, 1 );
      place_piece( b, B_KNIGHT, 1 );
      break;
    case 2:
      place_piece( b, W_KING, 0 );
      place_piece( b, W_QUEEN, 0 );
      place_piece( b, W_ROOK, 0 );
      place_piece( b, W_ROOK, 0 );
      place_piece( b, W_BISHOP, 0 );
      place_piece( b, W_BISHOP, 0 );
      place_piece( b, W_KNIGHT, 0 );
      place_piece( b, W_KNIGHT, 0 );
      b[0][7] = b[0][0] | BLACK;
      b[1][7] = b[1][0] | BLACK;
      b[2][7] = b[2][0] | BLACK;
      b[3][7] = b[3][0] | BLACK;
      b[4][7] = b[4][0] | BLACK;
      b[5][7] = b[5][0] | BLACK;
      b[6][7] = b[6][0] | BLACK;
      b[7][7] = b[7][0] | BLACK;
      break;
    case 3:
      place_piece( b, W_KING, 1 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      place_piece( b, (rand() % 4) + 2, 0 );
      b[0][7] = b[0][0] | BLACK;
      b[1][7] = b[1][0] | BLACK;
      b[2][7] = b[2][0] | BLACK;
      b[3][7] = b[3][0] | BLACK;
      b[4][7] = b[4][0] | BLACK;
      b[5][7] = b[5][0] | BLACK;
      b[6][7] = b[6][0] | BLACK;
      b[7][7] = b[7][0] | BLACK;
      break;
    case 4:
      place_piece( b, W_KING, 1 );
      place_piece( b, W_BISHOP, 1 );
      place_piece( b, W_BISHOP, 1 );
      place_piece( b, (rand() % 4) + 2, 1 );
      place_piece( b, (rand() % 4) + 2, 1 );
      place_piece( b, (rand() % 4) + 2, 1 );
      place_piece( b, (rand() % 4) + 2, 1 );
      place_piece( b, (rand() % 4) + 2, 1 );
      b[0][7] = b[0][0] | BLACK;
      b[1][7] = b[1][0] | BLACK;
      b[2][7] = b[2][0] | BLACK;
      b[3][7] = b[3][0] | BLACK;
      b[4][7] = b[4][0] | BLACK;
      b[5][7] = b[5][0] | BLACK;
      b[6][7] = b[6][0] | BLACK;
      b[7][7] = b[7][0] | BLACK;
      break;
    default:
      return;
      break;
  }
  {
  FILE *fp;
  char fname[MAX_FILENAME_SIZE + 1];
  int onPiece;

  sprintf( fname, "%s/wild/%d", board_dir, style );
  fp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Can't write file name %s\n", fname );
    return;
  }
  fprintf( fp, "W:" );
  onPiece = -1;
  for (r=1;r>=0;r--) {
    for (f=0;f<8;f++) {
      if (onPiece < 0 || b[f][r] != onPiece) {
        onPiece = b[f][r];
        fprintf( fp, " %s", wpstring[piecetype(b[f][r])] );
      }
      fprintf( fp, " %c%c", f+'a', r+'1' );
    }
  }
  fprintf( fp, "\nB:" );
  onPiece = -1;
  for (r=6;r<8;r++) {
    for (f=0;f<8;f++) {
      if (onPiece < 0 || b[f][r] != onPiece) {
        onPiece = b[f][r];
        fprintf( fp, " %s", wpstring[piecetype(b[f][r])] );
      }
      fprintf( fp, " %c%c", f+'a', r+'1' );
    }
  }
  fprintf( fp, "\n" );
  fclose(fp);
  }
}

PUBLIC void wild_init()
{
  wild_update( 1 );
  wild_update( 2 );
  wild_update( 3 );
  wild_update( 4 );
}
