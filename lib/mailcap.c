/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/mailutils.h>
#include <fnmatch.h>
#include <sys/wait.h>
#include <ctype.h>

/* FNM_CASEFOLD is a GNU extension. Provide a replacement for systems 
   lacking it. */
#ifndef FNM_CASEFOLD
# define FNM_CASEFOLD 0
#endif

/* Default mailcap path, the $HOME/.mailcap: entry is prepended to it */
#define DEFAULT_MAILCAP \
 "/usr/local/etc/mailcap:"\
 "/usr/etc/mailcap:"\
 "/etc/mailcap:"\
 "/etc/mail/mailcap:"\
 "/usr/public/lib/mailcap"

#define FLAGS_DRY_RUN      0x0001
#define FLAGS_INTERACTIVE  0x0002

struct mime_context
{
  mu_stream_t input;         
  mu_header_t hdr;
  char *content_type_buffer;
  char *content_type;
  mu_list_t values;
  char *temp_file;
  int unlink_temp_file;

  mu_list_t no_ask_types;
  int debug_level;
  int flags;
};

#define DEBUG(c,l,f) if ((c)->debug_level > (l)) printf f

static int
mime_context_fill (struct mime_context *ctx, const char *file,
		   mu_stream_t input, mu_header_t hdr, const char *no_ask,
		   int interactive, int dry_run, int debug_level)
{
  struct mu_wordsplit ws;
  size_t i;
  
  memset (ctx, 0, sizeof *ctx);
  ctx->input = input;
  ctx->hdr = hdr;
  if (mu_header_aget_value (hdr, MU_HEADER_CONTENT_TYPE,
			    &ctx->content_type_buffer))
    return 1;
  ws.ws_delim = ";";
  if (mu_wordsplit (ctx->content_type_buffer, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|MU_WRDSF_WS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    {
      mu_error (_("cannot split line `%s': %s"),
		ctx->content_type_buffer,
		mu_wordsplit_strerror (&ws));
      return 1;
    }

  ctx->content_type = ws.ws_wordv[0];
  ws.ws_wordv[0] = NULL;
  ctx->temp_file = file ? mu_strdup (file) : NULL; 
  ctx->unlink_temp_file = 0;

  if (interactive)
    ctx->flags |= FLAGS_INTERACTIVE;
  if (dry_run)
    ctx->flags |= FLAGS_DRY_RUN;
  ctx->debug_level = debug_level;
  
  mu_list_create (&ctx->values);

  for (i = 1; i < ws.ws_wordc; i++)
    {
      mu_list_append (ctx->values, ws.ws_wordv[i]);
      ws.ws_wordv[i] = NULL;
    }
  mu_wordsplit_free (&ws);
  
  if (no_ask)
    {
      mu_list_create (&ctx->no_ask_types);
      mu_list_set_destroy_item (ctx->no_ask_types, mu_list_free_item);
      if (mu_string_split (no_ask, ",", ctx->no_ask_types))
	return 1;
    }
  return 0;
}

static void
mime_context_release (struct mime_context *ctx)
{
  free (ctx->content_type_buffer);
  if (ctx->unlink_temp_file)
    unlink (ctx->temp_file);
  free (ctx->temp_file);
  mu_list_destroy (&ctx->values);
  mu_list_destroy (&ctx->no_ask_types);
}

static int
mime_context_do_not_ask (struct mime_context *ctx)
{
  int rc = 0;
  
  if (ctx->no_ask_types)
    {
      mu_iterator_t itr;
      mu_list_get_iterator (ctx->no_ask_types, &itr);
      for (mu_iterator_first (itr); !rc && !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  char *p;
	  mu_iterator_current (itr, (void**)&p);
	  rc = fnmatch (p, ctx->content_type, FNM_CASEFOLD) == 0;
	}
      mu_iterator_destroy (&itr);
    }
  return rc;
}

static int
dry_run_p (struct mime_context *ctx)
{
  return ctx->flags & FLAGS_DRY_RUN;
}

static int
interactive_p (struct mime_context *ctx)
{
  return ctx->flags & FLAGS_INTERACTIVE;
}

static void
mime_context_get_content_type (struct mime_context *ctx, char **ptr)
{
  *ptr = ctx->content_type;
}

static void
mime_context_get_input (struct mime_context *ctx, mu_stream_t *pinput)
{
  *pinput = ctx->input;
}

static int
mime_context_get_content_type_value (struct mime_context *ctx,
				     char *name, size_t len,
				     char **ptr, size_t *plen)
{
  mu_iterator_t itr = NULL;
  int rc = 1;
  
  mu_list_get_iterator (ctx->values, &itr);
  for (mu_iterator_first (itr);
       !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *item, *p;

      mu_iterator_current (itr, (void**) &item);
      p = strchr (item, '=');
      if (p - item == len && strncasecmp (item, name, len) == 0)
	{
	  rc = 0;
	  *ptr = ++p;
	  *plen = strlen (*ptr);
	  if (**ptr == '"')
	    {
	      ++*ptr;
	      *plen -= 2;
	    }
	  break;
	}
    }
  mu_iterator_destroy (&itr);
  return rc;
}

/* FIXME: Rewrite via mu_stream_copy */
static void
mime_context_write_input (struct mime_context *ctx, int fd)
{
  mu_stream_t input;
  char buf[512];
  size_t n;
  int status;
  
  mime_context_get_input (ctx, &input);
  status = mu_stream_seek (input, 0, SEEK_SET, NULL);
  if (status && status != ENOSYS)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", NULL, status);
      abort (); /* FIXME */
    }
  while ((status = mu_stream_read (input, buf, sizeof buf, &n)) == 0
	 && n)
    write (fd, buf, n);
}

static int
mime_context_get_temp_file (struct mime_context *ctx, char **ptr)
{
  if (!ctx->temp_file)
    {
      int fd;
      if (mu_tempfile (NULL, 0, &fd, &ctx->temp_file))
	return -1;
      mime_context_write_input (ctx, fd);
      close (fd);
      ctx->unlink_temp_file = 1;
    }
  *ptr = ctx->temp_file;
  return 0;
}


static mu_opool_t expand_pool;

static int
expand_string (struct mime_context *ct, char **pstr)
{
  char *p, *s;
  int rc = 0;
  
  for (p = *pstr; *p; )
    {
      switch (p[0])
	{
	  case '%':
	    switch (p[1])
	      {
	      case 's':
		mime_context_get_temp_file (ct, &s);
		mu_opool_appendz (expand_pool, s);
		rc = 1;
		p += 2;
		break;
		
	      case 't':
		mime_context_get_content_type (ct, &s);
		mu_opool_appendz (expand_pool, s);
		p += 2;
		break;
		
	      case '{':
		{
		  size_t n;
		  char *q;
		  
		  p += 2;
		  q = p;
		  while (*p && *p != '}')
		    p++;
		  if (mime_context_get_content_type_value (ct,
							   q, p-q,
							   &s, &n) == 0)
		    mu_opool_append (expand_pool, s, n);
		  if (*p)
		    p++;
		  break;
		}

	      case 'F':
	      case 'n':
		p++;
		break;
		
	      default:
		mu_opool_append_char (expand_pool, p[0]);
	      }
	    break;

	case '\\':
	  if (p[1])
	    {
	      mu_opool_append_char (expand_pool, p[1]);
	      p += 2;
	    }
	  else
	    {
	      mu_opool_append_char (expand_pool, p[0]);
	      p++;
	    }
	  break;

	case '"':
	  if (p[1] == p[0])
	    {
	      mu_opool_append_char (expand_pool, '%');
	      p++;
	    }
	  else
	    {
	      mu_opool_append_char (expand_pool, p[0]);
	      p++;
	    }
	  break;

	default:
	  mu_opool_append_char (expand_pool, p[0]);
	  p++;
	}
    }
  mu_opool_append_char (expand_pool, 0);
  *pstr = mu_opool_finish (expand_pool, NULL);
  return rc;
}

static int
confirm_action (struct mime_context *ctx, const char *str)
{
  char repl[128], *p;
  int len;
  char *type;

  mime_context_get_content_type (ctx, &type);
  if (dry_run_p (ctx) || !interactive_p (ctx) || mime_context_do_not_ask (ctx))
    return 1;
  
  printf (_("Run `%s'?"), str);
  fflush (stdout);

  p = fgets (repl, sizeof repl, stdin);
  if (!p)
    return 0;
  len = strlen (p);
  if (len > 0 && p[len-1] == '\n')
    p[len--] = 0;
  
  return mu_true_answer_p (p);
}

static void
dump_mailcap_entry (mu_mailcap_entry_t entry)
{
  char buffer[256];
  size_t i, count;
  
  mu_mailcap_entry_get_typefield (entry, buffer, 
				  sizeof (buffer), NULL);
  printf ("typefield: %s\n", buffer);
	  
  /* view-command.  */
  mu_mailcap_entry_get_viewcommand (entry, buffer, 
				    sizeof (buffer), NULL);
  printf ("view-command: %s\n", buffer);

  /* fields.  */
  mu_mailcap_entry_fields_count (entry, &count);
  for (i = 1; i <= count; i++)
    {
      int status = mu_mailcap_entry_get_field (entry, i, buffer, 
					       sizeof (buffer), NULL);
      if (status)
	{
	  mu_error (_("cannot retrieve field %lu: %s"),
		      (unsigned long) i,
		    mu_strerror (status));
	  break;
	}
      printf ("fields[%u]: %s\n", (unsigned) i, buffer);
    }
  printf ("\n");
}

/* Return 1 if CMD needs to be executed via sh -c */
static int
need_shell_p (const char *cmd)
{
  for (; *cmd; cmd++)
    if (strchr ("<>|&", *cmd))
      return 1;
  return 0;
}

static pid_t
create_filter (char *cmd, int outfd, int *infd)
{
  pid_t pid;
  int lp[2];

  if (infd)
    pipe (lp);
  
  pid = fork ();
  if (pid == -1)
    {
      if (infd)
	{
	  close (lp[0]);
	  close (lp[1]);
	}
      mu_error ("fork: %s", mu_strerror (errno));
      return -1;
    }

  if (pid == 0)
    {
      /* Child process */
      struct mu_wordsplit ws;
      int argc;
      char **argv;
      
      if (need_shell_p (cmd))
	{
	  char *x_argv[4];
	  argc = 3;
	  argv = x_argv;
	  argv[0] = getenv ("SHELL");
	  argv[1] = "-c";
	  argv[2] = cmd;
	  argv[3] = NULL;
	}
      else
	{
	  if (mu_wordsplit (cmd, &ws, MU_WRDSF_DEFFLAGS))
	    {
	      mu_error (_("%s failed: %s"), "mu_wordsplit",
			mu_wordsplit_strerror (&ws));
	      _exit (127);
	    }
	  argc = ws.ws_wordc;
	  argv = ws.ws_wordv;
	}      
      /* Create input channel: */
      if (infd)
	{
	  if (lp[0] != 0)
	    dup2 (lp[0], 0);
	  close (lp[1]);
	}
      
      /* Create output channel */
      if (outfd != -1 && outfd != 1)
	dup2 (outfd, 1);
      
      execvp (argv[0], argv);
      mu_error (_("cannot execute `%s': %s"), cmd, mu_strerror (errno));
      _exit (127);
    }

  /* Master process */
  if (infd)
    {
      *infd = lp[1];
      close (lp[0]);
    }
  return pid;
}

static void
print_exit_status (int status)
{
  if (WIFEXITED (status)) 
    printf (_("Command exited with status %d\n"), WEXITSTATUS(status));
  else if (WIFSIGNALED (status)) 
    printf(_("Command terminated on signal %d\n"), WTERMSIG(status));
  else
    printf (_("Command terminated\n"));
}

static char *
get_pager ()
{
  char *pager = getenv ("MIMEVIEW_PAGER");
  if (!pager)
    {
      pager = getenv ("METAMAIL_PAGER");
      if (!pager)
	{
	  pager = getenv ("PAGER");
	  if (!pager)
	    pager = "more";
	}
    }
  return pager;
}

static int
run_test (mu_mailcap_entry_t entry, struct mime_context *ctx)
{
  size_t size;
  int status = 0;
  
  if (mu_mailcap_entry_get_test (entry, NULL, 0, &size) == 0)
    {
      struct mu_wordsplit ws;
      char *str;

      mu_opool_alloc (expand_pool, size + 1);
      str = mu_opool_finish (expand_pool, NULL);
      mu_mailcap_entry_get_test (entry, str, size + 1, NULL);

      expand_string (ctx, &str);
      if (mu_wordsplit (str, &ws, MU_WRDSF_DEFFLAGS))
	{
	  mu_error (_("%s failed: %s"), "mu_wordsplit",
		    mu_wordsplit_strerror (&ws));
	  return 1;
	}
      
      if (mu_spawnvp (ws.ws_wordv[0], ws.ws_wordv, &status))
	status = 1;
      mu_wordsplit_free (&ws);
    }
  return status;
}

static int
run_mailcap (mu_mailcap_entry_t entry, struct mime_context *ctx)
{
  char *view_command;   
  size_t size;          
  int flag;             
  int status;           
  int fd;                
  int *pfd = NULL;      
  int outfd = -1;       
  pid_t pid, pager_pid;
  
  if (ctx->debug_level > 1)
    dump_mailcap_entry (entry);

  if (run_test (entry, ctx))
    return -1;

  if (interactive_p (ctx))
    {
      if (mu_mailcap_entry_get_viewcommand (entry, NULL, 0, &size))
	return 1;
      size++;
      mu_opool_alloc (expand_pool, size);
      view_command = mu_opool_finish (expand_pool, NULL);
      mu_mailcap_entry_get_viewcommand (entry, view_command, size, NULL);
    }
  else
    {
      if (mu_mailcap_entry_get_value (entry, "print", NULL, 0, &size))
	return 1;
      size++;
      mu_opool_alloc (expand_pool, size);
      view_command = mu_opool_finish (expand_pool, NULL);
      mu_mailcap_entry_get_value (entry, "print", view_command, size, NULL);
    }

  /* NOTE: We don't create temporary file for %s, we just use
     mimeview_file instead */
  if (expand_string (ctx, &view_command))
    pfd = NULL;
  else
    pfd = &fd;
  DEBUG (ctx, 0, (_("Executing %s...\n"), view_command));

  if (!confirm_action (ctx, view_command))
    return 1;
    
  flag = 0;
  if (interactive_p (ctx)
      && mu_mailcap_entry_copiousoutput (entry, &flag) == 0 && flag)
    pager_pid = create_filter (get_pager (), -1, &outfd);
  
  pid = create_filter (view_command, outfd, pfd);
  if (pid > 0)
    {
      if (pfd)
	{
	  mime_context_write_input (ctx, fd);
	  close (fd);
	}
	
      while (waitpid (pid, &status, 0) < 0)
	if (errno != EINTR)
	  {
	    mu_error ("waitpid: %s", mu_strerror (errno));
	    break;
	  }
      if (ctx->debug_level)
	print_exit_status (status);
    }
  return 0;
}

static int
find_entry (const char *file, struct mime_context *ctx)
{
  mu_mailcap_t mailcap;
  int status;
  mu_stream_t stream;
  int rc = 1;

  DEBUG (ctx, 2, (_("Trying %s...\n"), file));
  status = mu_file_stream_create (&stream, file, MU_STREAM_READ);
  if (status)
    {
      if (status != ENOENT)
	mu_error ("cannot open input stream %s: %s",
		  file, mu_strerror (status));
      return -1;
    }

  status = mu_mailcap_create (&mailcap, stream);
  if (status == 0)
    {
      size_t i, count = 0;
      char *type;

      mime_context_get_content_type (ctx, &type);
        
      mu_mailcap_entries_count (mailcap, &count);
      for (i = 1; i <= count; i++)
	{
	  mu_mailcap_entry_t entry;
	  char buffer[256];
	  
	  if (mu_mailcap_get_entry (mailcap, i, &entry))
	    continue;
	  
	  /* typefield.  */
	  mu_mailcap_entry_get_typefield (entry,
					  buffer, sizeof (buffer), NULL);
	  
	  if (fnmatch (buffer, type, FNM_CASEFOLD) == 0)
	    {
	      DEBUG (ctx, 2, (_("Found in %s\n"), file));
	      if (run_mailcap (entry, ctx) == 0)
                {
		  rc = 0;
		  break;
		}
	    }
	}
      mu_mailcap_destroy (&mailcap);
    }
  else
    {
      mu_error ("cannot create mailcap for %s: %s",
		file, mu_strerror (status));
    }
  return rc;
}

int
display_stream_mailcap (const char *ident, mu_stream_t stream, mu_header_t hdr,
			const char *no_ask, int interactive, int dry_run,
			int debug_level)
{
  char *mailcap_path, *mailcap_path_tmp = NULL;
  struct mu_wordsplit ws;
  struct mime_context ctx;
  int rc = 1;
  
  if (mime_context_fill (&ctx, ident, stream, hdr,
			 no_ask, interactive, dry_run, debug_level))
    return 1;
  mailcap_path = getenv ("MAILCAP");
  if (!mailcap_path)
    {
      char *home = mu_get_homedir ();
      mailcap_path_tmp = mu_make_file_name_suf (home, ".mailcap:",
						DEFAULT_MAILCAP);
      free (home);
      if (!mailcap_path_tmp)
        return 1;
      mailcap_path = mailcap_path_tmp;
    }
  
  mu_opool_create (&expand_pool, 1);

  ws.ws_delim = ":";
  if (mu_wordsplit (mailcap_path, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    {
      mu_error (_("cannot split line `%s': %s"), mailcap_path,
		mu_wordsplit_strerror (&ws));
    }
  else
    {
      size_t i;

      for (i = 0; i < ws.ws_wordc; i++)
	{
	  if ((rc = find_entry (ws.ws_wordv[i], &ctx)) == 0)
	    break;
	}
      mu_wordsplit_free (&ws);
    }
  mu_opool_destroy (&expand_pool);
  free (mailcap_path_tmp);
  mime_context_release (&ctx);
  return rc;
}
