/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/datetime.h>
#include <mailutils/kwd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_IMAP_DEFAULT_PORT 143
#define MU_IMAP_DEFAULT_SSL_PORT 993
  
typedef struct _mu_imap *mu_imap_t;

enum mu_imap_session_state
  {
    MU_IMAP_SESSION_INIT,     /* Initial state (disconnected) */
    MU_IMAP_SESSION_NONAUTH,  /* Non-Authenticated State */
    MU_IMAP_SESSION_AUTH,     /* Authenticated State */
    MU_IMAP_SESSION_SELECTED  /* Selected State */
  };
  
int mu_imap_create (mu_imap_t *pimap);
void mu_imap_destroy (mu_imap_t *pimap);

int mu_imap_connect (mu_imap_t imap);
int mu_imap_disconnect (mu_imap_t imap);

int mu_imap_iserror (mu_imap_t imap);
void mu_imap_clearerr (mu_imap_t imap);
  
int mu_imap_capability (mu_imap_t imap, int reread, mu_iterator_t *piter);
int mu_imap_capability_test (mu_imap_t imap, const char *name,
			     const char **pret);

int mu_imap_starttls (mu_imap_t imap);
  
int mu_imap_login (mu_imap_t imap, const char *user, const char *pass);
int mu_imap_login_secret (mu_imap_t imap, const char *user,
			  mu_secret_t secret);
int mu_imap_logout (mu_imap_t imap);

int mu_imap_id (mu_imap_t imap, char **idenv, mu_assoc_t *passoc);

int mu_imap_noop (mu_imap_t imap);
int mu_imap_check (mu_imap_t imap);

int mu_imap_fetch (mu_imap_t imap, int uid, mu_msgset_t msgset,
		   const char *items);
int mu_imap_store (mu_imap_t imap, int uid, mu_msgset_t msgset,
		   const char *items);

#define MU_IMAP_STORE_SET 0
#define MU_IMAP_STORE_ADD 1
#define MU_IMAP_STORE_CLR 2
#define MU_IMAP_STORE_SILENT 0x10

#define MU_IMAP_STORE_OPMASK 0xf

int mu_imap_store_flags (mu_imap_t imap, int uid, mu_msgset_t msgset,
			 int op, int flags);
  
int mu_imap_delete (mu_imap_t imap, const char *mailbox);
int mu_imap_rename (mu_imap_t imap, const char *mailbox,
		    const char *new_mailbox);
int mu_imap_copy (mu_imap_t imap, int uid, mu_msgset_t msgset,
		  const char *mailbox);

int mu_imap_close (mu_imap_t imap);
int mu_imap_unselect (mu_imap_t imap);

int mu_imap_expunge (mu_imap_t imap);

int mu_imap_mailbox_create (mu_imap_t imap, const char *mailbox);

int mu_imap_append_stream_size (mu_imap_t imap, const char *mailbox, int flags,
				struct tm *tm, struct mu_timezone *tz,
				mu_stream_t stream, mu_off_t size);
int mu_imap_append_stream (mu_imap_t imap, const char *mailbox, int flags,
			   struct tm *tm, struct mu_timezone *tz,
			   mu_stream_t stream);
int mu_imap_append_message (mu_imap_t imap, const char *mailbox, int flags,
			    struct tm *tm, struct mu_timezone *tz,
			    mu_message_t msg);
  
int mu_imap_genlist (mu_imap_t imap, int lsub,
		     const char *refname, const char *mboxname,
		     mu_list_t retlist);
int mu_imap_genlist_new (mu_imap_t imap, int lsub,
			 const char *refname, const char *mboxname,
			 mu_list_t *plist);
  
int mu_imap_list (mu_imap_t imap, const char *refname, const char *mboxname,
		  mu_list_t retlist);
int mu_imap_list_new (mu_imap_t imap, const char *refname,
		      const char *mboxname, mu_list_t *plist);

int mu_imap_lsub (mu_imap_t imap, const char *refname, const char *mboxname,
		  mu_list_t retlist);
int mu_imap_lsub_new (mu_imap_t imap, const char *refname,
		      const char *mboxname, mu_list_t *plist);

int mu_imap_subscribe (mu_imap_t imap, const char *mailbox);
int mu_imap_unsubscribe (mu_imap_t imap, const char *mailbox);
  
int mu_imap_set_carrier (mu_imap_t imap, mu_stream_t carrier);
int mu_imap_get_carrier (mu_imap_t imap, mu_stream_t *pcarrier);

#define MU_IMAP_TRACE_CLR 0
#define MU_IMAP_TRACE_SET 1
#define MU_IMAP_TRACE_QRY 2
int mu_imap_trace (mu_imap_t imap, int op);
int mu_imap_trace_mask (mu_imap_t imap, int op, int lev);

enum mu_imap_response mu_imap_response (mu_imap_t imap);
int mu_imap_response_code (mu_imap_t imap);
  
