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

struct mailbox_type
{
  char *name;
  int  id;
  struct url_type *utype;
  int  (*_init)    __P ((mailbox_t *, const char *name));
  void (*_destroy) __P ((mailbox_t *));
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

/* flags for mailbox_open () */
#define MU_MB_RDONLY ((int)1)
#define MU_MB_WRONLY (MU_MB_RDONLY << 1)
#define MU_MB_RDWR   (MU_MB_WRONLY << 1)
#define MU_MB_APPEND (MU_MB_RDWR << 1)
#define MU_MB_CREAT  (MU_MB_APPEND << 1)

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

/* Lock settings */
typedef enum { MB_RDLOCK, MB_WRLOCK } mailbox_lock_t;
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
						  int (*notification)
						  __P ((mailbox_t, void *))));

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
