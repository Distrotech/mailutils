/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILBOX_H
# define _MAILBOX_H

#include <url.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

/* forward declaration */
struct _mailbox;
typedef struct _mailbox *mailbox_t;

/* Lock settings */
typedef enum { MB_ULOCK, MB_RLOCK, MB_WLOCK } mailbox_lock_t;

struct mailbox_type
{
  char *name;
  int  id;
  struct url_type *utype;
  int  (*_init)    __P ((mailbox_t *, const char *name));
  void (*_destroy) __P ((mailbox_t *));
};

struct _mailbox
{
  /* Data */
  char *name;
  int messages;
  int num_deleted;
  long size;
  int timeout;
  int refresh;
  mailbox_lock_t lock;
  int (*func) __P ((mailbox_t));

  void *data;
  struct mailbox_type *mtype;

  /* Functions */

  //void (*_destroy)         __P ((mailbox_t *));

  int  (*_open)            __P ((mailbox_t, int flag));
  int  (*_close)           __P ((mailbox_t, int flag));

  /* type */
  int  (*_get_name)       __P ((mailbox_t, int *id, char *name,
				 int offset, int len));
  int  (*_get_mname)      __P ((mailbox_t, int *id, char **name,
				 int *len));

  /* passwd if needed */
  int  (*_get_passwd)      __P ((mailbox_t, char * passwd, int offset, int n));
  int  (*_get_mpasswd)     __P ((mailbox_t, char **passwd, int *n));
  int  (*_set_passwd)      __P ((mailbox_t, const char * passwd,
				 int offset, int n));
  /* deleting mesgs */
  int  (*_delete)          __P ((mailbox_t, int id));
  int  (*_undelete)        __P ((mailbox_t, int id));
  int  (*_is_deleted)      __P ((mailbox_t, int id));
  int  (*_expunge)         __P ((mailbox_t));

  /* appending messages */
  int  (*_new_msg)         __P ((mailbox_t, int *id));
  int  (*_set_header)      __P ((mailbox_t, int id, const char *h,
				 int offset, int n, int replace));
  int  (*_set_body)        __P ((mailbox_t, int id, const char *b,
				 int offset, int n, int replace));
  int  (*_append)          __P ((mailbox_t, int id));
  int  (*_destroy_msg)     __P ((mailbox_t, int id));

  /* locking */
  int  (*_lock)            __P ((mailbox_t, int flag));
  int  (*_unlock)          __P ((mailbox_t));

  /* reading mesgs */
  int  (*_get_body)        __P ((mailbox_t, int id, char *v,
				 int offset, int n));
  int  (*_get_mbody)       __P ((mailbox_t, int id, char **v, int *n));
  int  (*_get_header)      __P ((mailbox_t, int id, char *h,
				 int offset, int n));
  int  (*_get_mheader)     __P ((mailbox_t, int id, char **h, int *n));

  /* setting flags */
  int  (*_msg_is_read)     __P ((mailbox_t, int id));
  int  (*_msg_set_read)    __P ((mailbox_t, int id));
  int  (*_msg_is_seen)     __P ((mailbox_t, int id));
  int  (*_msg_set_seen)    __P ((mailbox_t, int id));

  /* miscellany */
  int  (*_scan)            __P ((mailbox_t, int *msgs));
  int  (*_is_updated)      __P ((mailbox_t));
  int  (*_get_timeout)     __P ((mailbox_t, int *timeout));
  int  (*_set_timeout)     __P ((mailbox_t, int timeout));
  int  (*_get_size)        __P ((mailbox_t, int id, size_t *size));
  int  (*_get_refresh)     __P ((mailbox_t, int *refresh));
  int  (*_set_refresh)     __P ((mailbox_t, int refresh));
  int  (*_set_notification) __P ((mailbox_t,
				  int (*func) __P ((mailbox_t, void * arg))));
};

/* constructor/destructor and possible types */
extern int  mailbox_init    __P ((mailbox_t *, const char *name, int id));
extern void mailbox_destroy __P ((mailbox_t *));

/* mailbox registration */
extern int mailbox_list_type   __P ((struct mailbox_type mtype[], int size));
extern int mailbox_list_mtype  __P ((struct mailbox_type *mtype[], int *size));
extern int mailbox_add_type    __P ((struct mailbox_type *mtype));
extern int mailbox_remove_type __P ((struct mailbox_type *mtype));
extern int mailbox_get_type    __P ((struct mailbox_type **mtype, int id));

#ifndef INLINE
# ifdef __GNUC__
#  define INLINE __inline__
# else
#  define INLINE
# endif
#endif

extern INLINE int mailbox_open        __P ((mailbox_t, int flag));
extern INLINE int mailbox_close       __P ((mailbox_t, int flag));

/* type */
extern INLINE int mailbox_get_name    __P ((mailbox_t, int *id, char *name,
					    int offset, int len));
extern INLINE int mailbox_get_mname   __P ((mailbox_t, int *id, char **name,
					    int *len));

/* passwd */
extern INLINE int mailbox_get_passwd  __P ((mailbox_t, char *passwd,
					    int offset, int len));
