/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009, 2010 Free Software Foundation, Inc.

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

#include "mail.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int isfilename (const char *);
static int msg_to_pipe (const char *cmd, mu_message_t msg);


/* Additional message headers */
struct add_header
{
  int mode;
  char *name;
  char *value;
};

static mu_list_t add_header_list;

static int
seed_headers (void *item, void *data)
{
  struct add_header *hp = item;
  compose_env_t *env = data;

  compose_header_set (env, hp->name, hp->value, hp->mode);
  return 0;
}

static int
list_headers (void *item, void *data)
{
  struct add_header *hp = item;
  char *name = data;

  if (!name || strcmp (name, hp->name) == 0)
    {
      printf ("%s: %s\n", hp->name, hp->value);
    }
  return 0;
}
    
static void
add_header (char *name, char *value, int mode)
{
  struct add_header *hp;
  
  if (!add_header_list)
    {
      int rc = mu_list_create (&add_header_list);
      if (rc)
	{
	  util_error (_("Cannot create header list: %s"), mu_strerror (rc));
	  exit (1);
	}
    }

  hp = xmalloc (sizeof (*hp));
  hp->mode = mode;
  hp->name = name;
  hp->value = value;
  mu_list_append (add_header_list, hp);
}

void
send_append_header (char *text)
{
  char *p;
  size_t len;
  char *name;

  p = strchr (text, ':');
  if (!p)
    {
      util_error (_("Invalid header: %s"), text);
      return;
    }
  len = p - text;
  name = xmalloc (len + 1);
  memcpy (name, text, len);
  name[len] = 0;
  for (p++; *p && mu_isspace (*p); p++)
    ;

  add_header (name, strdup (p), COMPOSE_APPEND);
}

void
send_append_header2 (char *name, char *value, int mode)
{
  add_header (strdup (name), strdup (value), mode);
}

int
mail_sendheader (int argc, char **argv)
{
  if (argc == 1)
    mu_list_do (add_header_list, list_headers, NULL);
  else if (argc == 2)
    {
      if (strchr (argv[1], ':'))
	send_append_header (argv[1]);
      else
	mu_list_do (add_header_list, list_headers, argv[1]);
    }
  else
    {
      size_t len = strlen (argv[1]);
      if (len > 0 && argv[1][len - 1] == ':') 
	argv[1][len - 1] = 0;
      add_header (strdup (argv[1]), strdup (argv[2]), COMPOSE_APPEND);
    }
  return 0;
}


/* Send-related commands */

static void
read_cc_bcc (compose_env_t *env)
{
  if (mailvar_get (NULL, "askcc", mailvar_type_boolean, 0) == 0)
    compose_header_set (env, MU_HEADER_CC,
			ml_readline_with_intr ("Cc: "), COMPOSE_REPLACE);
  if (mailvar_get (NULL, "askbcc", mailvar_type_boolean, 0) == 0)
    compose_header_set (env, MU_HEADER_BCC,
			ml_readline_with_intr ("Bcc: "), COMPOSE_REPLACE);
}

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
  int save_to = mu_isupper (argv[0][0]);
  compose_init (&env);

  if (argc < 2)
    compose_header_set (&env, MU_HEADER_TO, ml_readline_with_intr ("To: "),
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
	    compose_header_set (&env, MU_HEADER_TO, p, COMPOSE_SINGLE_LINE);
	}
    }

  if (mailvar_get (NULL, "mailx", mailvar_type_boolean, 0))
    read_cc_bcc (&env);

  if (mailvar_get (NULL, "asksub", mailvar_type_boolean, 0) == 0)
    compose_header_set (&env, MU_HEADER_SUBJECT,
			ml_readline_with_intr ("Subject: "), COMPOSE_REPLACE);

  status = mail_send0 (&env, save_to);
  compose_destroy (&env);
  return status;
}

void
compose_init (compose_env_t * env)
{
  memset (env, 0, sizeof (*env));
  mu_list_do (add_header_list, seed_headers, env);
}

