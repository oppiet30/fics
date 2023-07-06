/* fics_config.h
 *
 */

/* Configure file locations in this include file. */

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

#ifndef _FICS_CONFIG_H
#define _FICS_CONFIG_H

/* CONFIGURE THIS: The port on which the server binds */
#define DEFAULT_PORT 5000

/* CONFIGURE THESE: Locations of the data, players, and games directories */
/* These must be absolute paths because some mail daemons may be called */
/* from outside the pwd */
#define DEFAULT_MESS "/Users/fics/Source/FICS/data/messages"
#define DEFAULT_HELP "/Users/fics/Source/FICS/data/help"
#define DEFAULT_STATS "/Users/fics/Source/FICS/data/stats"
#define DEFAULT_CONFIG "/Users/fics/Source/FICS/config"
#define DEFAULT_PLAYERS "/Users/fics/Source/FICS/players"
#define DEFAULT_GAMES "/Users/fics/Source/FICS/games"
#define DEFAULT_BOARDS "/Users/fics/Source/FICS/data/boards"

/* Where the standard ucb mail program is */
#define MAILPROGRAM "/usr/ucb/mail"

#endif /* _FICS_CONFIG_H */
