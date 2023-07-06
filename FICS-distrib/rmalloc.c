/* rmalloc.c
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

PUBLIC unsigned int allocated_size=0;
PUBLIC unsigned int malloc_count=0;
PUBLIC unsigned int free_count=0;

PUBLIC void *rmalloc( int byteSize )
{
  void *retval;
#ifdef HASMALLOCSIZE
  allocated_size += byteSize;
#endif
  malloc_count++;
  retval = malloc(byteSize);
  if (!retval) {
    fprintf( stderr, "FICS: Error, out of memory!\n" );
    abort();
  }
  return retval;
}

PUBLIC void *rrealloc( void *ptr, int byteSize )
{
  void *retval;
#ifdef HASMALLOCSIZE
  allocated_size += (byteSize - malloc_size(ptr));
#endif
  retval = realloc(ptr,byteSize);
  if (!retval) {
    fprintf( stderr, "FICS: Error, out of memory!\n" );
    abort();
  }
  return retval;
}

PUBLIC void rfree( void *ptr )
{
#ifdef HASMALLOCSIZE
  allocated_size = allocated_size - malloc_size(ptr);
#endif
  free_count++;
  free(ptr);
}
