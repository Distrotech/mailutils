/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_IMAP_H
#define _MAILUTILS_IMAP_H

#include <mailutils/iterator.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_IMAP_DEFAULT_PORT 143
#define MU_IMAP_DEFAULT_SSL_PORT 993
  
typedef struct _mu_imap *mu_imap_t;

enum mu_imap_state
  {
    MU_IMAP_STATE_INIT,     /* Initial state */
    MU_IMAP_STATE_NONAUTH,  /* Non-Authenticated State */
    MU_IMAP_STATE_AUTH,     /* Authenticated State */
    MU_IMAP_STATE_SELECTED, /* Selected State */
    MU_IMAP_STATE_LOGOUT    /* Logout State */
  };
  
int mu_imap_create (mu_imap_t *pimap);
void mu_imap_destroy (mu_imap_t *pimap);

int mu_imap_connect (mu_imap_t imap);
int mu_imap_disconnect (mu_imap_t imap);

int mu_imap_capability (mu_imap_t imap, int reread, mu_iterator_t *piter);
int mu_imap_capability_test (mu_imap_t imap, const char *name,
			     const char **pret);

int mu_imap_login (mu_imap_t imap, const char *user, const char *pass);
int mu_imap_logout (mu_imap_t imap);

int mu_imap_id (mu_imap_t imap, char **idenv, mu_list_t *plist);
  
int mu_imap_set_carrier (mu_imap_t imap, mu_stream_t carrier);
int mu_imap_get_carrier (mu_imap_t imap, mu_stream_t *pcarrier);

#define MU_IMAP_TRACE_CLR 0
#define MU_IMAP_TRACE_SET 1
#define MU_IMAP_TRACE_QRY 2
int mu_imap_trace (mu_imap_t imap, int op);
int mu_imap_trace_mask (mu_imap_t imap, int op, int lev);

int mu_imap_strerror (mu_imap_t imap, const char **pstr);

int mu_imap_state (mu_imap_t imap, int *pstate);
int mu_imap_state_str (int state, const char **pstr);

int mu_imap_tag (mu_imap_t imap, const char **pseq);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_IMAP_H */
  
  
