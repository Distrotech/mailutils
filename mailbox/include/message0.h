/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _MESSAGE0_H
#define _MESSAGE0_H

#include <mailutils/message.h>
#include <mailutils/mime.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef _cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _message
{
  /* Who is the owner.  */
  void *owner;

  header_t header;
  stream_t stream;
  body_t body;
  attribute_t attribute;
  mime_t mime;
  observable_t observable;

  /* Holder for message_write. */
  char *hdr_buf;
  size_t hdr_buflen;
  int hdr_done;

  int (*_from)           __P ((message_t, char *, size_t, size_t *));
  int (*_received)       __P ((message_t, char *, size_t, size_t *));
  int (*_get_uidl)       __P ((message_t, char *, size_t, size_t *));
  int (*_get_num_parts)  __P ((message_t, size_t *));
  int (*_get_part)       __P ((message_t, size_t, message_t *));
  int (*_is_multipart)   __P ((message_t, int *));
  int (*_lines)          __P ((message_t, size_t *));
  int (*_size)           __P ((message_t, size_t *));
};

#ifdef _cplusplus
}
#endif

#endif /* _MESSAGE0_H */
