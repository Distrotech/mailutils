/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 
   2004, 2005, 2006, 2007, 2009 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "mail.h"
#include <mailutils/mutil.h>
#include <mailutils/mime.h>
#include <pwd.h>
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# include <sys/fcntl.h>
#endif
#include <mu_asprintf.h>

mu_list_t environment = NULL;

/*
 * expands command into its command and arguments, then runs command
 * cmd is the command to parse and run
 * returns exit status of the command
 */
int
util_do_command (const char *c, ...)
{
  int argc = 0;
  char **argv = NULL;
  int status = 0;
  const struct mail_command_entry *entry = NULL;
  char *cmd = NULL;
  va_list ap;
  
  va_start (ap, c);
  status = vasprintf (&cmd, c, ap);
  va_end (ap);
  if (status < 0)
    return 0;

  if (cmd)
    {
      /*  Ignore comments */
      if (cmd[0] == '#')
	{
	  free (cmd);
	  return 0;
	}

      if (cmd[0] == '\0')
	{
	  free (cmd);
	  
	  /* Hitting return i.e. no command, is equivalent to next
	     according to the POSIX spec. Note, that this applies
	     to interactive state only. */
	  if (interactive)
	    cmd = strdup ("next");
	  else
	    return 0;
	}

      if (mu_argcv_get (cmd, NULL, NULL, &argc, &argv) == 0 && argc > 0)
	{
	  char *p;

	  /* Special case: a number alone implies "print" */
	  if (argc == 1
	      && ((strtoul (argv[0], &p, 10) > 0 && *p == 0)
		  || (argv[0][1] == 0 && strchr("^$", argv[0][0]))))
	    {
	      asprintf (&p, "print %s", argv[0]);
	      mu_argcv_free (argc, argv);
	      mu_argcv_get (p, NULL, NULL, &argc, &argv);
	      free (p);
	    }

	  entry = mail_find_command (argv[0]);
	}
      free (cmd);
    }
  else
    entry = mail_find_command ("quit");

  if (!entry)
    {
      /* argv[0] might be a traditional /bin/mail contracted form, e.g.
	 `d*' or `p4'. */
	 
      char *p;
      
      for (p = argv[0] + strlen (argv[0]) - 1;
	   p > argv[0] && !mu_isalpha (*p);
	   p--)
	;

      p++;
      
      if (strlen (p))
	{
	  argc++;
	  argv = xrealloc (argv, (argc + 1)*sizeof argv[0]);
	  if (argc > 2)
	    memmove (argv + 2, argv + 1, (argc - 2)*sizeof argv[0]);
	  argv[1] = xstrdup (p);
	  *p = 0;
	}
      
      entry = mail_find_command (argv[0]);
    }
  
  if (entry)
    {
      /* Make sure we are not in any if/else */
      if (!(if_cond () == 0 && (entry->flags & EF_FLOW) == 0))
	status = entry->func (argc, argv);
    }
  else
    {
      if (argc)
	util_error (_("Unknown command: %s"), argv[0]);
      else
	util_error (_("Invalid command"));
      status = 1;
    }

  mu_argcv_free (argc, argv);
  return status;
}

int
util_foreach_msg (int argc, char **argv, int flags,
		  msg_handler_t func, void *data)
{
  msgset_t *list = NULL, *mp;
  int status = 0;

  if (msgset_parse (argc, argv, flags, &list))
      return 1;

  for (mp = list; mp; mp = mp->next)
    {
      mu_message_t mesg;

      if (util_get_message (mbox, mp->msg_part[0], &mesg) == 0)
	{
	  if (func (mp, mesg, data) != 0)
	    status = 1;
	  /* Bail out if we receive an interrupt.  */
	  if (ml_got_interrupt () != 0)
	    break;
	}
    }
  msgset_free (list);

  return status;
}

