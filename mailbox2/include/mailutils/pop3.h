/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_POP3_H
#define _MAILUTILS_POP3_H

#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/iterator.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef iterator_t pop3_capa_iterator_t;
typedef iterator_t pop3_list_iterator_t;
typedef iterator_t pop3_uidl_iterator_t;

extern int  pop3_create       __P ((pop3_t *));
extern void pop3_destroy      __P ((pop3_t *));

extern int  pop3_connect      __P ((pop3_t, const char *, unsigned int));
extern int  pop3_disconnect   __P ((pop3_t));

extern int  pop3_set_carrier  __P ((pop3_t, stream_t));
extern int  pop3_get_carrier  __P ((pop3_t, stream_t *));

extern int  pop3_set_timeout  __P ((pop3_t, int));
extern int  pop3_get_timeout  __P ((pop3_t, int *));

extern int  pop3_set_debug    __P ((pop3_t, mu_debug_t));
extern int  pop3_get_debug    __P ((pop3_t, mu_debug_t *));

extern int  pop3_apop         __P ((pop3_t, const char *, const char *));

extern int  pop3_capa         __P ((pop3_t, iterator_t *));
extern int  pop3_capa_first   __P ((pop3_capa_iterator_t));
extern int  pop3_capa_current __P ((pop3_capa_iterator_t, char **));
extern int  pop3_capa_next    __P ((pop3_capa_iterator_t));
extern int  pop3_capa_is_done __P ((pop3_capa_iterator_t));
extern void pop3_capa_destroy __P ((pop3_capa_iterator_t *));

extern int  pop3_dele         __P ((pop3_t, unsigned int));

extern int  pop3_list         __P ((pop3_t, unsigned int, size_t *));
extern int  pop3_list_all     __P ((pop3_t, pop3_list_iterator_t *));
extern int  pop3_list_first   __P ((pop3_list_iterator_t));
extern int  pop3_list_current __P ((pop3_list_iterator_t,
				    unsigned int *, size_t *));
extern int  pop3_list_next    __P ((pop3_list_iterator_t));
extern int  pop3_list_is_done __P ((pop3_list_iterator_t));
extern void pop3_list_destroy __P ((pop3_list_iterator_t *));

extern int  pop3_noop         __P ((pop3_t));
extern int  pop3_pass         __P ((pop3_t, const char *));
extern int  pop3_quit         __P ((pop3_t));
extern int  pop3_retr         __P ((pop3_t, unsigned int, stream_t *));
extern int  pop3_rset         __P ((pop3_t));
extern int  pop3_stat         __P ((pop3_t, unsigned int *, size_t *));
extern int  pop3_top          __P ((pop3_t, unsigned int,
				   unsigned int, stream_t *));

extern int  pop3_uidl         __P ((pop3_t, unsigned int, char **));
extern int  pop3_uidl_all     __P ((pop3_t, pop3_uidl_iterator_t *));
extern int  pop3_uidl_first   __P ((pop3_uidl_iterator_t));
extern int  pop3_uidl_is_done __P ((pop3_uidl_iterator_t));
extern int  pop3_uidl_current __P ((pop3_uidl_iterator_t,
				    unsigned int *, char **));
extern int  pop3_uidl_next    __P ((pop3_uidl_iterator_t));
extern void pop3_uidl_destroy __P ((pop3_uidl_iterator_t *));

extern int  pop3_user         __P ((pop3_t, const char *));

extern int  pop3_readline     __P ((pop3_t, char *, size_t, size_t *));
extern int  pop3_response     __P ((pop3_t, char *, size_t, size_t *));
extern int  pop3_writeline    __P ((pop3_t, const char *, ...));
extern int  pop3_sendline     __P ((pop3_t, const char *));
extern int  pop3_send         __P ((pop3_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_POP3_H */
