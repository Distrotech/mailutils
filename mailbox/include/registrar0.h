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

/*
  Builtin mailbox types.
  A circular list is use for the builtin.
  Proper locking is not done when accessing the list.
  FIXME: not thread-safe. */
struct _registrar
{
  struct url_registrar *ureg;
  struct mailbox_registrar *mreg;
  int is_allocated;
  struct _registrar *next;
};


/* This is function is obsolete use the registrar_entry_*() ones */
extern int registrar_list __P ((struct url_registrar **ureg,
				struct mailbox_registrar **mreg,
				int *id, registrar_t *reg));
extern int registrar_entry_count __P ((size_t *num));
extern int registrar_entry __P ((size_t num, struct url_registrar **ureg,
				struct mailbox_registrar **mreg,
				int *id));
/* IMAP */
extern struct mailbox_registrar _mailbox_imap_registrar;
extern struct url_registrar _url_imap_registrar;

/* FILE */
extern struct url_registrar  _url_file_registrar;
/* MBOX */
extern struct mailbox_registrar _mailbox_mbox_registrar;
extern struct url_registrar  _url_mbox_registrar;

/* MAILTO */
extern struct mailbox_registrar _mailbox_mailto_registrar;
extern struct url_registrar _url_mailto_registrar;

/* MDIR */
extern struct mailbox_registrar _mailbox_maildir_registrar;
extern struct url_registrar  _url_maildir_registrar;

/* MMDF */
extern struct mailbox_registrar _mailbox_mmdf_registrar;
extern struct url_registrar  _url_mmdf_registrar;

/* UNIX */
extern struct mailbox_registrar _mailbox_unix_registrar;
extern struct url_registrar  _url_unix_registrar;

/* POP */
extern struct mailbox_registrar _mailbox_pop_registrar;
extern struct url_registrar _url_pop_registrar;

#ifdef __cplusplus
}
#endif

#endif /* _REGISTRAR0_H */
