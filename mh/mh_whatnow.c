/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2007, 2009-2012 Free Software Foundation,
   Inc.

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

#include <mh.h>

typedef int (*handler_fp) (struct mh_whatnow_env *wh,
			   int argc, char **argv,
			   int *status);

/* ********************* Auxiliary functions *********************** */
/* Auxiliary function for option comparisons */
static char
strnulcmp (const char *str, const char *pattern)
{
  return strncmp (str, pattern, strlen (str));
}

/* Dispatcher functions */

struct action_tab {
  char *name;
  handler_fp fp;
};

static handler_fp
func (struct action_tab *p, const char *name)
{
  int len;
  
  if (!name)
    return func (p, "help");

  len = strlen (name);
  for (; p->name; p++)
    {
      int min = strlen (p->name);
      if (min > len)
	min = len;
      if (strncmp (p->name, name, min) == 0)
	return p->fp;
    }

  mu_error (_("%s is unknown. Hit <CR> for help"), name);
  return NULL;
}

struct helpdata {
  char *name;
  char *descr;
};

/* Functions for printing help information */

#define OPT_DOC_COL  29		/* column in which option text starts */
#define RMARGIN      79		/* right margin used for wrapping */

static int
print_short (const char *str)
{
  int n;
  const char *s;
  
  for (n = 0; *str; str++, n++)
    {
      switch (*str)
	{
	case '+':
	  putchar ('+');
	  s = _("FOLDER");
	  n += printf ("%s", s);
	  break;

	case '<':
	  switch (str[1]) 
	    {
	    case '>':
	      s = _("SWITCHES");
	      n += printf ("%s", s) - 1;
	      str++;
	      break;
	      
	    case 'e':
	      s = _("EDITOR");
	      n += printf ("%s", s) - 1;
	      str++;
	      break;

	    default:
	      putchar (*str);
	    }
	  break;

	default:
	  putchar (*str);
	}
    }
  return n;
}

static void
print_descr (int n, const char *s)
{
  do
    {
      const char *p;
      const char *space = NULL;
      
      for (; n < OPT_DOC_COL; n++)
	putchar (' ');

      for (p = s; *p && p < s + (RMARGIN - OPT_DOC_COL); p++)
	if (mu_isspace (*p))
	  space = p;
      
      if (!space || p < s + (RMARGIN - OPT_DOC_COL))
	{
	  printf ("%s", s);
	  s += strlen (s);
	}
      else
	{
	  for (; s < space; s++)
	    putchar (*s);
	  for (; *s && mu_isspace (*s); s++)
	    ;
	}
      putchar ('\n');
      n = 1;
    }
  while (*s);
}

static int
_help (struct helpdata *helpdata, char *argname)
{
  struct helpdata *p;

  printf ("%s\n", _("Options are:"));
  if (argname == NULL || argname[0] == '?')
    {
      /* Short version */
      for (p = helpdata; p->name; p++)
	{
	  printf ("  ");
	  print_short (p->name);
	  putchar ('\n');
	}
    }
  else
    {
      for (p = helpdata; p->name; p++)
	{
	  int n;
	  
	  n = printf ("  ");
	  n += print_short (p->name);

	  print_descr (n+1, _(p->descr));
	}
    }
  return 0;
}

/* Display the contents of the given file on the terminal */
static void
display_file (const char *name)
{
  const char *pager = mh_global_profile_get ("moreproc", getenv ("PAGER"));

  if (pager)
    mh_spawnp (pager, name);
  else
    {
      mu_stream_t stream;
      int rc;
      size_t n;
      char buffer[512];
      
      rc = mu_file_stream_create (&stream, name, MU_STREAM_READ);
      if (rc)
	{
	  mu_error ("mu_file_stream_create: %s", mu_strerror (rc));
	  return;
	}

      mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
      while (mu_stream_read (stream, buffer, sizeof buffer - 1, &n) == 0
	     && n != 0)
	{
	  buffer[n] = '\0';
	  printf ("%s", buffer);
	}
      mu_stream_destroy (&stream);
    }
}      

