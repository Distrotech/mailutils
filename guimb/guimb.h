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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>
#include <mailutils/error.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>

#include <libguile.h>

extern char *program_file;
extern char *program_expr;
extern char *user_name;
extern char *default_mailbox;
extern mailbox_t mbox;
extern size_t nmesg;
extern size_t current_mesg_no;
extern message_t current_message;
extern int debug_guile;

void collect_open_mailbox_file __P ((void));
int collect_append_file __P ((char *name));
void collect_create_mailbox __P ((void));
void collect_drop_mailbox __P ((void));
int collect_output (void);

void util_error (char *fmt, ...);
int util_tempfile (char **namep);
