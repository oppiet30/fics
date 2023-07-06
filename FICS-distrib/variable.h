/* variable.h
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

#ifndef _VARIABLE_H
#define _VARIABLE_H

typedef struct _var_list {
  char *name;
  int (*var_func)();
} var_list;

#define VAR_OK 0
#define VAR_NOSUCH 1
#define VAR_BADVAL 2
#define VAR_AMBIGUOUS 3

extern int var_set( );

extern var_list variables[];

#endif /* _VARIABLE_H */
