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

#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/url.h>
#include <mailutils/observable.h>
#include <mailutils/debug.h>
#include <mailutils/stream.h>
#include <mailutils/authority.h>
#include <mailutils/stream.h>
#include <mailutils/iterator.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration.  */

#define MU_FOLDER_ATTRIBUTE_DIRECTORY 0x001
#define MU_FOLDER_ATTRIBUTE_FILE      0x002
struct list_response
{
  int type;
  int separator;
  char *name;
};

extern int  folder_create        __P ((folder_t *, const char *));

extern int  folder_ref           __P ((folder_t));
extern void folder_destroy       __P ((folder_t *));

extern int  folder_open           __P ((folder_t, int));
extern int  folder_close          __P ((folder_t));

extern int  folder_delete         __P ((folder_t, const char *));
extern int  folder_rename         __P ((folder_t, const char *, const char *));
extern int  folder_subscribe      __P ((folder_t, const char *));
extern int  folder_unsubscribe    __P ((folder_t, const char *));
extern int  folder_list           __P ((folder_t, const char *, const char *,
					iterator_t *));
extern int  folder_lsub           __P ((folder_t, const char *, const char *,
					iterator_t *));

/* Stream settings.  */
extern int folder_get_stream     __P ((folder_t, stream_t *));
extern int folder_set_stream     __P ((folder_t, stream_t));

  /* Notifications.  */
extern int folder_get_observable  __P ((folder_t, observable_t *));
extern int folder_get_debug       __P ((folder_t, mu_debug_t *));
extern int folder_set_debug       __P ((folder_t, mu_debug_t));

/* Authentication.  */
extern int folder_get_authority  __P ((folder_t, authority_t *));
extern int folder_set_authority  __P ((folder_t, authority_t));

/* URL.  */
extern int folder_get_url        __P ((folder_t, url_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_FOLDER_H */
