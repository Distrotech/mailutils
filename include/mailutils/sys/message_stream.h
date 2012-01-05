/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_MESSAGE_STREAM_H
#define _MAILUTILS_SYS_MESSAGE_STREAM_H

#include <mailutils/sys/stream.h>

struct _mu_message_stream
{
  struct _mu_stream stream;
  mu_stream_t transport;  /* Actual stream */
  mu_off_t offset;

  int construct_envelope;
  char *envelope_string;
  size_t envelope_length; 
  char *from;
  char *date;
  size_t mark_offset;  /* Offset of the header separator */
  size_t mark_length;  /* Length of the header separator (not counting the
			  newline) */
  mu_off_t body_start;
  mu_off_t body_end;
};

struct _mu_body_stream
{
  struct _mu_stream stream;
  mu_off_t offset;
  struct _mu_message_stream *message_stream;
};

#endif


