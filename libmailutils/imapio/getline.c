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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/errno.h>
#include <mailutils/kwd.h>
#include <mailutils/sys/imapio.h>

#define STATUS_RESPONSE           1
#define STATUS_RESPONSE_UNTAGGED  2

static int
is_status_response (const char *word)
{
  static struct mu_kwd resptab[] = {
    /* RFC 3501, 7.1:

       Status responses are OK, NO, BAD, PREAUTH and BYE.  OK, NO, and BAD
       can be tagged or untagged.  PREAUTH and BYE are always untagged.
    */
    { "OK", STATUS_RESPONSE },
    { "NO", STATUS_RESPONSE },
    { "BAD", STATUS_RESPONSE },
    { "PREAUTH", STATUS_RESPONSE_UNTAGGED },
    { "BYE", STATUS_RESPONSE_UNTAGGED },
    { NULL }
  };
  int result;
  
  if (mu_kwd_xlat_name (resptab, word, &result))
    return 0;
  return result;
}

static int
get_response_code (struct _mu_imapio *io)
{
  size_t end = io->_imap_ws.ws_endp;
  size_t wc = io->_imap_ws.ws_wordc;
  int rc, i;
  
  do
    {
      if ((rc = mu_wordsplit (NULL, &io->_imap_ws, MU_WRDSF_INCREMENTAL)))
	{
	  if (rc == MU_WRDSE_NOINPUT)
	    break;
	  return MU_ERR_PARSE;
	}

      if (strcmp (io->_imap_ws.ws_wordv[io->_imap_ws.ws_wordc-1], "[") == 0)
	{
	  do
	    {
	      /* Get word */
	      if ((rc = mu_wordsplit (NULL, &io->_imap_ws, MU_WRDSF_INCREMENTAL)))
		{
		  if (rc == MU_WRDSE_NOINPUT)
		    break;
		  return MU_ERR_PARSE;
		}
	    }
	  while (strcmp (io->_imap_ws.ws_wordv[io->_imap_ws.ws_wordc-1], "]"));
	  if (rc)
	    break;
	  return 0;
	}
    }
  while (0);

  for (i = wc; i < io->_imap_ws.ws_wordc; i++)
    free (io->_imap_ws.ws_wordv[i]);
  io->_imap_ws.ws_wordv[wc] = NULL;
  io->_imap_ws.ws_wordc = wc;
  io->_imap_ws.ws_endp = end;
  return 0;
}

#define IMAPIO_OK   0
#define IMAPIO_RESP 1 
#define IMAPIO_ERR  2

static int
initial_parse (struct _mu_imapio *io)
{
  int rc, type;
  int eat_rest = 0;

  if ((rc = mu_wordsplit_len (io->_imap_buf_base, io->_imap_buf_level,
			      &io->_imap_ws,
			      io->_imap_ws_flags |
			      (io->_imap_server ? 0 : MU_WRDSF_INCREMENTAL))))
    {
      if (rc == MU_WRDSE_NOINPUT)
	return IMAPIO_OK;
      return IMAPIO_ERR;
    }
  io->_imap_ws_flags |= MU_WRDSF_REUSE;
  if (io->_imap_server)
    return IMAPIO_OK;
  
  if ((rc = mu_wordsplit (NULL, &io->_imap_ws, MU_WRDSF_INCREMENTAL)))
    {
      if (rc == MU_WRDSE_NOINPUT)
	return IMAPIO_OK;
      return IMAPIO_ERR;
    }

  if (strcmp (io->_imap_ws.ws_wordv[0], "+") == 0)
    eat_rest = 1;
  else if ((type = is_status_response (io->_imap_ws.ws_wordv[1]))
      && (type == STATUS_RESPONSE ||
	  strcmp (io->_imap_ws.ws_wordv[0], "*") == 0))
    {
      rc = get_response_code (io);
      if (rc)
	return IMAPIO_ERR;
      eat_rest = 1;
    }

  if (eat_rest)
    {
      while (io->_imap_ws.ws_endp < io->_imap_ws.ws_len &&
	     mu_isblank (io->_imap_ws.ws_input[io->_imap_ws.ws_endp]))
	io->_imap_ws.ws_endp++;
      io->_imap_ws.ws_flags |= MU_WRDSF_NOSPLIT;
      rc = mu_wordsplit (NULL, &io->_imap_ws, MU_WRDSF_INCREMENTAL);
      io->_imap_ws.ws_flags &= ~MU_WRDSF_NOSPLIT;
      if (rc)
	{
	  if (rc != MU_WRDSE_NOINPUT)
	    return IMAPIO_ERR;
	}
      return IMAPIO_RESP;
    }
  return IMAPIO_OK;
}  

int
mu_imapio_getline (struct _mu_imapio *io)
{
  int rc;
  char *last_arg;
  int xlev = MU_XSCRIPT_NORMAL;
  
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
	break;
      if (io->_imap_buf_level == 0)
	break;
      io->_imap_buf_level = mu_rtrim_class (io->_imap_buf_base,
					    MU_CTYPE_ENDLN);
      
      rc = initial_parse (io);
      if (rc == IMAPIO_ERR)
	{
	  rc = MU_ERR_PARSE;
	  break;
	}
      else if (rc == IMAPIO_RESP)
	{
	  rc = 0;
	  break;
	}
	
      rc = mu_wordsplit_len (io->_imap_buf_base + io->_imap_ws.ws_endp,
			     io->_imap_buf_level - io->_imap_ws.ws_endp,
			     &io->_imap_ws, io->_imap_ws_flags);
      if (rc)
	{
	  rc = MU_ERR_PARSE;
	  break;
	}
      
      if (io->_imap_ws.ws_wordc == 0)
	break;
      
      last_arg = io->_imap_ws.ws_wordv[io->_imap_ws.ws_wordc - 1];
      if (last_arg[0] == '{' && last_arg[strlen (last_arg)-1] == '}')
	{
	  int rc;
	  unsigned long number;
	  char *sp = NULL;

	  if (!io->_imap_trace_payload)
	    xlev = mu_imapio_set_xscript_level (io, MU_XSCRIPT_PAYLOAD);
	  
	  number = strtoul (last_arg + 1, &sp, 10);
	  /* Client can ask for non-synchronised literal,
	     if a '+' is appended to the octet count. */
	  if (*sp == '}')
	    {
	      if (io->_imap_server)
		mu_stream_printf (io->_imap_stream, "+ GO AHEAD\n");
	    }
	  else if (*sp != '+')
	    break;

	  if (number + 1 > io->_imap_buf_size)
	    {
	      size_t newsize = number + 1;
	      void *newp = realloc (io->_imap_buf_base, newsize);
	      if (!newp)
		{
		  rc = ENOMEM;
		  break;
		}
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
	    break;
	  io->_imap_buf_base[io->_imap_buf_level++] = 0;

	  free (last_arg);
	  io->_imap_ws.ws_wordv[--io->_imap_ws.ws_wordc] = NULL;
	  if (mu_wordsplit_len (io->_imap_buf_base, io->_imap_buf_level,
				&io->_imap_ws,
				io->_imap_ws_flags|MU_WRDSF_NOSPLIT))
	    {
	      rc = MU_ERR_PARSE;
	      break;
	    }
	}
      else
	break;
    }

  if (!io->_imap_trace_payload)
    mu_imapio_set_xscript_level (io, xlev);

  io->_imap_reply_ready = 1;
  return rc;
}
