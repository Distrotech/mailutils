/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "mail.h"
#include <mailutils/mutil.h>
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

typedef struct _node {
  /* for the msglist expander */
  int data;
  /* for the environment table */
  struct mail_env_entry env_entry;
  struct _node *next;
} node;

static node *environment = NULL;
static node *env_cursor = NULL;

/*
 * add a new node to the list
 */
static node *
util_ll_add (node *c, int data)
{
  c->next = xmalloc (sizeof (node));
  c->data = data;
  c->next->env_entry.var = NULL;
  c->next->env_entry.set = 0;
  c->next->env_entry.value.number = 0;
  c->next->next = NULL;
  return c->next;
}

/*
 * free a linked list
 */
static void
util_ll_free (node *c)
{
  node *t = c;
  while (t != NULL)
    {
      c = t;
      t = t->next;
      free (c);
    }
}

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
  function_t *command;
  int exec = 1;
  char *cmd = NULL;
  va_list ap;
  static const char *delim = "=";
  
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

      /* Hitting return i.e. no command, is equivalent to next
	 according to the POSIX spec.  */
      if (cmd[0] == '\0')
	{
	  free (cmd);
	  cmd = strdup ("next");
	}

      if (argcv_get (cmd, delim, NULL, &argc, &argv) == 0)
	{
	  struct mail_command_entry entry;
	  char *p;

	  /* Special case: a number alone implies "print" */
	  if (argc == 1 && strtoul (argv[0], &p, 10) > 0 && *p == 0)
	    {
	      asprintf (&p, "print %s", argv[0]);
	      argcv_free (argc, argv);
	      argcv_get (p, delim, NULL, &argc, &argv);
	      free (p);
	    }

	  entry = util_find_entry (mail_command_table, argv[0]);
	  command = entry.func;
	  /* Make sure we are not in any if/else */
	  exec = !(if_cond () == 0 && (entry.flags & EF_FLOW) == 0);
	}
      free (cmd);
    }
  else
    command = util_command_get ("quit");

  if (command != NULL)
    {
      if (exec)
	status = command (argc, argv);
    }
  else
    {
      util_error ("Unknown command: %s", argv[0]);
      status = 1;
    }

  argcv_free (argc, argv);
  return status;
}

/*
 * runs a function repeatedly on a msglist
 * func is the function to run
 * argc is the number of arguments inculding the command and msglist
 * argv is the list of strings containing the command and msglist
 * set_cursor means whether the function should set the cursor to
 * the number of the last message processed. If set_cursor = 0, the
 * cursor is not altered.
 */

int
util_msglist_command (function_t *func, int argc, char **argv, int set_cursor)
{
  msgset_t *list = NULL, *mp;
  int status = 0;

  if (msgset_parse (argc, argv, &list))
      return 1;

  realcursor = cursor;

  for (mp = list; mp; mp = mp->next)
    {
      cursor = mp->msg_part[0];
      /* NOTE: Should we bail on error also?  */
      if (func (1, argv) != 0)
	status = 1;
      /* Bail out if we receive an interrupt.  */
      if (ml_got_interrupt () != 0)
	break;
    }
  msgset_free (list);

  if (set_cursor)
    realcursor = cursor;
  else
    cursor = realcursor;
  return status;
}

/* Same as util_msglis_command but the function comes from the escape
   cmd table, so will have a different argument signature.  */
int
util_msglist_esccmd (int (*escfunc)
		     __P ((int, char **, compose_env_t *)),
		     int argc, char **argv, compose_env_t *env,
		     int set_cursor)
{
  msgset_t *list = NULL, *mp;
  int status = 0;

  if (msgset_parse (argc, argv, &list))
      return 1;

  realcursor = cursor;

  for (mp = list; mp; mp = mp->next)
    {
      cursor = mp->msg_part[0];
      /* NOTE: Should we bail on error also?  */
      if (escfunc (1, argv, env) != 0)
	status = 1;
      /* Bail out if we receive an interrupt.  */
      if (ml_got_interrupt () != 0)
	break;
    }
  msgset_free (list);

  if (set_cursor)
    realcursor = cursor;
  else
    cursor = realcursor;
  return status;
}