size_t
util_range_msg (size_t low, size_t high, int flags, 
		msg_handler_t func, void *data)
{
  msgset_t msgspec = { 0 };
  size_t count, expect_count;
  
  msgspec.next = NULL;
  msgspec.npart = 0;
  msgspec.msg_part = &low;
  if (!func)
    flags |= MSG_SILENT;

  if (low > total)
    return 0;
  if (!(flags & MSG_COUNT))
    {
      if (high < low)
	return 0;
      expect_count = high - low + 1;
    }
  else
    expect_count = high;

  for (count = 0; count < expect_count && low <= total; low++)
    {
     mu_message_t mesg;

     if ((flags & MSG_NODELETED) && util_isdeleted (low))
       {
	 if (!(flags & MSG_SILENT))
	   util_error (_("%lu: Inappropriate message (has been deleted)"),
		       (unsigned long) low);
	 continue;
       }
     
     if (util_get_message (mbox, low, &mesg) == 0)
       {
	 count ++;
	 if (func)
	   func (&msgspec, mesg, data) ;
	 /* Bail out if we receive an interrupt.  */
	 if (ml_got_interrupt () != 0)
	   break;
       }
    }
  return count;
}

/*
 * returns the function to run for command
 */
function_t *
util_command_get (const char *cmd)
{
  const struct mail_command_entry *entry = mail_find_command (cmd);
  return entry ? entry->func : NULL;
}

/*
 * returns the mail_command_entry structure for the command matching cmd
 */
void *
util_find_entry (void *table, size_t nmemb, size_t size, const char *cmd)
{
  int i;
  int len = strlen (cmd);
  char *p;
  
  for (p = table, i = 0; i < nmemb; i++, p += size)
    {
      struct mail_command *cp = (struct mail_command *)p;
      int ll = strlen (cp->longname);
      int sl = strlen (cp->shortname);
      
      if (sl > ll && !strncmp (cp->shortname, cmd, sl))
	return p;
      else if (sl == len && !strcmp (cp->shortname, cmd))
	return p;
      else if (sl < len && !strncmp (cp->longname, cmd, len))
	return p;
    }
  return NULL;
}

int
util_help (void *table, size_t nmemb, size_t size, const char *word)
{
  if (!word)
    {
      int i = 0;
      FILE *out = stdout;
      char *p;

      if (util_getenv (NULL, "crt", Mail_env_boolean, 0) == 0)
	out = popen (getenv ("PAGER"), "w");

  
      for (p = table, i = 0; i < nmemb; i++, p += size)
	{
	  struct mail_command *cp = (struct mail_command *)p;
	  fprintf (out, "%s\n", cp->synopsis);
	}
      
      if (out != stdout)
	pclose (out);

      return 0;
    }
  else
    {
      int status = 0;
      struct mail_command *cp = util_find_entry (table, nmemb, size, word);
      if (cp)
	fprintf (stdout, "%s\n", cp->synopsis);
      else
	{
	  status = 1;
	  fprintf (stdout, _("Unknown command: %s\n"), word);
	}
      return status;
    }
  return 1;
}

int
util_command_list (void *table, size_t nmemb, size_t size)
{
  int i;
  char *p;
  int cols = util_getcols ();
  int pos;
  
  for (p = table, i = 0; i < nmemb; i++, p += size)
    {
      const char *cmd;
      struct mail_command *cp = (struct mail_command *)p;
      int len = strlen (cp->longname);
      if (len < 1)
	{
	  cmd = cp->shortname;
	  len = strlen (cmd);
	}
      else
	cmd = cp->longname;

      pos += len + 1;

      if (pos >= cols)
	{
	  pos = len + 1;
	  fprintf (ofile, "\n%s ", cmd);
	}
      else
        fprintf (ofile, "%s ", cmd);
    }
  fprintf (ofile, "\n");
  return 0;
}

/*
 * removes whitespace from the beginning and end of a string
 */
char *
util_stripwhite (char *string)
{
  register char *s, *t;
  for (s = string; mu_isspace ((unsigned)*s); s++)
    ;
  if (*s == 0)
    return s;
  t = s + strlen (s) - 1;
  while (t > s && mu_isspace ((unsigned)*t))
    t--;
  *++t = '\0';
  return s;
}

/*
 * Get the number of columns on the screen
 * First try an ioctl() call not all shells set the COLUMNS environ.
 */
int
util_getcols (void)
{
  struct winsize ws;

  ws.ws_col = ws.ws_row = 0;
  if ((ioctl(1, TIOCGWINSZ, (char *) &ws) < 0) || ws.ws_row == 0)
    {
      const char *columns = getenv ("COLUMNS");
      if (columns)
	ws.ws_col = strtol (columns, NULL, 10);
    }

  /* FIXME: Should we exit()/abort() if col <= 0 ?  */
  return ws.ws_col;
}

