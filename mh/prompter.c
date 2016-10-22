/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

/* MH prompter command */

#include <mh.h>
#include "prompter.h"

static char prog_doc[] = N_("Prompting editor front-end for GNU MH");
static char args_doc[] = N_("FILE");

char *erase_seq;
char *kill_seq;
int prepend_option;
int rapid_option;
int doteof_option;

static struct mu_option options[] = {
  { "erase",         0,     N_("CHAR"), MU_OPTION_DEFAULT,
    N_("set erase character"),
    mu_c_string, &erase_seq },
  { "kill",          0,     N_("CHAR"), MU_OPTION_DEFAULT,
    N_("set kill character"),
    mu_c_string, &kill_seq },
  { "prepend",       0,     NULL, MU_OPTION_DEFAULT,
    N_("prepend user input to the message body"),
    mu_c_bool, &prepend_option },
  { "rapid",         0,     NULL, MU_OPTION_DEFAULT,
    N_("do not display message body"),
    mu_c_bool, &rapid_option },
  { "doteof",        0,     NULL, MU_OPTION_DEFAULT,
    N_("a period on a line marks end-of-file"),
    mu_c_bool, &doteof_option },
  MU_OPTION_END
};

static int
is_empty_string (const char *str)
{
  if (!str)
    return 1;
  return mu_str_skip_class (str, MU_CTYPE_BLANK)[0] == 0;
}

char *
doteof_filter (char *val)
{
  if (!val)
    return NULL;
  if (doteof_option && val[0] == '.')
    {
      if (val[1] == 0)
	{
	  free (val);
	  return NULL;
	}
      memmove (val, val + 1, strlen (val + 1) + 1);
    }
  return val;
}

mu_stream_t strout;

int
main (int argc, char **argv)
{
  int rc;
  mu_stream_t in, tmp;
  mu_message_t msg;
  mu_header_t hdr;
  mu_iterator_t itr;
  const char *file;
  char *newval;
  mu_off_t size;
  mu_body_t body;
  mu_stream_t bstr;
  
  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);

  if (argc == 0)
    {
      mu_error (_("file name not given"));
      exit (1);
    }
  else if (argc > 1)
    {
      mu_error (_("too many arguments"));
      exit (1);
    }
  
  file = argv[0];

  prompter_init ();
  if (erase_seq)
    prompter_set_erase (erase_seq);
  if (kill_seq)
    prompter_set_erase (kill_seq);

  if ((rc = mu_stdio_stream_create (&strout, MU_STDOUT_FD, MU_STREAM_WRITE)))
    {
      mu_error (_("cannot open stdout: %s"), mu_strerror (rc));
      return 1;
    }

  if ((rc = mu_file_stream_create (&in, file, MU_STREAM_RDWR)))
    {
      mu_error (_("cannot open input file `%s': %s"),
		file, mu_strerror (rc));
      return 1;
    }
  rc = mu_stream_to_message (in, &msg);
  if (rc)
    {
      mu_error (_("input stream %s is not a message (%s)"),
		file, mu_strerror (rc));
      return 1;
    }

  if ((rc = mu_temp_file_stream_create (&tmp, NULL, 0))) 
    {
      mu_error (_("Cannot open temporary file: %s"),
		mu_strerror (rc));
      return 1;
    }

  /* Copy headers */
  mu_message_get_header (msg, &hdr);
  mu_header_get_iterator (hdr, &itr);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *name, *val;
      
      mu_iterator_current_kv (itr, (const void **)&name, (void**)&val);
      if (!is_empty_string (val))
	{
	  mu_stream_printf (tmp, "%s: %s\n", name, val);
	  mu_stream_printf (strout, "%s: %s\n", name, val);
	}
      else
	{
	  int cont = 0;
	  mu_opool_t opool;
	  const char *prompt = name;
	  
	  mu_opool_create (&opool, 1);
	  do
	    {
	      size_t len;
	      char *p;
	      p = prompter_get_value (prompt);
	      if (!p)
		return 1;
	      prompt = NULL;
	      if (cont)
		{
		  mu_opool_append_char (opool, '\n');
		  if (!mu_isspace (p[0]))
		    mu_opool_append_char (opool, '\t');
		}
	      len = strlen (p);
	      if (len > 0 && p[len-1] == '\\')
		{
		  len--;
		  cont = 1;
		}
	      else
		cont = 0;
	      mu_opool_append (opool, p, len);
	      free (p);
	    }
	  while (cont);

	  mu_opool_append_char (opool, 0);
	  newval = mu_opool_finish (opool, NULL);
	  if (!is_empty_string (newval))
	    mu_stream_printf (tmp, "%s: %s\n", name, newval);
	  mu_opool_destroy (&opool);
	}
    }
  mu_iterator_destroy (&itr);
  mu_stream_printf (strout, "--------\n");
  mu_stream_write (tmp, "\n", 1, NULL);

  /* Copy body */
  
  if (prepend_option)
    {
      mu_stream_printf (strout, "\n--------%s\n\n", _("Enter initial text"));
      while ((newval = prompter_get_line ()))
	{
	  mu_stream_write (tmp, newval, strlen (newval), NULL);
	  free (newval);
	  mu_stream_write (tmp, "\n", 1, NULL);
	}
    }

  mu_message_get_body (msg, &body);
  mu_body_get_streamref (body, &bstr);

  if (!prepend_option && !rapid_option)
    {
      mu_stream_copy (strout, bstr, 0, NULL);
      mu_stream_seek (bstr, 0, MU_SEEK_SET, NULL);
    }

  mu_stream_copy (tmp, bstr, 0, NULL);
  mu_stream_unref (bstr);

  if (!prepend_option && !rapid_option)
    {
      mu_stream_printf (strout, "\n--------%s\n\n", _("Enter additional text"));
      while ((newval = prompter_get_line ()))
	{
	  mu_stream_write (tmp, newval, strlen (newval), NULL);
	  free (newval);
	  mu_stream_write (tmp, "\n", 1, NULL);
	}
    }

  /* Destroy the message */
  mu_message_destroy (&msg, mu_message_get_owner (msg));

  /* Rewind the streams and copy data back to in. */
  mu_stream_seek (in, 0, MU_SEEK_SET, NULL);
  mu_stream_seek (tmp, 0, MU_SEEK_SET, NULL);
  mu_stream_copy (in, tmp, 0, &size);
  mu_stream_truncate (in, size);

  mu_stream_destroy (&in);
  mu_stream_destroy (&tmp);
  mu_stream_destroy (&strout);

  prompter_done ();
  
  return 0;
}

