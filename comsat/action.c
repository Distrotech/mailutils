/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

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
#include <mailutils/prog.h>
#include <mailutils/opool.h>

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
expand_escape (char **pp, mu_message_t msg, mu_opool_t pool)
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
      mu_opool_appendz (pool, username);
      *pp = p;
      rc = 0;
      break;

    case 'h':
      mu_opool_appendz (pool, hostname);
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
	mu_opool_appendz (pool, sval);
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
	      mu_opool_append (pool, buf, size);
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
  mu_opool_t pool;

  if (!*str)
    return NULL;
  mu_opool_create (&pool, 1);
  for (p = str; *p; p++)
    {
      switch (*p)
	{
	case '\\':
	  p++;
	  if (*p)
	    {
	      c = mu_wordsplit_c_unquote_char (*p);
	      mu_opool_append_char (pool, c);
	    }
	  break;

	case '$':
	  if (expand_escape ((char**)&p, msg, pool) == 0)
	    break;

	  /*FALLTHRU*/
	default:
	  mu_opool_append_char (pool, *p);
	}
    }
  mu_opool_append_char (pool, 0);
  str = strdup (mu_opool_finish (pool, NULL));
  mu_opool_destroy (&pool);
  return (char *)str;
}

const char *default_action =
#include "biff.rc.h"
;

static int
need_crlf (mu_stream_t str)
{
#if defined(OPOST) && defined(ONLCR)
  mu_transport_t trans[2];
  struct termios tbuf;

  if (mu_stream_ioctl (str, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, trans))
    return 1; /* suppose we do need it */
  if (tcgetattr ((int)trans[0], &tbuf) == 0 &&
      (tbuf.c_oflag & OPOST) && (tbuf.c_oflag & ONLCR))
    return 0;
  else
    return 1;
#else
  return 1; /* Just in case */
#endif
}

static mu_stream_t
_open_tty (const char *device, int argc, char **argv)
{
  mu_stream_t dev, base_dev, prev_stream;
  int status;
  
  status = mu_file_stream_create (&dev, device, MU_STREAM_WRITE);
  if (status)
    {
      mu_error (_("cannot open device %s: %s"),
		device, mu_strerror (status));
      return NULL;
    }
  mu_stream_set_buffer (dev, mu_buffer_line, 0);

  prev_stream = base_dev = dev;
  while (argc)
    {
      int i;
      int mode;
      int qmark;
      char *fltname;
      
      fltname = argv[0];
      if (fltname[0] == '?')
	{
	  qmark = 1;
	  fltname++;
	}
      else
	qmark = 0;
      
      if (fltname[0] == '~')
	{
	  mode = MU_FILTER_DECODE;
	  fltname++;
	}
      else
	{
	  mode = MU_FILTER_ENCODE;
	}
      
      for (i = 1; i < argc; i++)
	if (strcmp (argv[i], "+") == 0)
	  break;
      
      if (qmark == 0 || need_crlf (base_dev))
	{
	  status = mu_filter_create_args (&dev, prev_stream, fltname,
					  i, (const char **)argv,
					  mode, MU_STREAM_WRITE);
	  mu_stream_unref (prev_stream);
	  if (status)
	    {
	      mu_error (_("cannot open filter stream: %s"),
			mu_strerror (status));
	      return NULL;
	    }
	  prev_stream = dev;
	}
      argc -= i;
      argv += i;
      if (argc)
	{
	  argc--;
	  argv++;
	}
    }
  return dev;
}

mu_stream_t
open_tty (const char *device, int argc, char **argv)
{
  mu_stream_t dev;

  if (!device || !*device || strcmp (device, "null") == 0)
    {
      int rc = mu_nullstream_create (&dev, MU_STREAM_WRITE);
      if (rc)
	mu_error (_("cannot open null stream: %s"), mu_strerror (rc));
    }
  else
    dev = _open_tty (device, argc, argv); 
  return dev;
}

static mu_stream_t
open_default_tty (const char *device)
{
  static char *default_filters[] = { "7bit", "+", "?CRLF", NULL };
  return open_tty (device, MU_ARRAY_SIZE (default_filters) - 1,
		   default_filters);
}


struct biffrc_environ
{
  mu_stream_t tty;
  mu_stream_t logstr;
  mu_message_t msg;
  mu_stream_t input;
  struct mu_locus locus;
  int use_default;
  char *errbuf;
  size_t errsize;
};