/*
 * Get the number of lines on the screen
 * First try an ioctl() call not all shells set the LINES environ.
 */
int
util_getlines (void)
{
  struct winsize ws;

  ws.ws_col = ws.ws_row = 0;
  if ((ioctl(1, TIOCGWINSZ, (char *) &ws) < 0) || ws.ws_row == 0)
    {
      const char *lines = getenv ("LINES");
      if (lines)
	ws.ws_row = strtol (lines, NULL, 10);
    }

  /* FIXME: Should we exit()/abort() if row <= 0 ?  */

  /* Reserve at least 2 line for the prompt.  */
  return ws.ws_row - 2;
}

int
util_screen_lines ()
{
  int screen;
  size_t n;

  if (util_getenv (&screen, "screen", Mail_env_number, 0) == 0)
    return screen;
  n = util_getlines();
  util_do_command ("set screen=%d", n);
  return n;
}

int
util_screen_columns ()
{
  int cols;
  size_t n;

  if (util_getenv (&cols, "columns", Mail_env_number, 0) == 0)
    return cols;
  n = util_getcols();
  util_do_command ("set columns=%d", n);
  return n;
}

int
util_get_crt ()
{
  int lines;

  if (util_getenv (&lines, "crt", Mail_env_number, 0) == 0)
    return lines;
  else if (util_getenv (NULL, "crt", Mail_env_boolean, 0) == 0)
    return util_getlines ();
  return 0;
}

/* Functions for dealing with internal environment variables */

/* Retrieve the value of a specified variable of given type.
   The value is stored in the location pointed to by PTR variable.
   VARIABLE and TYPE specify the variable name and type. If the
   variable is not found and WARN is not null, the warning message
   is issued.

   Return value is 0 if the variable is found, 1 otherwise.
   If PTR is not NULL, it must point to

   int           if TYPE is Mail_env_number or Mail_env_boolean
   const char *  if TYPE is Mail_env_string. 

   Passing PTR=NULL may be used to check whether the variable is set
   without retrieving its value. */
   
int
util_getenv (void *ptr, const char *variable, mail_env_data_t type, int warn)
{
  struct mail_env_entry *env = util_find_env (variable, 0);

  if (!mail_env_entry_is_set (env) || env->type != type)
    {
      if (warn)
	util_error (_("No value set for \"%s\""), variable);
      return 1;
    }
  if (ptr)
    switch (type)
      {
      case Mail_env_string:
	*(char**)ptr = env->value.string;
	break;

      case Mail_env_number:
	*(int*)ptr = env->value.number;
	break;

      case Mail_env_boolean:
	*(int*)ptr = env->value.bool;
	break;

      default:
	break;
      }
	
  return 0;
}

static int
env_comp (const void *a, const void *b)
{
  const struct mail_env_entry *epa = a;
  const struct mail_env_entry *epb = b;

  return strcmp (epa->var, epb->var);
}

/* Find environment entry var. If not found and CREATE is not null, then
   create the (unset and untyped) variable */
struct mail_env_entry *
util_find_env (const char *var, int create)
{
  struct mail_env_entry entry, *p;

  if (strcmp (var, "ask") == 0)
    entry.var = "asksub";
  else
    entry.var = var;
  
  if (environment == NULL)
    {
      mu_list_create (&environment);
      mu_list_set_comparator (environment, env_comp);
    }
  
  if (mu_list_locate (environment, &entry, (void**)&p))
    {
      if (!create)
	return 0;
	
      p = xmalloc (sizeof *p);
      p->var = xstrdup (entry.var);
      mu_list_prepend (environment, p);
      p->set = 0;
      p->type = Mail_env_whatever;
      p->value.number = 0;
    }

  return p;
}

struct var_iterator
{
  const char *prefix;
  int prefixlen;
  mu_iterator_t itr;
};
  
const char *
var_iterate_next (var_iterator_t itr)
{
  struct mail_env_entry *ep;
  
  while (!mu_iterator_is_done (itr->itr))
    {
      if (mu_iterator_current (itr->itr, (void **)&ep))
	return NULL;
      mu_iterator_next (itr->itr);
  
      if (strlen (ep->var) >= itr->prefixlen
	  && strncmp (ep->var, itr->prefix, itr->prefixlen) == 0)
	return ep->var;
    }
  return NULL;
}

