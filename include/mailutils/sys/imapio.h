/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_IMAPIO_H
# define _MAILUTILS_SYS_IMAPIO_H

#include <mailutils/types.h>
#include <mailutils/wordsplit.h>

struct _mu_imapio
{
  mu_stream_t _imap_stream;
  char *_imap_buf_base;
  size_t _imap_buf_size;
  size_t _imap_buf_level;
  struct mu_wordsplit _imap_ws;
  int _imap_ws_flags;
  int _imap_server:1;
  int _imap_transcript:1;
  int _imap_trace_payload:1;
  int _imap_reply_ready:1;
};
  
#endif
