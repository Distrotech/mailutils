/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mimeview.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

const char *program_version = "mimeview (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mimeview -- display files, using mailcap mechanism.\
Default mime.types file is ") DEFAULT_CUPS_CONFDIR "/mime.types"
N_("\n\nDebug flags are:\n\
  g - Mime.types parser traces\n\
  l - Mime.types lexical analyzer traces\n\
  0-9 - Set debugging level\n");

#define OPT_METAMAIL 256

static struct argp_option options[] = {
  {"no-ask", 'a', N_("TYPE-LIST"), OPTION_ARG_OPTIONAL,
   N_("Do not ask any questions before running an interpreter to view messages of given types. TYPE-LIST is a comma-separated list of MIME types. If TYPE-LIST is not given, do not ask any questions at all"), 0},
  {"no-interactive", 'h', NULL, 0,
   N_("Disable interactive mode"), 0 },
  {"print", 0, NULL, OPTION_ALIAS, NULL, 0 },
  {"debug",  'd', N_("FLAGS"),  OPTION_ARG_OPTIONAL,
   N_("Enable debugging output"), 0},
  {"mimetypes", 't', N_("FILE"), 0,
   N_("Use this mime.types file"), 0},
  {"dry-run", 'n', NULL, 0,
   N_("Do not do anything, just print what whould be done"), 0},
  {"metamail", OPT_METAMAIL, N_("FILE"), OPTION_ARG_OPTIONAL,
   N_("Use metamail to display files"), 0},
  {0, 0, 0, 0}
};

int debug_level;       /* Debugging level set by --debug option */
static int dry_run;    /* Dry run mode */
static char *metamail; /* Name of metamail program, if requested */
static char *mimetypes_config = DEFAULT_CUPS_CONFDIR;
static list_t no_ask_list;  /* List of MIME types for which no questions
			       should be asked */
static int interactive = -1; 
char *mimeview_file;   /* Name of the file to view */
FILE *mimeview_fp;     /* Its descriptor */ 

/* Default mailcap path, the $HOME/.mailcap: entry is prepended to it */
#define DEFAULT_MAILCAP \
 "/usr/local/etc/mailcap:"\
 "/usr/etc/mailcap:"\
 "/etc/mailcap:"\
 "/etc/mail/mailcap:"\
 "/usr/public/lib/mailcap"

static void
parse_typelist (const char *str)
{
  char *tmp = xstrdup (str);
  char *p, *sp;

  for (p = strtok_r (tmp, ",", &sp); p; p = strtok_r (NULL, ",", &sp))
    list_append (no_ask_list, xstrdup (p));
  free (tmp);
}

static int
match_typelist (const char *type)
{
  int rc = 0;
  
  if (no_ask_list)
    {
      iterator_t itr;
      list_get_iterator (no_ask_list, &itr);
      for (iterator_first (itr); !rc && !iterator_is_done (itr);
	   iterator_next (itr))
	{
	  char *p;
	  iterator_current (itr, (void**)&p);
	  rc = fnmatch (p, type, FNM_CASEFOLD) == 0;
	}
      iterator_destroy (&itr);
    }
  return rc;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      mimetypes_lex_debug (0);
      mimetypes_gram_debug (0);
      if (interactive == -1)
	interactive = isatty (fileno (stdin));
      break;

    case ARGP_KEY_FINI:
      if (dry_run && !debug_level)
	debug_level = 1;
      break;

    case 'a':
      list_create (&no_ask_list);
      parse_typelist (arg ? arg : "*");
      setenv ("MM_NOASK", arg, 1); /* In case we are given --metamail option */
      break;
      
    case 'd':
      if (!arg)
	arg = "9";
      for (; *arg; arg++)
	{
	  switch (*arg)
	    {
	    case 'l':
	      mimetypes_lex_debug (1);
	      break;

	    case 'g':
	      mimetypes_gram_debug (1);
	      break;

	    default:
	      debug_level = *arg - '0';
	    }
	}
      break;

    case 'h':
      interactive = 0;
      break;
      
    case 'n':
      dry_run = 1;
      break;
      
    case 't':
      mimetypes_config = arg;
      break;

    case OPT_METAMAIL:
      metamail = arg ? arg : "metamail";
      break;
      
    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("FILE [FILE ...]"),
  doc,
  NULL,
  NULL, NULL
};

static const char *capa[] = {
  "common",
  "license",
  NULL
};

