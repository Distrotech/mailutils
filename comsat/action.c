/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 2007, 2009, 2010 Free
   Software Foundation, Inc.

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

#include "comsat.h"
#include <mailutils/io.h>
#include <mailutils/argcv.h>
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <obstack.h>

/* This module implements user-configurable actions for comsat. The
   actions are kept in file .biffrc in the user's home directory and
   are executed in sequence. Possible actions:

   beep              --  Produce audible signal
   echo ARGS...      --  Output ARGS to the user's tty
   exec PROG ARGS... --  Execute given program (absolute pathname
                         required)

   Following metacharacters are accepted in strings:

   $u        --  Expands to username
   $h        --  Expands to hostname
   $H{name}  --  Expands to value of message header `name'
   $B(C,L)   --  Expands to message body. C and L give maximum
                 number of characters and lines in the expansion.
		 When omitted, they default to 400, 5. */

static int
expand_escape (char **pp, mu_message_t msg, struct obstack *stk)
{
  char *p = *pp;
  char *start, *sval, *namep;
  int len;
  mu_header_t hdr;
  mu_body_t body;
  mu_stream_t stream;
  int rc = 1;
  size_t size = 0, lncount = 0;

  switch (*++p) /* skip past $ */
    {
    case 'u':
      len = strlen (username);
      obstack_grow (stk, username, len);
      *pp = p;
      rc = 0;
      break;

    case 'h':
      len = strlen (hostname);
      obstack_grow (stk, hostname, len);
      *pp = p;
      rc = 0;
      break;

    case 'H':
      /* Header field */
      if (*++p != '{')
	break;
      start = ++p;
      p = strchr (p, '}');
      if (!p)
	break;
      len = p - start;
      if (len == 0
	  || (namep = malloc (len + 1)) == NULL)
	break;
      memcpy (namep, start, len);
      namep[len] = 0;
      if (mu_message_get_header (msg, &hdr) == 0
	  && mu_header_aget_value (hdr, namep, &sval) == 0)
	{
	  len = strlen (sval);
	  obstack_grow (stk, sval, len);
	}
      free (namep);
      *pp = p;
      rc = 0;
      break;

    case 'B':
      /* Message body */
      if (*++p == '(')
	{
	  size = strtoul (p + 1, &p, 0);
	  if (*p == ',')
	    lncount = strtoul (p + 1, &p, 0);
	  if (*p != ')')
	    break;
	}
      if (size == 0)
	size = 400;
      if (lncount == 0)
	lncount = maxlines;
      if (mu_message_get_body (msg, &body) == 0
	  && mu_body_get_streamref (body, &stream) == 0)
	{
	  size_t nread;
	  char *buf = malloc (size+1);

	  if (!buf)
	    break;
 	  if (mu_stream_read (stream, buf, size, &nread) == 0)
	    {
	      char *q;

	      buf[nread] = 0;
	      q = buf;
	      size = 0;
	      while (lncount--)
		{
		  char *s = strchr (q, '\n');
		  if (!s)
		    break;
		  size += s - q + 1;
		  q = s + 1;
		}
	      obstack_grow (stk, buf, size);
	    }
	  mu_stream_destroy (&stream);
	  free (buf);
	}
      *pp = p;
      rc = 0;
    }
  return rc;
}

static char *
expand_line (const char *str, mu_message_t msg)
{
  const char *p;
  int c = 0;
  struct obstack stk;

  if (!*str)
    return NULL;
  obstack_init (&stk);
  for (p = str; *p; p++)
    {
      switch (*p)
	{
	case '\\':
	  p++;
	  if (*p)
	    {
	      c = mu_wordsplit_c_unquote_char (*p);
	      obstack_1grow (&stk, c);
	    }
	  break;

	case '$':
	  if (expand_escape ((char**)&p, msg, &stk) == 0)
	    break;

	  /*FALLTHRU*/
	default:
	  obstack_1grow (&stk, *p);
	}
    }
  obstack_1grow (&stk, 0);
  str = strdup (obstack_finish (&stk));
  obstack_free (&stk, NULL);
  return (char *)str;
}

const char *default_action =
"Mail to \a$u@$h\a\n"
"---\n"
"From: $H{from}\n"
"Subject: $H{Subject}\n"
"---\n"
"$B(,5)\n"
"---\n";

static void
action_beep (mu_stream_t tty)
{
  mu_stream_write (tty, "\a\a", 2, NULL);
  mu_stream_flush (tty);
}

static void
echo_string (mu_stream_t tty, char *str)
{
  if (!str)
    return;
  mu_stream_write (tty, str, strlen (str), NULL);
}

static void
action_echo (mu_stream_t tty, int omit_newline, int argc, char **argv)
{
  int i;

  for (i = 0;;)
    {
      echo_string (tty, argv[i]);
      if (++i < argc)
	echo_string (tty, " ");
      else
	break;
    }
  if (!omit_newline)
    echo_string (tty, "\n");
}

static void
action_exec (mu_stream_t tty, int argc, char **argv)
{
  mu_stream_t pstream;
  struct stat stb;
  char *command;
  int status;
  
  if (argc == 0)
    {
      mu_diag_output (MU_DIAG_ERROR, _("no arguments for exec"));
      return;
    }

  if (argv[0][0] != '/')
    {
      mu_diag_output (MU_DIAG_ERROR, _("not an absolute pathname: %s"),
		      argv[0]);
      return;
    }

  if (stat (argv[0], &stb))
    {
      mu_diag_funcall (MU_DIAG_ERROR, "stat", argv[0], errno);
      return;
    }

  if (stb.st_mode & (S_ISUID|S_ISGID))
    {
      mu_diag_output (MU_DIAG_ERROR, _("will not execute set[ug]id programs"));
      return;
    }

  /* FIXME: Redirect stderr to out */
  /* FIXME: need this:
     status = mu_prog_stream_create_argv (&pstream, argv[0], argv,
                                          MU_STREAM_READ);
  */
  status = mu_argcv_join (argc, argv, " ", mu_argcv_escape_no, &command);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_argcv_join", NULL, status);
      return;
    }
  status = mu_prog_stream_create (&pstream, command, MU_STREAM_READ);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_prog_stream_create_argv", argv[0],
		       status);
      return;
    }
  mu_stream_copy (tty, pstream, 0, NULL);
  mu_stream_destroy (&pstream);
}

