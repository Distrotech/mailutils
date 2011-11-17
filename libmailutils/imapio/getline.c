/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imapio.h>

int
mu_imapio_getline (struct _mu_imapio *io)
{
  int rc;
  char *last_arg;

  io->_imap_ws_flags &= ~MU_WRDSF_APPEND;

  if (io->_imap_reply_ready)
    {
      mu_wordsplit_free_words (&io->_imap_ws);
      io->_imap_reply_ready = 0;
    }
  
  for (;;)
    {
      rc = mu_stream_getline (io->_imap_stream,
			      &io->_imap_buf_base, &io->_imap_buf_size,
			      &io->_imap_buf_level);
      if (rc)
	return rc;
      if (io->_imap_buf_level == 0)
	break;
      io->_imap_buf_level = mu_rtrim_class (io->_imap_buf_base,
					    MU_CTYPE_ENDLN);

      if (mu_wordsplit_len (io->_imap_buf_base, io->_imap_buf_level,
			    &io->_imap_ws, io->_imap_ws_flags))
	return MU_ERR_PARSE;
      io->_imap_ws_flags |= MU_WRDSF_REUSE|MU_WRDSF_APPEND;

      last_arg = io->_imap_ws.ws_wordv[io->_imap_ws.ws_wordc - 1];
      if (last_arg[0] == '{' && last_arg[strlen (last_arg)-1] == '}')
	{
	  int rc;
	  unsigned long number;
	  char *sp = NULL;
	  int xlev = mu_imapio_set_xscript_level (io, MU_XSCRIPT_PAYLOAD);
	  
	  number = strtoul (last_arg + 1, &sp, 10);
	  /* Client can ask for non-synchronised literal,
	     if a '+' is appended to the octet count. */
	  if (*sp == '}')
	    mu_stream_printf (io->_imap_stream, "+ GO AHEAD\n");
	  else if (*sp != '+')
	    break;

	  if (number + 1 > io->_imap_buf_size)
	    {
	      size_t newsize = number + 1;
	      void *newp = realloc (&io->_imap_buf_base, newsize);
	      if (!newp)
		return ENOMEM;
	      io->_imap_buf_base = newp;
	      io->_imap_buf_size = newsize;
	    }
	      
          for (io->_imap_buf_level = 0; io->_imap_buf_level < number; )
            {
               size_t sz;
	       rc = mu_stream_read (io->_imap_stream,
				    io->_imap_buf_base + io->_imap_buf_level,
				    number - io->_imap_buf_level,
				    &sz);
               if (rc || sz == 0)
                 break;
               io->_imap_buf_level += sz;
            }
	  mu_imapio_set_xscript_level (io, xlev);
	  if (rc)
	    return rc;
	  io->_imap_buf_base[io->_imap_buf_level++] = 0;

	  free (last_arg);
	  io->_imap_ws.ws_wordv[--io->_imap_ws.ws_wordc] = NULL;
	  if (mu_wordsplit_len (io->_imap_buf_base, io->_imap_buf_level,
				&io->_imap_ws,
				io->_imap_ws_flags|MU_WRDSF_NOSPLIT))
	    return MU_ERR_PARSE;
	}
      else
	break;
    }

  io->_imap_reply_ready = 1;
  return 0;
}