/*
 * returns the function to run for command
 */
function_t *
util_command_get (const char *cmd)
{
  struct mail_command_entry entry = util_find_entry (mail_command_table, cmd);
  return entry.func;
}

/*
 * returns the mail_command_entry structure for the command matching cmd
 */
struct mail_command_entry
util_find_entry (const struct mail_command_entry *table, const char *cmd)
{
  int i = 0, ll = 0, sl = 0;
  int len = strlen (cmd);

  while (table[i].shortname != 0)
    {
      sl = strlen (table[i].shortname);
      ll = strlen (table[i].longname);
      if (sl > ll && !strncmp (table[i].shortname, cmd, sl))
	return table[i];
      else if (sl == len && !strcmp (table[i].shortname, cmd))
	return table[i];
      else if (sl < len && !strncmp (table[i].longname, cmd, len))
	return table[i];
      i++;
    }
  return table[i];
}

/*
 * removes whitespace from the beginning and end of a string
 */
char *
util_stripwhite (char *string)
{
  register char *s, *t;
  for (s = string; isspace ((unsigned)*s); s++)
    ;
  if (*s == 0)
    return s;
  t = s + strlen (s) - 1;
  while (t > s && isspace ((unsigned)*t))
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
	util_error ("No value set for \"%s\"", variable);
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
	*(int*)ptr = env->set;
	break;

      default:
	break;
      }
	
  return 0;
}

/* Find environment entry var. If not found and CREATE is not null, then
   create the (unset and untyped) variable */
struct mail_env_entry *
util_find_env (const char *variable, int create)
{
  /* Annoying, variable "ask" is equivalent to "asksub".  */
  static const char *asksub = "asksub";
  const char *var = variable;
  size_t len = strlen (var);
  node *t;

  if (len < 1)
    return NULL;

  /* Catch "ask" --> "asksub".  */
  if (len == strlen ("ask") && !strcmp ("ask", var))
    {
      var = asksub;
      len = strlen (var);
    }

  if (environment == NULL)
    {
      if (!create)
	return 0;
      environment = xmalloc (sizeof (node));
      environment->env_entry.var = NULL;
      environment->env_entry.set = 0;
      environment->env_entry.value.number = 0;
      environment->next = NULL;
    }

  for (env_cursor = environment; env_cursor->next;
       env_cursor = env_cursor->next)
    {
      if (strlen (env_cursor->env_entry.var) == len &&
	  !strcmp (var, env_cursor->env_entry.var))
	return &env_cursor->env_entry;
    }

  env_cursor->env_entry.var = strdup (var);
  env_cursor->env_entry.set = 0;
  env_cursor->env_entry.type = Mail_env_whatever;
  env_cursor->env_entry.value.number = 0;
  t = env_cursor;
  env_cursor = util_ll_add (env_cursor, 0);
  return &t->env_entry;
}

/* print the environment */
int
util_printenv (int set)
{
  for (env_cursor = environment; env_cursor != NULL;
       env_cursor = env_cursor->next)
    {
      if (env_cursor->env_entry.set == set)
	{
	  fprintf (ofile, "%s", env_cursor->env_entry.var);
	  switch (env_cursor->env_entry.type)
	    {
	    case Mail_env_number:
	      fprintf (ofile, "=%d", env_cursor->env_entry.value.number);
	      break;
	      
	    case Mail_env_string:
	      fprintf (ofile, "=\"%s\"", env_cursor->env_entry.value.string);
	      break;
	      
	    case Mail_env_boolean:
	      break;
	      
	    case Mail_env_whatever:
	      fprintf (ofile, "oops?");
	    }
	  fprintf (ofile, "\n");
	}
    }
  return 0;
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
	  break;
		  
	default:
	  abort();
	}
    }

  return 0;
}

