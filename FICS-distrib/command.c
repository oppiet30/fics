/* command.c
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
#include "command.h"
#include "command_list.h"
#include "movecheck.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "utils.h"
#include "playerdb.h"
#include "gamedb.h"
#include "gameproc.h"
#include "ratings.h"
#include "vers.h"
#include "network.h"
#include "hostinfo.h"
#include "tally.h"

PUBLIC char *mess_dir = DEFAULT_MESS;
PUBLIC char *help_dir = DEFAULT_HELP;
PUBLIC char *stats_dir = DEFAULT_STATS;
PUBLIC char *config_dir = DEFAULT_CONFIG;
PUBLIC char *player_dir = DEFAULT_PLAYERS;
PUBLIC char *game_dir = DEFAULT_GAMES;
PUBLIC char *board_dir = DEFAULT_BOARDS;
PUBLIC char *def_prompt = DEFAULT_PROMPT;
PUBLIC int startuptime;
PUBLIC int MailGameResult;

PUBLIC int commanding_player = -1; /* The player whose command your in */

PRIVATE int lastCommandFound = -1;

/* Copies command into comm, and returns pointer to parameters in
 * parameters
 */
PRIVATE int parse_command( char *com_string,
                           char **comm,
                           char **parameters )
{
  *comm = com_string;
  *parameters = eatword( com_string );
  if (**parameters != '\0') {
    **parameters = '\0';
    (*parameters)++;
    *parameters = eatwhite(*parameters);
  }
  if (strlen(*comm) >= MAX_COM_LENGTH) {
    return COM_BADCOMMAND;
  }
  return COM_OK;
}

PUBLIC int alias_lookup( char *tmp, alias_type *alias_list )
{
  int i;

  for (i=0; alias_list[i].comm_name; i++) {
    if (!strcmp( tmp, alias_list[i].comm_name)) return i;
  }
  return -1; /* not found */
}

PUBLIC int alias_count( alias_type *alias_list )
{
  int i;

  for (i=0; alias_list[i].comm_name; i++);
  return i;
}

/* Puts alias substitution into alias_string */
PRIVATE void alias_substitute( int p, char *com_str, char **alias_str )
{
  char *save_str=com_str;
  char tmp[MAX_COM_LENGTH];
  static char outalias[MAX_STRING_LENGTH];
  int i=0, done=0;

  while (com_str && !iswhitespace(*com_str) && !done) {
    if (i>=MAX_COM_LENGTH) { /* Too long for an alias */
      *alias_str = save_str;
      return;
    }
    tmp[i] = *com_str;
    com_str++;
    if (ispunct(tmp[i])) done = 1;
    i++;
  }
  tmp[i]='\0';
#ifdef DEBUG
  if (alias_count(parray[p].alias_list) != parray[p].numAlias) {
    fprintf( stderr, "FICS ALERT: ALIAS COUNT SCREWED UP for player %s, count %d %d.\n", parray[p].name, alias_count(parray[p].alias_list), parray[p].numAlias );
  }
#endif
  if ((i = alias_lookup( tmp, parray[p].alias_list )) >= 0) {
    if (com_str)
      sprintf( outalias, "%s%s", parray[p].alias_list[i].alias, com_str );
    else
      sprintf( outalias, "%s", parray[p].alias_list[i].alias );
    *alias_str = outalias;
  } else if ((i = alias_lookup( tmp, g_alias_list )) >= 0) {
    if (com_str)
      sprintf( outalias, "%s%s", g_alias_list[i].alias, com_str );
    else
      sprintf( outalias, "%s", g_alias_list[i].alias );
    *alias_str = outalias;
  } else
    *alias_str = save_str;
}

/* Returns pointer to command that matches */
PRIVATE int match_command( char *comm )
{
  int i=0;
  int gotIt = -1;
  int len=strlen(comm);

  while (command_list[i].comm_name) {
    if (!strncmp(command_list[i].comm_name, comm, len)) {
      if (gotIt >= 0) return -COM_AMBIGUOUS;
      gotIt = i;
    }
    i++;
  }
  if (gotIt >= 0) {
    lastCommandFound = gotIt;
    return gotIt;
  }
  return -COM_FAILED;
}