static int
open_file (char *name)
{
  struct stat st;

  if (stat (name, &st))
    {
      mu_error (_("Cannot stat `%s': %s"), name, mu_strerror (errno));
      return -1;
    }
  if (!S_ISREG (st.st_mode) && !S_ISLNK (st.st_mode))
    {
      mu_error (_("Not a regular file or symbolic link: `%s'"), name);
      return -1;
    }

  mimeview_file = name;
  mimeview_fp = fopen (name, "r");
  if (mimeview_fp == NULL)
    {
      mu_error (_("Cannot open `%s': %s"), name, mu_strerror (errno));
      return -1;
    }
  return 0;
}

void
close_file ()
{
  fclose (mimeview_fp);
}

static struct obstack expand_stack;

static int
expand_string (char **pstr, const char *filename, const char *type)
{
  char *p;
  size_t namelen = strlen (filename);
  size_t typelen = strlen (type);
  int rc = 0;
  
  for (p = *pstr; *p; )
    {
      switch (p[0])
	{
	  case '%':
	    switch (p[1])
	      {
	      case 's':
		obstack_grow (&expand_stack, filename, namelen);
		p += 2;
		rc = 1;
		break;
		
	      case 't':
		obstack_grow (&expand_stack, type, typelen);
		p += 2;
		break;
		
	      case '{':
		/* Hmm, we don't have content-type field, sorry... */
		while (*p && *p != '}')
		  p++;
		if (*p)
		  p++;
		break;
		
		/* FIXME: Handle %F and %n */
	      default:
		obstack_1grow (&expand_stack, p[0]);
	      }
	    break;

	case '\\':
	  if (p[1])
	    {
	      obstack_1grow (&expand_stack, p[1]);
	      p += 2;
	    }
	  else
	    {
	      obstack_1grow (&expand_stack, p[0]);
	      p++;
	    }
	  break;

	case '"':
	  if (p[1] == p[0])
	    {
	      obstack_1grow (&expand_stack, '%');
	      p++;
	    }
	  else
	    {
	      obstack_1grow (&expand_stack, p[0]);
	      p++;
	    }
	  break;

	default:
	  obstack_1grow (&expand_stack, p[0]);
	  p++;
	}
    }
  obstack_1grow (&expand_stack, 0);
  free (*pstr);
  *pstr = obstack_finish (&expand_stack);
  return rc;
}

static int
confirm_action (const char *type, const char *str)
{
  char repl[128], *p;
  int len;

  if (!interactive || match_typelist (type))
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
      printf ("fields[%d]: %s\n", i, buffer);
    }
  printf ("\n");
}

pid_t
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
      int argc;
      char **argv;

      argcv_get (cmd, "", NULL, &argc, &argv);
      
      /* Create input channel: */
      if (lp[0] != 0)
	dup2 (lp[0], 0);
      close (lp[1]);
      
      /* Create output channel */
      if (outfd != -1 && outfd != 1)
	dup2 (outfd, 1);

      execvp (argv[0], argv);
      mu_error (_("Cannot execute `%s': %s"), cmd, mu_strerror (errno));
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

