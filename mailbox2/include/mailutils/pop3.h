/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_POP3_H
#define _MAILUTILS_POP3_H

#include <mailutils/iterator.h>
#include <mailutils/stream.h>

__MAILUTILS_BEGIN_DECLS

struct _pop3;
typedef struct _pop3* pop3_t;

enum pop3_state
{
  POP3_NO_STATE,
  POP3_OPEN, POP3_GREETINGS,
  POP3_APOP, POP3_APOP_ACK,
  POP3_AUTH, POP3_AUTH_ACK,
  POP3_CAPA, POP3_CAPA_ACK, POP3_CAPA_RX,
  POP3_DELE, POP3_DELE_ACK,
  POP3_LIST, POP3_LIST_ACK, POP3_LIST_RX,
  POP3_NOOP, POP3_NOOP_ACK,
  POP3_PASS, POP3_PASS_ACK,
  POP3_QUIT, POP3_QUIT_ACK,
  POP3_RETR, POP3_RETR_ACK, POP3_RETR_RX,
  POP3_RSET, POP3_RSET_ACK,
  POP3_STAT, POP3_STAT_ACK,
  POP3_TOP,  POP3_TOP_ACK,  POP3_TOP_RX,
  POP3_UIDL, POP3_UIDL_ACK, POP3_UIDL_RX,
  POP3_USER, POP3_USER_ACK,
  POP3_DONE, POP3_UNKNOWN,  POP3_ERROR
};

struct list_item
{
  unsigned int msgno;
  unsigned int size;
};

struct uidl_item
{
  unsigned int msgno;
  char *uidl;
};

extern int  pop3_create      __P ((pop3_t *));
extern void pop3_destroy     __P ((pop3_t));

extern int  pop3_open        __P ((pop3_t, const char *, unsigned int, int));
extern int  pop3_close       __P ((pop3_t));

extern int  pop3_set_stream  __P ((pop3_t, stream_t));
extern int  pop3_get_stream  __P ((pop3_t, stream_t *));

extern int  pop3_set_timeout __P ((pop3_t, unsigned int));
extern int  pop3_get_timeout __P ((pop3_t, unsigned int *));

extern int  pop3_get_state   __P ((pop3_t, enum pop3_state *));

extern int  pop3_apop        __P ((pop3_t, const char *, const char *));
extern int  pop3_capa        __P ((pop3_t, iterator_t *));
extern int  pop3_dele        __P ((pop3_t, unsigned int));
extern int  pop3_list        __P ((pop3_t, unsigned int, size_t *));
extern int  pop3_list_all    __P ((pop3_t, iterator_t *));
extern int  pop3_noop        __P ((pop3_t));
extern int  pop3_pass        __P ((pop3_t, const char *));
extern int  pop3_quit        __P ((pop3_t));
extern int  pop3_retr        __P ((pop3_t, unsigned int, stream_t *));
extern int  pop3_rset        __P ((pop3_t));
extern int  pop3_stat        __P ((pop3_t, unsigned int *, size_t *));
extern int  pop3_top         __P ((pop3_t, unsigned int, unsigned int, stream_t *));
extern int  pop3_uidl        __P ((pop3_t, unsigned int, char **));
extern int  pop3_uidl_all    __P ((pop3_t, iterator_t *));
extern int  pop3_user        __P ((pop3_t, const char *));

extern int  pop3_readline    __P ((pop3_t, char *, size_t, size_t *));
extern int  pop3_response    __P ((pop3_t, char *, size_t, size_t *));
extern int  pop3_writeline   __P ((pop3_t, const char *, ...));
extern int  pop3_sendline    __P ((pop3_t, const char *));
extern int  pop3_send        __P ((pop3_t));

__MAILUTILS_END_DECLS

#endif /* _MAILUTILS_POP3_H */
