/* variable.c
 * This is cludgy
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
#include "variable.h"
#include "playerdb.h"
#include "utils.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "rmalloc.h"
#include "board.h"
#include "command.h"

PRIVATE int set_open( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].open)
      parray[p].open = 0;
    else
      parray[p].open = 1;
    goto o_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].open = v;
o_return:
  if (parray[p].open)
    pprintf( p, "You are now open to receive match requests.\n" );
  else {
    player_decline_offers(p, -1, PEND_MATCH);
    player_withdraw_offers(p, -1, PEND_MATCH);
    pprintf( p, "You are no longer receiving match requests.\n" );
  }
  return VAR_OK;
}

PRIVATE int set_sopen( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].sopen)
      parray[p].sopen = 0;
    else
      parray[p].sopen = 1;
    goto so_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].sopen = v;
so_return:
  if (parray[p].sopen)
    pprintf( p, "You are now open to receive simul requests.\n" );
  else {
    pprintf( p, "You are no longer receiving simul requests.\n" );
  }
  return VAR_OK;
}

PRIVATE int set_rated( int p, char *var, char *val )
{
  int v= -1;

  if (!parray[p].registered) {
    pprintf( p, "You cannot change your rated status.\n" );
    return VAR_OK;
  }
  if (!val) {
    if (parray[p].rated)
      parray[p].rated = 0;
    else
      parray[p].rated = 1;
    goto r_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].rated = v;
r_return:
  if (parray[p].rated)
    pprintf( p, "You are allowing rated matches.\n" );
  else
    pprintf( p, "You are only playing unrated matches.\n" );
  return VAR_OK;
}

PRIVATE int set_shout( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].i_shout)
      parray[p].i_shout = 0;
    else
      parray[p].i_shout = 1;
    goto s_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].i_shout = v;
s_return:
  if (parray[p].i_shout)
    pprintf( p, "You will now hear shouts.\n" );
  else
    pprintf( p, "You will not hear shouts.\n" );
  return VAR_OK;
}

PRIVATE int set_kibitz( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].i_kibitz)
      parray[p].i_kibitz = 0;
    else
      parray[p].i_kibitz = 1;
    goto k_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].i_kibitz = v;
k_return:
  if (parray[p].i_kibitz)
    pprintf( p, "You will now hear kibitzes.\n" );
  else
    pprintf( p, "You will not hear kibitzes.\n" );
  return VAR_OK;
}

PRIVATE int set_tell( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].i_tell)
      parray[p].i_tell = 0;
    else
      parray[p].i_tell = 1;
    goto t_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].i_tell = v;
t_return:
  if (parray[p].i_tell)
    pprintf( p, "You will now hear tells.\n" );
  else
    pprintf( p, "You will not hear tells.\n" );
  return VAR_OK;
}

PRIVATE int set_pinform( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].i_login)
      parray[p].i_login = 0;
    else
      parray[p].i_login = 1;
    goto l_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].i_login = v;
l_return:
  if (parray[p].i_login)
    pprintf( p, "You will now hear logins/logouts.\n" );
  else
    pprintf( p, "You will not hear logins/logouts.\n" );
  return VAR_OK;
}

PRIVATE int set_ginform( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].i_game)
      parray[p].i_game = 0;
    else
      parray[p].i_game = 1;
    goto g_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].i_game = v;
g_return:
  if (parray[p].i_game)
    pprintf( p, "You will now hear game results.\n" );
  else
    pprintf( p, "You will not hear game results.\n" );
  return VAR_OK;
}

PRIVATE int set_private( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].private)
      parray[p].private = 0;
    else
      parray[p].private = 1;
    goto pr_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].private = v;
pr_return:
  if (parray[p].private)
    pprintf( p, "Your games will be private.\n" );
  else
    pprintf( p, "Your games may not be private.\n" );
  return VAR_OK;
}

PRIVATE int set_automail( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].automail)
      parray[p].automail = 0;
    else
      parray[p].automail = 1;
    goto a_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].automail = v;
a_return:
  if (parray[p].automail)
    pprintf( p, "Your games will be mailed to you.\n" );
  else
    pprintf( p, "Your games will not be mailed to you.\n" );
  return VAR_OK;
}

PRIVATE int set_bell( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].bell)
      parray[p].bell = 0;
    else
      parray[p].bell = 1;
    goto b_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].bell = v;
b_return:
  if (parray[p].bell)
    pprintf( p, "Bell on.\n" );
  else
    pprintf( p, "Bell off.\n" );
  return VAR_OK;
}

PRIVATE int set_highlight( int p, char *var, char *val )
{
  int v= -1;

  if (!val) {
    if (parray[p].highlight)
      parray[p].highlight = 0;
    else
      parray[p].highlight = 1;
    goto b_return;
  }
  if (sscanf( val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val,"off")) v = 0;
    if (!strcmp(val,"false")) v = 0;
    if (!strcmp(val,"on")) v = 1;
    if (!strcmp(val,"true")) v = 1;
  }
  if ((v != 0) && (v != 1)) return VAR_BADVAL;
  parray[p].highlight = v;
b_return:
  if (parray[p].highlight)
    pprintf( p, "Hightlight is on.\n" );
  else
    pprintf( p, "Hightlight is off.\n" );
  return VAR_OK;
}

PRIVATE int set_style( int p, char *var, char *val )
{
  int v= -1;

  if (!val) return VAR_BADVAL;
  if (sscanf( val, "%d", &v) != 1) return VAR_BADVAL;
  if ((v < 1) || (v > MAX_STYLES)) return VAR_BADVAL;
  parray[p].style = v-1;
  pprintf( p, "Style %d set.\n", v );
  return VAR_OK;
}

PRIVATE int set_uscf( int p, char *var, char *val )
{
  int v= -1;

  if (!val) return VAR_BADVAL;
  if (sscanf( val, "%d", &v) != 1) return VAR_BADVAL;
  if ((v < 0) || (v > 3000)) return VAR_BADVAL;
  parray[p].uscfRating = v;
  pprintf( p, "USCF Rating set to %d.\n", v );
  return VAR_OK;
}

PRIVATE int set_promote( int p, char *var, char *val )
{
  if (!val) return VAR_BADVAL;
  stolower(val);
  switch (val[0]) {
    case 'q':
      parray[p].promote = QUEEN;
      pprintf( p, "Promotion piece set to QUEEN.\n" );
      break;
    case 'r':
      parray[p].promote = ROOK;
      pprintf( p, "Promotion piece set to ROOK.\n" );
      break;
    case 'b':
      parray[p].promote = BISHOP;
      pprintf( p, "Promotion piece set to BISHOP.\n" );
      break;
    case 'n':
    case 'k':
      parray[p].promote = KNIGHT;
      pprintf( p, "Promotion piece set to KNIGHT.\n" );
      break;
    default:
      return VAR_BADVAL;
  }
  return VAR_OK;
}

PRIVATE int set_realname( int p, char *var, char *val )
{
  if (!val) {
    if (parray[p].fullName) rfree(parray[p].fullName);
    parray[p].fullName = NULL;
    return VAR_OK;
  }
  if (!printablestring(val)) return VAR_BADVAL;
  if (parray[p].fullName) rfree(parray[p].fullName);
  parray[p].fullName = strdup(val);
  return VAR_OK;
}

PRIVATE int set_prompt( int p, char *var, char *val )
{
  if (!val) {
    if (parray[p].prompt && (parray[p].prompt != def_prompt))
      rfree(parray[p].prompt);
    parray[p].prompt = def_prompt;
    return VAR_OK;
  }
  if (!printablestring(val)) return VAR_BADVAL;
  if (parray[p].prompt != def_prompt) rfree(parray[p].prompt);
  strcat( val, " " ); /* I don't think this will be a problem */
  parray[p].prompt = strdup(val);
  return VAR_OK;
}