/* Gets the parameters for this command */
PRIVATE int get_parameters( int command, char *parameters, param_list params)
{
  int i, parlen;
  int paramLower;
  char c;
  static char punc[16];

  punc[1] = '\0'; /* Holds punc parameters */
  for (i=0; i < MAXNUMPARAMS; i++)
    (params)[i].type = TYPE_NULL; /* Set all parameters to NULL */
  parlen = strlen(command_list[command].param_string);
  for (i=0; i < parlen; i++) {
    c = command_list[command].param_string[i];
    if (isupper(c)) {
      paramLower = 0;
      c = tolower(c);
    } else {
      paramLower = 1;
    }
    switch (c) {
      case 'w': /* word */
        if (!*parameters) return COM_BADPARAMETERS;
      case 'o': /* optional word */
        if (!*parameters) return COM_OK;
        (params)[i].val.word = parameters;
        (params)[i].type = TYPE_WORD;
        if (ispunct(*parameters)) {
          punc[0] = *parameters;
          (params)[i].val.word = punc;
          parameters++;
          parameters = eatwhite(parameters);
        } else {
          parameters = eatword( parameters );
          if (*parameters != '\0') {
            *parameters = '\0';
            parameters++;
            parameters = eatwhite(parameters);
          }
        }
        if (paramLower)
          stolower( (params)[i].val.word  );
        break;
      case 'd': /* integer */
        if (!*parameters) return COM_BADPARAMETERS;
      case 'p': /* optional integer */
        if (!*parameters) return COM_OK;
        if (sscanf(parameters, "%d", &(params)[i].val.integer) != 1)
          return COM_BADPARAMETERS;
        (params)[i].type = TYPE_INT;
        parameters = eatword( parameters );
        if (*parameters != '\0') {
          *parameters = '\0';
          parameters++;
          parameters = eatwhite(parameters);
        }
        break;
      case 'i': /* word or integer */
        if (!*parameters) return COM_BADPARAMETERS;
      case 'n': /* optional word or integer */
        if (!*parameters) return COM_OK;
        if (sscanf(parameters, "%d", &(params)[i].val.integer) != 1) {
          (params)[i].val.word = parameters;
          (params)[i].type = TYPE_WORD;
        } else {
          (params)[i].type = TYPE_INT;
        }
        if (ispunct(*parameters)) {
          punc[0] = *parameters;
          (params)[i].val.word = punc;
          (params)[i].type = TYPE_WORD;
          parameters++;
          parameters = eatwhite(parameters);
        } else {
          parameters = eatword( parameters );
          if (*parameters != '\0') {
            *parameters = '\0';
            parameters++;
            parameters = eatwhite(parameters);
          }
        }
        if ((params)[i].type == TYPE_WORD)
          if (paramLower)
            stolower( (params)[i].val.word  );
        break;
      case 's': /* string to end */
        if (!*parameters) return COM_BADPARAMETERS;
      case 't': /* optional string to end */
        if (!*parameters) return COM_OK;
        (params)[i].val.string = parameters;
        (params)[i].type = TYPE_STRING;
        while (*parameters) parameters = nextword(parameters);
        if (paramLower)
          stolower( (params)[i].val.string  );
        break;
    }
  }
  if (*parameters)
    return COM_BADPARAMETERS;
  else
    return COM_OK;
}