static int
check_exit_status (const char *progname, int status)
{
  if (WIFEXITED (status))
    {
      if (WEXITSTATUS (status))
	{
	  mu_error (_("command `%s' exited with status %d"),
		    progname, WEXITSTATUS(status));
	  return 1;
	}
      return 0;
    }
  else if (WIFSIGNALED (status))
    mu_error (_("command `%s' terminated on signal %d"),
	      progname, WTERMSIG (status));
  else
    mu_error (_("command `%s' terminated abnormally"), progname);
  return 1;
}

static int
invoke (const char *compname, const char *defval, int argc, char **argv,
	const char *extra0, const char *extra1)
{
  int i, rc;
  char **xargv;
  const char *progname;
  int status;
  
  progname = mh_global_profile_get (compname, defval);
  if (!progname)
    return -1;
  
  xargv = calloc (argc+3, sizeof (*xargv));
  if (!xargv)
    {
      mh_err_memory (0);
      return -1;
    }

  xargv[0] = (char*) progname;
  for (i = 1; i < argc; i++)
    xargv[i] = argv[i];
  if (extra0)
    xargv[i++] = (char*) extra0;
  if (extra1)
    xargv[i++] = (char*) extra1;
  xargv[i++] = NULL;
  rc = mu_spawnvp (xargv[0], xargv, &status);
  free (xargv);
  return rc ? rc : check_exit_status (progname, status);
}

struct anno_data
{
  const char *field;
  const char *value;
  int date;
};

static int
anno (void *item, void *data)
{
  struct anno_data *d = item; 
  mh_annotate (item, d->field, d->value, d->date);
  return 0;
}

static void
annotate (struct mh_whatnow_env *wh)
{
  mu_message_t msg;
  mu_address_t addr = NULL;
  size_t i, count;
  
  if (!wh->anno_field || !wh->anno_list)
    return;
  
  msg = mh_file_to_message (NULL, wh->file);
  if (!msg)
    return;
  
  mh_expand_aliases (msg, &addr, NULL, NULL);
  mu_address_get_count (addr, &count);
  for (i = 1; i <= count; i++)
    {
      mu_address_t subaddr;

      if (mu_address_get_nth (addr, i, &subaddr) == 0)
	{
	  struct anno_data d;

	  d.field = wh->anno_field;
	  d.date = i == 1;
	  if (mu_address_sget_printable (subaddr, &d.value) == 0)
	    mu_list_foreach (wh->anno_list, anno, &d);
	  mu_address_destroy (&subaddr);
	}
    }
  mu_address_destroy (&addr);
  mu_message_destroy (&msg, NULL);
}


/* ************************ Shell Function ************************* */

static int
_whatnow (struct mh_whatnow_env *wh, struct action_tab *tab)
{
  int rc, status = 0;
  char *line = NULL;
  size_t size = 0;
  struct mu_wordsplit ws;
  int wsflags = MU_WRDSF_DEFFLAGS|MU_WRDSF_COMMENT;
  
  do
    {
      size_t n;
      handler_fp fun;
      
      mu_printf ("%s ", wh->prompt);
      mu_stream_flush (mu_strout);
      rc = mu_stream_getline (mu_strin, &line, &size, &n);
      if (rc)
	{
	  mu_error (_("cannot read input stream: %s"), mu_strerror (rc));
	  status = 1;
	  break;
	}
      if (n == 0)
	break;
      
      ws.ws_comment = "#";
      rc = mu_wordsplit (line, &ws, wsflags);
      if (rc)
	{
	  mu_error (_("cannot split line `%s': %s"), line,
		    mu_wordsplit_strerror (&ws));
	  status = 1;
	  break;
	}
      wsflags |= MU_WRDSF_REUSE;
      fun = func (tab, ws.ws_wordv[0]);
      if (fun)
	rc = fun (wh, ws.ws_wordc, ws.ws_wordv, &status);
      else
	rc = 0;
    }
  while (rc == 0);
  if (wsflags & MU_WRDSF_REUSE)
    mu_wordsplit_free (&ws);
  free (line);
  return status;
}