extern INLINE int mailbox_get_mpasswd __P ((mailbox_t, char **passwd,
					    int *len));
extern INLINE int mailbox_set_passwd  __P ((mailbox_t, const char *passwd,
					    int offset, int len));

/* deleting */
extern INLINE int mailbox_delete      __P ((mailbox_t, int));
extern INLINE int mailbox_undelete    __P ((mailbox_t, int));
extern INLINE int mailbox_expunge     __P ((mailbox_t));
extern INLINE int mailbox_is_deleted  __P ((mailbox_t, int));

/* appending */
extern INLINE int mailbox_new_msg     __P ((mailbox_t, int * id));
extern INLINE int mailbox_set_header  __P ((mailbox_t, int id, const char *h,
					    int offset, int n, int replace));
extern INLINE int mailbox_set_body    __P ((mailbox_t, int id, const char *b,
					    int offset, int n, int replace));
extern INLINE int mailbox_append      __P ((mailbox_t, int id));
extern INLINE int mailbox_destroy_msg __P ((mailbox_t, int id));

/* reading */
extern INLINE int mailbox_get_body    __P ((mailbox_t, int id, char *b,
					    int offset, int n));
extern INLINE int mailbox_get_mbody   __P ((mailbox_t, int id, char **b,
					    int *n));
extern INLINE int mailbox_get_header  __P ((mailbox_t, int id, char *h,
					    int offset, int n));
extern INLINE int mailbox_get_mheader  __P ((mailbox_t, int id, char **h,
					     int *n));

/* locking */
extern INLINE int mailbox_lock        __P ((mailbox_t, int flag));
extern INLINE int mailbox_unlock      __P ((mailbox_t));

/* miscellany */
extern INLINE int mailbox_scan        __P ((mailbox_t, int *msgs));
extern INLINE int mailbox_is_updated  __P ((mailbox_t));
extern INLINE int mailbox_get_timeout __P ((mailbox_t, int *timeout));
extern INLINE int mailbox_set_timeout __P ((mailbox_t, int timeout));
extern INLINE int mailbox_get_refresh __P ((mailbox_t, int *refresh));
extern INLINE int mailbox_set_refresh __P ((mailbox_t, int refresh));
extern INLINE int mailbox_get_size    __P ((mailbox_t, int id, size_t *size));
extern INLINE int mailbox_set_notification  __P ((mailbox_t, int
						  (*func) __P ((mailbox_t))));

#ifdef MU_USE_MACROS
#define mailbox_open(m, f)	         m->_open (m, f)
#define mailbox_close(m, f)              m->_close (m, f)

/* type */
#define mailbox_get_name(m, t, d, o, n)  m->_get_name (m, t, d, o, n)
#define mailbox_get_mtype(m, t, d, n)    m->_get_mtype (m, t, d, n)

/* passwd */
#define mailbox_get_passwd(m, p, o, n)   m->_get_passwd (m, p, o, n)
#define mailbox_get_mpasswd(m, p, n)     m->_get_mpasswd (m, p, n)
#define mailbox_set_passwd(m, p, o, n)   m->_set_passwd (m, p, o, n)

/* deleting */
#define mailbox_delete(m, id)            m->_delete (m, id)
#define mailbox_undelete(m, id)          m->_undelete (m, id)
#define mailbox_is_deleted(m, id)        m->_is_deleted (m, id)
#define mailbox_expunge(m)               m->_expunge (m)

/* appending */
#define mailbox_new_msg(m, id)                m->_new_msg (m, id)
#define mailbox_set_header(m, id, h, o, n, r) m->_set_header(m, id, h, o, n, r)
#define mailbox_set_body(m, id, b, o, n, r)   m->_set_body (m, id, b, o, n, r)
#define mailbox_append(m, id)                 m->_append (m, id)
#define mailbox_destroy_msg(m, id)            m->_destroy_msg (m, id)

/* locking */
#define mailbox_lock(m, f)                    m->_lock (m, f)
#define mailbox_unlock(m)                     m->_unlock (m)

/* reading */
#define mailbox_get_header(m, id, h, o, n)    m->_get_header (m, id, h, o, n)
#define mailbox_get_mheader(m, id, h, n)      m->_get_header (m, id, h, n)
#define mailbox_get_body(m, id, b, o, n)      m->_get_body (m, id, b, o, n)
#define mailbox_get_mbody(m, id, b, n)        m->_get_body (m, id, b, n)

/* miscellany */
#define mailbox_scan(m, t)                    m->_scan (m, t)
#define mailbox_is_updated(m)                 m->_is_updated (m)
#define mailbox_get_timeout(m, t)             m->_get_timeout (m, t)
#define mailbox_set_timeout(m, t)             m->_set_timeout (m, t)
#define mailbox_get_refresh(m, r)             m->_get_refresh (m, r)
#define mailbox_set_refresh(m, r)             m->_set_refresh (m, r)
#define mailbox_get_size(m, id, size)         m->_get_size(m, id, size)
#define mailbox_set_notification(m, func)    m->_set_notification (m, func)
#endif /* MU_USE_MACROS */

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
