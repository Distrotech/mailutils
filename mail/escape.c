/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2005-2007, 2009-2012 Free Software
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

/* Functions for handling escape variables */

#include "mail.h"
#include <sys/stat.h>

static void
dump_headers (mu_stream_t out, compose_env_t *env)
{
  mu_stream_t stream = NULL;
  int rc;
  
  rc = mu_header_get_streamref (env->header, &stream);
  if (rc)
    {
      mu_error ("mu_header_get_streamref: %s",
		  mu_stream_strerror (stream, rc));
      return;
    }
  mu_stream_copy (out, stream, 0, NULL);
  mu_stream_destroy (&stream);
}

#define STATE_INIT 0
#define STATE_READ 1
#define STATE_BODY 2

static int
parse_headers (mu_stream_t input, compose_env_t *env)
{
  int status;
  mu_header_t header;
  char *name = NULL;
  char *value = NULL;
  int state = STATE_INIT;
  char *buf = NULL;
  size_t size = 0, n;
  int errcnt = 0, line = 0;
  
  if ((status = mu_header_create (&header, NULL, 0)) != 0)
    {
      mu_error (_("Cannot create header: %s"), mu_strerror (status));
      return 1;
    }

  mu_stream_seek (input, 0, MU_SEEK_SET, NULL);
  while (state != STATE_BODY &&
	 errcnt == 0 &&
	 mu_stream_getline (input, &buf, &size, &n) == 0 && n > 0)
    {
      mu_rtrim_class (buf, MU_CTYPE_SPACE);

      line++;
      switch (state)
	{
	case STATE_INIT:
	  if (!buf[0] || mu_isspace (buf[0]))
	    continue;
	  else
	    state = STATE_READ;
	  /*FALLTHRU*/
	  
	case STATE_READ:
	  if (buf[0] == 0)
	    state = STATE_BODY;
	  else if (mu_isspace (buf[0]))
	    {
	      /* A continuation line */
	      if (name)
		{
		  char *p = NULL;
		  mu_asprintf (&p, "%s\n%s", value, buf);
		  free (value);
		  value = p;
		}
	      else
		{
		  mu_error (_("%d: not a header line"), line);
		  errcnt++;
		}
	    }
	  else
	    {
	      char *p;
	      
	      if (name)
		{
		  mu_header_set_value (header, name, value[0] ? value : NULL, 0);
		  free (name);
		  free (value);
		  name = value = NULL;
		}
	      p = strchr (buf, ':');
	      if (p)
		{
		  *p++ = 0;
		  while (*p && mu_isspace (*p))
		    p++;
		  value = mu_strdup (p);
		  name = mu_strdup (buf);
		}
	      else
		{
		  mu_error (_("%d: not a header line"), line);
		  errcnt++;
		}
	    }
	  break;
	}
    }
  
  free (buf);
  if (name)
    {
      mu_header_set_value (header, name, value, 0);
      free (name);
      free (value);
    }     

  if (errcnt)
    {
      char *p;
      
      mu_header_destroy (&header);
      p = ml_readline (_("Edit again?"));
      if (mu_true_answer_p (p) == 1)
	return -1;
      else
	return 1;
    }

  mu_header_destroy (&env->header);
  env->header = header;
  return 0;
}

static void
escape_continue (void)
{
  mu_printf (_("(continue)\n"));
}

int 
escape_check_args (int argc, char **argv, int minargs, int maxargs)
{
  char *escape = "~";
  if (argc < minargs)
    {
      minargs--;
      mailvar_get (&escape, "escape", mailvar_type_string, 0);
      mu_error (ngettext ("%c%s requires at least %d argument",
			  "%c%s requires at least %d arguments",
			  minargs), escape[0], argv[0], minargs);
      return 1;
    }
  if (maxargs > 1 && argc > maxargs)
    {
      maxargs--;
      mailvar_get (&escape, "escape", mailvar_type_string, 0);
      mu_error (ngettext ("%c%s accepts at most %d argument",
			  "%c%s accepts at most %d arguments",
			  maxargs), escape[0], argv[0], maxargs);
      return 1;
    }
    
  return 0;
}

/* ~![shell-command] */
int
escape_shell (int argc, char **argv, compose_env_t *env)
{
  return mail_execute (1, argv[1], argc - 1, argv + 1);
}

/* ~:[mail-command] */
/* ~-[mail-command] */
int
escape_command (int argc, char **argv, compose_env_t *env)
{
  const struct mail_command_entry *entry;
  int status;
  
  if (escape_check_args (argc, argv, 2, 2))
    return 1;
  if (argv[1][0] == '#')
    return 0;
  entry = mail_find_command (argv[1]);
  if (!entry)
    {
      mu_error (_("Unknown command: %s"), argv[1]);
      return 1;
    }
  if (entry->flags & (EF_FLOW | EF_SEND))
    {
      mu_error (_("Command not allowed in an escape sequence\n"));
      return 1;
    }

  status = (*entry->func) (argc - 1, argv + 1);
  return status;
}

