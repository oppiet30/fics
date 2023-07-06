/* comproc.h
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

#ifndef _COMPROC_H
#define _COMPROC_H

extern int com_quit();
extern int com_help();
extern int com_shout();
extern int com_query();
extern int com_it();
extern int com_tell();
extern int com_say();
extern int com_whisper();
extern int com_kibitz();
extern int com_set();
extern int com_stats();
extern int com_password();
extern int com_uptime();
extern int com_date();
extern int com_llogons();
extern int com_logons();
extern int com_who();
extern int com_censor();
extern int com_uncensor();
extern int com_channel();
extern int com_inchannel();
extern int com_match();
extern int com_decline();
extern int com_withdraw();
extern int com_pending();
extern int com_accept();
extern int com_refresh();
extern int com_open();
extern int com_simopen();
extern int com_bell();
extern int com_highlight();
extern int com_style();
extern int com_promote();
extern int com_alias();
extern int com_unalias();
extern int create_new_match();
extern int com_servers();
extern int com_sendmessage();
extern int com_messages();
extern int com_clearmessages();
extern int com_variables();

#endif /* _COMPROC_H */