PRIVATE void printusage( int p, char *command_str )
{
  int i, parlen;
  int command;
  char c;

  if ((command = match_command( command_str )) < 0) {
    pprintf( p, "  UNKNOWN COMMAND\n" );
    return;
  }
  parlen = strlen(command_list[command].param_string);
  for (i=0; i < parlen; i++) {
    c = command_list[command].param_string[i];
    if (isupper(c)) c = tolower(c);
    switch (c) {
      case 'w': /* word */
        pprintf( p, " word" );
        break;
      case 'o': /* optional word */
        pprintf( p, " [word]" );
        break;
      case 'd': /* integer */
        pprintf( p, " integer" );
        break;
      case 'p': /* optional integer */
        pprintf( p, " [integer]" );
        break;
      case 'i': /* word or integer */
        pprintf( p, " {word, integer}" );
        break;
      case 'n': /* optional word or integer */
        pprintf( p, " [{word, integer}]" );
        break;
      case 's': /* string to end */
        pprintf( p, " string" );
        break;
      case 't': /* optional string to end */
        pprintf( p, " [string]" );
        break;
    }
  }
}

PUBLIC int process_command( int p, char *com_string )
{
  int which_command, retval;
  param_list params;
  char *alias_string;
  char *comm, *parameters;

#ifdef DEBUG
  if (strcasecmp(parray[p].name, parray[p].login)) {
    fprintf( stderr, "FICS: PROBLEM Name=%s, Login=%s\n", parray[p].name, parray[p].login );
  }
#endif
  if (!com_string) return COM_FAILED;
#ifdef DEBUG
  fprintf( stderr, "%s, %s, %d: >%s<\n", parray[p].name, parray[p].login, parray[p].socket, com_string );
#endif
  alias_substitute( p, com_string, &alias_string );
#ifdef DEBUG
  if (com_string != alias_string)
    fprintf( stderr, "%s -alias-: >%s<\n", parray[p].name, alias_string );
#endif
  if ((retval = parse_command( alias_string, &comm, &parameters )))
    return retval;
  if (is_move( comm )) return COM_ISMOVE;
  stolower( comm ); /* All commands are case-insensitive */
  if ((which_command = match_command( comm )) < 0)
    return -which_command;
  if (parray[p].adminLevel < command_list[which_command].adminLevel ) {
    return COM_RIGHTS;
  }
  if ((retval = get_parameters( which_command, parameters, params )))
    return retval;
  return command_list[which_command].comm_func(p, params);
}

PRIVATE void process_login( int p, char *loginname )
{
  loginname = eatwhite( loginname );
  if (!*loginname) {
    psend_file( p, mess_dir, MESS_LOGIN );
    pprintf( p, "Login: " );
    return;
  }
  stolower(loginname);
  if (!alphastring(loginname)) {
    pprintf( p, "Illegal characters in login name. Only A-Za-z allowed.\n" );
    psend_file( p, mess_dir, MESS_LOGIN );
    pprintf( p, "Login: " );
    return;
  }
  if (strlen(loginname) < 3) {
    pprintf( p, "Login name must be at least 3 characters long.\n" );
    psend_file( p, mess_dir, MESS_LOGIN );
    pprintf( p, "Login: " );
    return;
  }
  if (strlen(loginname) > 17) {
    pprintf( p, "Login name should less than 17 characters long.\n" );
    psend_file( p, mess_dir, MESS_LOGIN );
    pprintf( p, "Login: " );
    return;
  }
  if (player_find_bylogin(loginname) >= 0) {
    pprintf( p, "A player named \"%s\" is already logged in.\n", loginname );
    psend_file( p, mess_dir, MESS_LOGIN );
    pprintf( p, "Login: " );
    return;
  }
  if (player_read( p, loginname )) {
    pprintf( p, "You may use this name to login as an unregistered player.\n" );
    pprintf( p, "Hit return to enter server." );
  } else {
    pprintf( p, "Password: " );
  }
  parray[p].status = PLAYER_PASSWORD;
  player_resort();
  turn_echo_off(parray[p].socket);
  return;
}