/* ~? */
int
escape_help (int argc, char **argv, compose_env_t *env MU_ARG_UNUSED)
{
  int status;
  if (argc < 2)
    status = mail_escape_help (NULL);
  else
    while (--argc)
      status |= mail_escape_help (*++argv);
  escape_continue ();
  return status;
}

/* ~A */
/* ~a */
int
escape_sign (int argc MU_ARG_UNUSED, char **argv, compose_env_t *env)
{
  char *p;

  if (mailvar_get (&p, mu_isupper (argv[0][0]) ? "Sign" : "sign",
		   mailvar_type_string, 1) == 0)
    {
      mu_stream_printf (env->compstr, "-- \n");
      if (mu_isupper (argv[0][0]))
	{
	  char *name = util_fullpath (p);
	  int rc;
	  mu_stream_t signstr;

	  rc = mu_file_stream_create (&signstr, name, MU_STREAM_READ);
	  if (rc)
	    mu_error (_("Cannot open %s: %s"), name, mu_strerror (rc));
	  else
	    {
	      mu_printf (_("Reading %s\n"), name);
	      mu_stream_copy (env->compstr, signstr, 0, NULL);
	      mu_stream_destroy (&signstr);
	    }
	}
      else
	mu_stream_printf (env->compstr, "%s\n", p);
      escape_continue ();
    }
  return 0;
}

/* ~b[bcc-list] */
int
escape_bcc (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_BCC, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

/* ~c[cc-list] */
int
escape_cc (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_CC, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

static int
verbose_copy (mu_stream_t dest, mu_stream_t src, const char *filename,
	      mu_off_t *psize)
{
  int rc;
  char *buf = NULL;
  size_t size = 0, n;
  size_t lines;
  mu_off_t total;
  
  total = lines = 0;

  while ((rc = mu_stream_getline (src, &buf, &size, &n)) == 0 && n > 0)
    {
      lines++;
      rc = mu_stream_write (dest, buf, n, NULL);
      if (rc)
	break;
      total += n;
    }
  if (rc)
    mu_error (_("error copying data: %s"), mu_strerror (rc));
  mu_printf ("\"%s\" %lu/%lu\n", filename,
		    (unsigned long) lines, (unsigned long) total);
  if (psize)
    *psize = total;
  return rc;
}

/* ~d */
int
escape_deadletter (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED,
		   compose_env_t *env)
{
  const char *name = getenv ("DEAD");
  mu_stream_t str;
  int rc = mu_file_stream_create (&str, name, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("Cannot open file %s: %s"), name, strerror (rc));
      return 1;
    }
  verbose_copy (env->compstr, str, name, NULL);
  mu_stream_destroy (&str);
  return 0;
}

static int
run_editor (char *ed, char *arg)
{
  char *argv[3];

  argv[0] = ed;
  argv[1] = arg;
  argv[2] = NULL;
  return mail_execute (1, ed, 2, argv);
}

static int
escape_run_editor (char *ed, int argc, char **argv, compose_env_t *env)
{
  char *filename;
  int fd;
  mu_stream_t tempstream;
  int rc;
  
  rc = mu_tempfile (NULL, 0, &fd, &filename);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_tempfile", NULL, rc);
      return rc;
    }
  rc = mu_fd_stream_create (&tempstream, filename, fd, MU_STREAM_RDWR);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_fd_stream_create", filename, rc);
      unlink (filename);
      free (filename);
      close (fd);
      return rc;
    }

  mu_stream_seek (env->compstr, 0, MU_SEEK_SET, NULL);
  if (!mailvar_get (NULL, "editheaders", mailvar_type_boolean, 0))
    {
      dump_headers (tempstream, env);

      mu_stream_copy (tempstream, env->compstr, 0, NULL);
      
      do
	{
	  mu_stream_destroy (&tempstream);
	  run_editor (ed, filename);
	  rc = mu_file_stream_create (&tempstream, filename, MU_STREAM_RDWR);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create",
			       filename, rc);
	      unlink (filename);
	      free (filename);
	      return rc;
	    }
	}
      while ((rc = parse_headers (tempstream, env)) < 0);
    }
  else
    {
      mu_stream_copy (tempstream, env->compstr, 0, NULL);
      mu_stream_destroy (&tempstream);
      run_editor (ed, filename);
      rc = mu_file_stream_create (&tempstream, filename, MU_STREAM_RDWR);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create",
			   filename, rc);
	  unlink (filename);
	  free (filename);
	  return rc;
	}
    }

  if (rc == 0)
    {
      mu_off_t size;
      
      mu_stream_seek (env->compstr, 0, MU_SEEK_SET, NULL);
      mu_stream_copy (env->compstr, tempstream, 0, &size);
      mu_stream_truncate (env->compstr, size);
    }
  mu_stream_destroy (&tempstream);
  unlink (filename);
  free (filename);

  mu_stream_seek (env->compstr, 0, MU_SEEK_END, NULL);
  
  escape_continue ();
  return 0;
}

