/* tally.h
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

#ifndef _TALLY_H
#define _TALLY_H

#define IN_OTHER        0
#define IN_BOARD        1
#define IN_SHOUT        2
#define IN_TELL         3
#define IN_HELP         4
#define IN_INPUT        5
#define IN_MAX          IN_INPUT

extern void in_push();
extern void in_pop();
extern void in_reset();

extern void tally_init();
extern void tally_sent();
extern void tally_usage();
extern void tally_getusage();
extern void tally_getsent();

extern void tally_gotcommand();
extern void tall_getcommand();

#endif /* _TALLY_H */
