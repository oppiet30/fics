/* algcheck.c
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
#include "algcheck.h"
#include "movecheck.h"
#include "board.h"
#include "utils.h"

/* Well, lets see if I can list the possibilities
 * Piece moves
 * Ne4
 * Nxe4
 * Nce4
 * Ncxe4
 * R2f3
 * R2xf3
 * Special pawn moves
 * e4
 * ed
 * exd
 * exd5
 * ed5
 * (o-o, o-o-o) Castling is handled earlier, so don't worry about that
 * Of course any of these can have a + or ++ or = string on the end, just
 * cut that off.
 */

/* f - file
 * r - rank
 * p - piece
 * x - x
 */
char *alg_list[]= {
        "fxfr", "pxfr", /* These two get confused in case of bishop */
        "ffr", "pfr", /* These two get confused in case of bishop */
        "pffr",
        "pfxfr",
        "prfr",
        "prxfr",
        "fr",
        "ff",
        "fxf",
        NULL
};

#define ALG_UNKNOWN -1

PRIVATE int get_move_info( char *str, int *piece, int *ff, int *fr, int *tf, int *tr, int *bishconfusion )
{
  char tmp[1024];
  char *s;
  int i, j, len;
  char c;
  int matchVal= -1;
  int lpiece, lff, lfr, ltf, ltr;

  *bishconfusion = 0;
  strcpy( tmp, str );
  if ((s=index(tmp, '+'))) { /* Cut off any check marks */
    *s = '\0';
  }
  if ((s=index(tmp, '='))) { /* Cut off any promotion marks */
    *s = '\0';
  }
  *piece = *ff = *fr = *tf = *tr = ALG_UNKNOWN;
  len = strlen(tmp);
  for (i=0;alg_list[i];i++) {
    lpiece = lff = lfr = ltf = ltr = ALG_UNKNOWN;
    if (strlen(alg_list[i]) != len) continue;
    for (j=len-1;j>=0;j--) {
      switch (alg_list[i][j]) {
        case 'f':
          if ((tmp[j] < 'a') || (tmp[j] > 'h')) goto nomatch;
          if (ltf == ALG_UNKNOWN)
            ltf = tmp[j] - 'a';
          else
            lff = tmp[j] - 'a';
          break;
        case 'r':
          if ((tmp[j] < '1') || (tmp[j] > '8')) goto nomatch;
          if (ltr == ALG_UNKNOWN)
            ltr = tmp[j] - '1';
          else
            lfr = tmp[j] - '1';
          break;
        case 'p':
          if (isupper(tmp[j]))
            c = tolower(tmp[j]);
          else
            c = tmp[j];
          if (c == 'k')
            lpiece = KING;
          else if (c == 'q')
            lpiece = QUEEN;
          else if (c == 'r')
            lpiece = ROOK;
          else if (c == 'b')
            lpiece = BISHOP;
          else if (c == 'n')
            lpiece = KNIGHT;
          else if (c == 'p')
            lpiece = PAWN;
          else goto nomatch;
          break;
        case 'x':
          if ((tmp[j] != 'x') && (tmp[j] != 'X')) goto nomatch;
          break;
        default:
          fprintf( stderr, "Unknown character in algebraic parsing\n" );
          break;
      }
    }
    if (lpiece == ALG_UNKNOWN)
      lpiece = PAWN;
   if (lpiece == PAWN && (lfr == ALG_UNKNOWN)) { /* ffr or ff */
     if (lff != ALG_UNKNOWN) {
        if (lff == ltf) goto nomatch;
        if ((lff - ltf != 1) && (ltf - lff != 1)) goto nomatch;
     }
   }
   *piece = lpiece;  /* We have a match */
   *tf = ltf;
   *tr = ltr;
   *ff = lff;
   *fr = lfr;
    if (matchVal != -1) {
      /* We have two matches, it must be that Bxc4 vs. bxc4 problem */
      /* Or it could be the Bc4 vs bc4 problem */
      *bishconfusion = 1;
    }
    matchVal = i;
nomatch:;
  }
  if (matchVal != -1)
    return MS_ALG;
  else
    return MS_NOTMOVE;
}

PUBLIC int alg_is_move( char *mstr )
{
  int piece, ff, fr, tf, tr, bc;

  return get_move_info( mstr, &piece, &ff, &fr, &tf, &tr, &bc );
}

