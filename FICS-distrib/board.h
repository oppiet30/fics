/* board.h
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

#ifndef _BOARD_H
#define _BOARD_H

#define WHITE 0x00
#define BLACK 0x80
#define CString( c ) (((c) == WHITE) ? "White" : "Black" )
#define CToggle( c ) (((c) == BLACK) ? WHITE : BLACK )

/* These are indexes into an array so their values are not arbitrary */
#define NOPIECE 0x00
#define PAWN 0x01
#define KNIGHT 0x02
#define BISHOP 0x03
#define ROOK 0x04
#define QUEEN 0x05
#define KING 0x06

#define MAX_BOARD_STRING_LEGTH 1280     /* Abitrarily 80 * 16 */
#define MAX_STYLES 12

#define W_PAWN (PAWN | WHITE)
#define W_KNIGHT (KNIGHT | WHITE)
#define W_BISHOP (BISHOP | WHITE)
#define W_ROOK (ROOK | WHITE)
#define W_QUEEN (QUEEN | WHITE)
#define W_KING (KING | WHITE)

#define B_PAWN (PAWN | BLACK)
#define B_KNIGHT (KNIGHT | BLACK)
#define B_BISHOP (BISHOP | BLACK)
#define B_ROOK (ROOK | BLACK)
#define B_QUEEN (QUEEN | BLACK)
#define B_KING (KING | BLACK)

#define isblack(p) ((p) & BLACK)
#define iswhite(p) (!isblack(p))
#define iscolor(p,color) (((p) & BLACK) == (color))
#define piecetype(p) ((p) & 0x7f)
#define colorval(p) ((p) & 0x80)
#define square_color(r,f) ((((r)+(f)) & 0x01) ? BLACK : WHITE)

extern int pieceValues[7];

/* Treated at [rank][file] */
typedef int board_t[8][8];

typedef struct _game_state_t {
  board_t board;
  /* For castling */
  unsigned char wkmoved, wqrmoved, wkrmoved;
  unsigned char bkmoved, bqrmoved, bkrmoved;
  /* for ep */
  int ep_possible[2][8];
  /* For draws */
  int lastIrreversable;
  int onMove;
  int moveNum;
  /* Game num not saved, must be restored when read */
  int gameNum;
} game_state_t;

typedef struct _move_t {
  int color;
  int fromFile, fromRank;
  int toFile, toRank;
  int pieceCaptured;
  int piecePromotionTo;
  int enPassant; /* 0 = no, 1=higher -1= lower */
  int doublePawn; /* Only used for board display */
  char moveString[8];
  char algString[8];
  unsigned atTime;
  unsigned tookTime;
} move_t;

#define MoveToHalfMove( gs ) ((((gs)->moveNum - 1) * 2) + (((gs)->onMove == WHITE) ? 0 : 1))


extern int board_init();
extern void board_calc_strength();
extern char *board_to_string( );
extern char *wpstring[];
extern char *bpstring[];
extern char *move_and_time();
extern int board_read_file();
extern void wild_update();
extern void wild_init();

#endif
