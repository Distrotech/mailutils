/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "mail.h"

static int isfilename __P ((const char *));
static void msg_to_pipe __P ((const char *cmd, message_t msg));

/*
 * m[ail] address...
 if address is starting with
 
    '/'        it is considered a file and the message is saveed to a file;
    '.'        it is considered to be a relative path;
    '|'        it is considered to be a pipe, and the message is written to
               there;
	       
 example:
 
   mail joe '| cat >/tmp/save'

 mail will be sent to joe and the message saved in /tmp/save. */

int
mail_send (int argc, char **argv)
{
  compose_env_t env;
  int status;
  int save_to = isupper (argv[0][0]);
  compose_init (&env);

  if (argc < 2)
    compose_header_set (&env, MU_HEADER_TO, ml_readline ("To: "),
			COMPOSE_REPLACE);
  else
    {
      while (--argc)
	{
	  char *p = *++argv;
	  if (isfilename (p))
	    {
	      env.outfiles = realloc (env.outfiles,
				      (env.nfiles + 1) *
				      sizeof (*(env.outfiles)));
	      if (env.outfiles)
		{
		  env.outfiles[env.nfiles] = p;
		  env.nfiles++;
		}
	    }
	  else
	    {
	      compose_header_set (&env, MU_HEADER_TO, p, COMPOSE_SINGLE_LINE);
	      free (p);
	    }
	}
    }

  if (util_getenv (NULL, "askcc", Mail_env_boolean, 0) == 0)
    compose_header_set (&env, MU_HEADER_CC,
			ml_readline ("Cc: "), COMPOSE_REPLACE);
  if (util_getenv (NULL, "askbcc", Mail_env_boolean, 0) == 0)
    compose_header_set (&env, MU_HEADER_BCC,
			ml_readline ("Bcc: "), COMPOSE_REPLACE);

  if (util_getenv (NULL, "asksub", Mail_env_boolean, 0) == 0)
    compose_header_set (&env, MU_HEADER_SUBJECT,
			ml_readline ("Subject: "), COMPOSE_REPLACE);
  else
    {
      char *p;
      if (util_getenv (&p, "subject", Mail_env_string, 0) == 0)
	compose_header_set (&env, MU_HEADER_SUBJECT, p, COMPOSE_REPLACE);
    }

  status = mail_send0 (&env, save_to);
  compose_destroy (&env);
  return status;
}

void
compose_init (compose_env_t * env)
{
  memset (env, 0, sizeof (*env));
}

int
compose_header_set (compose_env_t * env, char *name, char *value, int mode)
{
  int status;
  char *old_value;

  if (!env->header
      && (status = header_create (&env->header, NULL, 0, NULL)) != 0)
    {
      util_error (_("can't create header: %s"), mu_errstring (status));
      return status;
    }

  switch (mode)
    {
    case COMPOSE_REPLACE:
    case COMPOSE_APPEND:
      status = header_set_value (env->header, name, value, mode);
      break;

    case COMPOSE_SINGLE_LINE:
      if (!value || value[0] == 0)
	return EINVAL;

      if (header_aget_value (env->header, name, &old_value) == 0
	  && old_value[0])
	{
	  status = util_merge_addresses (&old_value, value);
	  if (status == 0)
	    status = header_set_value (env->header, name, old_value, 1);
	  free (old_value);
	}
      else
	status = header_set_value (env->header, name, value, 1);
    }

  return status;
}

char *
compose_header_get (compose_env_t * env, char *name, char *defval)
{
  char *p;

  if (header_aget_value (env->header, name, &p))
    p = defval;
  return p;
}

void
compose_destroy (compose_env_t * env)
{
  header_destroy (&env->header, NULL);
  if (env->outfiles)
    {
      int i;
      for (i = 0; i < env->nfiles; i++)
	free (env->outfiles[i]);
      free (env->outfiles);
    }
}

/* mail_send0(): shared between mail_send() and mail_reply();

   If the variable "record" is set, the outgoing message is
   saved after being sent. If "save_to" argument is non-zero,
   the name of the save file is derived from "to" argument. Otherwise,
   it is taken from the value of "record" variable.

   sendmail

   contains a command, possibly with options, that mailx invokes to send
   mail. You must manually set the default for this environment variable
   by editing ROOTDIR/etc/mailx.rc to specify the mail agent of your
   choice. The default is sendmail, but it can be any command that takes
   addresses on the command line and message contents on standard input. */

