/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Functions for handling escape variables */

#include "mail.h"
#include <sys/stat.h>

static void
dump_headers (FILE *fp, compose_env_t *env)
{
  char buffer[512];
  stream_t stream = NULL;
  size_t off = 0, n;

  header_get_stream (env->header, &stream);
  while (stream_read (stream, buffer, sizeof buffer - 1, off, &n) == 0
	 && n != 0)
    {
      buffer[n] = 0;
      fprintf (fp, "%s", buffer);
      off += n;
    }
  fprintf (fp, "\n");
}

#define STATE_INIT 0
#define STATE_READ 1
#define STATE_BODY 2

static int
parse_headers (FILE *fp, compose_env_t *env)
{
  int status;
  header_t header;
  char *name = NULL;
  char *value = NULL;
  char *buf = NULL;
  int state = STATE_INIT;
  size_t n = 0;
  int errcnt = 0, line = 0;
  
  if ((status = header_create (&header, NULL, 0, NULL)) != 0)
    {
      util_error ("can't create header: %s", mu_errstring (status));
      return 1;
    }

  while (state != STATE_BODY
	 && errcnt == 0 && getline (&buf, &n, fp) > 0 && n > 0)
    {
      int len = strlen (buf);
      if (len > 0 && buf[len-1] == '\n')
	buf[len-1] = 0;

      line++;
      switch (state)
	{
	case STATE_INIT:
	  if (!buf[0] || isspace (buf[0]))
	    continue;
	  else
	    state = STATE_READ;
	  /*FALLTHRU*/
	  
	case STATE_READ:
	  if (buf[0] == 0)
	    state = STATE_BODY;
	  else if (isspace (buf[0]))
	    {
	      /* A continuation line */
	      if (name)
		{
		  char *p = NULL;
		  asprintf (&p, "%s\n%s", value, buf);
		  free (value);
		  value = p;
		}
	      else
		{
		  util_error ("%d: not a header line", line);
		  errcnt++;
		}
	    }
	  else
	    {
	      char *p;
	      
	      if (name)
		{
		  header_set_value (header, name, value, 0);
		  free (name);
		  free (value);
		  name = value = NULL;
		}
	      p = strchr (buf, ':');
	      if (p)
		{
		  *p++ = 0;
		  while (*p && isspace(*p))
		    p++;
		  value = strdup (p);
		  name = strdup (buf);
		}
	      else
		{
		  util_error ("%d: not a header line", line);
		  errcnt++;
		}
	    }
	  break;
	}
    }
  
  free (buf);
  if (name)
    {
      header_set_value (header, name, value, 0);
      free (name);
      free (value);
    }     

  if (errcnt)
    {
      char *p;
      
      header_destroy (&header, NULL);
      p = ml_readline ("Edit again?");
      if (*p == 'y' || *p == 'Y')
	return -1;
      else
	return 1;
    }

  header_destroy (&env->header, NULL);
  env->header = header;
  return 0;
}

static void
var_continue (void)
{
  fprintf (stdout, "(continue)\n");
}

static int 
var_check_args (int argc, char **argv)
{
  if (argc == 1)
    {
      char *escape = "~";
      util_getenv (&escape, "escape", Mail_env_string, 0);
      util_error ("%c%s requires an argument", escape[0], argv[0]);
      return 1;
    }
  return 0;
}

/* ~![shell-command] */
int
var_shell (int argc, char **argv, compose_env_t *env)
{
  int status;
  ofile = env->ofile;
  status = mail_shell (argc, argv);
  ofile = env->file;
  return status;
}

/* ~:[mail-command] */
/* ~-[mail-command] */
int
var_command (int argc, char **argv, compose_env_t *env)
{
  struct mail_command_entry entry;
  int status;

  if (var_check_args (argc, argv))
    return 1;
  if (argv[1][0] == '#')
    return 0;
  entry = util_find_entry (mail_command_table, argv[1]);
  if (!entry.func)
    {
      util_error ("Unknown command: %s", argv[1]);
      return 1;
    }
  if (entry.flags & (EF_FLOW | EF_SEND))
    {
      util_error ("Command not allowed in an escape sequence\n");
      return 1;
    }

  ofile = env->ofile;
  status = (*entry.func) (argc - 1, argv + 1);
  ofile = env->file;
  return status;
}