int mu_imap_strerror (mu_imap_t imap, const char **pstr);

int mu_imap_session_state (mu_imap_t imap);
int mu_imap_session_state_str (int state, const char **pstr);

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

  /* The following five callbacks correspond to members of struct
     mu_imap_stat and take a pointer to struct mu_imap_stat as their
     PDAT argument.  SDAT is always 0. */
#define MU_IMAP_CB_PERMANENT_FLAGS  0
#define MU_IMAP_CB_MESSAGE_COUNT    1
#define MU_IMAP_CB_RECENT_COUNT     2
#define MU_IMAP_CB_FIRST_UNSEEN     3
#define MU_IMAP_CB_UIDNEXT          4 
#define MU_IMAP_CB_UIDVALIDITY      5

  /* The following callbacks correspond to unsolicited server responses and
     take two arguments: a response code (see MU_IMAP_RESPONSE, below) in
     SDAT, and human-readable text string as returned by the server in PDAT.
     The latter can be NULL. */
#define MU_IMAP_CB_OK               6
#define MU_IMAP_CB_NO               7
#define MU_IMAP_CB_BAD              8
#define MU_IMAP_CB_BYE              9
#define MU_IMAP_CB_PREAUTH         10

  /* These corresponde to the tagged server responses.  The calling convention
     is the same as above. */
#define MU_IMAP_CB_TAGGED_OK       11  
#define MU_IMAP_CB_TAGGED_NO       12
#define MU_IMAP_CB_TAGGED_BAD      13
  
  /* FETCH callback.  Arguments: SDAT - message sequence number, PDAT - a
     list (mu_list_t) of union mu_imap_fetch_response (see below). */
#define MU_IMAP_CB_FETCH           14

#define _MU_IMAP_CB_MAX            15

typedef void (*mu_imap_callback_t) (void *, int code, size_t sdat, void *pdat);
  
void mu_imap_callback (mu_imap_t imap, int code, size_t sdat, void *pdat);

void mu_imap_register_callback_function (mu_imap_t imap, int code,
					 mu_imap_callback_t callback,
					 void *data);

#define MU_IMAP_RESPONSE_UNKNOWN           0 
#define MU_IMAP_RESPONSE_ALERT             1
#define MU_IMAP_RESPONSE_BADCHARSET        2
#define MU_IMAP_RESPONSE_CAPABILITY        3
#define MU_IMAP_RESPONSE_PARSE             4
#define MU_IMAP_RESPONSE_PERMANENTFLAGS    5
#define MU_IMAP_RESPONSE_READ_ONLY         6   
#define MU_IMAP_RESPONSE_READ_WRITE        7
#define MU_IMAP_RESPONSE_TRYCREATE         8
#define MU_IMAP_RESPONSE_UIDNEXT           9
#define MU_IMAP_RESPONSE_UIDVALIDITY      10
#define MU_IMAP_RESPONSE_UNSEEN           11

extern struct mu_kwd mu_imap_response_codes[];  

  /* FETCH Response Codes */

  /* BODY[<section>]<<origin octet>> */
#define MU_IMAP_FETCH_BODY                 0
  /* BODY & BODYSTRUCTURE */
#define MU_IMAP_FETCH_BODYSTRUCTURE        1
  /* ENVELOPE */
#define MU_IMAP_FETCH_ENVELOPE             2 
  /* FLAGS */
#define MU_IMAP_FETCH_FLAGS                3
  /* INTERNALDATE */
#define MU_IMAP_FETCH_INTERNALDATE         4
  /* RFC822.SIZE */
#define MU_IMAP_FETCH_RFC822_SIZE          5
  /* UID */
#define MU_IMAP_FETCH_UID                  6

struct mu_imap_fetch_body
{
  int type;
  size_t *partv;
  size_t partc;
  char *section;
  mu_list_t fields;
  char *text;
};

struct mu_imap_fetch_bodystructure
{
  int type;
  struct mu_bodystructure *bs;
};
  
struct mu_imap_fetch_envelope
{
  int type;
  struct mu_imapenvelope *imapenvelope;
};

struct mu_imap_fetch_flags
{
  int type;
  int flags;
};

struct mu_imap_fetch_internaldate
{
  int type;
  struct tm tm;
  struct mu_timezone tz;
};
  
struct mu_imap_fetch_rfc822_size
{
  int type;
  size_t size;
};
  
struct mu_imap_fetch_uid
{
  int type;
  size_t uid;
};

union mu_imap_fetch_response
{
  int type;
  struct mu_imap_fetch_body body;
  struct mu_imap_fetch_bodystructure bodystructure;
  struct mu_imap_fetch_envelope envelope;
  struct mu_imap_fetch_flags flags;
  struct mu_imap_fetch_internaldate internaldate;
  struct mu_imap_fetch_rfc822_size rfc822_size;
  struct mu_imap_fetch_uid uid;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_IMAP_H */
  
  
