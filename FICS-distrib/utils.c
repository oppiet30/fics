/* utils.c
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
#include "utils.h"
#include "playerdb.h"
#include "network.h"
#include "rmalloc.h"
#include "fics_config.h"

PUBLIC int iswhitespace( int c )
{
    if ((c < ' ') || (c == '\b') || (c == '\n') ||
        (c == '\t') || (c == ' ')) { /* white */
        return 1;
    } else {
        return 0;
    }
}

PUBLIC char *getword( char *str )
{
  int i;
  static char word[MAX_WORD_SIZE];

  i = 0;
  while (*str && !iswhitespace(*str)) {
    word[i] = *str;
    str++; i++;
    if (i == MAX_WORD_SIZE) {
      i = i - 1;
      break;
    }
  }
  word[i] = '\0';
  return word;
}

PUBLIC char *eatword( char *str )
{
  while (*str && !iswhitespace(*str)) str++;
  return str;
}

PUBLIC char *eatwhite( char *str )
{
  while (*str && iswhitespace(*str)) str++;
  return str;
}

PUBLIC char *nextword( char *str )
{
  return eatwhite(eatword(str));
}

PUBLIC int mail_string_to_address( char *addr, char *subj, char *str )
{
  char com[1000];
  FILE *fp;

  sprintf( com, "%s -s \"%s\" %s", MAILPROGRAM, subj, addr );
  fp = popen( com, "w" );
  if (!fp) return -1;
  fprintf( fp, "%s", str );
  pclose(fp);
  return 0;
}

PUBLIC int mail_string_to_user( int p, char *str )
{

  if (parray[p].emailAddress &&
      parray[p].emailAddress[0] &&
      safestring(parray[p].emailAddress) ) {
    return mail_string_to_address( parray[p].emailAddress, "FICS game report", str );
  } else {
    return -1;
  }
}

/* Process a command for a user */
PUBLIC int pcommand( int p, char *comstr, int v1, int v2, int v3, int v4,
                    int v5, int v6, int v7, int v8, int v9, int v10,
                    int v11, int v12, int v13, int v14, int v15, int v16,
                    int v17, int v18, int v19, int v20 )
{
  char tmp[MAX_LINE_SIZE];
  int retval;
  int current_socket=parray[p].socket;

  sprintf( tmp, comstr, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
                    v11, v12, v13, v14, v15, v16, v17, v18, v19, v20 );
  retval = process_input( current_socket, tmp );
  if (retval == COM_LOGOUT) {
    process_disconnection( current_socket );
    net_close_connection(current_socket);
  }
  return retval;
}

PUBLIC int pprintf( int p, char *format, int v1, int v2, int v3, int v4,
                    int v5, int v6, int v7, int v8, int v9, int v10,
                    int v11, int v12, int v13, int v14, int v15, int v16,
                    int v17, int v18, int v19, int v20 )
{
  char tmp[10 * MAX_LINE_SIZE]; /* Make sure you can handle 10 lines worth of stuff */
  int retval;

  retval = sprintf( tmp, format, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
                    v11, v12, v13, v14, v15, v16, v17, v18, v19, v20 );
  if (strlen( tmp) > 10 * MAX_LINE_SIZE) {
    fprintf( stderr, "FICS: pprintf buffer overflow\n" );
  }
  net_send_string( parray[p].socket, tmp, 1 );
  return retval;
}

PUBLIC int pprintf_prompt( int p, char *format, int v1, int v2, int v3, int v4,
                    int v5, int v6, int v7, int v8, int v9, int v10,
                    int v11, int v12, int v13, int v14, int v15, int v16,
                    int v17, int v18, int v19, int v20 )
{
  char tmp[10 * MAX_LINE_SIZE]; /* Make sure you can handle 10 lines worth of stuff */
  int retval;

  retval = sprintf( tmp, format, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
                    v11, v12, v13, v14, v15, v16, v17, v18, v19, v20 );
  if (strlen( tmp) > 10 * MAX_LINE_SIZE) {
    fprintf( stderr, "FICS: pprintf_prompt buffer overflow\n" );
  }
  net_send_string( parray[p].socket, tmp, 1 );
  net_send_string( parray[p].socket, parray[p].prompt, 1 );
  return retval;
}

PUBLIC int pprintf_noformat( int p, char *format, int v1, int v2, int v3,
                    int v4, int v5, int v6, int v7, int v8, int v9, int v10,
                    int v11, int v12, int v13, int v14, int v15, int v16,
                    int v17, int v18, int v19, int v20 )
{
  char tmp[10 * MAX_LINE_SIZE]; /* Make sure you can handle 10 lines worth of stuff */
  int retval;

  retval = sprintf( tmp, format, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
                    v11, v12, v13, v14, v15, v16, v17, v18, v19, v20 );
  if (strlen( tmp) > 10 * MAX_LINE_SIZE) {
    fprintf( stderr, "FICS: pprintf_noformat buffer overflow\n" );
  }
  net_send_string( parray[p].socket, tmp, 0 );
  return retval;
}