static void
report_error (struct biffrc_environ *env, const char *fmt, ...)
{
  if (biffrc_errors)
    {
      va_list ap;
      va_start (ap, fmt);
      mu_vasnprintf (&env->errbuf, &env->errsize, fmt, ap);
      if (biffrc_errors & BIFFRC_ERRORS_TO_TTY)
	mu_stream_printf (env->logstr, "%s\n", env->errbuf);
      if (biffrc_errors & BIFFRC_ERRORS_TO_ERR)
	mu_diag_output (MU_DIAG_ERROR, "%s", env->errbuf);
      va_end (ap);
    }
}

static void
action_beep (struct biffrc_environ *env, size_t argc, char **argv)
{
  mu_stream_write (env->tty, "\a\a", 2, NULL);
  mu_stream_flush (env->tty);
}

static void
echo_string (mu_stream_t tty, char *str)
{
  if (!str)
    return;
  mu_stream_write (tty, str, strlen (str), NULL);
}

static void
action_echo (struct biffrc_environ *env, size_t argc, char **argv)
{
  int i = 1;
  int omit_newline;

  if (argc > 2 && strcmp (argv[i], "-n") == 0)
    {
      omit_newline = 1;
      i++;
    }
  else
    omit_newline = 0;

  for (;;)
    {
      echo_string (env->tty, argv[i]);
      if (++i < argc)
	echo_string (env->tty, " ");
      else
	break;
    }
  if (!omit_newline)
    echo_string (env->tty, "\n");
}

static void
action_exec (struct biffrc_environ *env, size_t argc, char **argv)
{
  mu_stream_t pstream;
  struct stat stb;
  int status;

  argc--;
  argv++;
  
  if (argv[0][0] != '/')
    {
      report_error (env, _("not an absolute pathname: %s"), argv[0]);
      return;
    }

  if (stat (argv[0], &stb))
    {
      mu_diag_funcall (MU_DIAG_ERROR, "stat", argv[0], errno);
      return;
    }

  if (stb.st_mode & (S_ISUID|S_ISGID))
    {
      report_error (env, _("will not execute set[ug]id programs"));
      return;
    }

  status = mu_prog_stream_create (&pstream,
				  argv[0], argc, argv,
				  MU_PROG_HINT_ERRTOOUT,
				  NULL,
				  MU_STREAM_READ);
  if (status)
    {
      report_error (env, "mu_prog_stream_create(%s) failed: %s", argv[0],
		    mu_strerror (status));
      return;
    }
  mu_stream_copy (env->tty, pstream, 0, NULL);
  mu_stream_destroy (&pstream);
}

static void
action_default (struct biffrc_environ *env, size_t argc, char **argv)
{
  env->use_default = 1;
}

static void
action_tty (struct biffrc_environ *env, size_t argc, char **argv)
{
  mu_stream_t ntty = open_tty (argv[1], argc - 2, argv + 2);
  if (!ntty)
    report_error (env, _("cannot open tty %s"), argv[1]);

  mu_stream_destroy (&env->tty);
  env->tty = ntty;
}

