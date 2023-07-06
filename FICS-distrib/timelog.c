/* timelog.c
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
#include "timelog.h"
#include "rmalloc.h"

/* Maximum should be atleast 3 */
PUBLIC timelog *tl_new(int maximum)
{
  timelog *tmp;

  tmp = (timelog *)rmalloc(sizeof(timelog));
  tmp->numEvents=0;
  tmp->events = NULL;
  tmp->tail = NULL;
  tmp->maxNumEvents = maximum;
  return tmp;
}

PUBLIC int tl_free( timelog *tl )
{
  t_event *tmp, *next;

  tmp = tl->events;
  while (tmp) {
    next = tmp->next;
    rfree(tmp);
    tmp = next;
  }
  rfree( tl );
  return 0;
}

PUBLIC int tl_logevent( timelog *tl, unsigned num)
{
  t_event *tmp, *tmp1;
  int now = time(0);

  tl->numEvents++;
  if (tl->numEvents == tl->maxNumEvents) { /* cut one off tail */
    tmp = tl->tail; /* Reuse rather than calling malloc again */
    tmp1 = tl->tail->previous;
    tmp1->next = NULL;
    tl->tail = tmp1;
    tl->numEvents--;
  } else {
    tmp = (t_event *)rmalloc(sizeof(t_event));
  }
  tmp->atTime = now;
  tmp->howMany = num;

  tmp->previous = NULL;
  tmp->next = tl->events;
  if (tl->events)
    tl->events->previous = tmp;
  tl->events = tmp;
  if (!tl->tail) {
    tl->tail = tmp;
  }
  return 0;
}

PUBLIC unsigned int tl_numbetween(timelog *tl, int start, int end)
{
  t_event *tmp;
  unsigned total=0;

  for (tmp=tl->events;tmp;tmp=tmp->next) {
    if (tmp->atTime < start) break;
    if (tmp->atTime <= end) {
      total += tmp->howMany;
    }
  }
  return total;
}

PUBLIC unsigned int tl_numsince(timelog *tl, int start)
{
  int now=time(0);
  return tl_numbetween(tl,start,now);
}

PUBLIC unsigned int tl_numinlast(timelog *tl, int seconds)
{
  int now=time(0);
  int then=now-seconds;
  return tl_numbetween(tl,then,now);
}