const char *
var_iterate_first (const char *prefix, var_iterator_t *pitr)
{
  if (environment)
    {
      var_iterator_t itr = xmalloc (sizeof *itr);
      itr->prefix = prefix;
      itr->prefixlen = strlen (prefix);
      mu_list_get_iterator (environment, &itr->itr);
      mu_iterator_first (itr->itr);
      *pitr = itr;
      return var_iterate_next (itr);
    }
  *pitr = NULL;
  return NULL;
}

void
var_iterate_end (var_iterator_t *itr)
{
  mu_iterator_destroy (&(*itr)->itr);
  free (*itr);
  *itr = NULL;
}

/* print the environment */
static int
envp_comp (const void *a, const void *b)
{
  struct mail_env_entry * const *epa = a;
  struct mail_env_entry * const *epb = b;

  return strcmp ((*epa)->var, (*epb)->var);
}

void
util_printenv (int set)
{
  struct mail_env_entry **ep;
  size_t i, count = 0;
  
  mu_list_count (environment, &count);
  ep = xcalloc (count, sizeof *ep);
  mu_list_to_array (environment, (void**) ep, count, NULL);
  qsort (ep, count, sizeof *ep, envp_comp);
  for (i = 0; i < count; i++)
    {
      switch (ep[i]->type)
	{
	case Mail_env_number:
	  fprintf (ofile, "%s=%d", ep[i]->var, ep[i]->value.number);
	  break;
	  
	case Mail_env_string:
	  fprintf (ofile, "%s=\"%s\"", ep[i]->var, ep[i]->value.string);
	  break;
	  
	case Mail_env_boolean:
	  if (!ep[i]->value.bool)
	    fprintf (ofile, "no");
	  fprintf (ofile, "%s", ep[i]->var);
	  break;
	  
	case Mail_env_whatever:
	  fprintf (ofile, "%s %s", ep[i]->var, _("oops?"));
	}
      fprintf (ofile, "\n");
    }
  free (ep);
}

/* Initialize environment entry: clear set indicator and free any memory
   associated with the data */
void
util_mail_env_free (struct mail_env_entry *ep)
{
  if (!mail_env_entry_is_set (ep))
    return;
  
  switch (ep->type)
    {
    case Mail_env_string:
      free (ep->value.string);
      ep->value.string = NULL;
      break;
	      
    default:
      break;
    }
  ep->set = 0;
}

/* Set environement
   The  util_setenv() function adds the variable name to the envi-
   ronment with the value value, if  name  does  not  already
   exist.   If  name  does exist in the environment, then its
   value is changed to value if  overwrite  is  non-zero;  if
   overwrite  is zero, then the value of name is not changed.
 
   A side effect of the code is if value is null the variable name
   will be unset. */
int
util_setenv (const char *variable, void *value, mail_env_data_t type,
	     int overwrite)
{
  struct mail_env_entry *ep =  util_find_env (variable, 1);

  if (ep->set && !overwrite)
    return 0;

  util_mail_env_free (ep);
  
  ep->type = type;
  if (value)
    {
      ep->set = 1;
      switch (type)
	{
	case Mail_env_number:
	  ep->value.number = *(int*)value;
	  break;
	  
	case Mail_env_string:
	  ep->value.string = strdup (value);
	  break;
	  
	case Mail_env_boolean:
	  ep->value.bool = *(int*)value;
	  break;
		  
	default:
	  abort();
	}
    }
    
  /* Special handling for some variables */
  if (strcmp (variable, "replyregex") == 0)
    { 
      int rc;
      char *err;
	      
      if ((rc = mu_unre_set_regex (value, 0, &err)))
	{
	  fprintf (stderr, "%s", mu_strerror (rc));
	  if (err)
	    {
	      fprintf (stderr, "%s", err);
	      free (err);
	    }
	  fprintf (stderr, "\n");
	}
    }
  else if (strcmp (variable, "decode-fallback") == 0)
    {
      if (mu_set_default_fallback (value))
	mu_error (_("Incorrect value for decode-fallback"));
    }
  else if (strcmp (variable, "screen") == 0)
    page_invalidate (1);
  
  return 0;
}