static mu_stream_t
open_rc (const char *filename, mu_stream_t tty)
{
  struct stat stb;
  struct passwd *pw = getpwnam (username);
  mu_stream_t stream, input;
  int rc;
  static char *linecon_args[] = { "linecon", "-i", "#line", NULL };
  
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
  rc = mu_filter_create_args (&stream, input,
			      "LINECON",
			      MU_ARRAY_SIZE (linecon_args) - 1,
			      (const char **)linecon_args,
			      MU_FILTER_DECODE,
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

struct biffrc_stmt
{
  const char *id;
  int argmin;
  int argmax;
  int expand;
  void (*handler) (struct biffrc_environ *env, size_t argc, char **argv);
};

struct biffrc_stmt biffrc_tab[] = {
  { "beep", 1, 1, 0, action_beep },
  { "tty", 2, -1, 0, action_tty },
  { "echo", 2, -1, 1, action_echo },
  { "exec", 2, -1, 1, action_exec },
  { "default", 1, 1, 0, action_default },
  { NULL }
};

struct biffrc_stmt *
find_stmt (const char *name)
{
  struct biffrc_stmt *p;

  for (p = biffrc_tab; p->id; p++)
    if (strcmp (name, p->id) == 0)
      return p;
  return NULL;
}

void
eval_biffrc (struct biffrc_environ *env)
{
  char *stmt = NULL;
  size_t size = 0;
  size_t n;
  struct mu_wordsplit ws;
  int wsflags;
  
  ws.ws_comment = "#";
  wsflags = MU_WRDSF_DEFFLAGS | MU_WRDSF_COMMENT;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_LOCUS, &env->locus);
  mu_stream_ioctl (env->logstr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_LOCUS, &env->locus);
  while (mu_stream_getline (env->input, &stmt, &size, &n) == 0 && n > 0)
    {
      if (strncmp (stmt, "#line ", 6) == 0)
	{
	  char *p;
	  
	  env->locus.mu_line = strtoul (stmt + 6, &p, 10);
	  if (*p != '\n')
	    {
	      report_error (env, _("malformed #line directive: %s"));
	    }
	  else
	    {
	      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
			       MU_IOCTL_LOGSTREAM_SET_LOCUS_LINE,
			       &env->locus.mu_line);
	      mu_stream_ioctl (env->logstr, MU_IOCTL_LOGSTREAM,
			       MU_IOCTL_LOGSTREAM_SET_LOCUS_LINE,
			       &env->locus.mu_line);
	    }
	  continue;
	}
      
      if (mu_wordsplit (stmt, &ws, wsflags) == 0)
	{
	  struct biffrc_stmt *sp;
	  
	  if (ws.ws_wordc == 0)
	    {
	      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
			       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
	      mu_stream_ioctl (env->logstr, MU_IOCTL_LOGSTREAM,
			       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
	      continue;
	    }

	  sp = find_stmt (ws.ws_wordv[0]);
	  if (!sp)
	    report_error (env, _("unknown keyword"));
	  else if (ws.ws_wordc < sp->argmin)
	    report_error (env, _("too few arguments"));
	  else if (sp->argmax != -1 && ws.ws_wordc > sp->argmax)
	    report_error (env, _("too many arguments"));
	  else
	    {
	      if (sp->expand)
		{
		  size_t i;
		  
		  for (i = 1; i < ws.ws_wordc; i++)
		    {
		      char *oldarg = ws.ws_wordv[i];
		      ws.ws_wordv[i] = expand_line (ws.ws_wordv[i], env->msg);
		      free (oldarg);
		      if (!ws.ws_wordv[i])
			break;
		    }
		}

	      sp->handler (env, ws.ws_wordc, ws.ws_wordv);
	    }
	}
      else
	report_error (env, "%s", mu_wordsplit_strerror (&ws));
      
      wsflags |= MU_WRDSF_REUSE;
      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
      mu_stream_ioctl (env->logstr, MU_IOCTL_LOGSTREAM,
		       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
    }
  free (stmt);
  mu_wordsplit_free (&ws);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
  mu_stream_ioctl (env->logstr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
}

void
run_user_action (const char *device, mu_message_t msg)
{
  int rc, mode;
  mu_stream_t stream;
  struct biffrc_environ env;

  env.tty = open_default_tty (device);
  if (!env.tty)
    return;
  env.msg = msg;

  env.errbuf = NULL;
  env.errsize = 0;
  
  rc = mu_log_stream_create (&env.logstr, env.tty); 
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("cannot create log stream: %s"),
		      mu_strerror (rc));
      mu_stream_destroy (&env.tty);
      return;
    }
  mode = MU_LOGMODE_LOCUS;
  mu_stream_ioctl (env.logstr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_MODE, &mode);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_MODE, &mode);

  env.input = open_rc (biffrc, env.tty);
  if (env.input)
    {
      char *cwd = mu_getcwd ();
      char *rcname;
      rcname = mu_make_file_name (cwd, BIFF_RC);
      free (cwd);
      if (!rcname)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name", NULL, ENOMEM);
	  env.locus.mu_file = BIFF_RC;
	}
      else
	env.locus.mu_file = rcname;
      
      env.locus.mu_line = 1;
      env.locus.mu_col = 0;
      env.use_default = 0;
      eval_biffrc (&env);
      mu_stream_destroy (&env.input);
      free (rcname);
    }
  else
    env.use_default = 1;

  if (env.use_default &&
      mu_static_memory_stream_create (&stream, default_action,
				      strlen (default_action)) == 0)
    {
      int rc = mu_filter_create (&env.input, stream, "LINECON",
				 MU_FILTER_DECODE,
				 MU_STREAM_READ);
      mu_stream_unref (stream);
      if (rc)
	{
	  mu_stream_printf (env.tty,
			    _("Cannot create filter for the default action: %s\n"),
			    mu_strerror (rc));
	  mu_diag_output (MU_DIAG_NOTICE,
			  _("cannot create default filter for %s: %s"),
			  username, mu_strerror (rc));
	}
      else
	{
	  env.locus.mu_file = "<default>";
	  env.locus.mu_line = 1;
	  env.locus.mu_col = 0;
	  eval_biffrc (&env);
	  mu_stream_destroy (&env.input);
	}
    }
  
  mu_stream_destroy (&env.logstr);
  mu_stream_destroy (&env.tty);
  free (env.errbuf);
}
