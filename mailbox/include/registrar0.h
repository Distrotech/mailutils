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

#ifndef _REGISTRAR0_H
#define _REGISTRAR0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/registrar.h>

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

struct _record
{
  const char *scheme;
  mailbox_entry_t mailbox;
  mailer_entry_t mailer;
  folder_entry_t folder;
  int is_allocated;
  void *onwer;
  int (*_is_scheme)   __P ((record_t, const char *));
  int (*_get_mailbox) __P ((record_t, mailbox_entry_t *_mailbox));
  int (*_get_mailer)  __P ((record_t, mailer_entry_t *_mailer));
  int (*_get_folder)  __P ((record_t, folder_entry_t *_mailer));
};

#ifdef __cplusplus
}
#endif

#endif /* _REGISTRAR0_H */