/* ~e */
int
escape_editor (int argc, char **argv, compose_env_t *env)
{
  return escape_run_editor (getenv ("EDITOR"), argc, argv, env);
}

/* ~l -- escape_list_attachments (send.c) */

/* ~v */
int
escape_visual (int argc, char **argv, compose_env_t *env)
{
  return escape_run_editor (getenv ("VISUAL"), argc, argv, env);
}

/* ~f[mesg-list] */
/* ~F[mesg-list] */
int
escape_print (int argc, char **argv, compose_env_t *env MU_ARG_UNUSED)
{
  return mail_print (argc, argv);
}

void
reread_header (compose_env_t *env, char *hdr, char *prompt)
{
  char *p;
  p = mu_strdup (compose_header_get (env, hdr, ""));
  ml_reread (prompt, &p);
  compose_header_set (env, hdr, p, COMPOSE_REPLACE);
  free (p);
}

/* ~h */
int
escape_headers (int argc, char **argv, compose_env_t *env)
{
  reread_header (env, MU_HEADER_TO, "To: ");
  reread_header (env, MU_HEADER_CC, "Cc: ");
  reread_header (env, MU_HEADER_BCC, "Bcc: ");
  reread_header (env, MU_HEADER_SUBJECT, "Subject: ");
  escape_continue ();
  return 0;
}

/* ~i[var-name] */
int
escape_insert (int argc, char **argv, compose_env_t *env)
{
  if (escape_check_args (argc, argv, 2, 2))
    return 1;
  mailvar_variable_format (env->compstr, mailvar_find_variable (argv[1], 0),
			   NULL);
  return 0;
}

/* ~m[mesg-list] */
/* ~M[mesg-list] */

struct quote_closure
{
  mu_stream_t str;
  int islower;
};

int
quote0 (msgset_t *mspec, mu_message_t mesg, void *data)
{
  struct quote_closure *clos = data;
  int rc;
  mu_stream_t stream;
  char *prefix = "\t";
  mu_stream_t flt;
  char *argv[3];

  mu_printf (_("Interpolating: %lu\n"),
		    (unsigned long) mspec->msg_part[0]);

  mailvar_get (&prefix, "indentprefix", mailvar_type_string, 0);

  argv[0] = "INLINE-COMMENT";
  argv[1] = prefix;
  argv[2] = NULL;
  rc = mu_filter_create_args (&flt, clos->str, "INLINE-COMMENT",
			      2, (const char**) argv,
			      MU_FILTER_ENCODE,
			      MU_STREAM_WRITE);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_filter_create_args", NULL, rc);
      return rc;
    }
  
  if (clos->islower)
    {
      mu_header_t hdr;
      mu_body_t body;
      mu_iterator_t itr;
	
      mu_message_get_header (mesg, &hdr);
      mu_header_get_iterator (hdr, &itr);
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *name, *value;

	  if (mu_iterator_current_kv (itr, (const void **)&name,
				      (void**)&value) == 0 &&
	      mail_header_is_visible (name))
	    mu_stream_printf (flt, "%s: %s\n", name, value);
	}
      mu_iterator_destroy (&itr);
      mu_stream_write (flt, "\n", 1, NULL);
      mu_message_get_body (mesg, &body);
      rc = mu_body_get_streamref (body, &stream);
    }
  else
    rc = mu_message_get_streamref (mesg, &stream);

  if (rc)
    {
      mu_error (_("get_streamref error: %s"), mu_strerror (rc));
      return rc;
    }

  mu_stream_copy (flt, stream, 0, NULL);

  mu_stream_destroy (&stream);
  mu_stream_destroy (&flt);
  
  return 0;
}

int
escape_quote (int argc, char **argv, compose_env_t *env)
{
  struct quote_closure clos;
  
  clos.str = env->compstr;
  clos.islower = mu_islower (argv[0][0]);
  util_foreach_msg (argc, argv, MSG_NODELETED|MSG_SILENT, quote0, &clos);
  escape_continue ();
  return 0;
}

/* ~p */
int
escape_type_input (int argc, char **argv, compose_env_t *env)
{
  /* FIXME: Enable paging */
  mu_printf (_("Message contains:\n"));
  dump_headers (mu_strout, env);
  mu_stream_seek (env->compstr, 0, MU_SEEK_SET, NULL);
  mu_stream_copy (mu_strout, env->compstr, 0, NULL);
  escape_continue ();
  return 0;
}

