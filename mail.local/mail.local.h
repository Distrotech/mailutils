/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sysexits.h>
#include "getopt.h"

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/error.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/mutil.h>
#include <mu_dbm.h>

/* Debug */
extern int debug_level;
#define dbg() if (debug_level) debug

/* mailquota settings */
#define MQUOTA_OK         0
#define MQUOTA_EXCEEDED   1
#define MQUOTA_UNLIMITED  2

struct mda_data
{
  FILE *fp;
  char *progfile;
  char *progfile_pattern;
  char **argv;
  char *tempfile;
};

extern char *quotadbname;
extern int exit_code;

extern void setgroupquota (char *str);
extern int check_quota (char *name, size_t size, size_t *rest);

int mda (FILE *fp, char *username, mailbox_t mbox);
char *make_progfile_name (char *pattern, char *username);

#ifdef WITH_GUILE
int prog_mda (struct mda_data *data);
#endif