PRIVATE int process_password( int p, char *password )
{
  int p1;
  char salt[3];
  int fd;
  unsigned int fromHost;
  int messnum;

  turn_echo_on(parray[p].socket);

  if (parray[p].passwd && parray[p].registered) {
    salt[0] = parray[p].passwd[0];
    salt[1] = parray[p].passwd[1];
    salt[2] = '\0';
    if (strcmp(crypt(password, salt), parray[p].passwd)) {
      fd = parray[p].socket;
      fromHost = parray[p].thisHost;
      player_clear(p);
      parray[p].logon_time = time(0);
      parray[p].status = PLAYER_LOGIN;
      player_resort();
      parray[p].socket = fd;
      parray[p].thisHost = fromHost;
      pprintf( p, "\nLogin incorrect.\n" );
      pprintf( p, "Login: " );
      return 0;
    }
  }

  psend_file( p, mess_dir, MESS_MOTD );
  if (!parray[p].passwd && parray[p].registered)
    pprintf( p, "\n*** You have no password. Please set one with the password command." );
  if (!parray[p].registered)
    psend_file( p, mess_dir, MESS_UNREGISTERED );
  parray[p].status = PLAYER_PROMPT;
  player_resort( );
  player_write_login( p );
  for (p1=0; p1 < p_num; p1++) {
    if (p1 == p) continue;
    if (parray[p1].status != PLAYER_PROMPT) continue;
    if (parray[p1].thisHost == parray[p].thisHost) {
      fprintf( stderr, "FICS: Players %s and %s - same host: %s\n",
             parray[p].name, parray[p1].name, dotQuad(parray[p].thisHost) );
    }
    if (!parray[p1].i_login) continue;
    pprintf_prompt( p1, "\n[%s has connected.]\n", parray[p].name );
  }
  if (parray[p].registered && (parray[p].lastHost != 0) &&
      (parray[p].lastHost != parray[p].thisHost)) {
      fprintf( stderr, "FICS: Player %s: Last login: %s ", parray[p].name,
                        dotQuad(parray[p].lastHost));
      fprintf( stderr, "This login: %s\n", dotQuad(parray[p].thisHost) );
      pprintf( p, "\nPlayer %s: Last login: %s ", parray[p].name,
                        dotQuad(parray[p].lastHost));
      pprintf( p, "This login: %s", dotQuad(parray[p].thisHost) );
  }
  parray[p].lastHost = parray[p].thisHost;
  messnum = player_num_messages(p);
  if (messnum) {
    pprintf( p, "\nYou have %d messages.\nUse \"messages\" to display them, or \"clearmessages\" to remove them.\n", messnum );
  }
  pprintf( p, "\n%s", parray[p].prompt );
  return 0;
}

PRIVATE int process_prompt( int p, char *command )
{
  int retval;

  command = eatwhite(command);
  if (!*command) {
    pprintf( p, "%s", parray[p].prompt );
    return COM_OK;
  }
  retval = process_command( p, command );
  switch (retval) {
    case COM_OK:
      retval = COM_OK;
      pprintf( p, "%s", parray[p].prompt );
      break;
    case COM_OK_NOPROMPT:
      retval = COM_OK;
      break;
    case COM_ISMOVE:
      retval = COM_OK;
      process_move( p, command );
      pprintf( p, "%s", parray[p].prompt );
      break;
    case COM_RIGHTS:
      pprintf( p, "%s: Insufficient rights.\n", command );
      pprintf( p, "%s", parray[p].prompt );
      retval = COM_OK;
      break;
    case COM_AMBIGUOUS:
      pprintf( p, "%s: Ambiguous command.\n", command );
      pprintf( p, "%s", parray[p].prompt );
      retval = COM_OK;
      break;
    case COM_BADPARAMETERS:
      pprintf( p, "Usage: %s", command_list[lastCommandFound].comm_name );
      printusage( p, command_list[lastCommandFound].comm_name );
      pprintf( p, "\nSee 'help %s' for a complete description.\n", command_list[lastCommandFound].comm_name );
      pprintf( p, "%s", parray[p].prompt );
      retval = COM_OK;
      break;
    case COM_FAILED:
    case COM_BADCOMMAND:
      pprintf( p, "%s: Command not found.\n", command );
      retval = COM_OK;
      pprintf( p, "%s", parray[p].prompt );
      break;
    case COM_LOGOUT:
      retval = COM_LOGOUT;
      break;
  }
  return retval;
}