/* ~r[filename] */
int
escape_read (int argc, char **argv, compose_env_t *env MU_ARG_UNUSED)
{
  char *filename;
  mu_stream_t instr;
  int rc;
  
  if (escape_check_args (argc, argv, 2, 2))
    return 1;
  filename = util_fullpath (argv[1]);

  rc = mu_file_stream_create (&instr, filename, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("Cannot open %s: %s"), filename, mu_strerror (rc));
      free (filename);
      return 1;
    }

  verbose_copy (env->compstr, instr, filename, NULL);
  mu_stream_destroy (&instr);
  free (filename);
  return 0;
}

/* ~s[string] */
int
escape_subj (int argc, char **argv, compose_env_t *env)
{
  char *buf;
  if (escape_check_args (argc, argv, 2, 2))
    return 1;
  mu_argcv_string (argc - 1, argv + 1, &buf);
  compose_header_set (env, MU_HEADER_SUBJECT, buf, COMPOSE_REPLACE);
  free (buf);
  return 0;
}

/* ~t[name-list] */
int
escape_to (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_TO, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

/* ~w[filename] */
int
escape_write (int argc, char **argv, compose_env_t *env)
{
  char *filename;
  mu_stream_t wstr;
  int rc;
  mu_off_t size;
  
  if (escape_check_args (argc, argv, 2, 2))
    return 1;
  filename = util_fullpath (argv[1]);
  /* FIXME: check for existence first */
  rc = mu_file_stream_create (&wstr, filename,
			      MU_STREAM_WRITE|MU_STREAM_CREAT);
  if (rc)
    {
      mu_error (_("Cannot open %s for writing: %s"),
		  filename, mu_strerror (rc));
      free (filename);
      return 1;
    }

  mu_stream_seek (env->compstr, 0, MU_SEEK_SET, NULL);
  verbose_copy (wstr, env->compstr, filename, &size);
  mu_stream_truncate (wstr, size);
  mu_stream_destroy (&wstr);
  free (filename);
  return 0;
}

/* ~|[shell-command] */
int
escape_pipe (int argc, char **argv, compose_env_t *env)
{
  int rc, status;
  int p[2];
  pid_t pid;
  int fd;
  mu_off_t isize, osize = 0;
  mu_stream_t tstr;
  
  if (pipe (p))
    {
      mu_error ("pipe: %s", mu_strerror (errno));
      return 1;
    }

  if (mu_tempfile (NULL, 0, &fd, NULL))
    return 1;

  if ((pid = fork ()) < 0)
    {
      close (p[0]);
      close (p[1]);
      close (fd);
      mu_error ("fork: %s", mu_strerror (errno));
      return 1;
    }
  else if (pid == 0)
    {
      /* Child */

      /* Attach the pipes */
      close (0);
      dup (p[0]);
      close (p[0]);
      close (p[1]);

      close (1);
      dup (fd);
      close (fd);

      execvp (argv[1], argv + 1);
      mu_error (_("Cannot execute `%s': %s"), argv[1], mu_strerror (errno));
      _exit (127);
    }

  close (p[0]);

  rc = mu_stdio_stream_create (&tstr, p[1], MU_STREAM_WRITE);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create", NULL, rc);
      kill (pid, SIGKILL);
      close (fd);
      return rc;
    }

  mu_stream_seek (env->compstr, 0, MU_SEEK_SET, NULL);
  mu_stream_copy (tstr, env->compstr, 0, &isize);
  mu_stream_destroy (&tstr);

  waitpid (pid, &status, 0);
  if (!WIFEXITED (status))
    mu_error (_("Child terminated abnormally: %d"),
		WEXITSTATUS (status));
  else
    {
      struct stat st;
      if (fstat (fd, &st))
	mu_error (_("Cannot stat output file: %s"), mu_strerror (errno));
      else
	osize = st.st_size;
    }

  mu_stream_printf (mu_strout, "\"|%s\" in: %lu ", argv[1],
		    (unsigned long) isize);
  if (osize == 0)
    mu_stream_printf (mu_strout, _("no lines out\n"));
  else
    {
      mu_stream_t str;
      
      mu_stream_printf (mu_strout, "out: %lu\n", (unsigned long) osize);
      rc = mu_fd_stream_create (&str, NULL, fd,
				MU_STREAM_RDWR | MU_STREAM_SEEK);
      if (rc)
	{
	  mu_error (_("Cannot open composition stream: %s"),
		      mu_strerror (rc));
	  close (fd);
	}
      else
	{
	  mu_stream_destroy (&env->compstr);
	  env->compstr = str;
	}
    }
  return 0;
}