/* ************************* */

/*
 * return 1 if a message is deleted
 */
int
util_isdeleted (message_t msg)
{
  attribute_t attr;

  message_get_attribute (msg, &attr);
  return attribute_is_deleted (attr);
}

char *
util_get_homedir ()
{
  char *homedir = mu_get_homedir ();
  if (!homedir)
    {
      /* Shouldn't happen, but one never knows */
      util_error ("can't get homedir");
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

  if (util_getenv (&folder, "folder", Mail_env_string, 1))
    return NULL;
      
  if (!name)
    return NULL;
  if (name[0] == '+')
    name++;
  
  if (folder[0] != '/' && folder[1] != '~')
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
  name = tmp;

  return (char*) name;
}

char *
util_get_sender(int msgno, int strip)
{
  header_t header = NULL;
  address_t addr = NULL;
  message_t msg = NULL;
  char buffer[512], *p;

  mailbox_get_message (mbox, msgno, &msg);
  message_get_header (msg, &header);
  if (header_get_value (header, MU_HEADER_FROM, buffer, sizeof(buffer), NULL)
      || address_create (&addr, buffer))
    {
      envelope_t env = NULL;
      message_get_envelope (msg, &env);
      if (envelope_sender (env, buffer, sizeof (buffer), NULL)
	  || address_create (&addr, buffer))
	{
	  util_error("can't determine sender name (msg %d)", msgno);
	  return NULL;
	}
    }

  if (address_get_email (addr, 1, buffer, sizeof(buffer), NULL))
    {
      util_error("can't determine sender name (msg %d)", msgno);
      address_destroy (&addr);
      return NULL;
    }

  if (strip)
    {
      p = strchr (buffer, '@');
      if (p)
	*p = 0;
    }

  p = strdup (buffer);
  address_destroy (&addr);
  return p;
}

void
util_slist_print(list_t list, int nl)
{
  iterator_t itr;
  char *name;

  if (!list || iterator_create (&itr, list))
    return;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      fprintf (ofile, "%s%c", name, nl ? '\n' : ' ');

    }
  iterator_destroy (&itr);
}

int
util_slist_lookup(list_t list, char *str)
{
  iterator_t itr;
  char *name;
  int rc = 0;

  if (!list || iterator_create (&itr, list))
    return 0;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      if (strcasecmp (name, str) == 0)
	{
	  rc = 1;
	  break;
	}
    }
  iterator_destroy (&itr);
  return rc;
}

void
util_slist_add (list_t *list, char *value)
{
  char *p;

  if (!*list && list_create (list))
    return;

  if ((p = strdup(value)) == NULL)
    {
      util_error("not enough memory\n");
      return;
    }
  list_append (*list, p);
}

void
util_slist_destroy (list_t *list)
{
  iterator_t itr;
  char *name;

  if (!*list || iterator_create (&itr, *list))
    return;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      free (name);
    }
  iterator_destroy (&itr);
  list_destroy (list);
}

char *
util_slist_to_string (list_t list, const char *delim)
{
  iterator_t itr;
  char *name;
  char *str = NULL;

  if (!list || iterator_create (&itr, list))
    return NULL;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      if (str && delim)
	util_strcat(&str, delim);
      util_strcat(&str, name);
    }
  iterator_destroy (&itr);
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

/* Upper case the entire string.  Assume it is NULL terminated.  */
void
util_strupper (char *s)
{
  if (s)
    {
      size_t i;
      size_t len = strlen (s);
      for (i = 0; i < len; i++)
	s[i] = toupper ((unsigned int)(s[i]));
    }
}


