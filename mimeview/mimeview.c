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
#include <sys/stat.h>

const char *program_version = "mimeview (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mimeview -- display MIME files\
Default mime.types file is ") SYSCONFDIR "/cups/mime.types"
N_("\nDebug flags are:\n\
  g - Mime.types parser traces\n\
  l - Mime.types lexical analyzer traces\n\
  0-9 - Set debugging level\n");

#define OPT_METAMAIL 256

static struct argp_option options[] = {
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
static char *mimetypes_config = SYSCONFDIR "/cups";

char *mimeview_file;   /* Name of the file to view */
FILE *mimeview_fp;     /* Its descriptor */ 

/* Default mailcap path, the $HOME/.mailcap: entry is prepended to it */
#define DEFAULT_MAILCAP \
 "/usr/local/etc/mailcap:"\
 "/usr/etc/mailcap:"\
 "/etc/mailcap:"\
 "/etc/mail/mailcap:"\
 "/usr/public/lib/mailcap"


static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      mimetypes_lex_debug (0);
      mimetypes_gram_debug (0);
      break;

    case ARGP_KEY_FINI:
      if (dry_run && !debug_level)
	debug_level = 1;
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

static void
expand_string (char **pstr, const char *filename, const char *type)
{
  char *p;
  size_t namelen = strlen (filename);
  size_t typelen = strlen (type);

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
}

static int
find_entry (const char *file, const char *type,
	    mu_mailcap_entry_t *pentry,
	    mu_mailcap_t *pmc)
{
  mu_mailcap_t mailcap;
  int status;
  stream_t stream;

  DEBUG (2, (_("Trying %s...\n"), file));
  status = file_stream_create (&stream, file, MU_STREAM_READ);
  if (status)
    {
      mu_error ("cannot create file stream %s: %s",
		file, mu_strerror (status));
      return 0;
    }

  status = stream_open (stream);
  if (status)
    {
      stream_destroy (&stream, stream_get_owner (stream));
      if (status != ENOENT)
	mu_error ("cannot open file stream %s: %s",
		  file, mu_strerror (status));
      return 0;
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
	      /* FIXME: Run test entry, if any */
	      *pmc = mailcap;
	      *pentry = entry;
	      return 1; /* We leave stream open! */
	    }
	}
      mu_mailcap_destroy (&mailcap);
    }
  else
    {
      mu_error ("cannot create mailcap for %s: %s",
		file, mu_strerror (status));
    }
  return 0;
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

int
run_mailcap (mu_mailcap_entry_t entry, const char *type)
{
  char *view_command;
  size_t size;
  int flag;
  int status;
  int argc;
  char **argv;
  /*  pid_t pager = -1; */
  
  if (debug_level > 1)
    dump_mailcap_entry (entry);

  mu_mailcap_entry_get_viewcommand (entry, NULL, 0, &size);
  size++;
  view_command = xmalloc (size);
  mu_mailcap_entry_get_viewcommand (entry, view_command, size, NULL); 

  /* NOTE: We don't create temporary file for %s, we just use
     mimeview_file instead */
  expand_string (&view_command, mimeview_file, type);
  DEBUG (0, (_("Executing %s...\n"), view_command));

  if (dry_run)
    return 0;

  status = argcv_get (view_command, "", NULL, &argc, &argv);
  free (view_command);
  if (status)
    {
      mu_error (_("Cannot parse command line: %s"), mu_strerror (status));
      return 1;
    }
  
  /*
    if (mu_mailcap_entry_coupiousoutput (entry, &flag) == 0 && flag)
    pager = open_pager ();
  */

  if (pager <= 0)
    {
      if (mu_spawnvp (argv[0], argv, &status))
	mu_error (_("Cannot execute command: %s"), mu_strerror (status));

      if (debug_level)
	{
	  if (WIFEXITED (status)) 
	    printf (_("Command exited with status %d\n"), WEXITSTATUS(status));
	  else if (WIFSIGNALED (status)) 
	    printf(_("Command terminated on signal %d\n"), WTERMSIG(status));
	  else
	    printf (_("Command terminated"));
	}
    }
  argcv_free (argc, argv);
  /* close_pager (pager); */
}

void
display_file_mailcap (const char *type)
{
  char *p, *sp;
  char *mailcap_path;
  mu_mailcap_t mailcap = NULL;
  mu_mailcap_entry_t entry = NULL;
  
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
      if (find_entry (p, type, &entry, &mailcap))
	{
	  run_mailcap (entry, type);
	  mu_mailcap_destroy (&mailcap);
	  break;
	}
    }
}

void
display_file (const char *type)
{
  if (metamail)
    {
      int status;
      const char *argv[6];

      argv[0] = "metamail";
      argv[1] = "-b";
      argv[2] = "-c";
      argv[3] = type;
      argv[4] = mimeview_file;
      argv[5] = NULL;
      
      if (debug_level)
	{
	  char *string;
	  argcv_string (5, argv, &string);
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
