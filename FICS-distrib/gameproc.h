/* gameproc.h
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

#ifndef _GAMEPROC_H
#define _GAMEPROC_H

extern void game_ended();
extern void process_move();
extern int com_resign();
extern int com_draw();
extern int com_pause();
extern int com_unpause();
extern int com_abort();
extern int com_games();
extern int com_observe();
extern int com_allobservers();
extern int com_moves();
extern int com_mailmoves();
extern int com_oldmoves();
extern int com_mailoldmoves();
extern int com_courtesyabort();
extern int com_load();
extern int com_stored();
extern int com_adjourn();
extern int com_flag();
extern int com_smoves();
extern int com_sposition();
extern int com_takeback();
extern int com_switch();
extern int com_history();
extern int com_time();
extern int com_boards();

extern int com_simmatch();
extern int com_simnext();
extern int com_simgames();
extern int com_simpass();
extern int com_simabort();

#endif /* _GAMEPROC_H */
