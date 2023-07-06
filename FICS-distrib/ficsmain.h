/* ficsmain.h
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

#ifndef _FICEMAIN_H
#define _FICEMAIN_H

/* Heartbead functions occur approx in this time, including checking for
 * new connections and decrementing timeleft counters. */
#define HEARTBEATTIME 1

/* Number of seconds that an idle connection can stand at login or password
 * prompt. */
#define MAX_LOGIN_IDLE 60

/* This is the socket that the current command came in on */
extern int current_socket;

#define DEFAULT_PROMPT "FICS % "

#define MESS_WELCOME "welcome"
#define MESS_LOGIN "login"
#define MESS_LOGOUT "logout"
#define MESS_MOTD "motd"
#define MESS_UNREGISTERED "unregistered"

#define STATS_MESSAGES "messages"
#define STATS_LOGONS "logons"
#define STATS_GAMES "games"

/* Arguments */
extern int port;
extern int withConsole;

#endif /* _FICEMAIN_H */
