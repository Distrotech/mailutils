/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mailutils/argp.h>
#include <mailutils/nls.h>

#define MH_OPT_BOOL 1
#define MH_OPT_ARG  2

struct mh_option
{
  char *opt;
  int match_len;
  int flags;
  char *arg;
};

struct mh_argp_data
{
  struct mh_option *mh_option;
  int (*handler)();
  int errind;
  void *closure;
  char *doc;
};

#define ARG_ADD		        257
#define ARG_AFTER		258
#define ARG_ALIAS		259
#define ARG_ALL		        260
#define ARG_AND		        261
#define ARG_ANNOTATE		262
#define ARG_AUDIT		263
#define ARG_AUTO		264
#define ARG_BEFORE		265
#define ARG_BELL		266
#define ARG_BUILD		267
#define ARG_CC		        268
#define ARG_CFLAGS		269
#define ARG_CHANGECUR		270
#define ARG_CHECK		271
#define ARG_CLEAR		272
#define ARG_COMPAT		273
#define ARG_COMPONENT		274
#define ARG_COMPOSE		275
#define ARG_CREATE		276
#define ARG_DATE		277
#define ARG_DATEFIELD		278
#define ARG_DEBUG		279
#define ARG_DELETE		280
#define ARG_DRAFT		281
#define ARG_DRAFTFOLDER		282
#define ARG_DRAFTMESSAGE	283
#define ARG_DRY_RUN		284
#define ARG_DUMP		285
#define ARG_EDITOR		286
#define ARG_FAST		287
#define ARG_FCC		        288
#define ARG_FILE		289
#define ARG_FILTER		290
#define ARG_FOLDER		291
#define ARG_FORM		292
#define ARG_FORMAT		293
#define ARG_FORWARD		294
#define ARG_FROM		295
#define ARG_GROUP               296 
#define ARG_HEADER		297
#define ARG_INPLACE		298
#define ARG_INTERACTIVE		299
#define ARG_LBRACE		300
#define ARG_LENGTH		301
#define ARG_LICENSE		302
#define ARG_LIMIT		303
#define ARG_LINK		304
#define ARG_LIST		305
#define ARG_MIME		306
#define ARG_MOREPROC		307
#define ARG_MSGID		308
#define ARG_NOALIAS             309
#define ARG_NOAUDIT		310
#define ARG_NOAUTO		311
#define ARG_NOBELL		312
#define ARG_NOCC		313
#define ARG_NOCHANGECUR		314
#define ARG_NOCHECK		315
#define ARG_NOCLEAR		316
#define ARG_NOCOMPOSE		317
#define ARG_NOCREATE		318
#define ARG_NODATE		319
#define ARG_NODATEFIELD		320
#define ARG_NODRAFTFOLDER	321
#define ARG_NOEDIT		322
#define ARG_NOFAST		323
#define ARG_NOFILTER		324
#define ARG_NOFORMAT		325
#define ARG_NOFORWARD		326
#define ARG_NOHEADER		327
#define ARG_NOHEADERS		328
#define ARG_NOINTERACTIVE       329 
#define ARG_NOINPLACE		330 
#define ARG_NOLIMIT		331 
#define ARG_NOLIST		332 
#define ARG_NOMIME		333 
#define ARG_NOMOREPROC		334 
#define ARG_NOMSGID		335 
#define ARG_NOPAUSE		336 
#define ARG_NOPUBLIC		337 
#define ARG_NOPUSH		338 
#define ARG_NOREALSIZE		339 
#define ARG_NORECURSIVE         340 
#define ARG_NOREVERSE		341
#define ARG_NORMALIZE           342
#define ARG_NOSERIALONLY	343 
#define ARG_NOSHOW		344 
#define ARG_NOSTORE		345 
#define ARG_NOT		        346 
#define ARG_NOTEXTFIELD		347 
#define ARG_NOTOTAL		348 
#define ARG_NOTRUNCATE		349 
#define ARG_NOUSE		350 
#define ARG_NOVERBOSE		351 
#define ARG_NOWATCH		352 
#define ARG_NOWHATNOWPROC	353 
#define ARG_NOZERO		354 
#define ARG_NUMFIELD		355 
#define ARG_OR		        356 
#define ARG_PART		357 
#define ARG_PATTERN		358 
#define ARG_PAUSE		359 
#define ARG_POP		        360 
#define ARG_PRESERVE		361 
#define ARG_PRINT		362 
#define ARG_PROMPT		363 
#define ARG_PUBLIC		364 
#define ARG_PUSH		365 
#define ARG_QUERY		366 
#define ARG_QUIET		367 
#define ARG_RBRACE		368 
#define ARG_REALSIZE		369 
#define ARG_RECURSIVE		370 
#define ARG_REORDER		371 
#define ARG_REVERSE		372 
#define ARG_SEQUENCE		373 
#define ARG_SERIALONLY		374 
#define ARG_SHOW		375 
#define ARG_SOURCE		376 
#define ARG_SPLIT		377 
#define ARG_STORE		378 
#define ARG_SUBJECT		379 
#define ARG_TEXT		380 
#define ARG_TEXTFIELD		381 
#define ARG_TO		        382 
#define ARG_TOTAL		383 
#define ARG_TRUNCATE		384 
#define ARG_TYPE		385 
#define ARG_USE		        386
#define ARG_USER                387
#define ARG_VERBOSE		388 
#define ARG_WATCH		389 
#define ARG_WHATNOWPROC		390 
#define ARG_WIDTH		391 
#define ARG_ZERO		392 

void mh_argv_preproc __P((int argc, char **argv, struct mh_argp_data *data));
int mh_getopt __P((int argc, char **argv, struct mh_option *mh_opt,
		   const char *doc));
int mh_argp_parse __P((int *argc, char **argv[],
		       int flags,
		       struct argp_option *option,
		       struct mh_option *mh_option,
		       char *argp_doc, char *doc,
		       int (*handler)(), void *closure, int *index));
void mh_help __P((struct mh_option *mh_option, const char *doc));
void mh_license __P((const char *name));
