/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_FOLDER_H
#define _MAILUTILS_SYS_FOLDER_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/folder.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _folder_vtable
{
  int  (*ref)     __P ((folder_t));
  void (*destroy) __P ((folder_t *));

  int  (*open)    __P ((folder_t, int flag));
  int  (*close)   __P ((folder_t));

  int  (*delete)  __P ((folder_t, const char *));
  int  (*rename)  __P ((folder_t, const char *, const char *));
  int  (*subscribe) __P ((folder_t, const char *));
  int  (*unsubscribe) __P ((folder_t, const char *));
  int  (*list) __P ((folder_t, const char *, const char *, iterator_t *));
  int  (*lsub) __P ((folder_t, const char *, const char *, iterator_t *));

  /* Stream settings.  */
  int  (*get_stream) __P ((folder_t, stream_t *));
  int  (*set_stream) __P ((folder_t, stream_t));

  /* Notifications.  */
  int  (*get_observable) __P ((folder_t, observable_t *));
  int  (*get_debug) __P ((folder_t, mu_debug_t *));
  int  (*set_debug) __P ((folder_t, mu_debug_t));

  /* Authentication.  */
  int  (*get_authority) __P ((folder_t, authority_t *));
  int  (*set_authority) __P ((folder_t, authority_t));

  /* URL.  */
  int  (*get_url) __P ((folder_t, url_t *));

};

struct _folder
{
  struct _folder_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_FOLDER_H */
