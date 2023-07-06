/* ratings.h
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

#ifndef _RATINGS_H
#define _RATINGS_H

#define RESULT_WIN 0
#define RESULT_DRAW 1
#define RESULT_LOSS 2
#define RESULT_ABORT 3

#define MAX_BEST 20

extern void rating_init();
extern void rating_recalc();
extern int rating_delta();
extern int rating_update();
extern int rating_assess();

extern int com_assess();
extern int com_best();
extern int com_statistics();

extern void save_ratings();
extern void BestUpdate();
extern void rating_remove();
PUBLIC void rating_add();

#endif /* _RATINGS_H */
