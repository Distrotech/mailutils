/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SMTP_H
# define _MAILUTILS_SMTP_H

# include <sys/types.h>
# include <sys/socket.h>
# include <mailutils/types.h>

typedef struct _mu_smtp *mu_smtp_t;

int mu_smtp_create (mu_smtp_t *);
void mu_smtp_destroy (mu_smtp_t *);
int mu_smtp_set_carrier (mu_smtp_t smtp, mu_stream_t str);
int mu_smtp_get_carrier (mu_smtp_t smtp, mu_stream_t *pcarrier);

int mu_smtp_open (mu_smtp_t);
int mu_smtp_response (mu_smtp_t smtp);
int mu_smtp_write (mu_smtp_t smtp, const char *fmt, ...);

#define MU_SMTP_TRACE_CLR 0
#define MU_SMTP_TRACE_SET 1
#define MU_SMTP_TRACE_QRY 2
int mu_smtp_trace (mu_smtp_t smtp, int op);
int mu_smtp_trace_mask (mu_smtp_t smtp, int op, int lev);

int mu_smtp_disconnect (mu_smtp_t smtp);
int mu_smtp_ehlo (mu_smtp_t smtp);
int mu_smtp_set_domain (mu_smtp_t smtp, const char *newdom);
int mu_smtp_get_domain (mu_smtp_t smtp, const char **pdom);
int mu_smtp_capa_test (mu_smtp_t smtp, const char *capa, const char **pret);
int mu_smtp_starttls (mu_smtp_t smtp);

int mu_smtp_mail_basic (mu_smtp_t smtp, const char *email, const char *args);
int mu_smtp_rcpt_basic (mu_smtp_t smtp, const char *email, const char *args);
int mu_smtp_send_stream (mu_smtp_t smtp, mu_stream_t str);
int mu_smtp_rset (mu_smtp_t smtp);
int mu_smtp_quit (mu_smtp_t smtp);

#endif