/* We already know it is algebraic, get the move squares */
PUBLIC int alg_parse_move( char *mstr, game_state_t *gs, move_t *mt )
{
  int f, r, tmpr, posf, posr, posr2;
  int piece, ff, fr, tf, tr, bc;
  int bishf=0, bishr=0, pawnf=0, pawnr=0;

  if (get_move_info( mstr, &piece, &ff, &fr, &tf, &tr, &bc ) != MS_ALG) {
    fprintf( stderr, "FICS: Shouldn't try to algebraicly parse non-algabraic move string.\n" );
    return MOVE_ILLEGAL;
  }
  /* Resolve ambiguities in to-ness */
  if (tf == ALG_UNKNOWN) return MOVE_AMBIGUOUS; /* Must always know to file */
  if (tr == ALG_UNKNOWN) {
    posr = posr2 = ALG_UNKNOWN;
    if (piece != PAWN) return MOVE_AMBIGUOUS;
    if (ff == ALG_UNKNOWN) return MOVE_AMBIGUOUS;
    /* Need to find pawn on ff that can take to tf and fill in ranks */
    for (InitPieceLoop(gs->board,&f,&r, gs->onMove );
          NextPieceLoop(gs->board,&f,&r, gs->onMove ); ) {
      if ((ff != ALG_UNKNOWN) && (ff != f)) continue;
      if (piecetype(gs->board[f][r]) != piece) continue;
      if (gs->onMove == WHITE) {
        tmpr = r+1;
      } else {
        tmpr = r-1;
      }
      if ((gs->board[tf][tmpr] == NOPIECE) ||
          (iscolor(gs->board[tf][tmpr], gs->onMove))) continue;
      if (legal_andcheck_move( gs, f, r, tf, tmpr )) {
        if ((posr != ALG_UNKNOWN) && (posr2 != ALG_UNKNOWN))
          return MOVE_AMBIGUOUS;
        posr = tmpr;
        posr2 = r;
      }
    }
    tr = posr;
    fr = posr2;
  } else if (bc) { /* Could be bxc4 or Bxc4 | bc4 or Bc4, tr is known */
    posf = ALG_UNKNOWN;
    posr = ALG_UNKNOWN;
    for (InitPieceLoop(gs->board,&f,&r, gs->onMove );
          NextPieceLoop(gs->board,&f,&r, gs->onMove ); ) {
      if (piecetype(gs->board[f][r]) == PAWN) {
      } else if (piecetype(gs->board[f][r]) == BISHOP) {
      } else
        continue;
      if (legal_andcheck_move( gs, f, r, tf, tr )) {
        if (piecetype(gs->board[f][r]) == PAWN) {
          if (f != 1) continue; /* B-file */
          pawnf = f;
          pawnr = r;
        } else if (piecetype(gs->board[f][r]) == BISHOP) {
          bishf = f;
          bishr = r;
        } else
          continue;
        if ((posf != ALG_UNKNOWN) && (posr != ALG_UNKNOWN)) {
          if (mstr[0] == 'B') {
            posf = bishf;
            posr = bishr;
          } else { /* Little 'b' */
            posf = pawnf;
            posr = pawnr;
          }
        } else {
          posf = f;
          posr = r;
        }
      }
    }
    ff = posf;
    fr = posr;
  } else { /* The from position is unknown */
    posf = ALG_UNKNOWN;
    posr = ALG_UNKNOWN;
    if ((ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN)) {
      /* Need to find a piece that can go to tf, tr */
      for (InitPieceLoop(gs->board,&f,&r, gs->onMove );
            NextPieceLoop(gs->board,&f,&r, gs->onMove ); ) {
        if ((ff != ALG_UNKNOWN) && (ff != f)) continue;
        if ((fr != ALG_UNKNOWN) && (fr != r)) continue;
        if (piecetype(gs->board[f][r]) != piece) continue;
        if (legal_andcheck_move( gs, f, r, tf, tr )) {
          if ((posf != ALG_UNKNOWN) && (posr != ALG_UNKNOWN))
            return MOVE_AMBIGUOUS;
          posf = f;
          posr = r;
        }
      }
    }
    ff = posf;
    fr = posr;
  }
  if ((tf == ALG_UNKNOWN) || (tr == ALG_UNKNOWN) ||
      (ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN))
    return MOVE_ILLEGAL;
  mt->fromFile = ff;
  mt->fromRank = fr;
  mt->toFile = tf;
  mt->toRank = tr;
  return MOVE_OK;
}

/* A assumes the move has yet to be made on the board */
PUBLIC char *alg_unparse( game_state_t *gs, move_t *mt )
{
  static char mStr[20];

  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
       ((mt->fromFile == 4) && (mt->toFile == 6)) )
    return "o-o";
  if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
       ((mt->fromFile == 4) && (mt->toFile == 2)) )
    return "o-o-o";

  sprintf( mStr, "%c%d%c%d", mt->fromFile+'a', mt->fromRank+1,
                                       mt->toFile+'a', mt->toRank+1 );
  return mStr;
}
