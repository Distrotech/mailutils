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

#define ARG_ADD                 257
#define ARG_ALIAS		258
#define ARG_ANNOTATE		259
#define ARG_AUDIT		260
#define ARG_AUTO		261
#define ARG_BUILD		262
#define ARG_COMPONENT           263
#define ARG_CC		        264
#define ARG_CHANGECUR		265
#define ARG_CLEAR		266
#define ARG_COMPAT		267
#define ARG_DATE                268
#define ARG_DEBUG		269
#define ARG_DELETE              270
#define ARG_DRAFT		271
#define ARG_DRAFTFOLDER		272
#define ARG_DRAFTMESSAGE	273
#define ARG_DUMP		274
#define ARG_EDITOR		275
#define ARG_FCC		        276
#define ARG_FILE		277
#define ARG_FILTER		278
#define ARG_FOLDER		279
#define ARG_FORM		280
#define ARG_FORMAT		281
#define ARG_FORWARD		282
#define ARG_HEADER		283
#define ARG_INPLACE		284
#define ARG_INTERACTIVE		285
#define ARG_LICENSE		286
#define ARG_LINK		287
#define ARG_LIST                288
#define ARG_MIME		289
#define ARG_MSGID		290
#define ARG_NOAUDIT		291
#define ARG_NOCC		292
#define ARG_NODATE              293
#define ARG_NODRAFTFOLDER	294
#define ARG_NOEDIT		295
#define ARG_NOFILTER		296
#define ARG_NOFORMAT		297
#define ARG_NOFORWARD		298
#define ARG_NOINPLACE           299
#define ARG_NOMIME		300
#define ARG_NOMSGID		301
#define ARG_NOPUBLIC            302
#define ARG_NOPUSH		303
#define ARG_NOVERBOSE		304
#define ARG_NOWATCH		305
#define ARG_NOWHATNOWPROC	306
#define ARG_NOZERO              307   
#define ARG_PRESERVE		308   
#define ARG_PROMPT		309   
#define ARG_PUBLIC              310 
#define ARG_PUSH		311 
#define ARG_QUERY		312 
#define ARG_QUIET		313   
#define ARG_RECURSIVE		314   
#define ARG_REVERSE		315   
#define ARG_SEQUENCE            316   
#define ARG_SOURCE		317   
#define ARG_SPLIT		318   
#define ARG_TEXT                319   
#define ARG_TRUNCATE		320   
#define ARG_USE		        321   
#define ARG_VERBOSE		322 
#define ARG_WATCH		323 
#define ARG_WHATNOWPROC		324
#define ARG_WIDTH		325
#define ARG_ZERO                326

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
