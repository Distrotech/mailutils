/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_POP3_H
#define _MAILUTILS_POP3_H

#include <mailutils/list.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_pop3;
typedef struct _mu_pop3* mu_pop3_t;

extern int  mu_pop3_create       (mu_pop3_t *pop3);
extern void mu_pop3_destroy      (mu_pop3_t *pop3);

extern int  mu_pop3_set_carrier  (mu_pop3_t pop3, stream_t carrier);
extern int  mu_pop3_get_carrier  (mu_pop3_t pop3, stream_t *pcarrier);

extern int  mu_pop3_connect      (mu_pop3_t pop3);
extern int  mu_pop3_disconnect   (mu_pop3_t pop3);

extern int  mu_pop3_set_timeout  (mu_pop3_t pop3, int timeout);
extern int  mu_pop3_get_timeout  (mu_pop3_t pop3, int *timeout);

extern int  mu_pop3_set_debug    (mu_pop3_t pop3, void (*print) __P((const char *buffer)));

extern int  mu_pop3_apop         (mu_pop3_t pop3, const char *name, const char *digest);

/* It is the responsability of the caller to call list_destroy() when done
   with the list.  The item in the list is of type "const char *",
   no processing is done on the item except the removal of the trailing newline.  */
extern int  mu_pop3_capa         (mu_pop3_t pop3, list_t *plist);

extern int  mu_pop3_dele         (mu_pop3_t pop3, unsigned int mesgno);

extern int  mu_pop3_list         (mu_pop3_t pop3, unsigned int mesgno, size_t *mesg_octet);

/* A list is return with the multi-line answer.  It is the responsability of
   the caller to call list_destroy() to dipose of the list.  */
extern int  mu_pop3_list_all     (mu_pop3_t pop3, list_t *plist);

extern int  mu_pop3_noop         (mu_pop3_t pop3);

extern int  mu_pop3_pass         (mu_pop3_t pop3, const char *pass);

extern int  mu_pop3_quit         (mu_pop3_t pop3);

/* A stream is return with the multi-line answer.  It is the responsability of
   the caller to call stream_destroy() to dipose of the stream.  */
extern int  mu_pop3_retr         (mu_pop3_t pop3, unsigned int mesgno, stream_t *pstream);

extern int  mu_pop3_rset         (mu_pop3_t pop3);

extern int  mu_pop3_stat         (mu_pop3_t pop3, unsigned int *count, size_t *octets);

/* A stream is return with the multi-line answer.  It is the responsability of
   the caller to call stream_destroy() to dipose of the stream.  */
extern int  mu_pop3_top          (mu_pop3_t pop3, unsigned int mesgno, unsigned int lines, stream_t *pstream);

/* The uidl is malloc and return in puidl, it is the responsability of caller
   to free() the uild when done.  */
extern int  mu_pop3_uidl         (mu_pop3_t pop3, unsigned int mesgno, char **puidl);
/* A list is return with the multi-line answer.  It is the responsability of
   the caller to call list_destroy() to dipose of the list.  */
extern int  mu_pop3_uidl_all     (mu_pop3_t pop3, list_t *plist);

extern int  mu_pop3_user         (mu_pop3_t pop3, const char *user);


/* Reads the multi-line response of the server, nread will be 0 when the termination octets
   are detected.  Clients should not use this function unless they are sending direct command.  */
extern int  mu_pop3_readline     (mu_pop3_t pop3, char *buffer, size_t buflen, size_t *nread);

/* Returns the last command acknowledge.  If the server supports RESP-CODE, the message
   could be retrieve, but it is up the caller to do the parsing.  */
extern int  mu_pop3_response     (mu_pop3_t pop3, char *buffer, size_t buflen, size_t *nread);

/* pop3_writeline copies the line in the internal buffer, a mu_pop3_send() is
   needed to do the actual transmission.  */ 
extern int  mu_pop3_writeline    (mu_pop3_t pop3, const char *format, ...);

/* mu_pop3_sendline() is equivalent to:
       mu_pop3_writeline (pop3, line);
       mu_pop3_send (pop3);
 */
extern int  mu_pop3_sendline     (mu_pop3_t pop3, const char *line);

/* Transmit via the carrier the internal buffer data.  */
extern int  mu_pop3_send         (mu_pop3_t pop3);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_POP3_H */
