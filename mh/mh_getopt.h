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
#define ARG_HEADER		296
#define ARG_INPLACE		297
#define ARG_INTERACTIVE		298
#define ARG_LBRACE		299
#define ARG_LENGTH		300
#define ARG_LICENSE		301
#define ARG_LIMIT		302
#define ARG_LINK		303
#define ARG_LIST		304
#define ARG_MIME		305
#define ARG_MOREPROC		306
#define ARG_MSGID		307
#define ARG_NOAUDIT		308
#define ARG_NOAUTO		309
#define ARG_NOBELL		310
#define ARG_NOCC		311
#define ARG_NOCHANGECUR		312
#define ARG_NOCHECK		313
#define ARG_NOCLEAR		314
#define ARG_NOCOMPOSE		315
#define ARG_NOCREATE		316
#define ARG_NODATE		317
#define ARG_NODATEFIELD		318
#define ARG_NODRAFTFOLDER	319
#define ARG_NOEDIT		320
#define ARG_NOFAST		321
#define ARG_NOFILTER		322
#define ARG_NOFORMAT		323
#define ARG_NOFORWARD		324
#define ARG_NOHEADER		325
#define ARG_NOHEADERS		326
#define ARG_NOINTERACTIVE       327 
#define ARG_NOINPLACE		328 
#define ARG_NOLIMIT		329 
#define ARG_NOLIST		330 
#define ARG_NOMIME		331 
#define ARG_NOMOREPROC		332 
#define ARG_NOMSGID		333 
#define ARG_NOPAUSE		334 
#define ARG_NOPUBLIC		335 
#define ARG_NOPUSH		336 
#define ARG_NOREALSIZE		337 
#define ARG_NORECURSIVE         338 
#define ARG_NOREVERSE		339 
#define ARG_NOSERIALONLY	340 
#define ARG_NOSHOW		341 
#define ARG_NOSTORE		342 
#define ARG_NOT		        343 
#define ARG_NOTEXTFIELD		344 
#define ARG_NOTOTAL		345 
#define ARG_NOTRUNCATE		346 
#define ARG_NOUSE		347 
#define ARG_NOVERBOSE		348 
#define ARG_NOWATCH		349 
#define ARG_NOWHATNOWPROC	350 
#define ARG_NOZERO		351 
#define ARG_NUMFIELD		352 
#define ARG_OR		        353 
#define ARG_PART		354 
#define ARG_PATTERN		355 
#define ARG_PAUSE		356 
#define ARG_POP		        357 
#define ARG_PRESERVE		358 
#define ARG_PRINT		359 
#define ARG_PROMPT		360 
#define ARG_PUBLIC		361 
#define ARG_PUSH		362 
#define ARG_QUERY		363 
#define ARG_QUIET		364 
#define ARG_RBRACE		365 
#define ARG_REALSIZE		366 
#define ARG_RECURSIVE		367 
#define ARG_REORDER		368 
#define ARG_REVERSE		369 
#define ARG_SEQUENCE		370 
#define ARG_SERIALONLY		371 
#define ARG_SHOW		372 
#define ARG_SOURCE		373 
#define ARG_SPLIT		374 
#define ARG_STORE		375 
#define ARG_SUBJECT		376 
#define ARG_TEXT		377 
#define ARG_TEXTFIELD		378 
#define ARG_TO		        379 
#define ARG_TOTAL		380 
#define ARG_TRUNCATE		381 
#define ARG_TYPE		382 
#define ARG_USE		        383 
#define ARG_VERBOSE		384 
#define ARG_WATCH		385 
#define ARG_WHATNOWPROC		386 
#define ARG_WIDTH		387 
#define ARG_ZERO		388

void mh_argv_preproc __P((int argc, char **argv, struct mh_argp_data *data));
int mh_getopt __P((int argc, char **argv, struct mh_option *mh_opt,
		   const char *doc));
int mh_argp_parse __P((int argc, char **argv,
		       int flags,
		       struct argp_option *option,
		       struct mh_option *mh_option,
		       char *argp_doc, char *doc,
		       int (*handler)(), void *closure, int *index));
void mh_help __P((struct mh_option *mh_option, const char *doc));
void mh_license __P((const char *name));
