/* timelog.h
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

#ifndef _TIMELOG_H
#define _TIMELOG_H

typedef struct s_t_event{
  int atTime;
  unsigned int howMany;
  struct s_t_event *next;
  struct s_t_event *previous;
} t_event;

typedef struct s_timelog {
  t_event *events, *tail;
  int numEvents;
  int maxNumEvents;
} timelog;

extern timelog *tl_new();
extern int tl_free();

extern int tl_logevent();
extern unsigned int tl_numbetween();
extern unsigned int tl_numsince();
extern unsigned int tl_numinlast();

#endif /* _TIMELOG_H */
