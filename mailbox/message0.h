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

#include <attribute.h>
#include <header.h>
#include <message.h>
#include <mailbox.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef _cpluscplus
extern "C" {
#endif

struct _body
{
  FILE *file;
  char *content;
  size_t content_len;
};

typedef struct _body * body_t;

/* forward declaration */
struct _message
{
  mailbox_t mailbox;
  header_t header;
  message_t *parts;
  size_t part_num;
  body_t body;
  size_t num;
  attribute_t attribute;

  int (*_get_header)  __P ((message_t msg, header_t *hdr));
  int (*_set_header)  __P ((message_t msg, header_t hdr));

  int (*_get_attribute)  __P ((message_t msg, attribute_t *attr));
  int (*_set_attribute)  __P ((message_t msg, attribute_t attr));

  int (*_get_content) __P ((message_t msg, char *buf, size_t len,
			    size_t *nread, off_t offset));
  int (*_set_content) __P ((message_t msg, char *buf, size_t len));

  int (*_size)        __P ((message_t msg, size_t *size));

  int (*_clone)       __P ((message_t msg, message_t *cmsg));
};

#ifdef _cpluscplus
}
#endif

#endif /* _MESSAGE_H */
