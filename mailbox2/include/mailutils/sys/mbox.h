/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_SYS_MBOX_H
#define _MAILUTILS_SYS_MBOX_H

#include <time.h>
#include <stdarg.h>

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/lockfile.h>
#include <mailutils/mbox.h>

#define MU_MBOX_HSTREAM_SET 0x010000
#define MU_MBOX_BSTREAM_SET 0x001000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mbox_message* mbox_message_t;

struct _hcache
{
  char **values;
  size_t size;
};

/* The umessages is an array of pointers that contains umessages_count of
   mbox_message_t*; umessages[umessages_count].  We do it this way because
   realloc() can move everything to a new memory region and invalidate all
   the pointers.  Thanks to <Dave Inglis> for pointing this out.  The
   messages_count is the count number of messages parsed so far.  */
struct _mbox
{
  mbox_message_t *umessages; /* Array.  */
  unsigned int umessages_count; /* How big is the umessages[].  */

  unsigned int messages_count; /* Number of messages.  */

  stream_t carrier; /* File stream.  */

  off_t size; /* Size of the mailbox.  */
  mu_debug_t debug;
  unsigned long uidvalidity;
  unsigned long uidnext;
  char *filename;

  struct _hcache hcache;

  /* The variables below are use to hold the state when appending messages.  */
  enum mbox_state
  {
    MU_MBOX_NO_STATE = 0,
    MU_MBOX_STATE_APPEND_SEPARATOR,
    MU_MBOX_STATE_APPEND_HEADER,
    MU_MBOX_STATE_APPEND_BODY
  } state;

  lockfile_t  lockfile;
  struct
  {
    int (*cb) __P ((int, void *));
    void *arg;
  } newmsg, progress, error;

  /* Hold the offset of the stream when appending.  */
  off_t roffset, woffset;
};

/* Keep the file positions of where the headers and bodies start and end.
   attribute is the "Status:" message.  */
struct _mbox_message
{
  /* Offset of the messages in the mailbox.  */
  off_t from_;
  char *separator;

  /* Fast header retrieve, we save here the most common headers. This will
     speed the header search.  The entire headers are copied, when modified,
     by the header_t object, we do not have to worry about updating them.  */
  struct _hcache hcache;

  struct
  {
    stream_t stream;
    unsigned int lines;
    off_t start;
    off_t end;
  } header, body;

  /* UID i.e. see IMAP  */
  unsigned long uid;
  unsigned int attr_flags;
  unsigned int attr_userflags;
  attribute_t attribute; /* The attr_flags contains the "Status:" attribute  */
};

extern int  mbox_newmsg_cb       __P ((mbox_t, int));
extern int  mbox_progress_cb     __P ((mbox_t, int));
extern int  mbox_error_cb        __P ((mbox_t, int));

extern int  mbox_append_hb0     __P ((mbox_t, const char *, attribute_t,
				      stream_t, stream_t, int, unsigned long));

extern void mbox_release_hcache  __P ((mbox_t, unsigned int));
extern int  mbox_append_hcache   __P ((mbox_t, unsigned int, const char *,
                                       const char *));
extern int  mbox_set_default_hcache __P ((mbox_t));

extern int  mbox_debug_print      __P ((mbox_t, const char *, ...));

extern int  stream_mbox_create   __P ((stream_t *, mbox_t, unsigned int, int));
extern int  stream_mbox_set_msgno __P ((stream_t, unsigned int));
extern void _stream_mbox_dtor     __P ((stream_t));

extern int  attribute_mbox_create __P ((attribute_t *, mbox_t, unsigned int));
extern int  attribute_mbox_set_msgno __P ((attribute_t, unsigned int));
extern int  mbox_attribute_to_status __P ((attribute_t, char *, size_t,
                                           size_t *));
extern void _attribute_mbox_dtor     __P ((attribute_t));

extern void mbox_release_separator   __P ((mbox_t, unsigned int));
extern void mbox_release_attribute   __P ((mbox_t, unsigned int));
extern void mbox_release_hstream     __P ((mbox_t, unsigned int));
extern void mbox_release_bstream     __P ((mbox_t, unsigned int));
extern void mbox_release_msg         __P ((mbox_t, unsigned int));

extern int  mbox_scan0 __P ((mbox_t, unsigned int, unsigned int *, int));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_MBOX_H */
