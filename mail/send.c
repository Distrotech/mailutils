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


/* FIXME: Those headers should be in "mail.h".  */
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
 '/' it is consider a file and the message is save to a file.
 '.' by extension dot is consider relative path.
 '|' a pipe the message is writent to the pipe.
 example:
 mail joe '| cat >/tmp/save'
 mail will be send to joe and the message save in /tmp/save.
 */

int
mail_send (int argc, char **argv)
{
  struct send_environ env;
  int status;

  env.to = env.cc = env.bcc = env.subj = NULL;
  env.outfiles = NULL; env.nfiles = 0;

  if (argc < 2)
    env.to = readline ((char *)"To: ");
  else
    {
      while (--argc)
	{
	  char *p = alias_expand (*++argv);
	  if (isfilename(p))
	    {
	      env.outfiles = realloc (env.outfiles,
				      (env.nfiles+1)*sizeof (*(env.outfiles)));
	      if (env.outfiles)
		{
		  env.outfiles[env.nfiles] = p;
		  env.nfiles++;
		}
	    }
	  else
	    {
	      if (env.to)
		util_strcat(&env.to, ",");
	      util_strcat(&env.to, p);
	      free (p);
	    }
	}
    }

  if ((util_find_env ("askcc"))->set)
    env.cc = readline ((char *)"Cc: ");
  if ((util_find_env ("askbcc"))->set)
    env.bcc = readline ((char *)"Bcc: ");

  if ((util_find_env ("asksub"))->set)
    env.subj = readline ((char *)"Subject: ");
  else
    env.subj = (util_find_env ("subject"))->value;

  status = mail_send0 (&env, isupper(argv[0][0]));
  free_env_headers (&env);
  return status;
}


void
free_env_headers (struct send_environ *env)
{
  if (env->to)
    free (env->to);
  if (env->cc)
    free (env->cc);
  if (env->bcc)
    free (env->bcc);
  if (env->subj)
    free (env->subj);
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
   addresses on the command line and message contents on standard input.
*/

int
mail_send0 (struct send_environ *env, int save_to)
{
  int done = 0;
  int fd;
  char *filename;
  FILE *file;
  char *savefile = NULL;
  int int_cnt;

  fd = util_tempfile (&filename);

  if (fd == -1)
    {
      util_error ("Can not open temporary file");
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
      buf = readline ((char *)" \b");

      if (ml_got_interrupt ())
	{
	  if (util_find_env ("ignore")->set)
	    {
	      fprintf (stdout, "@\n");
	    }
	  else
	    {
	      if (buf)
		free (buf);
	      if (++int_cnt == 2)
		break;
	      util_error ("(Interrupt -- one more to kill letter)");
	    }
	  continue;
	}

      if (!buf)
	{
	  if (util_find_env ("ignoreeof")->set)
	    {
	      util_error (util_find_env ("dot")->set ?
			  "Use \".\" to terminate letter." :
			  "Use \"~.\" to terminate letter.");
	      continue;
	    }
	  else
	    break;
	}

      int_cnt = 0;

      if (buf[0] == '.' && util_find_env ("dot")->set)
	done = 1;
      else if (buf[0] == (util_find_env ("escape"))->value[0])
	{
	  if (buf[1] == buf[0])
	    fprintf (env->file, "%s\n", buf+1);
	  else if (buf[1] == '.')
	    done = 1;
	  else
	    {
	      int argc;
	      char **argv;
	      int status;

	      ofile = env->file;

	      if (argcv_get (buf+1, "", &argc, &argv) == 0)
		{
		  struct mail_command_entry entry;
		  entry = util_find_entry (mail_escape_table, argv[0]);

		  if (entry.escfunc)
		    {
		      status = (*entry.escfunc)(argc, argv, env);
		    }
		  else
		    util_error ("Unknown escape %s", argv[0]);
		}
	      else
		{
		  util_error ("can't parse escape sequence");
		}
	      argcv_free (argc, argv);

	      ofile =  env->ofile;
	    }
	}
      else
	fprintf (env->file, "%s\n", buf);
      fflush (env->file);
      free (buf);
    }

  /* If interrupted dumpt the file to dead.letter.  */
  if (int_cnt)
    {
      if (util_find_env ("save")->set)
	{
	  FILE *fp = fopen (getenv ("DEAD"),
			    util_find_env ("appenddeadletter")->set ?
			    "a" : "w");

	  if (!fp)
	    {
	      util_error ("can't open file %s: %s", getenv ("DEAD"),
			 strerror (errno));
	    }
	  else
	    {
	      char *buf = NULL;
	      unsigned int n;
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

  fclose (env->file); /*FIXME: freopen would be better*/

  file = fopen (filename, "r");
  if (file != NULL)
    {
      mailer_t mailer;
      message_t msg = NULL;
      message_create (&msg, NULL);

      /* Fill the header.  */
      {
	header_t header = NULL;
	message_get_header (msg, &header);
	if (env->to && *env->to != '\0')
	  header_set_value (header, MU_HEADER_TO, strdup (env->to), 0);
	if (env->cc && *env->cc != '\0')
	  header_set_value (header, MU_HEADER_CC, strdup (env->cc), 0);
	if (env->bcc && *env->bcc != '\0')
	  header_set_value (header, MU_HEADER_BCC , strdup (env->bcc), 0);
	if (env->subj && *env->subj != '\0')
	  header_set_value (header, MU_HEADER_SUBJECT, strdup (env->subj), 1);
	header_set_value (header, "X-Mailer", strdup(argp_program_version), 1);
      }

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
	  util_error("Null message body; hope that's ok\n");
	if (buf)
	    free (buf);
      }

      fclose (file);

      /* Save outgoing message */
      if (save_to)
	{
	  savefile = strdup (env->to);
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
		    util_error("can't create mailbox %s", env->outfiles[i]);
		}
	    }
	}

      /* Do we need to Send the message on the wire?  */
      if ((env->to && *env->to != '\0')
	  || (env->cc && *env->cc != '\0')
	  || (env->bcc && *env->bcc != '\0'))
	{
	  if (util_find_env ("sendmail")->set)
	    {
	      char *sendmail = util_find_env ("sendmail")->value;
	      int status = mailer_create (&mailer, sendmail);
	      if (status == 0)
		{
		  if (util_find_env ("verbose")->set)
		    {
		      mu_debug_t debug = NULL;
		      mailer_get_debug (mailer, &debug);
		      mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
		    }
		  status = mailer_open (mailer, MU_STREAM_RDWR);
		  if (status == 0)
		    {
		      mailer_send_message (mailer, msg);
		      mailer_close (mailer);
		    }
		  mailer_destroy (&mailer);
		}
	      if (status != 0)
		msg_to_pipe (sendmail, msg);
	    }
	  else
	    util_error ("variable sendmail not set: no mailer");
	}
      message_destroy (&msg, NULL);
      remove (filename);
      free (filename);
      return 0;
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
/* Call pope(cmd) and write the message to it.  */
static void
msg_to_pipe (const char *cmd, message_t msg)
{
  FILE *fp = popen (cmd, "w");
  if (fp)
    {
      stream_t stream = NULL;
      char buffer[512];
      off_t off = 0;
      unsigned int n = 0;
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
    util_error ("Piping %s failed", cmd);
}
