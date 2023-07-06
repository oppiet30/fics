/* ratings.c
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
#include "ratings.h"
#include "playerdb.h"
#include "gamedb.h"
#include "command.h"
#include "ficsmain.h"
#include "fics_config.h"
#include "utils.h"

typedef struct _rateStruct
{
  char name[MAX_LOGIN_NAME];
  int rating;
} rateStruct;

PUBLIC double Ratings_B_Average;
PUBLIC double Ratings_B_StdDev;

PUBLIC double Ratings_S_Average;
PUBLIC double Ratings_S_StdDev;

PRIVATE double Rb_M=0.0, Rb_S=0.0, Rb_total=0.0;
PRIVATE int Rb_count=0;

PRIVATE double Rs_M=0.0, Rs_S=0.0, Rs_total=0.0;
PRIVATE int Rs_count=0;

PUBLIC rateStruct bestS[MAX_BEST];
PUBLIC int numS=0;
PUBLIC rateStruct bestB[MAX_BEST];
PUBLIC int numB=0;

#define MAXHIST 30
#define LOWESTHIST 800
PUBLIC int sHist[MAXHIST];
PUBLIC int bHist[MAXHIST];

PUBLIC void rating_add( int rating, int type )
{
  int which;

  which = (rating - LOWESTHIST) / 100;
  if (which < 0) which = 0;
  if (which >= MAXHIST) which = MAXHIST-1;
  if (type == TYPE_BLITZ) {
    bHist[which] += 1;
    Rb_count++;
    Rb_total += rating;
    if (Rb_count == 1) {
      Rb_M = rating;
    } else {
      Rb_S = Rb_S + (rating - Rb_M) * (rating - Rb_M);
      Rb_M = Rb_M + (rating - Rb_M)/(Rb_count);
    }
    Ratings_B_StdDev = sqrt(Rb_S/Rb_count);
    Ratings_B_Average = Rb_total / (double)Rb_count;
  } else { /* TYPE_STAND */
    sHist[which] += 1;
    Rs_count++;
    Rs_total += rating;
    if (Rs_count == 1) {
      Rs_M = rating;
    } else {
      Rs_S = Rs_S + (rating - Rs_M) * (rating - Rs_M);
      Rs_M = Rs_M + (rating - Rs_M)/(Rs_count);
    }
    Ratings_S_StdDev = sqrt(Rs_S/Rs_count);
    Ratings_S_Average = Rs_total / (double)Rs_count;
  }
}

PUBLIC void rating_remove( int rating, int type )
{
  int which;

  which = (rating - LOWESTHIST) / 100;
  if (which < 0) which = 0;
  if (which >= MAXHIST) which = MAXHIST-1;
  if (type == TYPE_BLITZ) {
    bHist[which] = bHist[which] - 1;
    if (bHist[which]<0) bHist[which]=0;
    if (Rb_count == 0) return;
    Rb_count--;
    Rb_total -= rating;
    if (Rb_count == 0) {
      Rb_M = 0;
      Rb_S = 0;
    } else {
      Rb_M = Rb_M - (rating - Rb_M)/(Rb_count);
      Rb_S = Rb_S - (rating - Rb_M) * (rating - Rb_M);
    }
    if (Rb_count) {
      Ratings_B_StdDev = sqrt(Rb_S/Rb_count);
      Ratings_B_Average = Rb_total / (double)Rb_count;
    } else {
      Ratings_B_StdDev = 0;
      Ratings_B_Average = 0;
    }
  } else { /* TYPE_STAND */
    sHist[which] = sHist[which] - 1;
    if (sHist[which]<0) sHist[which]=0;
    if (Rs_count == 0) return;
    Rs_count--;
    Rs_total -= rating;
    if (Rs_count == 0) {
      Rs_M = 0;
      Rs_S = 0;
    } else {
      Rs_M = Rs_M - (rating - Rs_M)/(Rs_count);
      Rs_S = Rs_S - (rating - Rs_M) * (rating - Rs_M);
    }
    if (Rs_count) {
      Ratings_S_StdDev = sqrt(Rs_S/Rs_count);
      Ratings_S_Average = Rs_total / (double)Rs_count;
    } else {
      Ratings_S_StdDev = 0;
      Ratings_S_Average = 0;
    }
  }
}