PUBLIC int psend_file( int p, char *dir, char *file )
{
  FILE *fp;
  char tmp[MAX_LINE_SIZE];
  char fname[MAX_FILENAME_SIZE];
  int num;

  if (dir)
    sprintf( fname, "%s/%s", dir, file );
  else
    strcpy( fname, file );
  fp = fopen( fname, "r" );
  if (!fp) return -1;
  while (!feof(fp)) {
    num = fread( tmp, sizeof(char), MAX_LINE_SIZE-1, fp );
    tmp[num] = '\0';
    net_send_string( parray[p].socket, tmp, 1 );
  }
  fclose(fp);
  return 0;
}

PUBLIC int psend_command( int p, char *command, char *input )
{
  FILE *fp;
  char tmp[MAX_LINE_SIZE];
  int num;

  if (input)
    fp = popen( command, "w" );
  else
    fp = popen( command, "r" );
  if (!fp) return -1;
  if (input) {
    fwrite( input, sizeof(char), strlen(input), fp );
  } else {
    while (!feof(fp)) {
      num = fread( tmp, sizeof(char), MAX_LINE_SIZE-1, fp );
      tmp[num] = '\0';
      net_send_string( parray[p].socket, tmp, 1 );
    }
  }
  fclose(fp);
  return 0;
}

PUBLIC char *stolower( char *str )
{
  int i;

  if (!str) return NULL;
  for (i=0; str[i]; i++) {
    if (isupper(str[i])) {
      str[i] = tolower(str[i]);
    }
  }
  return str;
}

PUBLIC int safechar( int c )
{
  if ((c == '>') || (c == '!') || (c == '&') || (c == '*') || (c == '?') ||
      (c == '/') || (c == '<') || (c == '|') || (c == '`') || (c == '$'))
    return 0;
  return 1;
}

PUBLIC int safestring( char *str )
{
  int i;

  if (!str) return 1;
  for (i=0; str[i]; i++) {
    if (!safechar(str[i])) return 0;
  }
  return 1;
}

PUBLIC int alphastring( char *str )
{
  int i;

  if (!str) return 1;
  for (i=0; str[i]; i++) {
    if (!isalpha(str[i])) {
      return 0;
    }
  }
  return 1;
}

PUBLIC int printablestring( char *str )
{
  int i;

  if (!str) return 1;
  for (i=0; str[i]; i++) {
    if ((!isprint(str[i])) && (str[i] != '\t') && (str[i] != '\n')
        && (str[i] != '\t')) return 0;
  }
  return 1;
}

PUBLIC char *strdup( char *str )
{
  char *tmp;

  if (!str) return NULL;
  tmp = rmalloc(strlen(str)+1);
  strcpy( tmp, str );
  return tmp;
}

PUBLIC char *hms( int t, int showhour, int showseconds, int spaces )
{
  static char tstr[20];
  char tmp[10];
  int h, m, s;

  h = t / 3600;
  t = t % 3600;
  m = t / 60;
  s = t % 60;
  if (h || showhour) {
    if (spaces)
      sprintf( tstr, "%d : %02d", h, m );
    else
      sprintf( tstr, "%d:%02d", h, m );
  } else {
    sprintf( tstr, "%d", m );
  }
  if (showseconds) {
    if (spaces)
      sprintf( tmp, " : %02d", s );
    else
      sprintf( tmp, ":%02d", s );
    strcat( tstr, tmp );
  }
  return tstr;
}


PRIVATE char *dayarray[] =
  {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
   "Saturday" };

PRIVATE char *montharray[] =
  {"January", "February", "March", "April", "May", "June", "July", "August",
   "September", "October", "November", "December" };

PRIVATE char *strtime( struct tm * stm )
{
  static char tstr[100];
  int hour;
  char PA;

  hour = stm->tm_hour;
  if (hour < 12) {
    PA = 'A';
  } else {
    PA = 'P';
    if (hour != 12)
      hour = hour - 12;
  }
#if defined(SYSTEM_USL)
  /* USL doesn't have timezone? */
  sprintf( tstr, "%s %s %d, %d:%02d %cM GMT+0800",
           dayarray[ stm->tm_wday ],
           montharray[ stm->tm_mon ],
           stm->tm_mday,
           hour,
           stm->tm_min,
           PA );
#else
  sprintf( tstr, "%s %s %d, %d:%02d %cM %s",
           dayarray[ stm->tm_wday ],
           montharray[ stm->tm_mon ],
           stm->tm_mday,
           hour,
           stm->tm_min,
           PA,
           stm->tm_zone );
#endif
  return tstr;
}

