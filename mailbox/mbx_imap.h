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

#ifndef _MBX_IMAP_H
#define _MBX_IMAP_H	1

#include <mailbox.h>

extern int mailbox_imap_init    __P ((mailbox_t *mbox, const char *name));
extern int mailbox_imap_destroy __P ((mailbox_t *mbox));

extern struct mailbox_type _mailbox_imap_type;

#endif /* _MBX_IMAP_H */
