/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILBOX_H
#define _MAILBOX_H	1

#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*!__P */

/* These need to be documented */
#define mbox_close(m)		m->_close(m)
#define mbox_delete(m,n)	m->_delete(m,n)
#define mbox_undelete(m,n)      m->_undelete(m,n)
#define mbox_expunge(m)		m->_expunge(m)
#define mbox_scan(m)		m->_scan(m)
#define mbox_is_deleted(m,n)	m->_is_deleted(m,n)
#define mbox_is_updated(m)	m->_is_updated(m)
#define mbox_add_message(m,s)	m->_add_message(m,s)
#define mbox_get_body(m,n)	m->_get_body(m,n)
#define mbox_get_header(m,n)	m->_get_header(m,n)
#define mbox_lock(m,n)		m->_lock(m,n)
#ifdef TESTING
#define mbox_tester(m,n)      m->_tester(m,n)
#endif

#include <stdlib.h>

/* Lock settings */
/* define this way so that it is opaque and can later become a struct w/o
 * people noticing (-:
 */
enum _mailbox_lock_t {MO_ULOCK, MO_RLOCK, MO_WLOCK}; /* new type */
typedef enum _mailbox_lock_t mailbox_lock_t;

typedef struct _mailbox
  {
    /* Data */
    char *name;
    unsigned int messages;
    unsigned int num_deleted;
    size_t *sizes;
    void *_data;

    /* Functions */
    int (*_close) __P ((struct _mailbox *));
    int (*_delete) __P ((struct _mailbox *, unsigned int));
    int (*_undelete) __P ((struct _mailbox *, unsigned int));
    int (*_expunge) __P ((struct _mailbox *));
    int (*_add_message) __P ((struct _mailbox *, char *));
	int (*_scan) __P ((struct _mailbox *));
    int (*_is_deleted) __P ((struct _mailbox *, unsigned int));
	int (*_is_updated) __P ((struct mailbox *));
    int (*_lock) __P((struct _mailbox *, mailbox_lock_t));
    char *(*_get_body) __P ((struct _mailbox *, unsigned int));
    char *(*_get_header) __P ((struct _mailbox *, unsigned int));
#ifdef TESTING
    void (*_tester) __P ((struct _mailbox *, unsigned int));
#endif
  }
mailbox;

mailbox *mbox_open __P ((const char *name));

char *mbox_header_line __P ((mailbox *mbox, unsigned int num, const char *header));
char *mbox_body_lines __P ((mailbox *mbox, unsigned int num, unsigned int lines));

#endif