/* ************************** Actions ****************************** */

/* Display action */
static int
display (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  if (!wh->msg)
    mu_error (_("no alternate message to display"));
  else
    display_file (wh->msg);
  return 0;
}

/* Edit action */
static int
edit (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  char *name;
  
  mu_asprintf (&name, "%s-next", wh->editor);
  invoke (name, wh->editor, argc, argv, wh->file, NULL);
  free (name);
  
  return 0;
}

/* List action */
static int
list (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  if (!wh->file)
    mu_error (_("no draft file to display"));
  else
    display_file (wh->file);
  return 0;
}

/* Push action */
static int
push (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  if (invoke ("sendproc", MHBINDIR "/send", argc, argv, "-push", wh->file) == 0)
    annotate (wh);
  return 0;
}

/* Quit action */
static int
quit (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  *status = 0;

  if (wh->draftfile)
    {
      if (argc == 2 && strnulcmp (argv[1], "-delete") == 0)
	unlink (wh->draftfile);
      else
	{
	  mu_printf (_("draft left on \"%s\"."), wh->draftfile);
	  if (strcmp (wh->file, wh->draftfile))
	    rename (wh->file, wh->draftfile);
	}
    }
  mu_printf ("\n");
  return 1;
}

/* Refile action */
static int
refile (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  invoke ("fileproc", MHBINDIR "/refile", argc, argv, "-file", wh->file);
  return 0;
}

/* Send action */
static int
call_send (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  if (invoke ("sendproc", MHBINDIR "/send", argc, argv, wh->file, NULL) == 0)
    {
      annotate (wh);
      return 1;
    }
  return 0;
}

/* Whom action */
static int
whom (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  if (!wh->file)
    mu_error (_("no draft file to display"));
  else
    mh_whom (wh->file, (argc == 2
			&& (strcmp (argv[1], "-check") == 0
			    || strcmp (argv[1], "--check") == 0)));
  return 0;
}

/* Help table for whatnow */
static struct helpdata whatnow_helptab[] = {
  { "display [<>]",
    N_("List the message being distributed/replied-to on the terminal.") },
  { "edit [<e <>]",
    N_("Edit the message. If EDITOR is omitted use the one that was used on"
       " the preceding round unless the profile entry \"LASTEDITOR-next\""
       " names an alternate editor.") },
  { "list [<>]",
    N_("List the draft on the terminal.") },
  { "push [<>]",
    N_("Send the message in the background.") },
  { "quit [-delete]",
    N_("Terminate the session. Preserve the draft, unless -delete flag is given.") },
  { "refile [<>] +",
    N_("Refile the draft into the given FOLDER.") },
  { "send [-watch] [<>]",
    N_("Send the message. The -watch flag causes the delivery process to be "
       "monitored. SWITCHES are passed to send program verbatim.") },
  { "whom [-check] [<>]",
    N_("List the addresses and verify that they are acceptable to the "
       "transport service.") },
  { NULL },
};

/* Help action for whatnow shell */
static int
whatnow_help (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  return _help (whatnow_helptab, argv[0]);
}

/* Actions specific for the ``disposition'' shell */

/* Help table for ``disposition'' shell */
static struct helpdata disposition_helptab[] = {
  { "quit",           N_("Terminate the session. Preserve the draft.") },
  { "replace",        N_("Replace the draft with the newly created one") },
  { "use",            N_("Use this draft") },
  { "list",           N_("List the draft on the terminal.") },
  { "refile [<>] +",  N_("Refile the draft into the given FOLDER.") },
  { NULL },
};

/* Help action */
static int
disp_help (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  return _help (disposition_helptab, argv[0]);
}

/* Use action */
static int
use (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  *status = DISP_USE;
  return 1;
}

/* Replace action */
static int
replace (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  *status = DISP_REPLACE;
  return 1;
}


/* *********************** Interfaces ****************************** */

/* Whatnow shell */
static struct action_tab whatnow_tab[] = {
  { "help", whatnow_help },
  { "?", whatnow_help },
  { "display", display },
  { "edit", edit },
  { "list", list },
  { "push", push },
  { "quit", quit },
  { "refile", refile },
  { "send", call_send },
  { "whom", whom },
  { NULL }
};

