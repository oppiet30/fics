/* multicol.c
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
#include "multicol.h"
#include "utils.h"
#include "rmalloc.h"

PUBLIC multicol *multicol_start( int maxArray )
{
  int i;
  multicol *m;

  m = (multicol *)rmalloc( sizeof(multicol) );
  m->arraySize = maxArray;
  m->num = 0;
  m->strArray = (char **)rmalloc( sizeof(char *) * m->arraySize );
  for (i=0;i<m->arraySize;i++)
    m->strArray[i] = NULL;
  return m;
}

PUBLIC int multicol_store( multicol *m, char *str )
{
  if (m->num >= m->arraySize)
    return -1;
  if (!str)
    return -1;
  m->strArray[m->num] = strdup(str);
  m->num++;
  return 0;
}

PUBLIC int multicol_pprint( multicol *m, int player, int cols, int space )
{
  int i;
  int maxWidth=0;
  int numPerLine;
  int numLines;
  int on, theone, len;
  int done;

  for (i=0;i<m->num;i++)
    if (strlen(m->strArray[i]) > maxWidth)
      maxWidth = strlen(m->strArray[i]);
  maxWidth += space;
  numPerLine = cols / maxWidth;
  numLines = m->num / numPerLine;
  if (numLines * numPerLine < m->num)
    numLines++;
  on = 0;
  done = 0;
  while (!done) {
    for (i=0;i<numPerLine;i++) {
      theone = on + numLines * i;
      if (theone >= m->num) {
        break;
      }
      len = maxWidth - strlen(m->strArray[theone]);
      if (i==numPerLine-1)
        len -= space;
      pprintf( player, "%s", m->strArray[theone] );
      while (len) {
        pprintf( player, " " );
        len--;
      }
    }
    pprintf( player, "\n" );
    on += 1;
    if (on >= numLines)
      break;
  }
  return 0;
}

PUBLIC int multicol_end( multicol *m )
{
  int i;
  for (i=0;i<m->num;i++)
    rfree(m->strArray[i]);
  rfree(m);
  return 0;
}
