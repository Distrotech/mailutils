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

#define ARG_ADD		        266
#define ARG_AFTER		267
#define ARG_ALIAS		268
#define ARG_ALL		        269
#define ARG_AND		        270
#define ARG_ANNOTATE		271
#define ARG_AUDIT		272
#define ARG_AUTO		273
#define ARG_BEFORE		274
#define ARG_BELL		275
#define ARG_BUILD		276
#define ARG_CC		        277
#define ARG_CFLAGS		278
#define ARG_CHANGECUR		279
#define ARG_CHECK		280
#define ARG_CLEAR		281
#define ARG_COMPAT		282
#define ARG_COMPONENT		283
#define ARG_COMPOSE		284
#define ARG_CREATE		285
#define ARG_DATE		286
#define ARG_DATEFIELD		287
#define ARG_DEBUG               288
#define ARG_DELETE              289
#define ARG_DRAFT               290
#define ARG_DRAFTFOLDER         291
#define ARG_DRAFTMESSAGE        292
#define ARG_DRY_RUN             293
#define ARG_DUMP                294
#define ARG_EDITOR              295
#define ARG_FAST                296
#define ARG_FCC                 297
#define ARG_FILE                298
#define ARG_FILTER              299
#define ARG_FOLDER              300
#define ARG_FORM                301
#define ARG_FORMAT              302
#define ARG_FORWARD             303
#define ARG_FROM                304
#define ARG_HEADER              305
#define ARG_INPLACE             306
#define ARG_INTERACTIVE         307
#define ARG_LBRACE              308
#define ARG_LENGTH              309
#define ARG_LICENSE             310
#define ARG_LIMIT               311
#define ARG_LINK                312
#define ARG_LIST                313
#define ARG_MIME                314
#define ARG_MOREPROC            315
#define ARG_MSGID               316
#define ARG_NOAUDIT             317
#define ARG_NOAUTO              318
#define ARG_NOBELL              319
#define ARG_NOCC                320
#define ARG_NOCHECK             321
#define ARG_NOCLEAR             322
#define ARG_NOCOMPOSE           323
#define ARG_NODATE              324
#define ARG_NODATEFIELD         325
#define ARG_NODRAFTFOLDER       326
#define ARG_NOEDIT              327
#define ARG_NOFILTER            328
#define ARG_NOFORMAT            329
#define ARG_NOFORWARD           330
#define ARG_NOHEADERS           331
#define ARG_NOINPLACE           332
#define ARG_NOLIMIT             333
#define ARG_NOLIST              334
#define ARG_NOMIME              335
#define ARG_NOMOREPROC          336
#define ARG_NOMSGID             337
#define ARG_NOPAUSE             338
#define ARG_NOPUBLIC            339
#define ARG_NOPUSH              340
#define ARG_NOREALSIZE          341
#define ARG_NOSERIALONLY        342
#define ARG_NOSHOW              343
#define ARG_NOSTORE             344
#define ARG_NOT                 345
#define ARG_NOTEXTFIELD         346
#define ARG_NOVERBOSE           347
#define ARG_NOWATCH             348
#define ARG_NOWHATNOWPROC       349
#define ARG_NOZERO              350
#define ARG_NUMFIELD            351
#define ARG_OR                  352
#define ARG_PART                353
#define ARG_PATTERN             354
#define ARG_PAUSE               355
#define ARG_POP                 356
#define ARG_PRESERVE            357
#define ARG_PRINT               358
#define ARG_PROMPT              359
#define ARG_PUBLIC              360
#define ARG_PUSH                361
#define ARG_QUERY               362
#define ARG_QUIET               363
#define ARG_RBRACE              364
#define ARG_REALSIZE            365
#define ARG_RECURSIVE           366
#define ARG_REORDER             367
#define ARG_REVERSE             368
#define ARG_SEQUENCE            369
#define ARG_SERIALONLY          370
#define ARG_SHOW                371
#define ARG_SOURCE              372
#define ARG_SPLIT               373
#define ARG_STORE               374
#define ARG_SUBJECT             375
#define ARG_TEXT                376
#define ARG_TEXTFIELD           377
#define ARG_TO                  378
#define ARG_TOTAL               379
#define ARG_TRUNCATE            380
#define ARG_TYPE                381
#define ARG_USE                 382
#define ARG_VERBOSE             383
#define ARG_WATCH               384
#define ARG_WHATNOWPROC         385
#define ARG_WIDTH               386
#define ARG_ZERO                387

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
