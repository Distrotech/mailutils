/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_FOLDER_H
# define _MAILUTILS_FOLDER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mu_list_response
{
  int type;             /* MU_FOLDER_ATTRIBUTE_.* flags */
  int separator;
  char *name;
};

struct mu_folder_list
{
  struct mu_list_response **element;
  size_t num;
};

/* Constructor/destructor and possible types.  */
extern int  mu_folder_create         (folder_t *, const char *);
extern void mu_folder_destroy        (folder_t *);

extern int  mu_folder_open           (folder_t, int flag);
extern int  mu_folder_close          (folder_t);

extern int  mu_folder_delete         (folder_t, const char *);
extern int  mu_folder_rename         (folder_t, const char *, const char *);
extern int  mu_folder_subscribe      (folder_t, const char *);
extern int  mu_folder_unsubscribe    (folder_t, const char *);
extern int  mu_folder_list           (folder_t, const char *, const char *,
				   struct mu_folder_list *);
extern int  mu_folder_lsub           (folder_t, const char *, const char *,
				   struct mu_folder_list *);
extern int  mu_folder_list_destroy   (struct mu_folder_list *);

/* Stream settings.  */
extern int  mu_folder_get_stream     (folder_t, stream_t *);
extern int  mu_folder_set_stream     (folder_t, stream_t);

  /* Notifications.  */
extern int  mu_folder_get_observable (folder_t, observable_t *);

  /* Debug.  */
extern int  mu_folder_has_debug      (folder_t folder);
extern int  mu_folder_get_debug      (folder_t, mu_debug_t *);
extern int  mu_folder_set_debug      (folder_t, mu_debug_t);

/* Authentication.  */
extern int  mu_folder_get_authority  (folder_t, authority_t *);
extern int  mu_folder_set_authority  (folder_t, authority_t);

/* URL.  */
extern int  mu_folder_get_url        (folder_t, url_t *);
extern int  mu_folder_set_url        (folder_t, url_t);

/* FIXME: not implemented */
extern int  mu_folder_decrement      (folder_t);


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_FOLDER_H */