static mu_stream_t
open_rc (const char *filename, mu_stream_t tty)
{
  struct stat stb;
  struct passwd *pw = getpwnam (username);
  mu_stream_t stream, input;
  int rc;
  
  /* To be on the safe side, we do not allow root to have his .biffrc */
  if (!allow_biffrc || pw->pw_uid == 0)
    return NULL;
  if (stat (filename, &stb) == 0)
    {
      if (stb.st_uid != pw->pw_uid)
	{
	  mu_diag_output (MU_DIAG_NOTICE, _("%s's %s is not owned by %s"),
		  username, filename, username);
	  return NULL;
	}
      if ((stb.st_mode & 0777) != 0600)
	{
	  mu_stream_printf (tty, "%s\n",
			    _("Warning: your .biffrc has wrong permissions"));
	  mu_diag_output (MU_DIAG_NOTICE, _("%s's %s has wrong permissions"),
		  username, filename);
	  return NULL;
	}
    }
  rc = mu_file_stream_create (&input, filename, MU_STREAM_READ);
  if (rc)
    {
      if (rc != ENOENT)
	{
	  mu_stream_printf (tty, _("Cannot open .biffrc file: %s\n"),
			    mu_strerror (rc));
	  mu_diag_output (MU_DIAG_NOTICE, _("cannot open %s for %s: %s"),
			  filename, username, mu_strerror (rc));
	}
      return NULL;
    }
  rc = mu_filter_create (&stream, input, "LINECON", MU_FILTER_DECODE,
			 MU_STREAM_READ);
  mu_stream_unref (input);
  if (rc)
    {
      mu_stream_printf (tty,
			_("Cannot create filter for your .biffrc file: %s\n"),
			mu_strerror (rc));
      mu_diag_output (MU_DIAG_NOTICE,
		      _("cannot create filter for file %s of %s: %s"),
		      filename, username, mu_strerror (rc));
      return NULL;
    }
  return stream;
}

void
run_user_action (mu_stream_t tty, mu_message_t msg)
{
  mu_stream_t input;
  int nact = 0;

  input = open_rc (BIFF_RC, tty);
  if (input)
    {
      char *stmt = NULL;
      size_t size = 0;
      size_t n;
      char *cwd = mu_getcwd ();
      char *rcname;
      struct mu_locus locus;
      
      rcname = mu_make_file_name (cwd, BIFF_RC);
      free (cwd);
      if (!rcname)
        {
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name", NULL, ENOMEM);
	  locus.mu_file = BIFF_RC;
        }
      else
	locus.mu_file = rcname;
      locus.mu_line = 1;
      locus.mu_col = 0;
      while (mu_stream_getline (input, &stmt, &size, &n) == 0 && n > 0)
	{
	  struct mu_wordsplit ws;

	  ws.ws_comment = "#";
	  if (mu_wordsplit (stmt, &ws,
			    MU_WRDSF_DEFFLAGS | MU_WRDSF_COMMENT) == 0
	      && ws.ws_wordc)
	    {
	      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                               MU_IOCTL_LOGSTREAM_SET_LOCUS, &locus);
	      if (strcmp (ws.ws_wordv[0], "beep") == 0)
		{
		  /* FIXME: excess arguments are ignored */
		  action_beep (tty);
		  nact++;
		}
	      else
		{
		  /* Rest of actions require keyword expansion */
		  int i;
		  int n_option = ws.ws_wordc > 1 &&
		                 strcmp (ws.ws_wordv[1], "-n") == 0;
		  
		  for (i = 1; i < ws.ws_wordc; i++)
		    {
		      char *oldarg = ws.ws_wordv[i];
		      ws.ws_wordv[i] = expand_line (ws.ws_wordv[i], msg);
		      free (oldarg);
		      if (!ws.ws_wordv[i])
			break;
		    }
		  
		  if (strcmp (ws.ws_wordv[0], "echo") == 0)
		    {
		      int argc = ws.ws_wordc - 1;
		      char **argv = ws.ws_wordv + 1;
		      if (n_option)
			{
			  argc--;
			  argv++;
			}
		      action_echo (tty, n_option, argc, argv);
		      nact++;
		    }
		  else if (strcmp (ws.ws_wordv[0], "exec") == 0)
		    {
		      action_exec (tty, ws.ws_wordc - 1, ws.ws_wordv + 1);
		      nact++;
		    }
		  else
		    {
		      mu_stream_printf (tty,
					_(".biffrc:%d: unknown keyword\n"),
					locus.mu_line);
		      mu_diag_output (MU_DIAG_ERROR, _("unknown keyword %s"),
				      ws.ws_wordv[0]);
		      break;
		    }
		} 
	    }
	  mu_wordsplit_free (&ws);
	  /* FIXME: line number is incorrect if .biffrc contains
	     escaped newlines */
	  locus.mu_line++;
	}
      mu_stream_destroy (&input);
      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                       MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
      free (rcname);
    }

  if (nact == 0)
    echo_string (tty, expand_line (default_action, msg));
}