void
print_exit_status (int status)
{
  if (WIFEXITED (status)) 
    printf (_("Command exited with status %d\n"), WEXITSTATUS(status));
  else if (WIFSIGNALED (status)) 
    printf(_("Command terminated on signal %d\n"), WTERMSIG(status));
  else
    printf (_("Command terminated"));
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
run_test (mu_mailcap_entry_t entry, const char *type)
{
  size_t size;
  int status = 0;
  
  if (mu_mailcap_entry_get_test (entry, NULL, 0, &size) == 0)
    {
      int argc;
      char **argv;
      char *str = xmalloc (size + 1);
      mu_mailcap_entry_get_test (entry, str, size + 1, NULL);

      expand_string (&str, mimeview_file, type);
      argcv_get (str, "", NULL, &argc, &argv);
      free (str);
      
      if (mu_spawnvp (argv[0], argv, &status))
	status = 1;
      argcv_free (argc, argv);
    }
  return status;
}

int
run_mailcap (mu_mailcap_entry_t entry, const char *type)
{
  char *view_command;   
  size_t size;          
  int flag;             
  int status;           
  int fd;                
  int *pfd = NULL;      
  int outfd = -1;       
  pid_t pid, pager_pid;
  
  if (debug_level > 1)
    dump_mailcap_entry (entry);

  if (run_test (entry, type))
    return -1;

  if (interactive)
    {
      mu_mailcap_entry_get_viewcommand (entry, NULL, 0, &size);
      size++;
      view_command = xmalloc (size);
      mu_mailcap_entry_get_viewcommand (entry, view_command, size, NULL);
    }
  else
    {
      if (mu_mailcap_entry_get_value (entry, "print", NULL, 0, &size))
	return 1;
      size++;
      view_command = xmalloc (size);
      mu_mailcap_entry_get_value (entry, "print", view_command, size, NULL);
    }

  /* NOTE: We don't create temporary file for %s, we just use
     mimeview_file instead */
  if (expand_string (&view_command, mimeview_file, type))
    pfd = NULL;
  else
    pfd = &fd;
  DEBUG (0, (_("Executing %s...\n"), view_command));

  if (dry_run || !confirm_action (type, view_command))
    {
      free (view_command);
      return 1;
    }

  flag = 0;
  if (interactive
      && mu_mailcap_entry_copiousoutput (entry, &flag) == 0 && flag)
    pager_pid = create_filter (get_pager (), -1, &outfd);
  
  pid = create_filter (view_command, outfd, pfd);
  if (pid > 0)
    {
      if (pfd)
	{
	  char buf[512];
	  size_t n;
	
	  fseek (mimeview_fp, 0, SEEK_SET);
	  while ((n = fread (buf, 1, sizeof buf, mimeview_fp)) > 0)
	    {
	      write (fd, buf, n);
	    }
	  close (fd);
	}
	
      while (waitpid (pid, &status, 0) < 0)
	if (errno != EINTR)
	  {
	    mu_error ("waitpid: %s", mu_strerror (errno));
	    break;
	  }
      if (debug_level)
	print_exit_status (status);
    }
  free (view_command);
  return 0;
}

static int
find_entry (const char *file, const char *type)
{
  mu_mailcap_t mailcap;
  int status;
  stream_t stream;
  int rc = 1;
  
  DEBUG (2, (_("Trying %s...\n"), file));
  status = file_stream_create (&stream, file, MU_STREAM_READ);
  if (status)
    {
      mu_error ("cannot create file stream %s: %s",
		file, mu_strerror (status));
      return -1;
    }

  status = stream_open (stream);
  if (status)
    {
      stream_destroy (&stream, stream_get_owner (stream));
      if (status != ENOENT)
	mu_error ("cannot open file stream %s: %s",
		  file, mu_strerror (status));
      return -1;
    }

  status = mu_mailcap_create (&mailcap, stream);
  if (status == 0)
    {
      size_t i, count = 0;
      
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
	      DEBUG (2, (_("Found in %s\n"), file));
	      if (run_mailcap (entry, type) == 0)
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

void
display_file_mailcap (const char *type)
{
  char *p, *sp;
  char *mailcap_path;
  
  mailcap_path = getenv ("MAILCAP");
  if (!mailcap_path)
    {
      char *home = mu_get_homedir ();
      asprintf (&mailcap_path, "%s/.mailcap:%s", home, DEFAULT_MAILCAP);
    }
  else
    mailcap_path = strdup (mailcap_path);

  obstack_init (&expand_stack);

  for (p = strtok_r (mailcap_path, ":", &sp); p; p = strtok_r (NULL, ":", &sp))
    {
      if (find_entry (p, type) == 0)
	break;
    }
}

void
display_file (const char *type)
{
  if (metamail)
    {
      int status;
      const char *argv[7];
      
      argv[0] = "metamail";
      argv[1] = "-b";

      argv[2] = interactive ? "-p" : "-h";
      
      argv[3] = "-c";
      argv[4] = type;
      argv[5] = mimeview_file;
      argv[6] = NULL;
      
      if (debug_level)
	{
	  char *string;
	  argcv_string (6, argv, &string);
	  printf (_("Executing %s...\n"), string);
	  free (string);
	}
      
      if (!dry_run)
	mu_spawnvp (metamail, argv, &status);
    }
  else
    display_file_mailcap (type);
}

int
main (int argc, char **argv)
{
  int index;
  
  mu_init_nls ();
  mu_argp_init (program_version, NULL);
  mu_argp_parse (&argp, &argc, &argv, 0, capa, &index, NULL);

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      mu_error (_("No files given"));
      return 1;
    }

  if (mimetypes_parse (mimetypes_config))
    return 1;
  
  while (argc--)
    {
      const char *type;
      
      if (open_file (*argv++))
	continue;
      type = get_file_type ();
      DEBUG (1, ("%s: %s\n", mimeview_file, type ? type : "?"));
      if (type)
	display_file (type);
      close_file ();
    }
  
  return 0;
}