/* ~? */
int
var_help (int argc, char **argv, compose_env_t *env)
{
  (void) env;
  if (argc < 2)
    return util_help (mail_escape_table, NULL);
  else
    {
      int status = 0;

      while (--argc)
	status |= util_help (mail_escape_table, *++argv);

      return status;
    }
  return 1;
}

/* ~A */
/* ~a */
int
var_sign (int argc, char **argv, compose_env_t *env)
{
  char *p;

  (void) argc;
  (void) env;

  if (util_getenv (&p, isupper (argv[0][0]) ? "Sign" : "sign",
		   Mail_env_string, 1) == 0)
    {
      if (isupper (argv[0][0]))
	{
	  char *name = util_fullpath (p);
	  FILE *fp = fopen (name, "r");
	  char *buf = NULL;
	  size_t n = 0;

	  if (!fp)
	    {
	      util_error ("can't open %s: %s", name, strerror (errno));
	      free (name);
	    }

	  fprintf (stdout, "Reading %s\n", name);
	  while (getline (&buf, &n, fp) > 0)
	    fprintf (ofile, "%s", buf);

	  fclose (fp);
	  free (buf);
	  free (name);
	}
      else
	fprintf (ofile, "%s", p);
      var_continue ();
    }
  return 0;
}

/* ~b[bcc-list] */
int
var_bcc (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_BCC, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

/* ~c[cc-list] */
int
var_cc (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_CC, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

/* ~d */
int
var_deadletter (int argc, char **argv, compose_env_t *env)
{
  FILE *dead = fopen (getenv ("DEAD"), "r");
  int c;

  (void) argc;
  (void) argv;
  (void) env;

  while ((c = fgetc (dead)) != EOF)
    fputc (c, ofile);
  fclose (dead);
  return 0;
}

static int
var_run_editor (char *ed, int argc, char **argv, compose_env_t *env)
{
  if (!util_getenv (NULL, "editheaders", Mail_env_boolean, 0))
    {
      char *filename;
      int fd = mu_tempfile (NULL, &filename);
      FILE *fp = fdopen (fd, "w+");
      char buffer[512];
      int rc;
      
      dump_headers (fp, env);

      rewind (env->file);
      while (fgets (buffer, sizeof (buffer), env->file))
	fputs (buffer, fp);

      fclose (env->file);
      
      do
	{
	  fclose (fp);
	  util_do_command ("!%s %s", ed, filename);
	  fp = fopen (filename, "r");
	}
      while ((rc = parse_headers (fp, env)) < 0);

      if (rc == 0)
	{
	  env->file = fopen (env->filename, "w");
	  while (fgets (buffer, sizeof (buffer), fp))
	    fputs (buffer, env->file);

	  fclose (env->file);
	}
      fclose (fp);
      unlink (filename);
      free (filename);
    }
  else
    {
      fclose (env->file);
      ofile = env->ofile;
      util_do_command ("!%s %s", ed, env->filename);
    }

  env->file = fopen (env->filename, "a+");
  ofile = env->file;
      
  var_continue ();
  return 0;
}

/* ~e */
int
var_editor (int argc, char **argv, compose_env_t *env)
{
  return var_run_editor (getenv ("EDITOR"), argc, argv, env);
}

/* ~v */
int
var_visual (int argc, char **argv, compose_env_t *env)
{
  return var_run_editor (getenv ("VISUAL"), argc, argv, env);
}

/* ~f[mesg-list] */
/* ~F[mesg-list] */
int
var_print (int argc, char **argv, compose_env_t *env)
{
  (void) env;
  return mail_print (argc, argv);
}

void
reread_header (compose_env_t *env, char *hdr, char *prompt)
{
  char *p;
  p = strdup (compose_header_get (env, hdr, ""));
  ml_reread (prompt, &p);
  compose_header_set (env, hdr, p, COMPOSE_REPLACE);
  free (p);
}

/* ~h */
int
var_headers (int argc, char **argv, compose_env_t *env)
{
  reread_header (env, MU_HEADER_TO, "To: ");
  reread_header (env, MU_HEADER_CC, "Cc: ");
  reread_header (env, MU_HEADER_BCC, "Bcc: ");
  reread_header (env, MU_HEADER_SUBJECT, "Subject: ");
  var_continue ();
  return 0;
}

/* ~i[var-name] */
int
var_insert (int argc, char **argv, compose_env_t *send_env)
{
  struct mail_env_entry *env;

  (void) send_env;
  if (var_check_args (argc, argv))
    return 1;
  env = util_find_env (argv[1], 0);
  if (env)
    switch (env->type)
      {
      case Mail_env_string:
	fprintf (ofile, "%s", env->value.string);
	break;

      case Mail_env_number:
	fprintf (ofile, "%d", env->value.number);
	break;

      case Mail_env_boolean:
	fprintf (ofile, "%s", env->set ? "yes" : "no");
	break;

      default:
	break;
      }
  return 0;
}

/* ~m[mesg-list] */
/* ~M[mesg-list] */
int
var_quote (int argc, char **argv, compose_env_t *env)
{
  if (argc > 1)
    return util_msglist_esccmd (var_quote, argc, argv, env, 0);
  else
    {
      message_t mesg;
      header_t hdr;
      body_t body;
      stream_t stream;
      char buffer[512];
      off_t off = 0;
      size_t n = 0;
      char *prefix = "\t";

      if (mailbox_get_message (mbox, cursor, &mesg) != 0)
	return 1;

      fprintf (stdout, "Interpolating: %d\n", cursor);

      util_getenv (&prefix, "indentprefix", Mail_env_string, 0);

      if (islower (argv[0][0]))
	{
	  size_t i, num = 0;
	  char buf[512];

	  message_get_header (mesg, &hdr);
	  header_get_field_count (hdr, &num);

	  for (i = 1; i <= num; i++)
	    {
	      header_get_field_name (hdr, i, buf, sizeof buf, NULL);
	      if (mail_header_is_visible (buf))
		{
		  fprintf (ofile, "%s%s: ", prefix, buf);
		  header_get_field_value (hdr, i, buf, sizeof buf, NULL);
		  fprintf (ofile, "%s\n", buf);
		}
	    }
	  fprintf (ofile, "\n");
	  message_get_body (mesg, &body);
	  body_get_stream (body, &stream);
	}
      else
	message_get_stream (mesg, &stream);

      while (stream_readline (stream, buffer, sizeof buffer - 1, off, &n) == 0
	     && n != 0)
	{
	  buffer[n] = '\0';
	  fprintf (ofile, "%s%s", prefix, buffer);
	  off += n;
	}
      var_continue ();
    }
  return 0;
}

/* ~p */
int
var_type_input (int argc, char **argv, compose_env_t *env)
{
  char buffer[512];

  fprintf (env->ofile, "Message contains:\n");

  dump_headers (env->ofile, env);

  rewind (env->file);
  while (fgets (buffer, sizeof (buffer), env->file))
    fputs (buffer, env->ofile);

  var_continue ();

  return 0;
}

/* ~r[filename] */
int
var_read (int argc, char **argv, compose_env_t *env)
{
  char *filename;
  FILE *inf;
  size_t size, lines;
  char buf[512];

  (void) env;

  if (var_check_args (argc, argv))
    return 1;
  filename = util_fullpath (argv[1]);
  inf = fopen (filename, "r");
  if (!inf)
    {
      util_error ("can't open %s: %s\n", filename, strerror (errno));
      free (filename);
      return 1;
    }

  size = lines = 0;
  while (fgets (buf, sizeof (buf), inf))
    {
      lines++;
      size += strlen (buf);
      fputs (buf, ofile);
    }
  fclose (inf);
  fprintf (stdout, "\"%s\" %d/%d\n", filename, lines, size);
  free (filename);
  return 0;
}

/* ~s[string] */
int
var_subj (int argc, char **argv, compose_env_t *env)
{
  if (var_check_args (argc, argv))
    return 1;
  compose_header_set (env, MU_HEADER_SUBJECT, argv[1], COMPOSE_REPLACE);
  return 0;
}

/* ~t[name-list] */
int
var_to (int argc, char **argv, compose_env_t *env)
{
  while (--argc)
    compose_header_set (env, MU_HEADER_TO, *++argv, COMPOSE_SINGLE_LINE);
  return 0;
}

/* ~w[filename] */
int
var_write (int argc, char **argv, compose_env_t *env)
{
  char *filename;
  FILE *fp;
  size_t size, lines;
  char buf[512];

  if (var_check_args (argc, argv))
    return 1;

  filename = util_fullpath (argv[1]);
  fp = fopen (filename, "w");	/*FIXME: check for the existence first */

  if (!fp)
    {
      util_error ("can't open %s: %s\n", filename, strerror (errno));
      free (filename);
      return 1;
    }

  rewind (env->file);
  size = lines = 0;
  while (fgets (buf, sizeof (buf), env->file))
    {
      lines++;
      size += strlen (buf);
      fputs (buf, fp);
    }
  fclose (fp);
  fprintf (stdout, "\"%s\" %d/%d\n", filename, lines, size);
  free (filename);
  return 0;
}

/* ~|[shell-command] */
int
var_pipe (int argc, char **argv, compose_env_t *env)
{
  int p[2];
  pid_t pid;
  int fd;

  if (argc == 1)
    {
      util_error ("pipe: no command specified");
      return 1;
    }

  if (pipe (p))
    {
      util_error ("pipe: %s", strerror (errno));
      return 1;
    }

  fd = mu_tempfile (NULL, NULL);
  if (fd == -1)
    return 1;

  if ((pid = fork ()) < 0)
    {
      close (p[0]);
      close (p[1]);
      close (fd);
      util_error ("fork: %s", strerror (errno));
      return 1;
    }
  else if (pid == 0)
    {
      /* Child */
      int i;
      char **xargv;

      /* Attache the pipes */
      close (0);
      dup (p[0]);
      close (p[0]);
      close (p[1]);

      close (1);
      dup (fd);
      close (fd);

      /* Execute the process */
      xargv = xcalloc (argc, sizeof (xargv[0]));
      for (i = 0; i < argc - 1; i++)
	xargv[i] = argv[i + 1];
      xargv[i] = NULL;
      execvp (xargv[0], xargv);
      util_error ("cannot exec process `%s': %s", xargv[0], strerror (errno));
      exit (1);
    }
  else
    {
      FILE *fp;
      char *buf = NULL;
      size_t n;
      size_t lines, size;
      int rc = 1;
      int status;

      close (p[0]);

      /* Parent */
      fp = fdopen (p[1], "w");

      fclose (env->file);
      env->file = fopen (env->filename, "r");

      lines = size = 0;
      while (getline (&buf, &n, env->file) > 0)
	{
	  lines++;
	  size += n;
	  fputs (buf, fp);
	}
      fclose (env->file);
      fclose (fp);		/* Closes p[1] */

      waitpid (pid, &status, 0);
      if (!WIFEXITED (status))
	{
	  util_error ("child terminated abnormally: %d", WEXITSTATUS (status));
	}
      else
	{
	  struct stat st;
	  if (fstat (fd, &st))
	    {
	      util_error ("can't stat output file: %s", strerror (errno));
	    }
	  else if (st.st_size > 0)
	    rc = 0;
	}

      fprintf (stdout, "\"|%s\" in: %d/%d ", argv[1], lines, size);
      if (rc)
	{
	  fprintf (stdout, "no lines out\n");
	}
      else
	{
	  /* Ok, replace the old tempfile */
	  fp = fdopen (fd, "r");
	  rewind (fp);

	  env->file = fopen (env->filename, "w+");

	  lines = size = 0;
	  while (getline (&buf, &n, fp) > 0)
	    {
	      lines++;
	      size += n;
	      fputs (buf, env->file);
	    }
	  fclose (env->file);

	  fprintf (stdout, "out: %d/%d\n", lines, size);
	}

      /* Clean up the things */
      if (buf)
	free (buf);

      env->file = fopen (env->filename, "a+");
      ofile = env->file;
    }

  close (fd);

  return 0;
}
