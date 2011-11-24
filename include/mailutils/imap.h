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

#include <stdarg.h>
#include <mailutils/iterator.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>
#include <mailutils/kwd.h>

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
    MU_IMAP_STATE_LOGOUT,   /* Logout State */
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

int mu_imap_id (mu_imap_t imap, char **idenv, mu_assoc_t *passoc);
  
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


#define MU_IMAP_STAT_DEFINED_FLAGS   0x01
#define MU_IMAP_STAT_PERMANENT_FLAGS 0x02
#define MU_IMAP_STAT_MESSAGE_COUNT   0x04
#define MU_IMAP_STAT_RECENT_COUNT    0x08
#define MU_IMAP_STAT_FIRST_UNSEEN    0x10
#define MU_IMAP_STAT_UIDNEXT         0x20
#define MU_IMAP_STAT_UIDVALIDITY     0x40

struct mu_imap_stat
{
  int flags;                 /* Bitmap of what fields are filled */
  int defined_flags;         /* Flags defined for this mailbox */
  int permanent_flags;       /* Flags that can be changed permanently */
  size_t message_count;      /* Number of messages */
  size_t recent_count;       /* Number of recent messages */
  size_t first_unseen;       /* Sequence number of the first unseen message */
  size_t uidnext;            /* The next unique identifier value. */
  unsigned long uidvalidity; /* The unique identifier validity value. */
};

int mu_imap_select (mu_imap_t imap, const char *mbox, int writable,
		    struct mu_imap_stat *ps);

int mu_imap_status (mu_imap_t imap, const char *mbox, struct mu_imap_stat *ps);

extern struct mu_kwd _mu_imap_status_name_table[];

typedef void (*mu_imap_response_action_t) (mu_imap_t imap, mu_list_t resp,
					   void *data);

int mu_imap_foreach_response (mu_imap_t imap, mu_imap_response_action_t fun,
			      void *data);
  

  /* The following five callbacks correspond to members of struct
     mu_imap_stat and take a pointer to struct mu_imap_stat as their
     only argument. */
#define MU_IMAP_CB_PERMANENT_FLAGS  0
#define MU_IMAP_CB_MESSAGE_COUNT    1
#define MU_IMAP_CB_RECENT_COUNT     2
#define MU_IMAP_CB_FIRST_UNSEEN     3
#define MU_IMAP_CB_UIDNEXT          4 
#define MU_IMAP_CB_UIDVALIDITY      5

  /* The following callbacks correspond to server responses and take two
     argument: a response code (see MU_IMAP_RESPONSE, below), and
     human-readable text string as returned by the server.  The latter can
     be NULL. */
#define MU_IMAP_CB_OK               6
#define MU_IMAP_CB_NO               7
#define MU_IMAP_CB_BAD              8
#define MU_IMAP_CB_BYE              9
#define MU_IMAP_CB_PREAUTH         10
#define _MU_IMAP_CB_MAX            11

typedef void (*mu_imap_callback_t) (void *, int code, va_list ap);
  
void mu_imap_callback (mu_imap_t imap, int code, ...);

void mu_imap_register_callback_function (mu_imap_t imap, int code,
					 mu_imap_callback_t callback,
					 void *data);

#define MU_IMAP_RESPONSE_ALERT            0
#define MU_IMAP_RESPONSE_BADCHARSET       1
#define MU_IMAP_RESPONSE_CAPABILITY       2
#define MU_IMAP_RESPONSE_PARSE            3
#define MU_IMAP_RESPONSE_PERMANENTFLAGS   4
#define MU_IMAP_RESPONSE_READ_ONLY        5     
#define MU_IMAP_RESPONSE_READ_WRITE       6
#define MU_IMAP_RESPONSE_TRYCREATE        7
#define MU_IMAP_RESPONSE_UIDNEXT          8
#define MU_IMAP_RESPONSE_UIDVALIDITY      9
#define MU_IMAP_RESPONSE_UNSEEN          10

extern struct mu_kwd mu_imap_response_codes[];  
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_IMAP_H */
  
  