PRIVATE void load_ratings()
{
  FILE *fp;
  char fname[MAX_FILENAME_SIZE];
  int i;

  sprintf( fname, "%s/ratings_data", stats_dir );
  fp = fopen( fname, "r" );
  if (!fp) {
    fprintf( stderr, "FICS: Can't read ratings data!\n" );
    return;
  }
  fscanf( fp, "%lf %lf %lf %d", &Rb_M, &Rb_S, &Rb_total, &Rb_count );
  fscanf( fp, "%lf %lf %lf %d", &Rs_M, &Rs_S, &Rs_total, &Rs_count );
  fscanf( fp, "%d", &numB );
  for (i=0;i<numB;i++) {
    fscanf( fp, "%s %d", bestB[i].name, &(bestB[i].rating));
  }
  fscanf( fp, "%d", &numS );
  for (i=0;i<numS;i++) {
    fscanf( fp, "%s %d", bestS[i].name, &bestS[i].rating);
  }
  for (i=0;i<MAXHIST;i++) {
    fscanf( fp, "%d %d", &sHist[i], &bHist[i]);
  }
  fclose(fp);
  if (Rs_count) {
    Ratings_S_StdDev = sqrt(Rs_S/Rs_count);
    Ratings_S_Average = Rs_total / (double)Rs_count;
  } else {
    Ratings_S_StdDev = 0;
    Ratings_S_Average = 0;
  }
  if (Rb_count) {
    Ratings_B_StdDev = sqrt(Rb_S/Rb_count);
    Ratings_B_Average = Rb_total / (double)Rb_count;
  } else {
    Ratings_B_StdDev = 0;
    Ratings_B_Average = 0;
  }
}

PUBLIC void save_ratings()
{
  FILE *fp;
  char fname[MAX_FILENAME_SIZE];
  int i;

  sprintf( fname, "%s/ratings_data", stats_dir );
  fp = fopen( fname, "w" );
  if (!fp) {
    fprintf( stderr, "FICS: Can write ratings data!\n" );
    return;
  }
  fprintf( fp, "%10f %10f %10f %d\n", Rb_M, Rb_S, Rb_total, Rb_count );
  fprintf( fp, "%10f %10f %10f %d\n", Rs_M, Rs_S, Rs_total, Rs_count );
  fprintf( fp, "%d\n", numB );
  for (i=0;i<numB;i++) {
    fprintf( fp, "%s %d\n", bestB[i].name, bestB[i].rating);
  }
  fprintf( fp, "%d\n", numS );
  for (i=0;i<numS;i++) {
    fprintf( fp, "%s %d\n", bestS[i].name, bestS[i].rating);
  }
  for (i=0;i<MAXHIST;i++) {
    fprintf( fp, "%d %d\n", sHist[i], bHist[i]);
  }
  fclose(fp);
}

PRIVATE void BestRemove( int p )
{
  int i;

  for (i=0;i<numB;i++) {
    if (!strcmp(bestB[i].name,parray[p].name)) break;
  }
  if (i < numB) {
    for (;i<numB-1;i++) {
      strcpy( bestB[i].name, bestB[i+1].name );
      bestB[i].rating = bestB[i+1].rating;
    }
    numB--;
  }
  for (i=0;i<numS;i++) {
    if (!strcmp(bestS[i].name,parray[p].name)) break;
  }
  if (i < numS) {
    for (;i<numS-1;i++) {
      strcpy( bestS[i].name, bestS[i+1].name );
      bestS[i].rating = bestS[i+1].rating;
    }
    numS--;
  }
}

PRIVATE void BestAdd( int p )
{
  int where,j;

  if (parray[p].b_stats.rating <= 0) goto standard;
  for (where = 0; where < numB; where++) {
    if (parray[p].b_stats.rating > bestB[where].rating) break;
  }
  if (where < MAX_BEST) {
    for (j=numB;j>where;j--) {
      if (j == MAX_BEST) continue;
      strcpy( bestB[j].name, bestB[j-1].name );
      bestB[j].rating = bestB[j-1].rating;
    }
    strcpy( bestB[where].name, parray[p].name );
    bestB[where].rating = parray[p].b_stats.rating;
    if (numB < MAX_BEST) numB++;
  }
standard:
  if (parray[p].s_stats.rating <= 0) return;
  for (where = 0; where < numS; where++) {
    if (parray[p].s_stats.rating > bestS[where].rating) break;
  }
  if (where < MAX_BEST) {
    for (j=numS;j>where;j--) {
      if (j == MAX_BEST) continue;
      strcpy( bestS[j].name, bestS[j-1].name );
      bestS[j].rating = bestS[j-1].rating;
    }
    strcpy( bestS[where].name, parray[p].name );
    bestS[where].rating = parray[p].s_stats.rating;
    if (numS < MAX_BEST) numS++;
  }
}