static void
set_default_editor (struct mh_whatnow_env *wh)
{
  if (!wh->editor)
    {
      char *p;
      wh->editor = mh_global_profile_get ("Editor",
					  (p = getenv ("VISUAL")) ?
					    p : (p = (getenv ("EDITOR"))) ?
					          p : "prompter");	  
    }
}

int
mh_whatnow (struct mh_whatnow_env *wh, int initial_edit)
{
  set_default_editor (wh);
  
  if (initial_edit)
    mh_spawnp (wh->editor, wh->file);

  if (!wh->prompt)
    wh->prompt = (char*) _("What now?");
  
  return _whatnow (wh, whatnow_tab);
}

int
mh_whatnowproc (struct mh_whatnow_env *wh, int initial_edit, const char *prog)
{
  int rc;
  pid_t pid;
  
  if (!prog)
    return mh_whatnow (wh, initial_edit);

  pid = fork ();
  if (pid == -1)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "fork", NULL, errno);
      return 1;
    }
  
  if (pid == 0)
    {
      struct mu_wordsplit ws;
      int i;
      
      if (mu_wordsplit (prog, &ws,
			MU_WRDSF_DEFFLAGS & ~MU_WRDSF_CESCAPES))
	{
	  mu_error (_("cannot parse command line (%s): %s"), prog,
		    mu_wordsplit_strerror (&ws));
	  _exit (127);
	}
      
      set_default_editor (wh);
      mh_whatnow_env_to_environ (wh);
      for (i = mu_getmaxfd (); i > 2; i--)
	close (i);
      execvp (ws.ws_wordv[0], ws.ws_wordv);
      mu_diag_funcall (MU_DIAG_ERROR, "execvp", prog, errno);
      _exit (127);
    }

  /* Master */
  rc = 0;
  while (1)
    {
      int status;
      
      if (waitpid (pid, &status, 0) == (pid_t)-1)
	{
	  if (errno == EINTR)
	    continue;
	  mu_diag_funcall (MU_DIAG_ERROR, "waitpid", prog, errno);
	  rc = 1;
	}
      break;
    }
  return rc;
}

/* Disposition shell */
static struct action_tab disp_tab[] = {
  { "help", disp_help },
  { "?", disp_help },
  { "quit", quit },
  { "replace", replace },
  { "use", use },
  { "list", list },
  { "refile",  refile },
  { NULL }
};
    
int
mh_disposition (const char *filename)
{
  struct mh_whatnow_env wh;
  int rc;
  memset (&wh, 0, sizeof (wh));
  wh.file = mu_strdup (filename);
  wh.prompt = (char*) _("Disposition?");
  rc = _whatnow (&wh, disp_tab);
  free (wh.file);
  return rc;
}

/* Use draft shell */

/* Help table for ``use draft'' shell */
static struct helpdata usedraft_helptab[] = {
  { "no",             N_("Don't use the draft.") },
  { "yes",            N_("Use the draft.") },
  { "list",           N_("List the draft on the terminal.") },
  { NULL },
};

static int
usedraft_help (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  return _help (usedraft_helptab, argv[0]);
}

static int
yes (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  *status = 1;
  return 1;
}

static int
no (struct mh_whatnow_env *wh, int argc, char **argv, int *status)
{
  *status = 0;
  return 1;
}

static struct action_tab usedraft_tab[] = {
  { "help", usedraft_help },
  { "?", usedraft_help },
  { "yes", yes },
  { "no", no },
  { "list", list },
  { NULL }
};

int
mh_usedraft (const char *filename)
{
  struct mh_whatnow_env wh;
  int rc;
  
  memset (&wh, 0, sizeof (wh));
  wh.file = mu_strdup (filename);
  mu_asprintf (&wh.prompt, _("Use \"%s\"?"), filename);
  rc = _whatnow (&wh, usedraft_tab);
  free (wh.prompt);
  free (wh.file);
  return rc;
}

