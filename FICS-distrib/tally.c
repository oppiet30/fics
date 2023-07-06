/* tally.c
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
   Richard Nash nash@visus.com  94/03/18        Created
*/

#include "stdinclude.h"
#include "common.h"
#include "tally.h"
#include "timelog.h"
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_STATE_STACK  10
PRIVATE int state_stack[MAX_STATE_STACK];
PRIVATE int num_stack=0;

PRIVATE unsigned bytes_sent=0;
PRIVATE unsigned bytes_broken[IN_MAX+1];
PRIVATE timelog *bytes_log;

PRIVATE timelog *uusage_log;
PRIVATE unsigned long last_u_usage=0;
PRIVATE timelog *susage_log;
PRIVATE unsigned long last_s_usage=0;

PRIVATE timelog *command_log;
PRIVATE unsigned long command_total=0;

/* Returns in 1/100ths of a second */
PRIVATE double getutime()
{
    struct rusage ru;

    getrusage(0, &ru);
    return ru.ru_utime.tv_sec*100.0 + ru.ru_utime.tv_usec/10000.0;
}

PRIVATE double getstime()
{
    struct rusage ru;

    getrusage(0, &ru);
    return ru.ru_stime.tv_sec*100.0 + ru.ru_stime.tv_usec/10000.0;
}

PUBLIC void in_push( int state )
{
  if (num_stack == MAX_STATE_STACK) {
    fprintf( stderr, "FICS:State stack full?\n" );
    return;
  }
  state_stack[num_stack] = state;
  num_stack++;
}

PUBLIC void in_pop()
{
  if (num_stack <= 0) return;
  num_stack--;
}

PUBLIC void in_reset()
{
  num_stack = 0;
}

PUBLIC void tally_init()
{
  int i;
  for (i=0;i<=IN_MAX;i++)
    bytes_broken[i] = 0;
  bytes_log = tl_new(5000);
  uusage_log = tl_new(500);
  susage_log = tl_new(500);
  command_log = tl_new(500);
}

PUBLIC void tally_sent( int num )
{
  int in_state;

  if (num <= 0) return;
  if (num_stack)
    in_state = state_stack[num_stack-1];
  else
    in_state = IN_OTHER;
  tl_logevent( bytes_log, num);
  bytes_broken[in_state] += num;
  bytes_sent += num;
}

PUBLIC void tally_usage()
{
  unsigned long utime = getutime();
  unsigned long stime = getstime();

  tl_logevent( uusage_log, utime - last_u_usage);
  last_u_usage = utime;
  tl_logevent( susage_log, stime - last_s_usage);
  last_s_usage = stime;
}

PUBLIC void tally_getusage( unsigned long *total, unsigned long *last1,
                            unsigned long *last5 )
{
  *total = getutime() + getstime();
  *last1 = tl_numinlast(uusage_log,60)+tl_numinlast(susage_log,60);
  *last5 = tl_numinlast(uusage_log,300)+tl_numinlast(susage_log,300);
}

PUBLIC void tally_getsent( unsigned long *total, unsigned long *last1,
                            unsigned long *last5,
                            unsigned long *broken )
{
  int i;

  *total = bytes_sent;
  *last1 = tl_numinlast(bytes_log,60);
  *last5 = tl_numinlast(bytes_log,300);
  for (i=0;i<=IN_MAX;i++) {
    broken[i] = bytes_broken[i];
  }
}

PUBLIC void tally_gotcommand()
{
  tl_logevent( command_log, 1);
  command_total++;
}

PUBLIC void tall_getcommand( unsigned long *total, unsigned long *last1,
                             unsigned long *last5 )
{
  *total = command_total;
  *last1 = tl_numinlast(command_log,60);
  *last5 = tl_numinlast(command_log,300);
}
