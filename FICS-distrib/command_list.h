/* command_list.h
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

#ifndef _COMMAND_LIST_H
#define _COMMAND_LIST_H

#include "comproc.h"
#include "gameproc.h"
#include "adminproc.h"
#include "ratings.h"

/*
  Parameter string format
  w - a word
  o - an optional word
  d - integer
  p - optional integer
  i - word or integer
  n - optional word or integer
  s - string to end
  t - optional string to end

  If the parameter option is given in lower case then the parameter is
  converted to lower case before being passsed to the function. If it is
  in upper case, then the parameter is passed as typed.
 */
/* Try to keep this list in alpha order, that is the way it is shown to
 * the 'help commands' command.
 */
 /* Name        Options Functions       Security */
PUBLIC command_type command_list[] = {
  {"accept",            "n",    com_accept,     ADMIN_USER },
  {"adjourn",           "",     com_adjourn,    ADMIN_USER },
  {"abort",             "",     com_abort,      ADMIN_USER },
  {"alias",             "OT",   com_alias,      ADMIN_USER },
  {"allobservers",      "n",    com_allobservers,       ADMIN_USER },
  {"asetadmin",         "wd",   com_asetadmin,  ADMIN_USER },
  {"assess",            "oo",   com_assess,     ADMIN_USER },
  {"bell",              "",     com_bell,       ADMIN_USER },
  {"best",              "",     com_best,       ADMIN_USER },
  {"boards",            "o",    com_boards,     ADMIN_USER },
  {"censor",            "o",    com_censor,     ADMIN_USER },
  {"channel",           "p",    com_channel,    ADMIN_USER },
  {"clearmessages",     "",     com_clearmessages,      ADMIN_USER },
  {"courtesyabort",     "",     com_courtesyabort,      ADMIN_USER },
  {"date",              "",     com_date,       ADMIN_USER },
  {"decline",           "n",    com_decline,    ADMIN_USER },
  {"draw",              "",     com_draw,       ADMIN_USER },
  {"finger",            "o",    com_stats,      ADMIN_USER },
  {"flag",              "",     com_flag,       ADMIN_USER },
  {"games",             "",     com_games,      ADMIN_USER },
  {"help",              "o",    com_help,       ADMIN_USER },
  {"highlight",         "",     com_highlight,  ADMIN_USER },
  {"history",           "o",    com_history,    ADMIN_USER },
  {"inchannel",         "pp",   com_inchannel,  ADMIN_USER },
  {"it",                "S",    com_it,         ADMIN_USER },
  {"kibitz",            "S",    com_kibitz,     ADMIN_USER },
  {"load",              "ww",   com_load,       ADMIN_USER },
  {"llogons",           "",     com_llogons,    ADMIN_USER },
  {"logons",            "o",    com_logons,     ADMIN_USER },
  {"mailmoves",         "n",    com_mailmoves,  ADMIN_USER },
  {"mailoldmoves",      "o",    com_mailoldmoves,       ADMIN_USER },
  {"match",             "wppppoo",com_match,    ADMIN_USER },
  {"messages",          "oT",   com_messages,   ADMIN_USER },
  {"moves",             "n",    com_moves,      ADMIN_USER },
  {"observe",           "n",    com_observe,    ADMIN_USER },
  {"oldmoves",          "o",    com_oldmoves,   ADMIN_USER },
  {"open",              "",     com_open,       ADMIN_USER },
  {"password",          "WW",   com_password,   ADMIN_USER },
  {"pause",             "",     com_pause,      ADMIN_USER },
  {"pending",           "",     com_pending,    ADMIN_USER },
  {"promote",           "w",    com_promote,    ADMIN_USER },
  {"query",             "S",    com_query,      ADMIN_USER },
  {"quit",              "",     com_quit,       ADMIN_USER },
  {"refresh",           "p",    com_refresh,    ADMIN_USER },
  {"resign",            "",     com_resign,     ADMIN_USER },
  {"say",               "S",    com_say,        ADMIN_USER },
  {"servers",           "",     com_servers,    ADMIN_USER },
  {"set",               "wT",   com_set,        ADMIN_USER },
  {"shout",             "S",    com_shout,      ADMIN_USER },
  {"simabort",          "",     com_simabort,   ADMIN_USER },
  {"simgames",          "o",    com_simgames,   ADMIN_USER },
  {"simmatch",          "w",    com_simmatch,   ADMIN_USER },
  {"simnext",           "",     com_simnext,    ADMIN_USER },
  {"simopen",           "",     com_simopen,    ADMIN_USER },
  {"simpass",           "",     com_simpass,    ADMIN_USER },
  {"smoves",            "ww",   com_smoves,     ADMIN_USER },
  {"sposition",         "ww",   com_sposition,  ADMIN_USER },
  {"statistics",        "",     com_statistics, ADMIN_USER },
  {"stored",            "o",    com_stored,     ADMIN_USER },
  {"style",             "d",    com_style,      ADMIN_USER },
  {"switch",            "",     com_switch,     ADMIN_USER },
  {"takeback",          "p",    com_takeback,   ADMIN_USER },
  {"time",              "n",    com_time,       ADMIN_USER },
  {"tell",              "nS",   com_tell,       ADMIN_USER },
  {"unalias",           "W",    com_unalias,    ADMIN_USER },
  {"uncensor",          "o",    com_uncensor,   ADMIN_USER },
  {"unpause",           "",     com_unpause,    ADMIN_USER },
  {"uptime",            "",     com_uptime,     ADMIN_USER },
  {"variables",         "o",    com_variables,  ADMIN_USER },
  {"whisper",           "S",    com_whisper,    ADMIN_USER },
  {"withdraw",          "n",    com_withdraw,   ADMIN_USER },
  {"who",               "o",    com_who,        ADMIN_USER },
  {"addplayer",         "WO",   com_addplayer,  ADMIN_ADMIN },
  {"announce",          "S",    com_announce,   ADMIN_ADMIN },
  {"asetpasswd",        "wW",   com_asetpasswd, ADMIN_ADMIN },
  {"asetemail",         "wo",   com_asetemail,  ADMIN_ADMIN },
  {"muzzle",            "o",    com_muzzle,     ADMIN_ADMIN },
  {"pose",              "wS",   com_pose,       ADMIN_ADMIN },
  {"shutdown",          "n",    com_shutdown,   ADMIN_ADMIN },
  {NULL, NULL, NULL, ADMIN_USER}
};

PUBLIC alias_type g_alias_list[] = {
  {"w",         "who"},
  {"t",         "tell"},
  {"m",         "match"},
  {"f",         "finger"},
  {"a",         "accept"},
  {"sh",        "shout"},
  {"vars",      "variables"},
  {"players",   "who a"},
  {"p",         "who a"},
  {".",         "tell ."},
  {",",         "tell ,"},
  {"!",         "shout "},
  {":",         "it "},
  {"?",         "query "},
  {"exit",      "quit"},
  {"logout",    "quit"},
  {NULL, NULL}
};

#endif /* _COMMAND_LIST_H */
