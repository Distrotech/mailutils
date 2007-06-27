/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MIME0_H
#define _MIME0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <mailutils/mime.h>

#ifdef __cplusplus
extern "C" { 
#endif

#define MIME_MAX_HDR_LEN           256
#define MIME_DFLT_BUF_SIZE         2048

/* Parser states */
#define MIME_STATE_BEGIN_LINE      1
#define MIME_STATE_SCAN_BOUNDARY   2
#define MIME_STATE_HEADERS         3

#define MIME_FLAG_MASK             0x0000ffff

/* private */
#define MIME_PARSER_ACTIVE         0x80000000
#define MIME_PARSER_HAVE_CR        0x40000000
#define MIME_NEW_MESSAGE           0x20000000
#define MIME_ADDED_CT              0x10000000
#define MIME_ADDED_MULTIPART_CT    0x08000000
#define MIME_INSERT_BOUNDARY       0x04000000
#define MIME_ADDING_BOUNDARY       0x02000000

struct _mu_mime
{
  mu_message_t       msg;
  mu_header_t        hdrs;
  mu_stream_t        stream;
  int             flags;
  char           *content_type;

  int             tparts;
  int             nmtp_parts;
  struct _mime_part **mtp_parts;      /* list of parts in the msg */
  char           *boundary;
  int             cur_offset;
  int             cur_part;
  int             part_offset;
  int             boundary_len;
  int             preamble;
  int             postamble;
  /* parser state */
  char           *cur_line;
  int             line_ndx;
  char           *cur_buf;
  int             buf_size;
  char           *header_buf;
  int             header_buf_size;
  int             header_length;
  int             body_offset;
  int             body_length;
  int             body_lines;
  int             parser_state;
};

struct _mime_part
{
  mu_mime_t          mime;
  mu_message_t       msg;
  int             body_created;
  int             offset;
  size_t          len;
  size_t          lines;
};

#ifdef __cplusplus
}
#endif

#endif                          /* MIME0_H */
