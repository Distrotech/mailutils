/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003-2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#ifndef _MAILUTILS_POP3_H
#define _MAILUTILS_POP3_H

#include <mailutils/iterator.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_pop3;
typedef struct _mu_pop3 *mu_pop3_t;

#define MU_POP3_DEFAULT_PORT 110
#define MU_POP3_DEFAULT_SSL_PORT 995

int  mu_pop3_create (mu_pop3_t *pop3);
void mu_pop3_destroy (mu_pop3_t *pop3);

int  mu_pop3_set_carrier (mu_pop3_t pop3, mu_stream_t carrier);
int  mu_pop3_get_carrier (mu_pop3_t pop3, mu_stream_t *pcarrier);

int  mu_pop3_connect (mu_pop3_t pop3);
int  mu_pop3_disconnect (mu_pop3_t pop3);

int  mu_pop3_set_timeout (mu_pop3_t pop3, int timeout);
int  mu_pop3_get_timeout (mu_pop3_t pop3, int *timeout);

#define MU_POP3_TRACE_CLR 0
#define MU_POP3_TRACE_SET 1
#define MU_POP3_TRACE_QRY 2
int mu_pop3_trace (mu_pop3_t pop3, int op);
int mu_pop3_trace_mask (mu_pop3_t pop3, int op, int lev);

int  mu_pop3_apop (mu_pop3_t pop3, const char *name, const char *digest);

int  mu_pop3_stls (mu_pop3_t pop3);

/* It is the responsability of the caller to call mu_iterator_destroy() when
   done with the iterator.  The items returned by the iterator are of type
   "const char *", no processing is done on the item except the removal of
   the trailing newline.  */
int mu_pop3_capa (mu_pop3_t pop3, int reread, mu_iterator_t *piter);
int mu_pop3_capa_test (mu_pop3_t pop3, const char *name, const char **pret);

int  mu_pop3_dele (mu_pop3_t pop3, unsigned int mesgno);

int  mu_pop3_list (mu_pop3_t pop3, unsigned int mesgno, size_t *mesg_octet);

/* Send the LIST command and prepare pop3 for receiving a multiline answer.
   The caller is expected to obtain a stream via mu_pop3_stream_create,
   or an iterator via mu_pop3_iterator_create and read data from them.

   This function is not intended for use by a casual user, better use
   mu_pop3_list_all or mu_pop3_list_all_stream. */
int mu_pop3_list_cmd (mu_pop3_t pop3);
int mu_pop3_list_all (mu_pop3_t pop3, mu_iterator_t *pitr);
int mu_pop3_list_all_stream (mu_pop3_t pop3, mu_stream_t *pstream);

int  mu_pop3_noop (mu_pop3_t pop3);

int  mu_pop3_pass (mu_pop3_t pop3, const char *pass);

int  mu_pop3_quit (mu_pop3_t pop3);

/* A stream is returned with the multi-line answer.  It is the responsability
   of the caller to call mu_stream_destroy() to dipose of the stream.  */
int  mu_pop3_retr (mu_pop3_t pop3, unsigned int mesgno,
		   mu_stream_t *pstream);

int  mu_pop3_rset (mu_pop3_t pop3);

int  mu_pop3_stat (mu_pop3_t pop3, size_t *count, mu_off_t *octets);

/* A stream is returned with the multi-line answer.  It is the responsability
   of the caller to call mu_stream_destroy() to dipose of the stream.  */
int  mu_pop3_top (mu_pop3_t pop3, unsigned int mesgno,
		  unsigned int lines, mu_stream_t *pstream);

/* The uidl is malloc'ed and returned in puidl; it is the responsability of
   the caller to free() the uild when done.  */
int  mu_pop3_uidl (mu_pop3_t pop3, unsigned int mesgno, char **puidl);

/* Send the UIDL command and prepare pop3 for receiving a multiline answer.
   The caller is expected to obtain a stream via mu_pop3_stream_create,
   or an iterator via mu_pop3_iterator_create and read data from them.

   This function is not intended for use by a casual user, better use
   mu_pop3_uidl_all or mu_pop3_uidl_all_stream. */
int mu_pop3_uidl_all_cmd (mu_pop3_t pop3);
int mu_pop3_uidl_all (mu_pop3_t pop3, mu_iterator_t *piterator);
int mu_pop3_uidl_all_stream (mu_pop3_t pop3, mu_stream_t *pstream);

int mu_pop3_user (mu_pop3_t pop3, const char *user);

/* Returns the last command acknowledge.  If the server supports RESP-CODE,
   the message could be retrieved, but it is up to the caller to do the
   parsing.  */
int  mu_pop3_response (mu_pop3_t pop3, size_t *nread);

int  mu_pop3_writeline (mu_pop3_t pop3, const char *format, ...)
                                  MU_PRINTFLIKE(2,3);

int  mu_pop3_sendline (mu_pop3_t pop3, const char *line);
int mu_pop3_getline (mu_pop3_t pop3);
int mu_pop3_read_list (mu_pop3_t pop3, mu_list_t list);
int mu_pop3_stream_to_list (mu_pop3_t pop3, mu_stream_t stream,
			    mu_list_t list);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_POP3_H */