PUBLIC char *strltime( int *clock )
{
  struct tm *stm=localtime( clock );
  return strtime(stm);
}

PUBLIC char *strgtime( int *clock )
{
  struct tm *stm=gmtime( clock );
  return strtime(stm);
}

/* This is used only for relative timeing since it reports seconds since
 * about 5:00pm on Feb 16, 1994
 */
PUBLIC unsigned tenth_secs()
{
  struct timeval tp;
  struct timezone tzp;

  gettimeofday(&tp, &tzp);
/* .1 seconds since 1970 almost fills a 32 bit int! So lets subtract off
 * the time right now */
  return ((tp.tv_sec-331939277) * 10) + (tp.tv_usec / 100000);
}

PUBLIC char *tenth_str( unsigned t, int spaces )
{
  return hms((t+5)/10, 0, 1, spaces); /* Round it */
}

#define MAX_TRUNC_SIZE 100

/* Warning, if lines in the file are greater than 1024 bytes in length, this
   won't work! */
PUBLIC int truncate_file( char *file, int lines )
{
  FILE *fp;
  int bptr=0, trunc=0, i;
  char tBuf[MAX_TRUNC_SIZE][MAX_LINE_SIZE];

  if (lines > MAX_TRUNC_SIZE) lines = MAX_TRUNC_SIZE;
  fp = fopen( file, "r+" );
  if (!fp) return 1;
  while (!feof(fp)) {
    fgets(tBuf[bptr], MAX_LINE_SIZE, fp);
    if (feof(fp)) break;
    if (tBuf[bptr][strlen(tBuf[bptr])-1] != '\n') { /* Line too long */
      fclose(fp);
      return -1;
    }
    bptr++;
    if (bptr == lines) {
      trunc = 1;
      bptr = 0;
    }
  }
  if (trunc) {
    fseek( fp, 0, SEEK_SET);
    ftruncate(fileno(fp), 0);
    for (i=0;i<lines;i++) {
      fputs( tBuf[bptr], fp );
      bptr++;
      if (bptr == lines) {
        bptr = 0;
      }
    }
  }
  fclose(fp);
  return 0;
}

/* Warning, if lines in the file are greater than 1024 bytes in length, this
   won't work! */
PUBLIC int lines_file( char *file )
{
  FILE *fp;
  int lcount=0;
  char tmp[MAX_LINE_SIZE];

  fp = fopen( file, "r" );
  if (!fp) return 0;
  while (!feof(fp)) {
    if (fgets(tmp, MAX_LINE_SIZE, fp))
      lcount++;
  }
  fclose(fp);
  return lcount;
}

PUBLIC int file_has_pname(char *fname, char *plogin)
{
  if (!strcmp(file_wplayer(fname),plogin)) return 1;
  if (!strcmp(file_bplayer(fname),plogin)) return 1;
  return 0;
}

PUBLIC char *file_wplayer(char *fname)
{
  static char tmp[MAX_FILENAME_SIZE];
  char *ptr;
  strcpy( tmp, fname );
  ptr = rindex(tmp, '-');
  if (!ptr) return "";
  *ptr = '\0';
  return tmp;
}

PUBLIC char *file_bplayer(char *fname)
{
  char *ptr;

  ptr = rindex(fname, '-');
  if (!ptr) return "";
  return ptr+1;
}

PUBLIC char *dotQuad(unsigned int a)
{
  static char tmp[20];

#ifdef LITTLE_ENDIAN
  sprintf( tmp, "%d.%d.%d.%d", (a & 0xff),
                               (a & 0xff00) >> 8,
                               (a & 0xff0000) >> 16,
                               (a & 0xff000000) >> 24 );
#else
  sprintf( tmp, "%d.%d.%d.%d", (a & 0xff000000) >> 24,
                               (a & 0xff0000) >> 16,
                               (a & 0xff00) >> 8,
                               (a & 0xff) );
#endif
  return tmp;
}

PUBLIC int available_space()
{
#if defined(SYSTEM_NEXT)
    struct statfs buf;

    statfs( player_dir, &buf );
    return( (buf.f_bsize * buf.f_bavail) );
#else
  return 100000000; /* Infinite space */
#endif
}

PUBLIC int file_exists(char *fname )
{
  FILE *fp;

  fp = fopen(fname,"r");
  if (!fp) return 0;
  fclose(fp);
  return 1;
}

PUBLIC char *ratstr( int rat )
{
  static int on=0;
  static char tmp[10][10];

  if (rat) {
    sprintf( tmp[on], "%d", rat );
  } else {
    sprintf( tmp[on], "----" );
  }
  on++;
  return tmp[on-1];
}
