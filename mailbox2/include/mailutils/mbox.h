/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_MBOX_H
#define _MAILUTILS_MBOX_H

#include <mailutils/message.h>
#include <mailutils/observer.h>

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

struct _mbox;
typedef struct _mbox *mbox_t;


extern int mbox_create          __P ((mbox_t *));
extern int mbox_destroy         __P ((mbox_t));

extern int mbox_uidvalidity     __P ((mbox_t, unsigned long *));
extern int mbox_uidnext         __P ((mbox_t, unsigned long));

extern int mbox_open            __P ((mbox_t, const char *, int));
extern int mbox_close           __P ((mbox_t));

extern int mbox_get_message     __P ((mbox_t, unsigned int, message_t *));
extern int mbox_get_envelope    __P ((mbox_t, unsigned int, envelope_t *));
extern int mbox_get_header      __P ((mbox_t, unsigned int, header_t *));
extern int mbox_get_body        __P ((mbox_t, unsigned int, body_t *));

extern int mbox_get_flags       __P ((mbox_t, unsigned int, int *));

extern int mbox_get_size        __P ((mbox_t, unsigned long *));

extern int mbox_save_attributes __P ((mbox_t));
extern int mbox_expunge         __P ((mbox_t));
extern int mbox_is_modified     __P ((mbox_t));

extern int mbox_scan            __P ((mbox_t, unsigned int, unsigned int *));
extern int mbox_messages_count  __P ((mbox_t, unsigned int *));
extern int mbox_get_obervable   __P ((mbox_t, observable_t *));

extern int mbox_append          __P ((mbox_t, message_t));


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MBOX_H */
