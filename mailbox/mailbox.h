/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <unistd.h>
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
  uid_t owner;
  gid_t group;
  size_t messages;
  size_t num_deleted;
  off_t size;
  int lock;
  size_t timeout;
  size_t refresh;
  int (*func) __P ((mailbox_t, void *arg));
  struct mailbox_type *mtype;

  /* back pointer to the specific mailbox */
  void *data;

  /* Functions */

#define MU_MB_RDONLY ((int)1)
#define MU_MB_WRONLY (MU_MB_RDONLY << 1)
#define MU_MB_RDWR   (MU_MB_WRONLY << 1)
#define MU_MB_APPEND (MU_MB_RDWR << 1)
#define MU_MB_CREAT  (MU_MB_APPEND << 1)

  int  (*_open)            __P ((mailbox_t, int flag));
  int  (*_close)           __P ((mailbox_t));

  /* type */
  int  (*_get_name)       __P ((mailbox_t, int *id, char *name,
				 size_t len, size_t *n));
  int  (*_get_mname)      __P ((mailbox_t, int *id, char **name, size_t *n));

  /* passwd if needed */
  int  (*_get_passwd)      __P ((mailbox_t, char *passwd,
				 size_t len, size_t *n));
  int  (*_get_mpasswd)     __P ((mailbox_t, char **passwd, size_t *n));
  int  (*_set_passwd)      __P ((mailbox_t, const char *passwd, size_t len));
  /* deleting mesgs */
  int  (*_is_deleted)      __P ((mailbox_t, size_t msgno));
  int  (*_delete)          __P ((mailbox_t, size_t msgno));
  int  (*_undelete)        __P ((mailbox_t, size_t msgno));
  int  (*_expunge)         __P ((mailbox_t));
  int  (*_is_updated)      __P ((mailbox_t));
  int  (*_scan)            __P ((mailbox_t, size_t *msgs));

  /* appending messages */
  int  (*_new_msg)         __P ((mailbox_t, size_t *msgno));
  int  (*_set_header)      __P ((mailbox_t, size_t msgno, const char *h,
				 size_t len, int replace));
  int  (*_set_body)        __P ((mailbox_t, size_t msgno, const char *b,
				 size_t len, int replace));
  int  (*_append)          __P ((mailbox_t, size_t msgno));
  int  (*_destroy_msg)     __P ((mailbox_t, size_t msgno));

#define MU_MB_RDLOCK 0
#define MU_MB_WRLOCK 1
  /* locking */
  int  (*_lock)            __P ((mailbox_t, int flag));
  int  (*_unlock)          __P ((mailbox_t));
  int  (*_ilock)           __P ((mailbox_t, int flag));
  int  (*_iunlock)         __P ((mailbox_t));

  /* reading mesgs */
  int  (*_get_body)        __P ((mailbox_t, size_t msgno, off_t off,
				 char *b, size_t len, size_t *n));
  int  (*_get_mbody)       __P ((mailbox_t, size_t msgno, off_t off,
				 char **b, size_t *n));
  int  (*_get_header)      __P ((mailbox_t, size_t msgno, off_t off,
				 char *h, size_t len, size_t *n));
  int  (*_get_mheader)     __P ((mailbox_t, size_t msgno, off_t off,
				 char **h, size_t *n));
  int  (*_get_size)        __P ((mailbox_t, size_t msgno,
				 size_t *h, size_t *b));

  /* setting flags */
  int  (*_is_read)         __P ((mailbox_t, size_t msgno));
  int  (*_set_read)        __P ((mailbox_t, size_t msgno));
  int  (*_is_seen)         __P ((mailbox_t, size_t msgno));
  int  (*_set_seen)        __P ((mailbox_t, size_t msgno));

  /* owner and group */
  int  (*_set_owner)       __P ((mailbox_t, uid_t uid));
  int  (*_get_owner)       __P ((mailbox_t, uid_t *uid));
  int  (*_set_group)       __P ((mailbox_t, gid_t gid));
  int  (*_get_group)       __P ((mailbox_t, gid_t *gid));

  /* miscellany */
  int  (*_size)            __P ((mailbox_t, off_t *size));
  int  (*_get_timeout)     __P ((mailbox_t, size_t *timeout));
  int  (*_set_timeout)     __P ((mailbox_t, size_t timeout));
  int  (*_get_refresh)     __P ((mailbox_t, size_t *refresh));
  int  (*_set_refresh)     __P ((mailbox_t, size_t refresh));
  int  (*_set_notification) __P ((mailbox_t,
				  int (*func) __P ((mailbox_t, void * arg))));
};

/* constructor/destructor and possible types */
extern int  mailbox_init    __P ((mailbox_t *, const char *name, int id));
extern void mailbox_destroy __P ((mailbox_t *));

/* mailbox registration */
extern int mailbox_list_type   __P ((struct mailbox_type mtype[],
				     size_t len, size_t *n));
extern int mailbox_list_mtype  __P ((struct mailbox_type **mtype, size_t *n));
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
extern INLINE int mailbox_close       __P ((mailbox_t));

/* type */
extern INLINE int mailbox_get_name    __P ((mailbox_t, int *id, char *name,
					    size_t len, size_t *n));
extern INLINE int mailbox_get_mname   __P ((mailbox_t, int *id, char **name,
					    size_t *n));

/* passwd */
extern INLINE int mailbox_get_passwd  __P ((mailbox_t, char *passwd,
					    size_t len, size_t *n));
extern INLINE int mailbox_get_mpasswd __P ((mailbox_t, char **passwd,
					    size_t *n));
extern INLINE int mailbox_set_passwd  __P ((mailbox_t, const char *passwd,
					    size_t len));