int
mail_send0 (compose_env_t * env, int save_to)
{
  int done = 0;
  int fd;
  char *filename;
  FILE *file;
  char *savefile = NULL;
  int int_cnt;
  char *escape;

  fd = mu_tempfile (NULL, &filename);

  if (fd == -1)
    {
      util_error (_("Can not open temporary file"));
      return 1;
    }

  /* Prepare environment */
  env = env;
  env->filename = filename;
  env->file = fdopen (fd, "w+");
  env->ofile = ofile;

  ml_clear_interrupt ();
  int_cnt = 0;
  while (!done)
    {
      char *buf;
      buf = ml_readline (" \b");

      if (ml_got_interrupt ())
	{
	  if (util_getenv (NULL, "ignore", Mail_env_boolean, 0) == 0)
	    {
	      fprintf (stdout, "@\n");
	    }
	  else
	    {
	      if (buf)
		free (buf);
	      if (++int_cnt == 2)
		break;
	      util_error (_("(Interrupt -- one more to kill letter)"));
	    }
	  continue;
	}

      if (!buf)
	{
	  if (util_getenv (NULL, "ignore", Mail_env_boolean, 0) == 0)
	    {
	      util_error (util_getenv (NULL, "dot", Mail_env_boolean, 0) == 0 ?
			  _("Use \".\" to terminate letter.") :
			  _("Use \"~.\" to terminate letter."));
	      continue;
	    }
	  else
	    break;
	}

      int_cnt = 0;

      if (strcmp (buf, ".") == 0
	  && util_getenv (NULL, "dot", Mail_env_boolean, 0) == 0)
	done = 1;
      else if (util_getenv (&escape, "escape", Mail_env_string, 0) == 0
	       && buf[0] == escape[0])
	{
	  if (buf[1] == buf[0])
	    fprintf (env->file, "%s\n", buf + 1);
	  else if (buf[1] == '.')
	    done = 1;
	  else if (buf[1] == 'x')
	    {
	      int_cnt = 2;
	      done = 1;
	    }
	  else
	    {
	      int argc;
	      char **argv;
	      int status;

	      ofile = env->file;

	      if (argcv_get (buf + 1, "", NULL, &argc, &argv) == 0)
		{
		  struct mail_command_entry entry;
		  entry = util_find_entry (mail_escape_table, argv[0]);

		  if (entry.escfunc)
		    status = (*entry.escfunc) (argc, argv, env);
		  else
		    util_error (_("Unknown escape %s"), argv[0]);
		}
	      else
		{
		  util_error (_("can't parse escape sequence"));
		}
	      argcv_free (argc, argv);

	      ofile = env->ofile;
	    }
	}
      else
	fprintf (env->file, "%s\n", buf);
      fflush (env->file);
      free (buf);
    }

  /* If interrupted dump the file to dead.letter.  */
  if (int_cnt)
    {
      if (util_getenv (NULL, "save", Mail_env_boolean, 0) == 0)
	{
	  FILE *fp = fopen (getenv ("DEAD"),
			    util_getenv (NULL, "appenddeadletter",
					 Mail_env_boolean, 0) == 0 ?
			    "a" : "w");

	  if (!fp)
	    {
	      util_error (_("can't open file %s: %s"), getenv ("DEAD"),
			  strerror (errno));
	    }
	  else
	    {
	      char *buf = NULL;
	      size_t n;
	      rewind (env->file);
	      while (getline (&buf, &n, env->file) > 0)
		fputs (buf, fp);
	      fclose (fp);
	      free (buf);
	    }
	}

      fclose (env->file);
      remove (filename);
      free (filename);
      return 1;
    }

  fclose (env->file);		/* FIXME: freopen would be better */

  /* Prepare the header */
  header_set_value (env->header, "X-Mailer", argp_program_version, 1);

  if (util_header_expand (&env->header) == 0)
    {
      file = fopen (filename, "r");
      if (file != NULL)
	{
	  mailer_t mailer;
	  message_t msg = NULL;

	  message_create (&msg, NULL);

	  message_set_header (msg, env->header, NULL);

	  /* Fill the body.  */
	  {
	    body_t body = NULL;
	    stream_t stream = NULL;
	    off_t offset = 0;
	    char *buf = NULL;
	    size_t n = 0;
	    message_get_body (msg, &body);
	    body_get_stream (body, &stream);
	    while (getline (&buf, &n, file) >= 0)
	      {
		size_t len = strlen (buf);
		stream_write (stream, buf, len, offset, &n);
		offset += len;
	      }
	    
	    if (offset == 0)
	      util_error (_("Null message body; hope that's ok\n"));
	    if (buf)
	      free (buf);
	  }

	  fclose (file);

	  /* Save outgoing message */
	  if (save_to)
	    {
	      char *tmp = compose_header_get (env, MU_HEADER_TO, NULL);
	      address_t addr = NULL;
	      
	      address_create (&addr, tmp);
	      address_aget_email (addr, 1, &savefile);
	      address_destroy (&addr);
	      if (savefile)
		{
		  char *p = strchr (savefile, '@');
		  if (p)
		    *p = 0;
		}
	    }
	  util_save_outgoing (msg, savefile);
	  if (savefile)
	    free (savefile);

	  /* Check if we need to save the message to files or pipes.  */
	  if (env->outfiles)
	    {
	      int i;
	      for (i = 0; i < env->nfiles; i++)
		{
		  /* Pipe to a cmd.  */
		  if (env->outfiles[i][0] == '|')
		    msg_to_pipe (&(env->outfiles[i][1]), msg);
		  /* Save to a file.  */
		  else
		    {
		      int status;
		      mailbox_t mbx = NULL;
		      status = mailbox_create_default (&mbx, env->outfiles[i]);
		      if (status == 0)
			{
			  status = mailbox_open (mbx, MU_STREAM_WRITE
						 | MU_STREAM_CREAT);
			  if (status == 0)
			    {
			      mailbox_append_message (mbx, msg);
			      mailbox_close (mbx);
			    }
			  mailbox_destroy (&mbx);
			}
		      if (status)
			util_error (_("can't create mailbox %s"), env->outfiles[i]);
		    }
		}
	    }

	  /* Do we need to Send the message on the wire?  */
	  if (compose_header_get (env, MU_HEADER_TO, NULL)
	      || compose_header_get (env, MU_HEADER_CC, NULL)
	      || compose_header_get (env, MU_HEADER_BCC, NULL))
	    {
	      char *sendmail;
	      if (util_getenv (&sendmail, "sendmail", Mail_env_string, 0) == 0)
		{
		  int status = mailer_create (&mailer, sendmail);
		  if (status == 0)
		    {
		      if (util_getenv (NULL, "verbose", Mail_env_boolean, 0)
			  == 0)
			{
			  mu_debug_t debug = NULL;
			  mailer_get_debug (mailer, &debug);
			  mu_debug_set_level (debug,
					      MU_DEBUG_TRACE | MU_DEBUG_PROT);
			}
		      status = mailer_open (mailer, MU_STREAM_RDWR);
		      if (status == 0)
			{
			  mailer_send_message (mailer, msg, NULL, NULL);
			  mailer_close (mailer);
			}
		      mailer_destroy (&mailer);
		    }
		  if (status != 0)
		    msg_to_pipe (sendmail, msg);
		}
	      else
		util_error (_("variable sendmail not set: no mailer"));
	    }
	  message_destroy (&msg, NULL);
	  remove (filename);
	  free (filename);
	  return 0;
	}
    }

  remove (filename);
  free (filename);
  return 1;
}

/* Starting with '|' '/' or not consider addresses and we cheat
   by adding '.' in the mix for none absolute path.  */
static int
isfilename (const char *p)
{
  if (p)
    if (*p == '/' || *p == '.' || *p == '|')
      return 1;
  return 0;
}

/* FIXME: Should probably be in util.c.  */
/* Call popen(cmd) and write the message to it.  */
static void
msg_to_pipe (const char *cmd, message_t msg)
{
  FILE *fp = popen (cmd, "w");
  if (fp)
    {
      stream_t stream = NULL;
      char buffer[512];
      off_t off = 0;
      size_t n = 0;
      message_get_stream (msg, &stream);
      while (stream_read (stream, buffer, sizeof buffer - 1, off, &n) == 0
	     && n != 0)
	{
	  buffer[n] = '\0';
	  fprintf (fp, "%s", buffer);
	  off += n;
	}
      fclose (fp);
    }
  else
    util_error (_("Piping %s failed"), cmd);
}
