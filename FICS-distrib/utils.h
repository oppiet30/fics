/* utils.h
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

#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>

#define MAX_WORD_SIZE 1024

/* Maximum length of an output line */
#define MAX_LINE_SIZE 1024

/* Maximum size of a filename */
#ifdef FILENAME_MAX
#  define MAX_FILENAME_SIZE FILENAME_MAX
#else
#  define MAX_FILENAME_SIZE 1024
#endif

extern int iswhitespace( );
extern char *getword( );
/* Returns a pointer to the first whitespace in the argument */
extern char *eatword( );
/* Returns a pointer to the first non-whitespace char in the argument */
extern char *eatwhite( );
/* Returns the next word in a given string >eatwhite(eatword(foo))< */
extern char *nextword( );

extern int mail_string_to_address();
extern int mail_string_to_user();
extern int pcommand();
extern int pprintf();
extern int pprintf_prompt();
extern int pprintf_noformat();
extern int psend_file( );
extern int psend_command( );

extern char *stolower( );

extern int safechar( );
extern int safestring( );
extern int alphastring( );
extern int printablestring( );
extern char *strdup( );

extern char *hms();
extern char *strltime();
extern char *strgtime();
extern unsigned tenth_secs();
extern char *tenth_str();

extern int truncate_file();
extern int lines_file();

extern int file_has_pname();
extern char *file_wplayer();
extern char *file_bplayer();

extern int available_space();
extern int file_exists();
extern char *ratstr();

#endif /* _UTILS_H */