PUBLIC void BestUpdate( int p )
{
  BestRemove(p);
  BestAdd(p);
}

PUBLIC void zero_stats()
{
  int i;
  for (i=0;i<MAXHIST;i++) {
    sHist[i] = 0;
    bHist[i] = 0;
  }
  Rb_M=0.0, Rb_S=0.0, Rb_total=0.0;
  Rb_count=0;

  Rs_M=0.0, Rs_S=0.0, Rs_total=0.0;
  Rs_count=0;

  numS=0;
  numB=0;
}

PUBLIC void rating_init()
{
  zero_stats();
  load_ratings();
}

/* This recalculates the rating info from the player data, it can take
   a long time! */
PUBLIC void rating_recalc()
{
  char dname[MAX_FILENAME_SIZE];
  int p1;
  int c;
  int t=time(0);
  DIR *dirp;
  struct direct *dp;

  fprintf( stderr, "FICS: Recalculating ratings at %s\n", strltime(&t));
  zero_stats();
  for (c='a';c<='z';c++) {
    /* Never done as ratings server */
    sprintf( dname, "%s/%c", player_dir, c );
    dirp = opendir( dname );
    if (!dirp) continue;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
      if (!strcmp(dp->d_name, ".")) continue;
      if (!strcmp(dp->d_name, "..")) continue;
      p1 = player_new();
      if (player_read( p1, dp->d_name )) {
        player_remove( p1 );
        fprintf( stderr, "FICS: Problem reading player %s.\n", dp->d_name );
        continue;
      }
      if (parray[p1].b_stats.rating > 0) {
        rating_add( parray[p1].b_stats.rating, TYPE_BLITZ );
      }
      if (parray[p1].s_stats.rating > 0) {
        rating_add( parray[p1].s_stats.rating, TYPE_STAND );
      }
      BestUpdate(p1);
      player_remove( p1 );
    }
    closedir(dirp);
  }
  if (Rs_count) {
    Ratings_S_StdDev = sqrt(Rs_S/Rs_count);
    Ratings_S_Average = Rs_total / (double)Rs_count;
  } else {
    Ratings_S_StdDev = 0;
    Ratings_S_Average = 0;
  }
  if (Rb_count) {
    Ratings_B_StdDev = sqrt(Rb_S/Rb_count);
    Ratings_B_Average = Rb_total / (double)Rb_count;
  } else {
    Ratings_B_StdDev = 0;
    Ratings_B_Average = 0;
  }
  save_ratings();
  t=time(0);
  fprintf( stderr, "FICS: Finished at %s\n", strltime(&t));
}

/*
                          ICS  Rating system

A player is said to be provisional if he/she has played fewer than 20 games,
and established otherwise.  An entirely different system is used for
established and provisional players.

The rating during the provisional period is the average of a set of values,
one for each game played.  The values (of which the rating is an average)
depend on whether the opponent is provisional, or is established.  The value
against a provisional player is the average of the two ratings (using 1600 if
the player has never played) plus 200 times the outcome (which is -1, 0, 1
for loss, draw and win).  The value against an established player is the
opponent's rating plus 400 times the outcome.

---THIS ISN'T DONE--
Some extra points are now added
to the rating for the purpose of keeping the average rating of all established
active players close to 1720.  In particular, 1/5th of 1720 minus the current
average is added to the rating.
---             ---

To explain the extablished period requires the use of a formula.  Suppose your
rating is r1, and the opponent's is r2.  Let w be 1 if you win, .5 if you
draw, and 0 if you lose.  After a game, your new rating will be:

                                        1
                   r1 + K (w - (-----------------))
                                      (r2-r1)/400
                                1 + 10

I still need to explain the variable K.  This is the largest change your
rating can experience as a result of the game.  The value K=32 is always used
for established player versus established player.  (The USCF has a system in
which this K-factor diminishes for more highly rated players.)  If your
playing a provisional player, the factor K is scaled by n/20, where n is the
number of games your opponent has played.  So, as in the provisional case, if
you play an opponent who has never played, your rating can't change.

This formula has the property that if both players are established then the
sum of the rating changes is zero.  It turns out that if the rating difference
is more than 719 points, then if the strong player wins, there is no change in
either rating.

Note that during the provisional period, BEATING a player whose rating is more
than 400 points below yours will DECREASE your rating.  This is a consequence
of the averaging process.  It's useful too, because it prevents the technique
of getting an inflated provisional rating after one game, and then beating 19
weak players to get an established rating that is too high.
*/