int
compose_header_set (compose_env_t * env, const char *name,
		    const char *value, int mode)
{
  int status;
  char *old_value;

  if (!value || value[0] == 0)
    return EINVAL;

  if (!env->header
      && (status = mu_header_create (&env->header, NULL, 0)) != 0)
    {
      util_error (_("Cannot create header: %s"), mu_strerror (status));
      return status;
    }

  switch (mode)
    {
    case COMPOSE_REPLACE:
    case COMPOSE_APPEND:
      if (is_address_field (name)
	  && mailvar_get (NULL, "inplacealiases", mailvar_type_boolean, 0) == 0)
	{
	  char *exp = alias_expand (value);
	  status = mu_header_set_value (env->header, name, exp ? exp : value,
					mode);
	  free (exp);
	}
      else
	status = mu_header_set_value (env->header, name, value, mode);
      break;

    case COMPOSE_SINGLE_LINE:
      if (mu_header_aget_value (env->header, name, &old_value) == 0
	  && old_value[0])
	{
	  if (is_address_field (name)
	      && mailvar_get (NULL, "inplacealiases", mailvar_type_boolean, 0) == 0)
	    {
	      char *exp = alias_expand (value);
	      status = util_merge_addresses (&old_value, exp ? exp : value);
	      if (status == 0)
		status = mu_header_set_value (env->header, name, old_value, 1);
	      free (exp);
	    }
	  else
	    {
	      size_t size = strlen (old_value) + strlen (value) + 2;
	      char *p = realloc (old_value, size);
	      if (!p)
		status = ENOMEM;
	      else
		{
		  old_value = p;
		  strcat (old_value, ",");
		  strcat (old_value, value);
		  status = mu_header_set_value (env->header, name, old_value,
						1);
		}
	    }
	  free (old_value);
	}
      else
	status = compose_header_set (env, name, value, COMPOSE_REPLACE);
    }

  return status;
}

char *
compose_header_get (compose_env_t * env, char *name, char *defval)
{
  char *p;

  if (mu_header_aget_value (env->header, name, &p))
    p = defval;
  return p;
}

void
compose_destroy (compose_env_t *env)
{
  mu_header_destroy (&env->header);
  free (env->outfiles);
}

static int
fill_body (mu_message_t msg, mu_stream_t instr)
{
  int rc;
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  mu_off_t n;
  
  rc = mu_message_get_body (msg, &body);
  if (rc)
    {
      mu_error (_("cannot get message body: %s"), mu_strerror (rc));
      return 1;
    }
  rc = mu_body_get_streamref (body, &stream);
  if (rc)
    {
      mu_error (_("cannot get body: %s"), mu_strerror (rc));
      return 1;
    }

  rc = mu_stream_copy (stream, instr, 0, &n);
  mu_stream_destroy (&stream);

  if (rc)
    {
      mu_error (_("cannot copy temporary stream: %s"), mu_strerror (rc));
      return 1;
    }
  
  if (n == 0)
    {
      if (mailvar_get (NULL, "nullbody", mailvar_type_boolean, 0) == 0)
	{
	  char *str;
	  if (mailvar_get (&str, "nullbodymsg", mailvar_type_string, 0) == 0)
	    util_error ("%s\n", _(str));
	}
      else
	return 1;
    }

  return 0;
}

