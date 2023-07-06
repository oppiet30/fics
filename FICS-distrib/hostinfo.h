/* hostinfo.h
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
   Richard Nash nash@visus.com  94/03/08        Created
*/

#ifndef _HOSTINFO_H
#define _HOSTINFO_H

typedef struct _hostinfo {
  char *email_address;
  char *secret_code;
  char *telnet_address;
  int port;
  int update_interval; /* In seconds */
  int last_update;
} hostinfo;

#define MAXNUMHOSTS 100
#define MAXNUMSERVERS 30

extern int numHosts;
extern hostinfo hArray[];
extern int iamserver;
extern FILE *mail_log_file;

extern int numServers;
extern char *serverNames[];
extern int serverPorts[];

extern int hostinfo_init();
extern int hostinfo_write();
extern int hostinfo_updatehosts();
extern int hostinfo_mailresults();
extern int hostinfo_checkcomfile();

#endif /* _HOSTINFO_H */
