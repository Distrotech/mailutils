/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/message.h>
#include <mailutils/attribute.h>
#include <mailutils/cstr.h>

int
mu_stream_scan_message (mu_stream_t stream, struct mu_message_scan *sp)
{
  char buf[1024];
  mu_off_t off;
  size_t n;
  int status;
  int in_header = 1;
  size_t hlines = 0;
  size_t blines = 0;
  size_t body_start = 0;
  int attr_flags = 0;
  unsigned long uidvalidity = 0;
  
  if (sp->flags & MU_SCAN_SEEK)
    {
      status = mu_stream_seek (stream, sp->message_start, MU_SEEK_SET, NULL);
      if (status)
	return status;
    }
  
  off = 0;
  while (1)
    {
      size_t rdsize;
      
      status = mu_stream_readline (stream, buf, sizeof (buf), &n);
      if (status || n == 0)
	break;
      
      if (sp->flags & MU_SCAN_SIZE)
	{
	  rdsize = sp->message_size - off;
	  if (n > rdsize)
	    n = rdsize;
	}
      
      if (in_header)
	{
	  if (buf[0] == '\n')
	    {
	      in_header = 0;
	      body_start = off + 1;
	    }
	  if (buf[n - 1] == '\n')
	    hlines++;
	    
	  /* Process particular attributes */
	  if (mu_c_strncasecmp (buf, "status:", 7) == 0)
	    mu_string_to_flags (buf, &attr_flags);
	  else if (mu_c_strncasecmp (buf, "x-imapbase:", 11) == 0)
	    {
	      char *p;
	      uidvalidity = strtoul (buf + 11, &p, 10);
	      /* second number is next uid. Ignored */
	    }
	}
      else
	{
	  if (buf[n - 1] == '\n')
	    blines++;
	}
      off += n;
    }

  if (status == 0)
    {
      if (!body_start)
	body_start = off;
      sp->body_start = body_start;
      sp->body_end = off;
      sp->header_lines = hlines;
      sp->body_lines = blines;
      sp->attr_flags = attr_flags;
      sp->uidvalidity = uidvalidity;
    }
  return status;
}
