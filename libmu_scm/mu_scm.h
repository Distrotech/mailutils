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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>
#include <mailutils/error.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>
#include <mailutils/mutil.h>

#include <libguile.h>

extern SCM _mu_scm_mailer;
extern SCM _mu_scm_debug;

extern SCM scm_makenum __P((unsigned long val));
extern void mu_scm_init __P((void));

extern void mu_scm_mailbox_init __P((void));
extern SCM mu_scm_mailbox_create __P((mailbox_t mbox));
extern int mu_scm_is_mailbox __P((SCM scm));

extern void mu_scm_message_init __P((void));
extern SCM mu_scm_message_create __P((SCM owner, message_t msg));
extern int mu_scm_is_message __P((SCM scm));
extern const message_t mu_scm_message_get __P((SCM MESG));

extern int mu_scm_is_body __P((SCM scm));
extern void mu_scm_body_init __P((void));
extern SCM mu_scm_body_create __P((SCM mesg, body_t body));

extern void mu_scm_address_init __P((void));
extern void mu_scm_logger_init __P((void));

extern void mu_scm_port_init __P((void));
extern SCM mu_port_make_from_stream __P((SCM msg, stream_t stream, long mode));

extern void mu_scm_mime_init __P((void));
extern void mu_scm_message_add_owner __P((SCM MESG, SCM owner));
