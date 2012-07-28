/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

enum
  {
    MU_SMTP_PARAM_DOMAIN,
    MU_SMTP_PARAM_USERNAME,
    MU_SMTP_PARAM_PASSWORD,
    MU_SMTP_PARAM_SERVICE,
    MU_SMTP_PARAM_REALM,
    MU_SMTP_PARAM_HOST,
    MU_SMTP_PARAM_URL
  };

#define MU_SMTP_MAX_PARAM (MU_SMTP_PARAM_URL+1)

int mu_smtp_create (mu_smtp_t *);
void mu_smtp_destroy (mu_smtp_t *);
int mu_smtp_set_carrier (mu_smtp_t smtp, mu_stream_t str);
int mu_smtp_get_carrier (mu_smtp_t smtp, mu_stream_t *pcarrier);

int mu_smtp_open (mu_smtp_t);
int mu_smtp_response (mu_smtp_t smtp);
int mu_smtp_write (mu_smtp_t smtp, const char *fmt, ...) MU_PRINTFLIKE(2,3);

int mu_smtp_replcode (mu_smtp_t smtp, char *buf);
int mu_smtp_sget_reply (mu_smtp_t smtp, const char **pbuf);
int mu_smtp_get_reply_iterator (mu_smtp_t smtp, mu_iterator_t *pitr);

int mu_smtp_cmd (mu_smtp_t smtp, int argc, char **argv);

#define MU_SMTP_TRACE_CLR 0
#define MU_SMTP_TRACE_SET 1
#define MU_SMTP_TRACE_QRY 2
int mu_smtp_trace (mu_smtp_t smtp, int op);
int mu_smtp_trace_mask (mu_smtp_t smtp, int op, int lev);

int mu_smtp_disconnect (mu_smtp_t smtp);
int mu_smtp_ehlo (mu_smtp_t smtp);
int mu_smtp_set_param (mu_smtp_t smtp, int code, const char *val);
int mu_smtp_get_param (mu_smtp_t smtp, int code, const char **param);
int mu_smtp_test_param (mu_smtp_t smtp, int pcode);
int mu_smtp_set_url (mu_smtp_t smtp, mu_url_t url);
int mu_smtp_get_url (mu_smtp_t smtp, mu_url_t *purl);
int mu_smtp_set_secret (mu_smtp_t smtp, mu_secret_t secret);
int mu_smtp_get_secret (mu_smtp_t smtp, mu_secret_t *secret);

int mu_smtp_capa_test (mu_smtp_t smtp, const char *capa, const char **pret);
int mu_smtp_capa_iterator (mu_smtp_t smtp, mu_iterator_t *itr);
int mu_smtp_starttls (mu_smtp_t smtp);

int mu_smtp_mail_basic (mu_smtp_t smtp, const char *email,
			const char *fmt, ...) MU_PRINTFLIKE(3,4);
int mu_smtp_rcpt_basic (mu_smtp_t smtp, const char *email,
			const char *fmt, ...) MU_PRINTFLIKE(3,4);

int mu_smtp_data (mu_smtp_t smtp, mu_stream_t *pstream);
int mu_smtp_send_stream (mu_smtp_t smtp, mu_stream_t str);
int mu_smtp_dot (mu_smtp_t smtp);
int mu_smtp_rset (mu_smtp_t smtp);
int mu_smtp_quit (mu_smtp_t smtp);

int mu_smtp_auth (mu_smtp_t smtp);

int mu_smtp_add_auth_mech (mu_smtp_t smtp, const char *mech);
int mu_smtp_clear_auth_mech (mu_smtp_t smtp);
int mu_smtp_add_auth_mech_list (mu_smtp_t smtp, mu_list_t list);
int mu_smtp_mech_select (mu_smtp_t smtp, const char **pmech);

#endif
