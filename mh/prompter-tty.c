/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

/* MH prompter - traditional tty/termios support */

#include <mh.h>
#include "prompter.h"
#include "muaux.h"
#if HAVE_TERMIOS_H
# include <termios.h>
#endif

mu_stream_t strin;
#if HAVE_TCGETATTR
struct termios tios;
#endif

static int
decode_key (const char *keyseq)
{
  if (keyseq[0] == '^' && keyseq[1])
    {
      if (keyseq[2])
	{
	  mu_error (_("invalid keysequence (%s near %d)"),
		    keyseq, 2);
	  exit (1);
	}
      return keyseq[1] + 0100;
    }
  else if (keyseq[0] == '\\')
    {
      int i;
      int c = 0;
      for (i = 1; i <= 3; i++)
	{
	  int n;
	  switch (keyseq[i])
	    {
	    case '0':
	      n = 0;
	      break;
	    case '1':
	      n = 1;
	      break;
	    case '2':
	      n = 2;
	      break;
	    case '3':
	      n = 3;
	      break;
	    case '4':
	      n = 4;
	      break;
	    case '5':
	      n = 5;
	      break;
	    case '6':
	      n = 6;
	      break;
	    case '7':
	      n = 7;
	      break;
	    default:
	      mu_error (_("invalid keysequence (%s near %d)"),
			keyseq, i);
	      exit (1);
	    }
	  c = (c << 3) + n;
	}
      if (keyseq[i])
	{
	  mu_error (_("invalid keysequence (%s near %d)"),
		    keyseq, i);
	  exit (1);
	}
      return c;
    }
  else if (keyseq[1])
    {
      mu_error (_("invalid keysequence (%s near %d)"),
		keyseq, 2);
      exit (1);
    }

  return keyseq[0];
}

static RETSIGTYPE
sighan (int signo)
{
  prompter_done ();
  exit (0);
}

void
prompter_init ()
{
  int rc;
  if ((rc = mu_stdio_stream_create (&strin, MU_STDIN_FD, MU_STREAM_READ)))
    {
      mu_error (_("cannot open stdout: %s"), mu_strerror (rc));
      exit (1);
    }
#if HAVE_TCGETATTR
  rc = tcgetattr (MU_STDOUT_FD, &tios);
  if (rc)
    {
      mu_error (_("tcgetattr failed: %s"), mu_strerror (rc));
      exit (1);
    }
  else
    {
      static int sigtab[] = { SIGPIPE, SIGABRT, SIGINT, SIGQUIT, SIGTERM };
      mu_set_signals (sighan, sigtab, MU_ARRAY_SIZE (sigtab));
    }
#endif
}

void
prompter_done ()
{
#if HAVE_TCGETATTR
  int rc = tcsetattr (MU_STDOUT_FD, TCSANOW, &tios);
  if (rc)
    mu_error (_("tcsetattr failed: %s"), mu_strerror (rc));
#endif
}

void
prompter_set_erase (const char *keyseq)
{
#if HAVE_TCGETATTR
  int rc;
  struct termios t = tios;
  
  t.c_lflag |= ICANON;
  t.c_cc[VERASE] = decode_key (keyseq);
  rc = tcsetattr (MU_STDOUT_FD, TCSANOW, &t);
  if (rc)
    mu_error (_("tcsetattr failed: %s"), mu_strerror (rc));
#endif
}

void
prompter_set_kill (const char *keyseq)
{
#if HAVE_TCGETATTR
  int rc;
  struct termios t = tios;
  
  t.c_lflag |= ICANON;
  t.c_cc[VKILL] = decode_key (keyseq);
  rc = tcsetattr (MU_STDOUT_FD, TCSANOW, &t);
  if (rc)
    mu_error (_("tcsetattr failed: %s"), mu_strerror (rc));
#endif
}

char *
prompter_get_value (const char *name)
{
  size_t size = 0, n;
  char *buf = NULL;
  
  mu_stream_printf (strout, "%s: ", name);
  mu_stream_flush (strout);
  if (mu_stream_getline (strin, &buf, &size, &n) || n == 0)
    return NULL;
  mu_rtrim_cset (buf, "\n");
  return buf;
}

char *
prompter_get_line ()
{
  size_t size = 0, n;
  char *buf = NULL;
  
  if (mu_stream_getline (strin, &buf, &size, &n) || n == 0)
    return NULL;
  mu_rtrim_cset (buf, "\n");
  return doteof_filter (buf);
}
