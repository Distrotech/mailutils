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

/*
 * m[ail] address...
 */

int
mail_send (int argc, char **argv)
{
  struct send_environ env;
  int status;

  env.to = env.cc = env.bcc = env.subj = NULL;
  
  if (argc < 2)
    env.to = readline ("To: ");
  else
    {
      while (--argc)
	{
	  char *p = alias_expand (*++argv);
	  if (env.to)
	    util_strcat(&env.to, ",");
	  util_strcat(&env.to, p);
	  free (p);
	}
    }

  if ((util_find_env ("askcc"))->set)
    env.cc = readline ("Cc: ");
  if ((util_find_env ("askbcc"))->set)
    env.bcc = readline ("Bcc: ");

  if ((util_find_env ("asksub"))->set)
    env.subj = readline ("Subject: ");
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
}

/* mail_send0(): shared between mail_send() and mail_reply();

   If the variable "record" is set, the outgoing message is
   saved after being sent. If "save_to" argument is non-zero,
   the name of the save file is derived from "to" argument. Otherwise,
   it is taken from the value of "record" variable. */

int
mail_send0 (struct send_environ *env, int save_to)
{
  char *buf = NULL;
  size_t n = 0;
  int done = 0;
  int fd;
  char *filename;
  FILE *file;
  char *savefile = NULL;
  int int_cnt;

  fd = util_tempfile(&filename);

  if (fd == -1)
    {
      util_error("Can not open temporary file");
      return 1;
    }

  /* Prepare environment */
  env = env;
  env->filename = filename;
  env->file = fdopen (fd, "w+");
  env->ofile = ofile;
  
  ml_clear_interrupt();
  int_cnt = 0;
  while (!done)
    {
      buf = readline(NULL);
      
      if (ml_got_interrupt())
	{
	  if (util_find_env("ignore")->set)
	    {
	      fprintf (stdout, "@\n");
	    }
	  else
	    {
	      if (++int_cnt == 2)
		break;
	      util_error("(Interrupt -- one more to kill letter)");
	      if (buf)
		free (buf);
	    }
	  continue;
	}

      if (!buf)
	{
	  if (util_find_env ("ignoreeof")->set)
	    {
	      util_error ("Use \".\" to terminate letter.");
	      continue;
	    }
	  else
	    break;
	}
      
      int_cnt = 0;
      
      if (buf[0] == (util_find_env("escape"))->value[0])
	{
	  if (buf[1] == buf[0])
	    fprintf (env->file, "%s\n", buf+1);
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
		  
		  if (entry.func)
		    status = (*entry.func)(argc, argv, env);
		  else
		    util_error("Unknown escape %s", argv[0]);
		}
	      else
		{
		  util_error("can't parse escape sequence");
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

  if (int_cnt)
    {
      if (util_find_env ("save")->set)
	{
	  FILE *fp = fopen (getenv("DEAD"), "a");

	  if (!fp)
	    {
	      util_error("can't open file %s: %s", getenv("DEAD"),
			 strerror(errno));
	    }
	  else
	    {
	      char *buf = NULL;
	      int n;
	      rewind (env->file);
	      while (getline (&buf, &n, env->file) > 0)
		fputs(buf, fp);
	      fclose(fp);
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
      int status;
      char *mailer_name = alloca (strlen ("sendmail:")
				  + strlen (_PATH_SENDMAIL) + 1);
      sprintf (mailer_name, "sendmail:%s", _PATH_SENDMAIL);
      if ((status = mailer_create (&mailer, mailer_name)) != 0
          || (status = mailer_open (mailer, MU_STREAM_RDWR)) != 0)
        {
          util_error("%s: %s", mailer_name, strerror (status));
	  remove (filename);
	  free (filename);
          return 1;
        }
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
	header_set_value (header, "X-Mailer", strdup (argp_program_version), 1);
      }

      /* Fill the body.  */
      {
	body_t body = NULL;
	stream_t stream = NULL;
	off_t offset = 0;
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
	      char *p = strchr(savefile, '@');
	      if (p)
		*p = 0;
	    }
	}
      util_save_outgoing (msg, savefile);
      if (savefile)
	free (savefile);
      
      /* Send the message.  */
      mailer_send_message (mailer, msg);
      message_destroy (&msg, NULL);
      mailer_destroy (&mailer);
      remove (filename);
      free (filename);
      return 0;
    }

  remove (filename);
  free (filename);
  return 1;
}