static int
save_dead_message (compose_env_t *env)
{
  if (mailvar_get (NULL, "save", mailvar_type_boolean, 0) == 0)
    {
      FILE *fp = fopen (getenv ("DEAD"),
			mailvar_get (NULL, "appenddeadletter",
				     mailvar_type_boolean, 0) == 0 ?
			"a" : "w");
      
      if (!fp)
	{
	  util_error (_("Cannot open file %s: %s"), getenv ("DEAD"),
		      strerror (errno));
	  return 1;
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
  return 0;
}

static int
send_message (mu_message_t msg)
{
  char *sendmail;
  int status;
  
  if (mailvar_get (&sendmail, "sendmail", mailvar_type_string, 0) == 0)
    {
      if (sendmail[0] == '/')
	status = msg_to_pipe (sendmail, msg);
      else
	{
	  mu_mailer_t mailer;
	  
	  status = mu_mailer_create (&mailer, sendmail);
	  if (status == 0)
	    {
	      if (mailvar_get (NULL, "verbose", mailvar_type_boolean, 0) == 0)
		{
		  mu_debug_set_category_level (MU_DEBCAT_MAILER,
				      MU_DEBUG_LEVEL_UPTO (MU_DEBUG_PROT));
		}
	      status = mu_mailer_open (mailer, MU_STREAM_RDWR);
	      if (status == 0)
		{
		  status = mu_mailer_send_message (mailer, msg, NULL, NULL);
		  mu_mailer_close (mailer);
		}
	      else
		util_error (_("Cannot open mailer: %s"), mu_strerror (status));
	      mu_mailer_destroy (&mailer);
	    }
	  else
	    util_error (_("Cannot create mailer: %s"),
			mu_strerror (status));
	}
    }
  else
    {
      util_error (_("Variable sendmail not set: no mailer"));
      status = ENOSYS;
    }
  return status;
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
  int fd, rc;
  char *filename;
  char *savefile = NULL;
  int int_cnt;
  char *escape;

  rc = mu_tempfile (NULL, 0, &fd, &filename);
  if (rc)
    {
      util_error (_("Cannot open temporary file: %s"), mu_strerror (rc));
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
	  if (mailvar_get (NULL, "ignore", mailvar_type_boolean, 0) == 0)
	    {
	      fprintf (stdout, "@\n");
	    }
	  else
	    {
	      if (buf)
		free (buf);
	      if (++int_cnt == 2)
		break;
	      util_error (_("\n(Interrupt -- one more to kill letter)"));
	    }
	  continue;
	}

      if (!buf)
	{
	  if (interactive 
	      && mailvar_get (NULL, "ignoreeof", mailvar_type_boolean, 0) == 0)
	    {
	      util_error (mailvar_get (NULL, "dot", mailvar_type_boolean, 0) == 0 ?
			  _("Use \".\" to terminate letter.") :
			  _("Use \"~.\" to terminate letter."));
	      continue;
	    }
	  else
	    break;
	}

      int_cnt = 0;

      if (strcmp (buf, ".") == 0
	  && mailvar_get (NULL, "dot", mailvar_type_boolean, 0) == 0)
	done = 1;
      else if (mailvar_get (&escape, "escape", mailvar_type_string, 0) == 0
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
	      struct mu_wordsplit ws;
	      int status;

	      ofile = env->file;

	      if (mu_wordsplit (buf + 1, &ws, MU_WRDSF_DEFFLAGS) == 0)
		{
		  if (ws.ws_wordc > 0)
		    {
		      const struct mail_escape_entry *entry = 
			mail_find_escape (ws.ws_wordv[0]);

		      if (entry)
			status = (*entry->escfunc) (ws.ws_wordc, ws.ws_wordv,
						    env);
		      else
			util_error (_("Unknown escape %s"), ws.ws_wordv[0]);
		    }
		  else
		    util_error (_("Unfinished escape"));
		  mu_wordsplit_free (&ws);
		}
	      else
		{
		  util_error (_("Cannot parse escape sequence: %s"),
			      mu_wordsplit_strerror (&ws));
		}

	      ofile = env->ofile;
	    }
	}
      else
	fprintf (env->file, "%s\n", buf);
      fflush (env->file);
      free (buf);
    }

  /* If interrupted, dump the file to dead.letter.  */
  if (int_cnt)
    {
      save_dead_message (env);
      fclose (env->file);
      remove (filename);
      free (filename);
      return 1;
    }

  /* In mailx compatibility mode, ask for Cc and Bcc after editing
     the body of the message */
  if (mailvar_get (NULL, "mailx", mailvar_type_boolean, 0) == 0)
    read_cc_bcc (env);

  /* Prepare the header */
  if (mailvar_get (NULL, "xmailer", mailvar_type_boolean, 0) == 0)
    mu_header_set_value (env->header, MU_HEADER_X_MAILER, 
                         program_version, 1);

  if (util_header_expand (&env->header) == 0)
    {
      int rc;
      mu_stream_t instr;

      rc = mu_file_stream_create (&instr, filename, MU_STREAM_READ);
      if (rc)
	mu_error (_("cannot open temporary stream `%s' for reading: %s"),
		  filename, mu_strerror (rc));
      else
	{
	  mu_message_t msg = NULL;
	  int rc;
	  int status = 0;
	  
	  mu_message_create (&msg, NULL);
	  
	  /* Fill the body.  */
	  rc = fill_body (msg, instr);
	  mu_stream_destroy (&instr);

	  if (rc == 0)
	    {
	      /* Save outgoing message */
	      if (save_to)
		{
		  char *tmp = compose_header_get (env, MU_HEADER_TO, NULL);
		  mu_address_t addr = NULL;
	      
		  mu_address_create (&addr, tmp);
		  mu_address_aget_email (addr, 1, &savefile);
		  mu_address_destroy (&addr);
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
			status = msg_to_pipe (env->outfiles[i] + 1, msg);
		      /* Save to a file.  */
		      else
			{
			  mu_mailbox_t mbx = NULL;
			  status = mu_mailbox_create_default (&mbx, 
                                                              env->outfiles[i]);
			  if (status == 0)
			    {
			      status = mu_mailbox_open (mbx, MU_STREAM_WRITE
							| MU_STREAM_CREAT);
			      if (status == 0)
				{
				  status = mu_mailbox_append_message (mbx, msg);
				  if (status)
				    util_error (_("Cannot append message: %s"),
						mu_strerror (status));
				  mu_mailbox_close (mbx);
				}
			      mu_mailbox_destroy (&mbx);
			    }
			  if (status)
			    util_error (_("Cannot create mailbox %s: %s"), 
					env->outfiles[i],
					mu_strerror (status));
			}
		    }
		}

	      /* Do we need to Send the message on the wire?  */
 	      if (status == 0 &&
 		  (compose_header_get (env, MU_HEADER_TO, NULL) ||
 		   compose_header_get (env, MU_HEADER_CC, NULL) ||
 		   compose_header_get (env, MU_HEADER_BCC, NULL)))
		{
		  mu_message_set_header (msg, env->header, NULL);
		  env->header = NULL;
		  status = send_message (msg);
 		  if (status)
		    {
		      mu_error (_("cannot send message: %s"),
				mu_strerror (status));
		      save_dead_message (env);
		    }
		}
	    }
	  fclose (env->file);
	  mu_message_destroy (&msg, NULL);
	  remove (filename);
	  free (filename);
	  return status;
	}
    }
  else
    save_dead_message (env);
  
  fclose (env->file);

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
static int
msg_to_pipe (const char *cmd, mu_message_t msg)
{
  mu_stream_t progstream, msgstream;
  int status, rc;
  
  status = mu_prog_stream_create (&progstream, cmd, MU_STREAM_WRITE);
  if (status)
    {
      util_error (_("Cannot pipe to %s: %s"), cmd, mu_strerror (status));
      return status;
    }

  mu_message_get_streamref (msg, &msgstream);
  status = mu_stream_copy (progstream, msgstream, 0, NULL);
  rc = mu_stream_close (progstream);

  if (status == 0 && rc)
    status = rc;

  mu_stream_destroy (&progstream);
  mu_stream_destroy (&msgstream);
  
  if (status)
    {
      util_error (_("Sending data to %s failed: %s"), cmd,
		  mu_strerror (status));
    }
  return status;
}
