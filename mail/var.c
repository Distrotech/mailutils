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
var_continue()
{
  fprintf(stdout, "(continue)\n");
}

/* ~![shell-command] */
int
var_shell(int argc, char **argv, struct send_environ *env)
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
var_command(int argc, char **argv, struct send_environ *env)
{
  struct mail_command_entry entry;
  int status;
  
  if (argv[1][0] == '#')
    return 0;
  entry = util_find_entry (mail_command_table, argv[1]);
  if (!entry.func)
    {
      util_error("Unknown command: %s", argv[1]);
      return 1;
    }
  if (entry.flags & (EF_FLOW|EF_SEND))
    {
      util_error("Command not allowed in an escape sequence\n");
      return 1;
    }

  ofile = env->ofile;
  status = (*entry.func)(argc-1, argv+1);
  ofile = env->file;
  return status;
}

/* ~? */
int
var_help(int argc, char **argv, struct send_environ *env)
{
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
var_sign(int argc, char **argv, struct send_environ *env)
{
  struct mail_env_entry *p;
  if (isupper(argv[0][0]))
    p = util_find_env("Sign");
  else
    p = util_find_env("sign");
  if (p->set)
    fprintf (ofile, "%s", p->value);
  else
    util_error("\"sign\" not set");
  return 0;
}

/* ~b[bcc-list] */
int
var_bcc(int argc, char **argv, struct send_environ *env)
{
  while (--argc)
    util_strcat(&env->bcc, *++argv);
  return 0;
}

/* ~c[cc-list] */
int
var_cc(int argc, char **argv, struct send_environ *env)
{
  while (--argc)
    util_strcat(&env->cc, *++argv);
  return 0;
}

/* ~d */
int
var_deadletter(int argc, char **argv, struct send_environ *env)
{
  FILE *dead = fopen (getenv("DEAD"), "r");
  int c;
  
  while ((c = fgetc(dead)) != EOF)
    fputc (c, ofile);
  fclose (dead);
  return 0;
}

static int
var_run_editor(char *ed, int argc, char **argv, struct send_environ *env)
{
  fclose (env->file);
  ofile = env->ofile;
  util_do_command ("!%s %s", ed, env->filename);
  env->file = fopen (env->filename, "a+");
  ofile =  env->file;
  var_continue();
  return 0;
}

/* ~e */
int
var_editor(int argc, char **argv, struct send_environ *env)
{
  return var_run_editor(getenv("EDITOR"), argc, argv, env);
}

/* ~v */
int
var_visual(int argc, char **argv, struct send_environ *env)
{
  return var_run_editor(getenv("VISUAL"), argc, argv, env);
}

/* ~f[mesg-list] */
/* ~F[mesg-list] */
int
var_print(int argc, char **argv, struct send_environ *env)
{
  return mail_print(argc, argv);
}

/* ~h */
int
var_headers(int argc, char **argv, struct send_environ *env)
{
  ml_reread("To:", &env->to);
  ml_reread("Cc:", &env->cc);
  ml_reread("Bcc:", &env->bcc);
  ml_reread("Subject:", &env->subj);
  var_continue();
  return 0;
}

/* ~i[var-name] */
int
var_insert(int argc, char **argv, struct send_environ *env)
{
  fprintf (ofile, "%s", util_find_env(argv[1])->value);
  return 1;
}

/* ~m[mesg-list] */
/* ~M[mesg-list] */
int
var_quote(int argc, char **argv, struct send_environ *env)
{
  if (argc > 1)
    return util_msglist_command (var_quote, argc, argv, 0);
  else
    {
      message_t mesg;
      header_t hdr;
      body_t body;
      stream_t stream;
      char buffer[512];
      off_t off = 0;
      size_t n = 0;
      char *prefix = util_find_env ("indentprefix")->value;
      
      if (mailbox_get_message (mbox, cursor, &mesg) != 0)
	return 1;

      fprintf(stdout, "Interpolating: %d\n", cursor);
      
      if (islower(argv[0][0]))
	{
	  size_t i, num = 0;
	  char buffer[512];
	  
	  message_get_header (mesg, &hdr);
	  header_get_field_count (hdr, &num);

	  for (i = 1; i <= num; i++)
	    {
	      header_get_field_name (hdr, i, buffer, sizeof(buffer), NULL);
	      if (mail_header_is_visible (buffer))
		{
		  fprintf (ofile, "%s%s: ", prefix, buffer);
		  header_get_field_value (hdr, i, buffer, sizeof(buffer),
					  NULL);
		  fprintf (ofile, "%s\n", buffer);
		}
	    }
	  fprintf (ofile, "\n");
	  message_get_body (mesg, &body);
	  body_get_stream (body, &stream);
	}
      else
	message_get_stream (mesg, &stream);

      while (stream_readline(stream, buffer, sizeof(buffer) - 1, off, &n) == 0
             && n != 0)
        {
          buffer[n] = '\0';
	  fprintf (ofile, "%s%s", prefix, buffer);
          off += n;
        }
      var_continue();
    }
  return 0;
}

/* ~p */
int
var_type_input(int argc, char **argv, struct send_environ *env)
{
  char buf[512];

  fprintf(env->ofile, "Message contains:\n");
  
  if (env->to)
    fprintf(env->ofile, "To: %s\n", env->to);
  if (env->cc)
    fprintf(env->ofile, "Cc: %s\n", env->cc);
  if (env->bcc)
    fprintf(env->ofile, "Bcc: %s\n", env->bcc);
  if (env->subj)
    fprintf(env->ofile, "Subject: %s\n\n", env->subj);
  
  rewind(env->file);
  while (fgets(buf, sizeof(buf), env->file))
    fputs(buf, env->ofile);

  var_continue();

  return 0;
}

/* ~r[filename] */
int
var_read(int argc, char **argv, struct send_environ *env)
{
  char *filename = util_fullpath(argv[1]);
  FILE *inf = fopen(filename, "r");
  size_t size, lines;
  char buf[512];
  
  if (!inf)
    {
      util_error("can't open %s: %s\n", filename, strerror(errno));
      free(filename);
      return 1;
    }

  size = lines = 0;
  while (fgets(buf, sizeof(buf), inf))
    {
      lines++;
      size += strlen(buf);
      fputs(buf, ofile);
    }
  fclose(inf);
  fprintf(stdout, "\"%s\" %d/%d\n", filename, lines, size);
  free(filename);
  return 0;
}

/* ~s[string] */
int
var_subj(int argc, char **argv, struct send_environ *env)
{
  free(env->subj);
  env->subj = strdup(argv[1]);
  return 0;
}

/* ~t[name-list] */
int
var_to(int argc, char **argv, struct send_environ *env)
{
  while (--argc)
    util_strcat(&env->to, *++argv);
  return 0;
}

/* ~w[filename] */
int
var_write(int argc, char **argv, struct send_environ *env)
{
  char *filename = util_fullpath(argv[1]);
  FILE *fp = fopen(filename, "w"); /*FIXME: check for the existence first */
  size_t size, lines;
  char buf[512];
  
  if (!fp)
    {
      util_error("can't open %s: %s\n", filename, strerror(errno));
      free(filename);
      return 1;
    }

  rewind(env->file);
  size = lines = 0;
  while (fgets(buf, sizeof(buf), env->file))
    {
      lines++;
      size += strlen(buf);
      fputs(buf, fp);
    }
  fclose(fp);
  fprintf(stdout, "\"%s\" %d/%d\n", filename, lines, size);
  free(filename);
  return 0;
}

/* ~x */
int
var_exit(int argc, char **argv, struct send_environ *env)
{
  return util_do_command ("quit");
}

/* ~|[shell-command] */
int
var_pipe(int argc, char **argv, struct send_environ *env)
{
  int p[2];
  pid_t pid;
  int fd;
  
  if (pipe(p))
    {
      util_error("pipe: %s", strerror(errno));
      return 1;
    }

  fd = util_tempfile(NULL);
  if (fd == -1)
    return 1;

  if ((pid = fork()) < 0)
    {
      close(p[0]);
      close(p[1]);
      close(fd);
      util_error("fork: %s", strerror(errno));
      return 1;
    }
  else if (pid == 0)
    {
      /* Child */
      int i;
      char **xargv;
      
      /* Attache the pipes */
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      
      close(1);
      dup(fd);
      close(fd);
      
      /* Execute the process */
      xargv = calloc(argc, sizeof(xargv[0]));
      if (!xargv)
	{
	  util_error("not enough memory");
	  exit(1);
	}
      for (i = 0; i < argc-1; i++)
	xargv[i] = argv[i+1];
      xargv[i] = NULL;
      execvp(xargv[0], xargv);
      util_error("cannot exec process `%s': %s", xargv[0], strerror(errno));
      exit(1);
    }
  else
    {
      FILE *fp;
      char *buf = NULL;
      size_t n;
      size_t lines, size;
      int rc = 1;
      int status;
      
      close(p[0]);

      /* Parent */
      fp = fdopen(p[1], "w");

      fclose(env->file);
      env->file = fopen(env->filename, "r"); 
      
      lines = size = 0;
      while (getline (&buf, &n, env->file) > 0)
	{
	  lines++;
	  size += n;
	  fputs(buf, fp);
	}
      fclose(env->file);
      fclose(fp);  /* Closes p[1] */
      
      waitpid(pid, &status, 0);
      if (!WIFEXITED(status))
	{
	  util_error("child terminated abnormally: %d", WEXITSTATUS(status));
	}
      else
	{
	  struct stat st;
	  if (fstat(fd, &st))
	    {
	      util_error("can't stat output file: %s", strerror(errno));
	    }
	  else if (st.st_size > 0)
	    rc = 0;
	}

      fprintf(stdout, "\"|%s\" out: %d/%d ", argv[1], lines, size); 
      if (rc)
	{
	  fprintf(stdout, "no lines in\n");
	}
      else
	{
	  /* Ok, replace the old tempfile */
	  fp = fdopen(fd, "r");
	  rewind(fp);

	  env->file = fopen(env->filename, "w+");
	  
	  lines = size = 0;
	  while (getline (&buf, &n, fp) > 0)
	    {
	      lines++;
	      size += n;
	      fputs(buf, env->file);
	    }
	  fclose(env->file);

	  fprintf(stdout, "in: %d/%d\n", lines, size); 
	}

      /* Clean up the things */
      if (buf)
	free(buf);

      env->file = fopen(env->filename, "a+");
      ofile =  env->file;
    }

  close(fd);
  
  return 0;
}