void
util_escape_percent (char **str)
{
  int count;
  char *p, *q;
  char *newstr;

  /* Count ocurrences of % in the string */
  count = 0;
  for (p = *str; *p; p++)
    if (*p == '%')
      count++;

  if (!count)
    return; /* nothing to do */

  /* expand the string */
  newstr = xmalloc (strlen (*str) + 1 + count);

  /* and escape percent signs */
  p = newstr;
  q = *str;
  while ((*p = *q++))
    {
      if (*p == '%')
	*++p = '%';
      p++;
    }
  *str = newstr;
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
util_save_outgoing (message_t msg, char *savefile)
{
  char *record;
  
  if (util_getenv (&record, "record", Mail_env_string, 0) == 0)
    {
      FILE *outfile;
      char *filename = util_outfolder_name (savefile ? savefile : record);

      outfile = fopen (filename, "a");
      if (!outfile)
	{
	  util_error("can't open save file %s: %s",
		     filename, strerror (errno));
	}
      else
	{
	  char *buf = NULL;
	  size_t bsize = 0;

	  message_size (msg, &bsize);

	  /* Try to allocate large buffer */
	  for (; bsize > 1; bsize /= 2)
	    if ((buf = malloc (bsize)))
	      break;

	  if (!bsize)
	    {
	      util_error("not enough memory for creating save file");
	    }
	  else
	    {
	      stream_t stream;
	      size_t n, off = 0;
	      time_t t;
	      struct tm *tm;
	      char date[64];

	      time(&t);
	      tm = gmtime(&t);
	      strftime (date, sizeof (date), "%a %b %e %H:%M:%S %Y", tm);
	      fprintf (outfile, "From %s %s\n", mail_whoami(), date);

	      message_get_stream (msg, &stream);
	      while (stream_read (stream, buf, bsize, off, &n) == 0
		     && n != 0)
		{
		  fwrite (buf, 1, n, outfile);
		  off += n;
		}
	      free (buf);
	      fprintf (outfile, "\n");
	    }
	  fclose (outfile);
	}
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

int
util_help (const struct mail_command_entry *table, char *word)
{
  if (!word)
    {
      int i = 0;
      FILE *out = stdout;

      if (util_getenv (NULL, "crt", Mail_env_boolean, 0) == 0)
	out = popen (getenv ("PAGER"), "w");

      while (table[i].synopsis != 0)
	fprintf (out, "%s\n", table[i++].synopsis);

      if (out != stdout)
	pclose (out);

      return 0;
    }
  else
    {
      int status = 0;
      struct mail_command_entry entry = util_find_entry(table, word);
      if (entry.synopsis != NULL)
	fprintf (stdout, "%s\n", entry.synopsis);
      else
	{
	  status = 1;
	  fprintf (stdout, "Unknown command: %s\n", word);
	}
      return status;
    }
  return 1;
}

static int
util_descend_subparts (message_t mesg, msgset_t *msgset, message_t *part)
{
  unsigned int i;

  for (i = 1; i < msgset->npart; i++)
    {
      message_t submsg = NULL;
      size_t nparts = 0;
      char *type = NULL;
      header_t hdr = NULL;

      message_get_header (mesg, &hdr);
      util_get_content_type (hdr, &type);
      if (strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	{
	  if (message_unencapsulate (mesg, &submsg, NULL))
	    {
	      util_error ("can't unencapsulate message/part");
	      return 1;
	    }
	  mesg = submsg;
	}

      message_get_num_parts (mesg, &nparts);
      if (nparts < msgset->msg_part[i])
	{
	  util_error ("no such (sub)part in the message: %d",
		      msgset->msg_part[i]);
	  return 1;
	}

      if (message_get_part (mesg, msgset->msg_part[i], &submsg))
	{
	  util_error ("can't get (sub)part from the message: %d",
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
		     int (*fun) __P ((message_t, msgset_t *, void *)),
		     void *closure)
{
  for (; msgset; msgset = msgset->next)
    {
      message_t mesg;

      if (mailbox_get_message (mbox, msgset->msg_part[0], &mesg) != 0)
	return;

      if (util_descend_subparts (mesg, msgset, &mesg) == 0)
	  (*fun)(mesg, msgset, closure);
    }
}

int
util_get_content_type (header_t hdr, char **value)
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
util_get_hdr_value (header_t hdr, const char *name, char **value)
{
  int status = header_aget_value (hdr, name, value);
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
  address_t addr, new_addr;
  int rc;

  if ((rc = address_create (&new_addr, value)) != 0)
    return rc;
      
  if ((rc = address_create (&addr, *addr_str)) != 0)
    {
      address_destroy (&new_addr);
      return rc;
    }

  rc = address_union (&addr, new_addr);
  if (rc == 0)
    {
      size_t n;

      rc = address_to_string (addr, NULL, 0, &n);
      if (rc == 0)
	{
	  free (*addr_str);
	  *addr_str = malloc (n + 1);
	  if (!*addr_str)
	    rc = ENOMEM;
	  else
	    address_to_string (addr, *addr_str, n + 1, &n);
	}
    }

  address_destroy (&addr);
  address_destroy (&new_addr);
  return rc;
}

static int
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
    if (strcasecmp (*p, name) == 0)
      return 1;
  return 0;
}

int
util_header_expand (header_t *phdr)
{
  size_t i, nfields = 0;
  header_t hdr;
  int errcnt = 0, rc;
  
  rc = header_create (&hdr, "", 0, NULL);
  if (rc)
    {
      util_error ("can't create temporary header: %s", mu_errstring (rc));
      return 1;
    }
      
  header_get_field_count (*phdr, &nfields);
  for (i = 1; i <= nfields; i++)
    {
      char *name, *value;
      
      if (header_aget_field_name (*phdr, i, &name))
	continue;

      if (header_aget_field_value (*phdr, i, &value))
	{
	  free (name);
	  continue;
	}
      
      if (is_address_field (name))
	{
	  char *p, *s, *exp;
	  address_t addr = NULL;

	  if (header_aget_value (hdr, name, &exp) == 0)
	    {
	      address_create (&addr, exp);
	      free (exp);
	    }
	  
	  for (p = strtok_r (value, ",", &s); p; p = strtok_r (NULL, ",", &s))
	    {
	      address_t new_addr;
	      
	      while (*p && isspace (*p))
		p++;
	      exp = alias_expand (p);
	      rc = address_create (&new_addr, exp ? exp : p);
	      if (rc)
		{
		  errcnt++;
		  if (exp)
		    util_error ("can't parse address `%s' (while expanding `%s'): %s",
				exp, p, mu_errstring (rc));
		  else
		    util_error ("can't parse address `%s': %s",
				p, mu_errstring (rc));
		}
	      
	      free (exp);
	      address_union (&addr, new_addr);
	      address_destroy (&new_addr);
	    }
	  
	  if (addr)
	    {
	      size_t n = 0;
	      
	      free (value);
	      address_to_string (addr, NULL, 0, &n);
	      value = xmalloc (n + 1);
	      address_to_string (addr, value, n + 1, NULL);
	      address_destroy (&addr);
	      header_set_value (hdr, name, value, 1);
	    }
	}
      else
	header_set_value (hdr, name, value, 0);
      
      free (value);
      free (name);
    }

  if (errcnt == 0)
    {
      header_destroy (phdr, NULL);
      *phdr = hdr;
    }
  else
    header_destroy (&hdr, NULL);

  return errcnt;
}

int
util_get_message (mailbox_t mbox, size_t msgno, message_t *msg, int delflag)
{
  int status;

  if (msgno > total)
    {
      util_error_range (msgno);
      return ENOENT;
    }
  
  status = mailbox_get_message (mbox, msgno, msg);
  if (status)
    {
      util_error ("can't get message %lu: %s",
		  (unsigned long) msgno, mu_errstring (status));
      return status;
    }

  if (delflag && util_isdeleted (*msg))
    {
      util_error ("%d: Inappropriate message (has been deleted)");
      return ENOENT;
    }
  return 0;
}

int
util_error_range (size_t msgno)
{
  util_error ("%d: invalid message number", msgno);
  return 1;
}
