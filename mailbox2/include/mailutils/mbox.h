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

#include <mailutils/stream.h>
#include <mailutils/attribute.h>

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


extern int  mbox_create __P ((mbox_t *));
extern void mbox_destroy __P ((mbox_t *));

extern int  mbox_get_uidvalidity __P ((mbox_t, unsigned long *));
extern int  mbox_get_uidnext     __P ((mbox_t, unsigned long *));

extern int  mbox_open            __P ((mbox_t, const char *, int));
extern int  mbox_close           __P ((mbox_t));

extern int  mbox_get_attribute   __P ((mbox_t, unsigned int, attribute_t *));
extern int  mbox_get_uid         __P ((mbox_t, unsigned int, unsigned long *));

extern int  mbox_get_separator   __P ((mbox_t, unsigned int, char **));
extern int  mbox_set_separator   __P ((mbox_t, unsigned int, const char *));

extern int  mbox_get_hstream     __P ((mbox_t, unsigned int, stream_t *));
extern int  mbox_set_hstream     __P ((mbox_t, unsigned int, stream_t));
extern int  mbox_get_hsize       __P ((mbox_t, unsigned int, unsigned int *));
extern int  mbox_get_hlines      __P ((mbox_t, unsigned int, unsigned int *));

extern int  mbox_get_bstream     __P ((mbox_t, unsigned int, stream_t *));
extern int  mbox_set_bstream     __P ((mbox_t, unsigned int, stream_t));
extern int  mbox_get_bsize       __P ((mbox_t, unsigned int, unsigned int *));
extern int  mbox_get_blines      __P ((mbox_t, unsigned int, unsigned int *));

extern int  mbox_get_size        __P ((mbox_t, off_t *));

extern int  mbox_save_attributes __P ((mbox_t));
extern int  mbox_mark_deleted    __P ((mbox_t, unsigned int));
extern int  mbox_unmark_deleted  __P ((mbox_t, unsigned int));
extern int  mbox_expunge         __P ((mbox_t, int));
extern int  mbox_changed_on_disk __P ((mbox_t));

extern int  mbox_set_progress_cb __P ((mbox_t, int (*) __P ((int, void *)), void *));
extern int  mbox_set_newmsg_cb __P ((mbox_t, int (*) __P ((int, void *)), void *));
extern int  mbox_newmsg_cb     __P ((mbox_t, int));
extern int  mbox_progress_cb   __P ((mbox_t, int));

extern int  mbox_scan __P ((mbox_t, unsigned int, unsigned int *, int));
extern int  mbox_messages_count __P ((mbox_t, unsigned int *));

extern int  mbox_append         __P ((mbox_t, const char *, stream_t));
extern int  mbox_append_hb __P ((mbox_t, const char *, stream_t, stream_t));

extern int  mbox_get_carrier __P ((mbox_t, stream_t *));
extern int  mbox_set_carrier __P ((mbox_t, stream_t));

extern int  mbox_set_hcache  __P ((mbox_t, const char **, size_t));
extern int  mbox_set_hcache_default  __P ((mbox_t));
extern void mbox_hcache_free __P ((mbox_t, unsigned int));
extern int  mbox_hcache_append __P ((mbox_t, unsigned int, const char *,
				     const char *));

extern int  mbox_header_get_value __P ((mbox_t, unsigned int, const char *,
					char *, size_t, size_t *));

extern int  stream_mbox_create __P ((stream_t *, mbox_t, unsigned int, int));
extern int  attribute_mbox_create __P ((attribute_t *, mbox_t, unsigned int));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MBOX_H */
