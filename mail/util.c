/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
  c->next->env_entry.value = NULL;
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
      struct mail_command_entry entry;

      if (cmd[0] == '#')
	return 0;

      if (argcv_get (cmd, delim, NULL, &argc, &argv) != 0)
	return argcv_free (argc, argv);

      entry = util_find_entry (mail_command_table, argv[0]);

      if (if_cond () == 0 && (entry.flags & EF_FLOW) == 0)
	{
	  argcv_free (argc, argv);
	  return 0;
	}
      command = entry.func;
    }
  else
    command = util_command_get ("quit");

  if (command != NULL)
    status = command (argc, argv);
  else
    {
      util_error("Unknown command: %s", argv[0]);
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
		     __P ((int, char **, struct send_environ *)),
		     int argc, char **argv, struct send_environ *env,
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
  struct mail_env_entry *ep = util_find_env ("screen");
  size_t n;

  if (ep && ep->set && (n = strtoul (ep->value, NULL, 10)) != 0)
    return n;
  n = util_getlines();
  util_do_command ("set screen=%d", n);
  return n;
}

int
util_screen_columns ()
{
  struct mail_env_entry *ep = util_find_env("columns");
  size_t n;

  if (ep && ep->set && (n = strtoul (ep->value, NULL, 10)) != 0)
    return n;
  n = util_getcols();
  util_do_command ("set columns=%d", n);
  return n;
}


/*
 * Find environment entry var

 FIXME: We should probably call this util_getenv to be consitent with
 util_printenv(), util_setenv() etc ..
 */
struct mail_env_entry *
util_find_env (const char *variable)
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
      environment = xmalloc (sizeof (node));
      environment->env_entry.var = NULL;
      environment->env_entry.set = 0;
      environment->env_entry.value = NULL;
      environment->next = NULL;
    }

  for (env_cursor = environment; env_cursor->next != NULL;
       env_cursor = env_cursor->next)
    {
      if (strlen (env_cursor->env_entry.var) == len &&
	  !strcmp (var, env_cursor->env_entry.var))
	{
	  return &(env_cursor->env_entry);
	}
    }

  env_cursor->env_entry.var = strdup (var);
  env_cursor->env_entry.set = 0;
  env_cursor->env_entry.value = NULL;
  t = env_cursor;
  env_cursor = util_ll_add (env_cursor, 0);
  return &(t->env_entry);
}

/*
 * print the environment
 */
int
util_printenv (int set)
{
  for (env_cursor = environment; env_cursor != NULL;
       env_cursor = env_cursor->next)
    {
      if (env_cursor->env_entry.set == set)
	{
	  fprintf (ofile, "%s", env_cursor->env_entry.var);
	  if (env_cursor->env_entry.value != NULL)
	    fprintf (ofile, "=\"%s\"", env_cursor->env_entry.value);
	  fprintf (ofile, "\n");
	}
    }
  return 0;
}

/*
 * Set environement
 * The  util_setenv() function adds the variable name to the envi-
 * ronment with the value value, if  name  does  not  already
 * exist.   If  name  does exist in the environment, then its
 * value is changed to value if  overwrite  is  non-zero;  if
 * overwrite  is zero, then the value of name is not changed.
 *
 * A side effect of the code is if value is null the variable name
 * will be unset.
 */
int
util_setenv (const char *variable, const char *value, int overwrite)
{
  struct mail_env_entry *ep =  util_find_env (variable);
  if (ep->set)
    {
      if (overwrite)
	{
	  ep->set = 0;
	  if (ep->value)
	    free (ep->value);
	  ep->value = NULL;
	  if (value)
	    {
	      ep->set = 1;
	      ep->value = strdup (value);
	    }
	}
    }
  else
    {
      ep->set = 1;
      if (ep->value)
	free (ep->value);
      ep->value = strdup (value);
    }
  return 0;
}

/*
 * return 1 if a message is deleted
 */
int
util_isdeleted (int n)
{
  message_t msg;
  attribute_t attr;
  if (mailbox_get_message (mbox, n, &msg) != 0)
    return 0;
  message_get_attribute (msg, &attr);
  if (attribute_is_deleted (attr))
    return 1;
  return 0;
}

char *
util_get_homedir()
{
  char *homedir = mu_get_homedir();
  if (!homedir)
    {
      /* Shouldn't happen, but one never knows */
      util_error("can't get homedir");
      exit (EXIT_FAILURE);
    }
  return strdup(homedir);
}

char *
util_fullpath(const char *inpath)
{
  return mu_tilde_expansion(inpath, "/", NULL);
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
  struct mail_env_entry *ep = util_find_env("outfolder");

  switch (*str)
    {
    case '/':
    case '~':
    case '+':
      str = util_fullpath (str);
      break;

    default:
      if (ep && ep->set)
	{
	  char *ns = NULL;
	  asprintf (&ns, "%s/%s", ep->value, str);
	  str = util_fullpath (ns);
	  free (ns);
	}
      break;

    }

  return str;
}

/* Save an outgoing message. "savefile" allows to override the setting
   of the "record" variable. */
void
util_save_outgoing (message_t msg, char *savefile)
{
  struct mail_env_entry *ep = util_find_env("record");
  if (ep->set)
    {
      FILE *outfile;
      char *filename = util_outfolder_name (savefile ? savefile : ep->value);

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
	      strftime (date, sizeof (date), "%a %b %e %H:%M:%S %Y%n", tm);
	      fprintf (outfile, "From %s %s\n", mail_whoami(), date);

	      message_get_stream (msg, &stream);
	      while (stream_read (stream, buf, bsize, off, &n) == 0
		     && n != 0)
		{
		  fwrite (buf, 1, n, outfile);
		  off += n;
		}
	      free (buf);
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

      if ((util_find_env("crt"))->set)
	out = popen (getenv("PAGER"), "w");

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
      unsigned int nparts = 0;
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
