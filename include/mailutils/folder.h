/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_FOLDER_H
# define _MAILUTILS_FOLDER_H

#include <sys/types.h>

#include <mailutils/url.h>
#include <mailutils/observer.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>
#include <mailutils/auth.h>
#include <mailutils/stream.h>


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

/* Forward declaration.  */
struct _folder;
typedef struct _folder *folder_t;

#define MU_FOLDER_ATTRIBUTE_DIRECTORY 0x001
#define MU_FOLDER_ATTRIBUTE_FILE      0x002
struct list_response
{
  int type;
  int separator;
  char *name;
};

struct folder_list
{
  struct list_response **element;
  size_t num;
};

/* Constructor/destructor and possible types.  */
extern int  folder_create         __P ((folder_t *, const char *));
extern void folder_destroy        __P ((folder_t *));

extern int  folder_open           __P ((folder_t, int flag));
extern int  folder_close          __P ((folder_t));

extern int  folder_delete         __P ((folder_t, const char *));
extern int  folder_rename         __P ((folder_t, const char *, const char *));
extern int  folder_subscribe      __P ((folder_t, const char *));
extern int  folder_unsubscribe    __P ((folder_t, const char *));
extern int  folder_list           __P ((folder_t, const char *, const char *,
				       struct folder_list *));
extern int  folder_lsub           __P ((folder_t, const char *, const char *,
				       struct folder_list *));
extern int  folder_list_destroy   __P ((struct folder_list *));

/* Stream settings.  */
extern int  folder_get_stream     __P ((folder_t, stream_t *));
extern int  folder_set_stream     __P ((folder_t, stream_t));

  /* Notifications.  */
extern int  folder_get_observable __P ((folder_t, observable_t *));

  /* Debug.  */
extern int  folder_has_debug      __P ((folder_t folder));
extern int  folder_get_debug      __P ((folder_t, mu_debug_t *));
extern int  folder_set_debug      __P ((folder_t, mu_debug_t));

/* Authentication.  */
extern int  folder_get_authority  __P ((folder_t, authority_t *));
extern int  folder_set_authority  __P ((folder_t, authority_t));

/* URL.  */
extern int  folder_get_url        __P ((folder_t, url_t *));
extern int  folder_set_url        __P ((folder_t, url_t));

/* FIXME: not implemented */
extern int  folder_decrement      __P ((folder_t));


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_FOLDER_H */
