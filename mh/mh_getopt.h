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
#define ARG_CHANGECUR		269
#define ARG_CLEAR		270
#define ARG_COMPAT		271
#define ARG_COMPONENT		272
#define ARG_CREATE		273
#define ARG_DATE		274
#define ARG_DATEFIELD		275
#define ARG_DEBUG		276
#define ARG_DELETE		277
#define ARG_DRAFT		278
#define ARG_DRAFTFOLDER		279
#define ARG_DRAFTMESSAGE	280
#define ARG_DUMP		281
#define ARG_EDITOR		282
#define ARG_FAST		283
#define ARG_FCC		        284
#define ARG_FILE		285
#define ARG_FILTER		286
#define ARG_FOLDER		287
#define ARG_FORM		288
#define ARG_FORMAT		289
#define ARG_FORWARD		290
#define ARG_FROM		291
#define ARG_HEADER		292
#define ARG_INPLACE		293
#define ARG_INTERACTIVE		294
#define ARG_LBRACE		295
#define ARG_LENGTH		296
#define ARG_LICENSE		297
#define ARG_LINK		298
#define ARG_LIST		299
#define ARG_MIME		300
#define ARG_MOREPROC		301
#define ARG_MSGID		302
#define ARG_NOAUDIT		303
#define ARG_NOBELL		304
#define ARG_NOCC		305
#define ARG_NOCLEAR		306
#define ARG_NODATE		307
#define ARG_NODRAFTFOLDER	308
#define ARG_NOEDIT		309
#define ARG_NOFILTER		310
#define ARG_NOFORMAT		311
#define ARG_NOFORWARD		312
#define ARG_NOINPLACE		313
#define ARG_NOLIST		314
#define ARG_NOMIME		315
#define ARG_NOMOREPROC		316
#define ARG_NOMSGID		317
#define ARG_NOPUBLIC		318
#define ARG_NOPUSH		319
#define ARG_NOT		        320
#define ARG_NOVERBOSE		321
#define ARG_NOWATCH		322
#define ARG_NOWHATNOWPROC	323
#define ARG_NOZERO		324
#define ARG_OR		        325
#define ARG_PATTERN		326
#define ARG_POP		        327
#define ARG_PRESERVE		328
#define ARG_PRINT		329
#define ARG_PROMPT		330
#define ARG_PUBLIC		331
#define ARG_PUSH		332
#define ARG_QUERY		333
#define ARG_QUIET		334
#define ARG_RBRACE		335
#define ARG_RECURSIVE		336
#define ARG_REVERSE		337
#define ARG_SEQUENCE		338
#define ARG_SOURCE		339
#define ARG_SPLIT		340
#define ARG_SUBJECT		341
#define ARG_TEXT		342
#define ARG_TO		        343
#define ARG_TOTAL		344
#define ARG_TRUNCATE		345
#define ARG_USE		        346
#define ARG_VERBOSE		347
#define ARG_WATCH		348
#define ARG_WHATNOWPROC		349
#define ARG_WIDTH		350
#define ARG_ZERO		351

void mh_argv_preproc __P((int argc, char **argv, struct mh_argp_data *data));
int mh_getopt __P((int argc, char **argv, struct mh_option *mh_opt,
		   const char *doc));
int mh_argp_parse __P((int argc, char **argv,
		       struct argp_option *option,
		       struct mh_option *mh_option,
		       char *argp_doc, char *doc,
		       int (*handler)(), void *closure, int *index));
void mh_help __P((struct mh_option *mh_option, const char *doc));
void mh_license __P((const char *name));