const char *
util_reply_prefix ()
{
  char *prefix = "Re: ";
  util_getenv (&prefix, "replyprefix", Mail_env_string, 0);
  return prefix;
}


/* ************************* */

/*
 * return 1 if a message is deleted
 */

int
util_isdeleted (size_t n)
{
  mu_message_t msg = NULL;
  mu_attribute_t attr = NULL;

  mu_mailbox_get_message (mbox, n, &msg);
  mu_message_get_attribute (msg, &attr);
  return mu_attribute_is_deleted (attr);
}

void
util_mark_read (mu_message_t msg)
{
  mu_attribute_t attr;

  mu_message_get_attribute (msg, &attr);
  mu_attribute_set_read (attr);
  mu_attribute_set_userflag (attr, MAIL_ATTRIBUTE_SHOWN);
}

char *
util_get_homedir ()
{
  char *homedir = mu_get_homedir ();
  if (!homedir)
    {
      /* Shouldn't happen, but one never knows */
      util_error (_("Cannot get homedir"));
      exit (EXIT_FAILURE);
    }
  return homedir;
}

char *
util_fullpath (const char *inpath)
{
  return mu_tilde_expansion(inpath, "/", NULL);
}

char *
util_folder_path (const char *name)
{
  char *folder;
  char *tmp;
  char *p;
  
  if (util_getenv (&folder, "folder", Mail_env_string, 1))
    return NULL;
      
  if (!name)
    return NULL;
  if (name[0] == '+')
    name++;
  
  if (folder[0] != '/' && folder[0] != '~')
    {
      char *home = mu_get_homedir ();
      tmp  = xmalloc (strlen (home) + 1 +
		      strlen (folder) + 1 +
		      strlen (name) + 1);
      sprintf (tmp, "%s/%s/%s", home, folder, name);
    }
  else
    {
      tmp  = xmalloc (strlen (folder) + 1 +
		      strlen (name) + 1);
      sprintf (tmp, "%s/%s", folder, name);
    }
  p = util_fullpath (tmp);
  free (tmp);
  
  return p;
}

char *
util_get_sender (int msgno, int strip)
{
  mu_message_t msg = NULL;
  mu_address_t addr = NULL;
  char *buf = NULL, *p;

  mu_mailbox_get_message (mbox, msgno, &msg);
  addr = get_sender_address (msg);
  if (!addr)
    {
      mu_envelope_t env = NULL;
      const char *buffer;
      mu_message_get_envelope (msg, &env);
      if (mu_envelope_sget_sender (env, &buffer)
	  || mu_address_create (&addr, buffer))
	{
	  util_error (_("Cannot determine sender name (msg %d)"), msgno);
	  return NULL;
	}
    }

  if (mu_address_aget_email (addr, 1, &buf))
    {
      util_error (_("Cannot determine sender name (msg %d)"), msgno);
      mu_address_destroy (&addr);
      return NULL;
    }

  if (strip)
    {
      p = strchr (buf, '@');
      if (p)
	*p = 0;
    }

  mu_address_destroy (&addr);
  return buf;
}

void
util_slist_print (mu_list_t list, int nl)
{
  mu_iterator_t itr;
  char *name;

  if (!list || mu_list_get_iterator (list, &itr))
    return;

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&name);
      fprintf (ofile, "%s%c", name, nl ? '\n' : ' ');

    }
  mu_iterator_destroy (&itr);
}

int
util_slist_lookup (mu_list_t list, const char *str)
{
  mu_iterator_t itr;
  char *name;
  int rc = 0;

  if (!list || mu_list_get_iterator (list, &itr))
    return 0;

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&name);
      if (mu_c_strcasecmp (name, str) == 0)
	{
	  rc = 1;
	  break;
	}
    }
  mu_iterator_destroy (&itr);
  return rc;
}

void
util_slist_add (mu_list_t *list, char *value)
{
  char *p;

  if (!*list && mu_list_create (list))
    return;

  if ((p = strdup(value)) == NULL)
    {
      util_error(_("Not enough memory"));
      return;
    }
  mu_list_append (*list, p);
}

static int
comp (const void *item, const void *data)
{
  return strcmp ((char*)item, (char*)data);
}

