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

#ifndef _MAILUTILS_SYS_MAILBOX_H
#define _MAILUTILS_SYS_MAILBOX_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/mailbox.h>

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

struct _mailbox_vtable
{
  /* Constructor/destructor and possible types.  */
  int  (*ref)             __P ((mailbox_t));
  void (*destroy)         __P ((mailbox_t *));
  int  (*get_folder)      __P ((mailbox_t, folder_t *));

  int  (*open)            __P ((mailbox_t, int flag));
  int  (*close)           __P ((mailbox_t));
  int  (*uidvalidity)     __P ((mailbox_t, unsigned long *));
  int  (*uidnext)         __P ((mailbox_t, size_t *));

  /* Messages.  */
  int  (*get_message)     __P ((mailbox_t, size_t msgno, message_t *));
  int  (*append_message)  __P ((mailbox_t, message_t));
  int  (*messages_count)  __P ((mailbox_t, size_t *));
  int  (*messages_recent) __P ((mailbox_t, size_t *));
  int  (*messages_unseen) __P ((mailbox_t, size_t *));
  int  (*expunge)         __P ((mailbox_t));
  int  (*save_attributes) __P ((mailbox_t));

  /* Update and scanning.  */
  int  (*get_size)        __P ((mailbox_t, off_t *size));
  int  (*is_updated)      __P ((mailbox_t));
  int  (*scan)            __P ((mailbox_t, size_t no, size_t *count));

  /* Mailbox Stream.  */
  int  (*get_stream)      __P ((mailbox_t, stream_t *));

  /* Authentication.  */
  int  (*get_authority)   __P ((mailbox_t, authority_t *));
  int  (*set_authority)   __P ((mailbox_t, authority_t));

  /* Property.  */
  int  (*get_property)    __P ((mailbox_t, property_t *));

  /* URL.  */
  int  (*get_url)         __P ((mailbox_t, url_t *));

  /* For any debuging */
  int  (*get_debug)       __P ((mailbox_t, mu_debug_t *));
  int  (*set_debug)       __P ((mailbox_t, mu_debug_t));

  /* Events.  */
  int  (*get_observable)  __P ((mailbox_t, observable_t *));
};

struct _mailbox
{
  struct _mailbox_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_MAILBOX_H */
