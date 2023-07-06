/* movecheck.h
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

#ifndef _MOVECHECK_H
#define _MOVECHECK_H

#define MOVE_OK 0
#define MOVE_ILLEGAL 1
#define MOVE_STALEMATE 2
#define MOVE_CHECKMATE 3
#define MOVE_AMBIGUOUS 4

#define MS_NOTMOVE 0
#define MS_COMP 1
#define MS_COMPDASH 2
#define MS_ALG 3
#define MS_KCASTLE 4
#define MS_QCASTLE 5

#define isrank(c) (((c) <= '8') && ((c) >= '1'))
#define isfile(c) (((c) >= 'a') && ((c) <= 'h'))
extern int is_move();
extern int parse_move();
extern int execute_move();
extern int backup_move();

/* Some useful chess utilities */
extern int NextPieceLoop();
extern int InitPieceLoop();
extern int legal_move();
extern int legal_andcheck_move();
extern int in_check();

#endif /* _MOVECHECK_H */