/* Return 1 to disconnect */
PUBLIC int process_input( int fd, char *com_string )
{
  int p = player_find( fd );
  int retval = 0;

  if (p < 0) {
    fprintf( stderr, "FICS: Input from a player not in array!\n" );
    return -1;
  }
  commanding_player = p;
  parray[p].last_command_time = time(0);
  switch (parray[p].status) {
    case PLAYER_EMPTY:
      fprintf( stderr, "FICS: Command from an empty player!\n" );
      break;
    case PLAYER_NEW:
      fprintf( stderr, "FICS: Command from a new player!\n" );
      break;
    case PLAYER_INQUEUE:
      /* Ignore input from player in queue */
      break;
    case PLAYER_LOGIN:
      process_login( p, com_string );
      break;
    case PLAYER_PASSWORD:
      retval = process_password( p, com_string );
      break;
    case PLAYER_PROMPT:
      retval = process_prompt( p, com_string );
      break;
  }
  commanding_player = -1;
  return retval;
}

PUBLIC int process_new_connection( int fd, unsigned int fromHost )
{
  int p = player_new();
  char hostname[512];

  gethostname( hostname, 512 );
  parray[p].status = PLAYER_LOGIN;
  player_resort();
  parray[p].socket = fd;
  parray[p].logon_time = time(0);
  parray[p].thisHost = fromHost;
  psend_file( p, mess_dir, MESS_WELCOME );
  pprintf( p, "Server version : %s\n", VERS_NUM );
  pprintf( p, "Server location: %s\n", hostname );
  psend_file( p, mess_dir, MESS_LOGIN );
  pprintf( p, "Login: " );
  return 0;
}

PUBLIC int process_disconnection( int fd )
{
  int p = player_find( fd );
  int p1;

  if (p < 0) {
    fprintf( stderr, "FICS: Disconnect from a player not in array!\n" );
    return -1;
  }
  if (parray[p].status == PLAYER_PROMPT) {
    for (p1=0; p1 < p_num; p1++) {
      if (p1 == p) continue;
      if (parray[p1].status != PLAYER_PROMPT) continue;
      if (!parray[p1].i_login) continue;
      pprintf( p1, "\n[%s has disconnected.]\n%s", parray[p].name, parray[p1].prompt );
    }
    player_write_logout( p );
    if (parray[p].registered) player_save(p);
  }
  player_remove( p );
  return 0;
}

PUBLIC int process_incomplete( int fd, char *com_string )
{
  int p = player_find( fd );
  char last_char;

  if (com_string[0])
    last_char = com_string[strlen(com_string) - 1];
  else
    last_char = '\0';

  if (p < 0) {
    fprintf( stderr, "FICS: Incomplete command from a player not in array!\n" );
    return -1;
  }
  if ((strlen(com_string) == 1) && (com_string[0] == '\4')) { /* ctrl-d */
    if (parray[p].status == PLAYER_PROMPT)
      process_input( fd, "quit" );
    return COM_LOGOUT;
  }
  if (last_char == '\3') { /* ctrl-c */
    if (parray[p].status == PLAYER_PROMPT) {
      pprintf( p, "\n%s", parray[p].prompt );
      return COM_FLUSHINPUT;
    } else {
      return COM_LOGOUT;
    }
  }
  return COM_OK;
}

