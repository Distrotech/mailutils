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

#ifndef _MIME_H
#define _MIME_H

#include <header.h>
#include <io.h>
#include <sys/types.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

/* mime flags */
#define MIME_INCREAMENTAL_PARSER	0x00000001

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _mime;
typedef struct _mime *mime_t;

int mime_create						__P ((mime_t *pmime, message_t msg, int flags));
void mime_destroy					__P ((mime_t *pmime));
int mime_is_multi_part				__P ((mime_t mime));
int mime_get_part					__P ((mime_t mime, int part, message_t *msg));
int mime_add_part					__P ((mime_t mime, message_t msg));
int mime_get_num_parts				__P ((mime_t mime, int *nparts));
int mime_get_message				__P ((mime_t mime, message_t *msg));
int mime_unencapsulate				__P ((mime_t mime, message_t msg, message_t *newmsg));

#ifdef _cplusplus
}           
#endif

#endif /* _MIME_H */
