/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_NNTP_H
#define _MAILUTILS_NNTP_H

#include <mailutils/debug.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_nntp;
typedef struct _mu_nntp* mu_nntp_t;

extern int  mu_nntp_create       (mu_nntp_t *nntp);
extern void mu_nntp_destroy      (mu_nntp_t *nntp);

extern int  mu_nntp_set_carrier  (mu_nntp_t nntp, stream_t carrier);
extern int  mu_nntp_get_carrier  (mu_nntp_t nntp, stream_t *pcarrier);

extern int  mu_nntp_connect      (mu_nntp_t nntp);
extern int  mu_nntp_disconnect   (mu_nntp_t nntp);

extern int  mu_nntp_set_timeout  (mu_nntp_t nntp, int timeout);
extern int  mu_nntp_get_timeout  (mu_nntp_t nntp, int *timeout);

extern int  mu_nntp_set_debug    (mu_nntp_t nntp, mu_debug_t debug);

extern int  mu_nntp_stls         (mu_nntp_t nntp);


extern int  mu_nntp_article      (mu_nntp_t nntp, unsigned long num, unsigned long *pnum, char **mid, stream_t *stream);
extern int  mu_nntp_article_id   (mu_nntp_t nntp, const char *id, unsigned long *pnum, char **mid, stream_t *stream);

extern int  mu_nntp_head         (mu_nntp_t nntp, unsigned long num, unsigned long *pnum, char **mid, stream_t *stream);
extern int  mu_nntp_head_id      (mu_nntp_t nntp, const char *name, unsigned long *pnum, char **mid, stream_t *stream);

extern int  mu_nntp_body         (mu_nntp_t nntp, unsigned long num, unsigned long *pnum, char **mid, stream_t *stream);
extern int  mu_nntp_body_id      (mu_nntp_t nntp, const char *name, unsigned long *pnum, char **mid, stream_t *stream);

extern int  mu_nntp_stat         (mu_nntp_t nntp, unsigned long num, char **id);
extern int  mu_nntp_stat_id      (mu_nntp_t nntp, const char *name, char **id);

extern int  mu_nntp_group        (mu_nntp_t nntp, const char *group, long *total, long *first, long *last, char **name);


/* Reads the multi-line response of the server, nread will be 0 when the termination octets
   are detected.  Clients should not use this function unless they are sending direct command.  */
extern int  mu_nntp_readline     (mu_nntp_t nntp, char *buffer, size_t buflen, size_t *nread);

/* Returns the last command acknowledge.  If the server supports RESP-CODE, the message
   could be retrieve, but it is up the caller to do the parsing.  */
extern int  mu_nntp_response     (mu_nntp_t nntp, char *buffer, size_t buflen, size_t *nread);

/* pop3_writeline copies the line in the internal buffer, a mu_pop3_send() is
   needed to do the actual transmission.  */
extern int  mu_nntp_writeline    (mu_nntp_t nntp, const char *format, ...);

/* mu_pop3_sendline() is equivalent to:
       mu_pop3_writeline (pop3, line);
       mu_pop3_send (pop3);
 */
extern int  mu_nntp_sendline     (mu_nntp_t nntp, const char *line);

/* Transmit via the carrier the internal buffer data.  */
extern int  mu_nntp_send         (mu_nntp_t nntp);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_POP3_H */