/* Called every few seconds */
PUBLIC int process_heartbeat( int *fd )
{
  static int last_space=0;
  static int last_comfile=0;
  static int last_ratings=0;
  static int last_tally=0;
  static int lastcalled=0;
  int time_since_last;
  int p;
  static int rpid=0;
  int now = time(0);

/*  game_update_times(); */

  if (lastcalled == 0)
    time_since_last = 0;
  else
    time_since_last = now - lastcalled;
  lastcalled = now;

  /* Check for timed out connections */
  for (p=0; p < p_num; p++) {
    if (((parray[p].status == PLAYER_LOGIN) ||
         (parray[p].status == PLAYER_PASSWORD)) &&
        (player_idle(p) > MAX_LOGIN_IDLE)) {
      pprintf( p, "**** LOGIN TIMEOUT ****\n" );
      *fd = parray[p].socket;
      return COM_LOGOUT;
    }
  }
  if (rpid) { /* Rating calculating going on */
    union wait statusp;

    if (wait3(&statusp, WNOHANG, NULL) == rpid) {
      fprintf( stderr, "FICS: Reinitting statistics.\n" );
      rating_init(); /* Child finished, get the results. */
      rpid = 0;
    }
  }

  /* Recalc ratings every 3 hours */
  /* This is done because the ratings stats and players can get out of sync if
     there is a system crash. */
  /* This is done as a child process and read results when complete */
  if (last_ratings==0)
    last_ratings = (now - (5 * 60 * 60)) + 120; /* Do one in 2 minutes */
  else {
    if (last_ratings + 6 * 60 * 60  < now) {
      last_ratings = now;
      rpid = fork();
      if (rpid < 0) {
        fprintf( stderr, "FICS: Couldn't fork\n" );
      } else {
        if (rpid == 0) { /* The child */
          rating_recalc();
          exit(0);
          fprintf( stderr, "Recalc process should never get here!\n" );
          ASSERT(0);
        }
      }
    }
  }

  if (last_tally==0)
    last_tally = now;
  else {
    if (last_tally + 20 < now) { /* Every 20 seconds */
      last_tally = now;
      tally_usage();
    }
  }

  /* Check for the communication file from mail updates every 10 minutes */
  /* That is probably too often, but who cares */
  if (MailGameResult) {
    if (last_comfile==0)
      last_comfile = now;
    else {
      if (last_comfile + 10 * 60 < now) {
        last_comfile = now;
        /* Check for com file */
        hostinfo_checkcomfile();
      }
    }
  }
  if (last_space==0)
    last_space = now;
  else {
    if (last_space + 60 < now) { /* Check the disk space every minute */
      last_space = now;
      if (available_space() < 1000000) {
        server_shutdown( 60, "    **** Disk space is dangerously low!!! ****\n" );
      }
      /* Check for com file */
      hostinfo_checkcomfile();
    }
  }
  ShutHeartBeat();
  return COM_OK;
}

PUBLIC void commands_init()
{
  FILE *fp, *afp;
  char fname[MAX_FILENAME_SIZE];
  int i=0;

  sprintf( fname, "%s/commands", help_dir );
  fp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Could not write commands help file.\n" );
    return;
  }
  sprintf( fname, "%s/admin_commands", help_dir );
  afp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Could not write admin_commands help file.\n" );
    fclose(fp);
    return;
  }
  while (command_list[i].comm_name) {
    if (command_list[i].adminLevel >= ADMIN_ADMIN) {
      fprintf( afp, "%s\n", command_list[i].comm_name );
    } else {
      fprintf( fp, "%s\n", command_list[i].comm_name );
    }
    i++;
  }
  fclose( fp );
  fclose( afp );
}

/* Need to save rated games */
PUBLIC void TerminateCleanup()
{
  int p1;
  int g;

  for (g=0; g < g_num; g++) {
    if (garray[g].status != GAME_ACTIVE) continue;
    if (garray[g].rated) {
      game_ended( g, WHITE, END_ADJOURN );
    }
  }
  for (p1=0; p1 < p_num; p1++) {
    if (parray[p1].status == PLAYER_EMPTY) continue;
    pprintf(p1, "\n    **** Server shutting down immediately. ****\n\n" );
    if (parray[p1].status != PLAYER_PROMPT) {
      close( parray[p1].socket );
    } else {
      pprintf(p1, "Logging you out.\n" );
      psend_file( p1, mess_dir, MESS_LOGOUT );
      player_write_logout( p1 );
      if (parray[p1].registered) player_save(p1);
    }
  }
}