void
util_slist_remove (mu_list_t *list, char *value)
{
  mu_list_comparator_t cp;
  
  if (!*list)
    return;
  cp = mu_list_set_comparator (*list, comp);
  mu_list_remove (*list, value);
  mu_list_set_comparator (*list, cp);
}

void
util_slist_destroy (mu_list_t *list)
{
  mu_iterator_t itr;
  char *name;

  if (!*list || mu_list_get_iterator (*list, &itr))
    return;

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&name);
      free (name);
    }
  mu_iterator_destroy (&itr);
  mu_list_destroy (list);
}

char *
util_slist_to_string (mu_list_t list, const char *delim)
{
  mu_iterator_t itr;
  char *name;
  char *str = NULL;

  if (!list || mu_list_get_iterator (list, &itr))
    return NULL;

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&name);
      if (str && delim)
	util_strcat(&str, delim);
      util_strcat(&str, name);
    }
  mu_iterator_destroy (&itr);
  return str;
}

void
util_strcat(char **dest, const char *str)
{
  if (!*dest)
    *dest = strdup (str);
  else
    {
      int dlen = strlen (*dest) + 1;
      int slen = strlen (str) + 1;
      char *newp = realloc (*dest, dlen + slen);

      if (!newp)
	return;

      *dest = newp;
      memcpy (newp + dlen - 1, str, slen);
    }
}

char *
util_outfolder_name (char *str)
{
  char *outfolder;

  if (!str)
    return NULL;
  
  switch (*str)
    {
    case '/':
    case '~':
      str = util_fullpath (str);
      break;
      
    case '+':
      str = util_folder_path (str);
      break;

    default:
      if (util_getenv (&outfolder, "outfolder", Mail_env_string, 0) == 0)
	{
	  char *ns = NULL;
	  asprintf (&ns, "%s/%s", outfolder, str);
	  str = util_fullpath (ns);
	  free (ns);
	}
      break;

    }

  return strdup (str);
}

/* Save an outgoing message. "savefile" allows to override the setting
   of the "record" variable. */
void
util_save_outgoing (mu_message_t msg, char *savefile)
{
  char *record;
  
  if (util_getenv (&record, "record", Mail_env_string, 0) == 0)
    {
      int rc;
      mu_mailbox_t outbox;
      char *filename = util_outfolder_name (savefile ? savefile : record);

      rc = mu_mailbox_create_default (&outbox, filename);
      if (rc)
	{
	  util_error (_("Cannot create output mailbox `%s': %s"),
		      filename, strerror (rc));
	  free (filename);
	  return;
	}

      rc = mu_mailbox_open (outbox, MU_STREAM_WRITE | MU_STREAM_CREAT);
      if (rc)
	util_error (_("Cannot open output mailbox `%s': %s"),
		    filename, strerror (rc));
      else
	{
	  rc = mu_mailbox_append_message (outbox, msg);
	  if (rc)
	    util_error (_("Cannot append message to `%s': %s"),
			filename, strerror (rc));
	}

      mu_mailbox_close (outbox);
      mu_mailbox_destroy (&outbox);
      
      free (filename);
    }
}

#ifdef HAVE_STDARG_H
void
util_error (const char *format, ...)
#else
void
util_error (va_alist)
     va_dcl
#endif
{
  va_list ap;

#ifdef HAVE_STDARG_H
  va_start(ap, format);
#else
  char *format;

  va_start (ap);
  format = va_arg (ap, char *);
#endif

  vfprintf (stderr, format, ap);
  fprintf (stderr, "\n");

  va_end(ap);
}