/* Calculates rating change for p1 */
PUBLIC int rating_delta( int p1, int p2, int type, int result )
{
  double k, w, delta;
  int new, idelta;
  statistics *p1_stats;
  statistics *p2_stats;

  if (type == TYPE_BLITZ) {
    p1_stats = &parray[p1].b_stats;
    p2_stats = &parray[p2].b_stats;
  } else {
    p1_stats = &parray[p1].s_stats;
    p2_stats = &parray[p2].s_stats;
  }
  if ((p2_stats->num == 0) && (p1_stats->num == 0)) {
    /* If two unrateds play eachother, give them an average 1600 rating */
    if (result == RESULT_WIN) {
      return 1800;
    } else if (result == RESULT_DRAW) {
      return 1600;
    } else {
      return 1400;
    }
  }
  if (p2_stats->num == 0) {
    return 0;
  }
  if (p1_stats->num == 0) {
    if (result == RESULT_WIN) {
      return p2_stats->rating + 400;
    } else if (result == RESULT_DRAW) {
      return p2_stats->rating;
    } else {
      return p2_stats->rating - 400;
    }
  }
  if (result == RESULT_WIN) {
    w = 1.0;
  } else if (result == RESULT_DRAW) {
    w = 0.5;
  } else {
    w = 0.0;
  }
  if (p1_stats->num < 20) { /* Provisional */
    if (result == RESULT_WIN) {
      new = ((p1_stats->num * p1_stats->rating) + p2_stats->rating + 400) /
            (p1_stats->num + 1);
    } else if (result == RESULT_DRAW) {
      new = ((p1_stats->num * p1_stats->rating) + p2_stats->rating) /
            (p1_stats->num + 1);
    } else {
      new = ((p1_stats->num * p1_stats->rating) + p2_stats->rating - 400) /
            (p1_stats->num + 1);
    }
    delta = new - p1_stats->rating;
  } else {
    if (p2_stats->num < 20) {
      k = 32.0 * (double)p2_stats->num / 20.0;
    } else {
      k = 32.0;
    }
    delta = k * (w - (1 / (1 +
                     pow(10,
                      ((float)(p2_stats->rating-p1_stats->rating)/400.0)))));
    if ((delta <= 1.0) && (result == RESULT_WIN)) delta = 1.0;
    if ((delta >= -1.0) && (result == RESULT_LOSS)) delta = -1.0;
  }
  idelta = delta;
  return idelta;
}

PUBLIC int rating_update( int g )
{
  int wDelta, bDelta;
  int wRes, bRes;
  statistics *w_stats;
  statistics *b_stats;

  if (garray[g].type == TYPE_BLITZ) {
    w_stats = &parray[garray[g].white].b_stats;
    b_stats = &parray[garray[g].black].b_stats;
  } else if (garray[g].type == TYPE_STAND) {
    w_stats = &parray[garray[g].white].s_stats;
    b_stats = &parray[garray[g].black].s_stats;
  } else {
    fprintf( stderr, "FICS: Can't update untimed ratings!\n" );
    return -1;
  }
  switch (garray[g].result) {
    case END_CHECKMATE:
    case END_RESIGN:
    case END_FLAG:
      if (garray[g].winner == WHITE) {
        wRes = RESULT_WIN;
        bRes = RESULT_LOSS;
      } else {
        bRes = RESULT_WIN;
        wRes = RESULT_LOSS;
      }
      break;
    case END_AGREEDDRAW:
    case END_REPITITION:
    case END_50MOVERULE:
    case END_STALEMATE:
    case END_BOTHFLAG:
      wRes = bRes = RESULT_DRAW;
      break;
    default:
      fprintf( stderr, "FICS: Update undecided game %d?\n", garray[g].result );
      return -1;
  }
  wDelta = rating_delta( garray[g].white, garray[g].black,
                          garray[g].type, wRes );
  bDelta = rating_delta( garray[g].black, garray[g].white,
                          garray[g].type, bRes );
  if (wRes == RESULT_WIN) {
    w_stats->win++;
  } else if (wRes == RESULT_LOSS) {
    w_stats->los++;
  } else {
    w_stats->dra++;
  }
  w_stats->num++;
  if (bRes == RESULT_WIN) {
    b_stats->win++;
  } else if (bRes == RESULT_LOSS) {
    b_stats->los++;
  } else {
    b_stats->dra++;
  }
  b_stats->num++;
  rating_remove( w_stats->rating, garray[g].type );
  rating_remove( b_stats->rating, garray[g].type );
  w_stats->rating += wDelta;
  b_stats->rating += bDelta;
  rating_add( w_stats->rating, garray[g].type );
  rating_add( b_stats->rating, garray[g].type );
  BestUpdate(garray[g].white);
  BestUpdate(garray[g].black);
  pprintf( garray[g].white, "Your rating changed by %d\n", wDelta );
  pprintf( garray[g].black, "Your rating changed by %d\n", bDelta );
  save_ratings();
  player_resort( );
  return 0;
}