/* deleting */
extern INLINE int mailbox_delete      __P ((mailbox_t, size_t msgno));
extern INLINE int mailbox_undelete    __P ((mailbox_t, size_t msgno));
extern INLINE int mailbox_expunge     __P ((mailbox_t));
extern INLINE int mailbox_is_deleted  __P ((mailbox_t, size_t msgno));

/* appending */
extern INLINE int mailbox_new_msg     __P ((mailbox_t, size_t * msgno));
extern INLINE int mailbox_set_header  __P ((mailbox_t, size_t msgno,
					    const char *h, size_t len,
					    int replace));
extern INLINE int mailbox_set_body    __P ((mailbox_t, size_t msgno,
					    const char *b, size_t len,
					    int replace));
extern INLINE int mailbox_append      __P ((mailbox_t, size_t msgno));
extern INLINE int mailbox_destroy_msg __P ((mailbox_t, size_t msgno));

/* reading */
extern INLINE int mailbox_get_body    __P ((mailbox_t, size_t msgno,
					    off_t off, char *b,
					    size_t len, size_t *n));
extern INLINE int mailbox_get_mbody   __P ((mailbox_t, size_t msgno, off_t off,
					    char **b, size_t *n));
extern INLINE int mailbox_get_header  __P ((mailbox_t, size_t msgno, off_t off,
					    char *h, size_t len, size_t *n));
extern INLINE int mailbox_get_mheader __P ((mailbox_t, size_t msgno, off_t off,
					    char **h, size_t *n));

/* locking */
extern INLINE int mailbox_lock        __P ((mailbox_t, int flag));
extern INLINE int mailbox_unlock      __P ((mailbox_t));

/* owner and group */
extern INLINE int mailbox_set_owner   __P ((mailbox_t, uid_t uid));
extern INLINE int mailbox_set_group   __P ((mailbox_t, gid_t gid));

/* miscellany */
extern INLINE int mailbox_scan        __P ((mailbox_t, size_t *msgs));
extern INLINE int mailbox_is_updated  __P ((mailbox_t));
extern INLINE int mailbox_get_timeout __P ((mailbox_t, size_t *timeout));
extern INLINE int mailbox_set_timeout __P ((mailbox_t, size_t timeout));
extern INLINE int mailbox_get_refresh __P ((mailbox_t, size_t *refresh));
extern INLINE int mailbox_set_refresh __P ((mailbox_t, size_t refresh));
extern INLINE int mailbox_get_size    __P ((mailbox_t, size_t msgno,
					    size_t *header, size_t *body));
extern INLINE int mailbox_set_notification  __P ((mailbox_t,
						  int (*func)
						  __P ((mailbox_t, void *))));

#ifdef MU_USE_MACROS
#define mailbox_open(m, f)	         m->_open (m, f)
#define mailbox_close(m)                 m->_close (m)

/* type */
#define mailbox_get_name(m, t, d, l, n)  m->_get_name (m, t, d, l, n)
#define mailbox_get_mtype(m, t, d, n)    m->_get_mtype (m, t, d, n)

/* passwd */
#define mailbox_get_passwd(m, p, l, n)   m->_get_passwd (m, p, l, n)
#define mailbox_get_mpasswd(m, p, n)     m->_get_mpasswd (m, p, n)
#define mailbox_set_passwd(m, p, l)      m->_set_passwd (m, p, l)

/* deleting */
#define mailbox_delete(m, mid)           m->_delete (m, mid)
#define mailbox_undelete(m, mid)         m->_undelete (m, mid)
#define mailbox_is_deleted(m, mid)       m->_is_deleted (m, mid)
#define mailbox_expunge(m)               m->_expunge (m)

/* appending */
#define mailbox_new_msg(m, mid)               m->_new_msg (m, mid)
#define mailbox_set_header(m, mid, h, l, r)   m->_set_header(m, mid, h, n, r)
#define mailbox_set_body(m, mid, b, l r)      m->_set_body (m, mid, b, l, r)
#define mailbox_append(m, mid)                m->_append (m, mid)
#define mailbox_destroy_msg(m, mid)           m->_destroy_msg (m, mid)

/* locking */
#define mailbox_lock(m, f)                    m->_lock (m, f)
#define mailbox_unlock(m)                     m->_unlock (m)

/* reading */
#define mailbox_get_header(m, mid, h, l, n)   m->_get_header (m, mid, h, o, n)
#define mailbox_get_mheader(m, mid, h, l)     m->_get_header (m, mid, h, l)
#define mailbox_get_body(m, mid, b, l, n)     m->_get_body (m, mid, b, l, n)
#define mailbox_get_mbody(m, mid, b, n)       m->_get_body (m, mid, b, n)

/* owner and group */
#define mailbox_set_owner(m, uid)             m->_set_owner(m, uid)
#define mailbox_get_owner(m, uid)             m->_set_owner(m, uid)
#define mailbox_set_group(m, gid)             m->_set_group(m, gid)
#define mailbox_get_group(m, gid)             m->_set_group(m, gid)

/* miscellany */
#define mailbox_scan(m, t)                    m->_scan (m, t)
#define mailbox_is_updated(m)                 m->_is_updated (m)
#define mailbox_get_timeout(m, t)             m->_get_timeout (m, t)
#define mailbox_set_timeout(m, t)             m->_set_timeout (m, t)
#define mailbox_get_refresh(m, r)             m->_get_refresh (m, r)
#define mailbox_set_refresh(m, r)             m->_set_refresh (m, r)
#define mailbox_get_size(m, mid, sh, sb)      m->_get_size(m, mid, sh, sb)
#define mailbox_set_notification(m, func)    m->_set_notification (m, func)
#endif /* MU_USE_MACROS */

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