static int
util_descend_subparts (mu_message_t mesg, msgset_t *msgset, mu_message_t *part)
{
  unsigned int i;

  for (i = 1; i < msgset->npart; i++)
    {
      mu_message_t submsg = NULL;
      size_t nparts = 0;
      char *type = NULL;
      mu_header_t hdr = NULL;

      mu_message_get_header (mesg, &hdr);
      util_get_content_type (hdr, &type);
      if (mu_c_strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	{
	  if (mu_message_unencapsulate (mesg, &submsg, NULL))
	    {
	      util_error (_("Cannot unencapsulate message/part"));
	      return 1;
	    }
	  mesg = submsg;
	}

      mu_message_get_num_parts (mesg, &nparts);
      if (nparts < msgset->msg_part[i])
	{
	  util_error (_("No such (sub)part in the message: %d"),
		      msgset->msg_part[i]);
	  return 1;
	}

      if (mu_message_get_part (mesg, msgset->msg_part[i], &submsg))
	{
	  util_error (_("Cannot get (sub)part from the message: %d"),
		      msgset->msg_part[i]);
	  return 1;
	}

      mesg = submsg;
    }

  *part = mesg;
  return 0;
}

void
util_msgset_iterate (msgset_t *msgset,
		     int (*fun) (mu_message_t, msgset_t *, void *),
		     void *closure)
{
  for (; msgset; msgset = msgset->next)
    {
      mu_message_t mesg;

      if (mu_mailbox_get_message (mbox, msgset->msg_part[0], &mesg) != 0)
	return;

      if (util_descend_subparts (mesg, msgset, &mesg) == 0)
	  (*fun)(mesg, msgset, closure);
    }
}

int
util_get_content_type (mu_header_t hdr, char **value)
{
  char *type = NULL;
  util_get_hdr_value (hdr, MU_HEADER_CONTENT_TYPE, &type);
  if (type == NULL || *type == '\0')
    {
      if (type)
	free (type);
      type = strdup ("text/plain"); /* Default.  */
    }
  *value = type;
  return 0;
}

int
util_get_hdr_value (mu_header_t hdr, const char *name, char **value)
{
  int status = mu_header_aget_value (hdr, name, value);
  if (status == 0)
    {
      /* Remove the newlines.  */
      char *nl;
      while ((nl = strchr (*value, '\n')) != NULL)
	{
	  *nl = ' ';
	}
    }
  return status;
}

int
util_merge_addresses (char **addr_str, const char *value)
{
  mu_address_t addr, new_addr;
  int rc;

  if ((rc = mu_address_create (&new_addr, value)) != 0)
    return rc;
      
  if ((rc = mu_address_create (&addr, *addr_str)) != 0)
    {
      mu_address_destroy (&new_addr);
      return rc;
    }

  rc = mu_address_union (&addr, new_addr);
  if (rc == 0)
    {
      size_t n;

      rc = mu_address_to_string (addr, NULL, 0, &n);
      if (rc == 0)
	{
	  free (*addr_str);
	  *addr_str = malloc (n + 1);
	  if (!*addr_str)
	    rc = ENOMEM;
	  else
	    mu_address_to_string (addr, *addr_str, n + 1, &n);
	}
    }

  mu_address_destroy (&addr);
  mu_address_destroy (&new_addr);
  return rc;
}

int
is_address_field (const char *name)
{
  static char *address_fields[] = {
    MU_HEADER_TO,
    MU_HEADER_CC,
    MU_HEADER_BCC,
    0
  };
  char **p;
  
  for (p = address_fields; *p; p++)
    if (mu_c_strcasecmp (*p, name) == 0)
      return 1;
  return 0;
}

int
util_header_expand (mu_header_t *phdr)
{
  size_t i, nfields = 0;
  mu_header_t hdr;
  int errcnt = 0, rc;
  
  rc = mu_header_create (&hdr, "", 0, NULL);
  if (rc)
    {
      util_error (_("Cannot create temporary header: %s"), mu_strerror (rc));
      return 1;
    }
      
  mu_header_get_field_count (*phdr, &nfields);
  for (i = 1; i <= nfields; i++)
    {
      char *name, *value;
      
      if (mu_header_aget_field_name (*phdr, i, &name))
	continue;

      if (mu_header_aget_field_value (*phdr, i, &value))
	{
	  free (name);
	  continue;
	}
      
      if (is_address_field (name))
	{
	  char *p, *s, *exp = NULL;
	  mu_address_t addr = NULL;

	  if (mu_header_aget_value (hdr, name, &exp) == 0)
	    {
	      mu_address_create (&addr, exp);
	      free (exp);
	    }
	  
	  for (p = strtok_r (value, ",", &s); p; p = strtok_r (NULL, ",", &s))
	    {
	      mu_address_t new_addr;
	      
	      while (*p && mu_isspace (*p))
		p++;
	      /* If inplacealiases was set, the value was already expanded */
	      if (util_getenv (NULL, "inplacealiases", Mail_env_boolean, 0))
		exp = alias_expand (p);
	      rc = mu_address_create (&new_addr, exp ? exp : p);
	      if (rc)
		{
		  errcnt++;
		  if (exp)
		    util_error (_("Cannot parse address `%s' (while expanding `%s'): %s"),
				exp, p, mu_strerror (rc));
		  else
		    util_error (_("Cannot parse address `%s': %s"),
				p, mu_strerror (rc));
		}
	      
	      free (exp);
	      mu_address_union (&addr, new_addr);
	      mu_address_destroy (&new_addr);
	    }
	  
	  if (addr)
	    {
	      size_t n = 0;
	      
	      free (value);
	      mu_address_to_string (addr, NULL, 0, &n);
	      value = xmalloc (n + 1);
	      mu_address_to_string (addr, value, n + 1, NULL);
	      mu_address_destroy (&addr);
	      mu_header_set_value (hdr, name, value, 1);
	    }
	}
      else
	mu_header_set_value (hdr, name, value, 0);
      
      free (value);
      free (name);
    }

  if (errcnt == 0)
    {
      mu_header_destroy (phdr, NULL);
      *phdr = hdr;
    }
  else
    mu_header_destroy (&hdr, NULL);

  return errcnt;
}

int
util_get_message (mu_mailbox_t mbox, size_t msgno, mu_message_t *msg)
{
  int status;

  if (msgno > total)
    {
      util_error_range (msgno);
      return MU_ERR_NOENT;
    }
  
  status = mu_mailbox_get_message (mbox, msgno, msg);
  if (status)
    {
      util_error (_("Cannot get message %lu: %s"),
		  (unsigned long) msgno, mu_strerror (status));
      return status;
    }

  return 0;
}

int
util_error_range (size_t msgno)
{
  util_error (_("%d: invalid message number"), msgno);
  return 1;
}

void
util_noapp ()
{
  util_error (_("No applicable messages"));
}

void
util_cache_command (mu_list_t *list, const char *fmt, ...)
{
  char *cmd;
  va_list ap;

  va_start (ap, fmt);
  vasprintf (&cmd, fmt, ap);
  va_end (ap);

  if (!*list)
    mu_list_create (list);

  mu_list_append (*list, cmd);
}

static int
_run_and_free (void *item, void *data)
{
  util_do_command ((char *) item);
  free (item);
  return 0;
}

void
util_run_cached_commands (mu_list_t *list)
{
  mu_list_do (*list, _run_and_free, NULL);
  mu_list_destroy (list);
}

void
util_rfc2047_decode (char **value)
{
  char locale[32];
  char *charset = NULL;
  char *tmp;
  int rc;

  if (!*value || util_getenv (&charset, "charset", Mail_env_string, 0))
    return;

  if (mu_c_strcasecmp (charset, "auto") == 0)
    {
      memset (locale, 0, sizeof (locale));

      /* Try to deduce the charset from LC_ALL or LANG variables */

      tmp = getenv ("LC_ALL");
      if (!tmp)
	tmp = getenv ("LANG");

      if (tmp)
	{
	  char *sp;
	  char *lang;
	  char *terr;

	  strncpy (locale, tmp, sizeof (locale) - 1);

	  lang = strtok_r (locale, "_", &sp);
	  terr = strtok_r (NULL, ".", &sp);
	  charset = strtok_r (NULL, "@", &sp);

	  if (!charset)
	    charset = mu_charset_lookup (lang, terr);
	}
    }

  if (!charset)
    return;
  
  rc = mu_rfc2047_decode (charset, *value, &tmp);
  if (rc)
    {
      if (util_getenv (NULL, "verbose", Mail_env_boolean, 0) == 0)
	mu_error (_("Cannot decode line `%s': %s"), *value, mu_strerror (rc));
    }
  else
    {
      free (*value);
      *value = tmp;
    }
}

const char *
util_url_to_string (mu_url_t url)
{
  const char *scheme;
  if (mu_url_sget_scheme (url, &scheme) == 0)
    {
      if (strcmp (scheme, "file") == 0 || strcmp (scheme, "mbox") == 0)
	{
	  const char *path;
	  if (mu_url_sget_path (url, &path) == 0)
	    return path;
	}
    }
  return mu_url_to_string (url);
}
