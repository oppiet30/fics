/* command.h
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

#ifndef _COMMAND_H
#define _COMMAND_H

extern char *mess_dir;
extern char *help_dir;
extern char *stats_dir;
extern char *config_dir;
extern char *player_dir;
extern char *game_dir;
extern char *board_dir;
extern char *def_prompt;

extern int startuptime;
extern int MailGameResult;

/* Maximum length of a login name */
#define MAX_LOGIN_NAME 20

/* Maximum number of parameters per command */
#define MAXNUMPARAMS 10

/* Maximum string length of a single command word */
#define MAX_COM_LENGTH 50

/* Maximum string length of the whole command line */
#define MAX_STRING_LENGTH 1024

#define COM_OK 0
#define COM_FAILED 1
#define COM_ISMOVE 2
#define COM_AMBIGUOUS 3
#define COM_BADPARAMETERS 4
#define COM_BADCOMMAND 5
#define COM_LOGOUT 6
#define COM_FLUSHINPUT 7
#define COM_RIGHTS 8
#define COM_OK_NOPROMPT 9

#define ADMIN_USER      0
#define ADMIN_ADMIN     10
#define ADMIN_MASTER    20
#define ADMIN_GOD       100

#define TYPE_NULL 0
#define TYPE_WORD 1
#define TYPE_STRING 2
#define TYPE_INT 3
typedef struct u_parameter {
  int type;
  union {
    char *word;
    char *string;
    int integer;
  } val;
} parameter;

typedef parameter param_list[MAXNUMPARAMS];

typedef struct s_command_type {
  char *comm_name;
  char *param_string;
  int (*comm_func)();
  int adminLevel;
} command_type;

typedef struct s_alias_type {
  char *comm_name;
  char *alias;
} alias_type;

extern int commanding_player; /* The player whose command your in */

extern int process_input( );
extern int process_new_connection();
extern int process_disconnection();
extern int process_incomplete();
extern int process_heartbeat();

extern void commands_init();

extern void TerminateCleanup();
extern int process_command();

extern int alias_lookup();

#endif /* _COMMAND_H */