PUBLIC int com_assess( int p, param_list param )
{
  int p1=p, p2;

  if (param[0].type == TYPE_NULL) {
   if (parray[p].game < 0) {
     pprintf( p, "You are not playing a game.\n" );
     return COM_OK;
   }
   p2 = parray[p].opponent;
  } else {
    p2 = player_find_part_login( param[0].val.word );
    if (p2 < 0) {
      pprintf( p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    if (param[1].type != TYPE_NULL) {
      p1 = p2;
      p2 = player_find_part_login( param[1].val.word );
      if (p2 < 0) {
        pprintf( p, "No user named \"%s\" is logged in.\n", param[1].val.word);
        return COM_OK;
      }
    }
  }
  if (p1 == p2) {
    pprintf( p, "You can't assess the same players.\n" );
    return COM_OK;
  }
  pprintf( p, "\nBlitz\n     %10s (%4s, %4d)     %10s (%4s, %4d)\n",
    parray[p1].name, ratstr(parray[p1].b_stats.rating), parray[p1].b_stats.num,
  parray[p2].name, ratstr(parray[p2].b_stats.rating), parray[p2].b_stats.num );
  pprintf( p, "Win :         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_BLITZ, RESULT_WIN ),
                rating_delta( p2, p1, TYPE_BLITZ, RESULT_LOSS ) );
  pprintf( p, "Draw:         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_BLITZ, RESULT_DRAW ),
                rating_delta( p2, p1, TYPE_BLITZ, RESULT_DRAW ) );
  pprintf( p, "Loss:         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_BLITZ, RESULT_LOSS ),
                rating_delta( p2, p1, TYPE_BLITZ, RESULT_WIN ) );

  pprintf( p, "\nStandard\n     %10s (%4s, %4d)     %10s (%4s, %4d)\n",
   parray[p1].name, ratstr(parray[p1].s_stats.rating), parray[p1].s_stats.num,
  parray[p2].name, ratstr(parray[p2].s_stats.rating), parray[p2].s_stats.num );
  pprintf( p, "Win :         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_STAND, RESULT_WIN ),
                rating_delta( p2, p1, TYPE_STAND, RESULT_LOSS ) );
  pprintf( p, "Draw:         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_STAND, RESULT_DRAW ),
                rating_delta( p2, p1, TYPE_STAND, RESULT_DRAW ) );
  pprintf( p, "Loss:         %3d                         %3d\n",
                rating_delta( p1, p2, TYPE_STAND, RESULT_LOSS ),
                rating_delta( p2, p1, TYPE_STAND, RESULT_WIN ) );

  return COM_OK;
}

PUBLIC int com_best( int p, param_list param )
{
  int i;

  pprintf( p, "Standard           Blitz\n" );
  for (i=0;i<MAX_BEST;i++) {
    if ((i >= numS) && (i >= numB)) break;
    if (i < numS) {
      pprintf( p, "%4d %-14s", bestS[i].rating, bestS[i].name );
    } else {
      pprintf( p, "                   " );
    }
    if (i < numB) {
      pprintf( p, "%4d %-14s\n", bestB[i].rating, bestB[i].name );
    } else {
      pprintf( p, "\n" );
    }
  }
  return COM_OK;
}

PUBLIC int com_statistics( int p, param_list param )
{
  pprintf( p, "                Standard      Blitz\n" );
  pprintf( p, "average:         %7.2f     %7.2f\n", Ratings_S_Average, Ratings_B_Average );
  pprintf( p, "std dev:         %7.2f     %7.2f\n", Ratings_S_StdDev, Ratings_B_StdDev );
  pprintf( p, "number :      %7d     %7d\n", Rs_count, Rb_count);
  return COM_OK;
}
