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

#ifndef _MBX_UNIX_H
#define _MBX_UNIX_H	1

#include <mailbox0.h>

#ifdef __cplucplus
extern "C" {
#endif

extern int  mailbox_unix_init    __P ((mailbox_t *mbox, const char *name));
extern void mailbox_unix_destroy __P ((mailbox_t *mbox));

extern struct mailbox_type _mailbox_unix_type;

#ifdef __cplucplus
}
#endif

#endif /* _MBX_UNIX_H */
