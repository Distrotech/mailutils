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

#ifndef _MAILUTILS_MIME_H
#define _MAILUTILS_MIME_H

#include <sys/types.h>
#include <mailutils/header.h>
#include <mailutils/stream.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

/* mime flags */
#define MIME_INCREAMENTAL_PARSER	0x1

#define MIME_MULTIPART_MIXED		0x1
#define MIME_MULTIPART_ALT          0x2

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _mime;
typedef struct _mime *mime_t;

int mime_create		__P ((mime_t *pmime, message_t msg, int flags));
void mime_destroy	__P ((mime_t *pmime));
int mime_is_multipart	__P ((mime_t mime));
int mime_get_num_parts	__P ((mime_t mime, size_t *nparts));

int mime_get_part	__P ((mime_t mime, size_t part, message_t *msg));

int mime_add_part	__P ((mime_t mime, message_t msg));

int mime_get_message	__P ((mime_t mime, message_t *msg));

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_MIME_H */