PRIVATE int set_plan( int p, char *var, char *val )
{
  int which;
  int i;

  if (val && !printablestring(val)) return VAR_BADVAL;
  which = atoi(var); /* Must be an integer, no test needed */
  if (which == 0) {
    if (!val) return VAR_BADVAL;
    if (parray[p].num_plan >= MAX_PLAN) {
      pprintf( p, "You cannot add another plan line. Maximum is %d\n", MAX_PLAN );
      return VAR_BADVAL;
    }
    for (i=parray[p].num_plan; i > 0; i--)
      parray[p].planLines[i] = parray[p].planLines[i-1];
    parray[p].num_plan++;
    parray[p].planLines[0] = strdup(val);
    return VAR_OK;
  }
  if (which > parray[p].num_plan)
    which = parray[p].num_plan;
  else
    which = which - 1;
  if (which >= MAX_PLAN) {
    pprintf( p, "You cannot add another plan line. Maximum is %d\n", MAX_PLAN );
    return VAR_BADVAL;
  }
  if (which == parray[p].num_plan) {
    parray[p].num_plan++;
  } else {
    if (parray[p].planLines[which] &&
        parray[p].planLines[which][0] != ' ')
           rfree(parray[p].planLines[which]);
  }
  if (val)
    parray[p].planLines[which] = strdup(val);
  else {
    if (which == MAX_PLAN-1) {
      parray[p].planLines[which] = NULL;
      parray[p].num_plan--;
    } else {
      parray[p].planLines[which] = strdup(" ");
    }
  }
  return VAR_OK;
}

PUBLIC var_list variables[] = {
  {"open",      set_open},
  {"simopen",   set_sopen},
  {"rated",     set_rated},
  {"shout",     set_shout},
  {"kibitz",    set_kibitz},
  {"tell",      set_tell},
  {"pinform",   set_pinform},
  {"i_login",   set_pinform},
  {"ginform",   set_ginform},
  {"i_game",    set_ginform},
  {"automail",  set_automail},
  {"bell",      set_bell},
  {"private",   set_private},
  {"style",     set_style},
  {"uscf",      set_uscf},
  {"realname",  set_realname},
  {"prompt",    set_prompt},
  {"promote",   set_promote},
  {"highlight", set_highlight},
  {"0", set_plan},
  {"1", set_plan},
  {"2", set_plan},
  {"3", set_plan},
  {"4", set_plan},
  {"5", set_plan},
  {"6", set_plan},
  {"7", set_plan},
  {"8", set_plan},
  {"9", set_plan},
  {NULL, NULL}
};

PRIVATE int set_find( char *var )
{
  int i=0;
  int gotIt = -1;
  int len=strlen(var);

  while (variables[i].name) {
    if (!strncmp(variables[i].name, var, len)) {
      if (gotIt >= 0) return -VAR_AMBIGUOUS;
      gotIt = i;
    }
    i++;
  }
  if (gotIt >= 0) {
    return gotIt;
  }
  return -VAR_NOSUCH;
}

PUBLIC int var_set( int p, char *var, char *val, int *wh )
{
  int which;

  if (!var) return VAR_NOSUCH;
  if ((which = set_find( var )) < 0 ) {
    return -which;
  }
  *wh = which;
  return variables[which].var_func(p, variables[which].name, val);
}
