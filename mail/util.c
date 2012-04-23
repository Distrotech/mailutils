/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2007, 2009-2012 Free Software Foundation, Inc.

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
#include <mailutils/util.h>
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
#include <mailutils/io.h>

int
util_do_command (const char *fmt, ...)
{
  struct mu_wordsplit ws;
  int argc;
  char **argv;
  int status = 0;
  const struct mail_command_entry *entry = NULL;
  char *cmd = NULL;
  size_t size = 0;
  va_list ap;
  
  va_start (ap, fmt);
  status = mu_vasnprintf (&cmd, &size, fmt, ap);
  va_end (ap);
  if (status)
    return status;

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
	    cmd = mu_strdup ("next");
	  else
	    return 0;
	}

      ws.ws_offs = 1; /* Keep one extra slot in case we need it
			 for expansion (see below) */
      if (mu_wordsplit (cmd, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_DOOFFS))
	{
	  mu_error (_("%s failed: %s"), "mu_wordsplit",
		    mu_wordsplit_strerror (&ws));
	}
      else
	{
	  char *p;

	  argc = ws.ws_wordc;
	  argv = ws.ws_wordv + 1;
	  
	  /* Special case: a number alone implies "print" */
	  if (argc == 1
	      && ((strtoul (argv[0], &p, 10) > 0 && *p == 0)
		  || (argv[0][1] == 0 && strchr("^$", argv[0][0]))))
	    {
	      /* Use the extra slot for "print" command */
	      argc++;
	      argv--;
	      argv[0] = "print";
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
	  /* Expand contracted form.  That's what we have kept an extra
	     ws slot for. */
	  argc++;
	  argv--;
	  argv[0] = argv[1];
	  argv[1] = mu_strdup (p);
	  *p = 0;
	  /* Register the new entry in WS */
	  ws.ws_wordc++;
	  ws.ws_offs = 0;
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
	mu_error (_("Unknown command: %s"), argv[0]);
      else
	mu_error (_("Invalid command"));
      status = 1;
    }

  mu_wordsplit_free (&ws);
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
	   mu_error (_("%lu: Inappropriate message (has been deleted)"),
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
      mu_stream_t out;
      char *p;

      out = open_pager (util_screen_lines () + 1);

      for (p = table, i = 0; i < nmemb; i++, p += size)
	{
	  struct mail_command *cp = (struct mail_command *)p;
	  if (cp->synopsis == NULL)
	    continue;
	  mu_stream_printf (out, "%s\n", cp->synopsis);
	}
      
      mu_stream_unref (out);

      return 0;
    }
  else
    {
      int status = 0;
      struct mail_command *cp = util_find_entry (table, nmemb, size, word);
      if (cp && cp->synopsis)
	mu_printf ("%s\n", cp->synopsis);
      else
	{
	  status = 1;
	  mu_printf (_("Unknown command: %s\n"), word);
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
	  mu_printf ("\n%s ", cmd);
	}
      else
        mu_printf ("%s ", cmd);
    }
  mu_printf ("\n");
  return 0;
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

  if (mailvar_get (&screen, "screen", mailvar_type_number, 0) == 0)
    return screen;
  n = util_getlines();
  util_do_command ("set screen=%lu", (unsigned long) n);
  return n;
}

int
util_screen_columns ()
{
  int cols;
  size_t n;

  if (mailvar_get (&cols, "columns", mailvar_type_number, 0) == 0)
    return cols;
  n = util_getcols();
  util_do_command ("set columns=%lu", (unsigned long) n);
  return n;
}

int
util_get_crt ()
{
  int lines;

  if (mailvar_get (&lines, "crt", mailvar_type_number, 0) == 0)
    return lines;
  else if (mailvar_get (NULL, "crt", mailvar_type_boolean, 0) == 0)
    return util_getlines ();
  return 0;
}

const char *
util_reply_prefix ()
{
  char *prefix = "Re: ";
  mailvar_get (&prefix, "replyprefix", mailvar_type_string, 0);
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
      mu_error (_("Cannot get homedir"));
      exit (EXIT_FAILURE);
    }
  return homedir;
}

char *
util_fullpath (const char *inpath)
{
  return mu_tilde_expansion (inpath, MU_HIERARCHY_DELIMITER, NULL);
}

char *
util_folder_path (const char *name)
{
  char *folder;
  char *tmp;
  char *p;
  
  if (mailvar_get (&folder, "folder", mailvar_type_string, 1))
    return NULL;
      
  if (!name)
    return NULL;
  if (name[0] == '+')
    name++;
  
  if (folder[0] != '/' && folder[0] != '~')
    {
      char *home = mu_get_homedir ();
      tmp  = mu_alloc (strlen (home) + 1 +
		       strlen (folder) + 1 +
		       strlen (name) + 1);
      sprintf (tmp, "%s/%s/%s", home, folder, name);
    }
  else
    {
      tmp  = mu_alloc (strlen (folder) + 1 +
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
	  mu_error (_("Cannot determine sender name (msg %d)"), msgno);
	  return NULL;
	}
    }

  if (mu_address_aget_email (addr, 1, &buf))
    {
      mu_error (_("Cannot determine sender name (msg %d)"), msgno);
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
      mu_printf ("%s%c", name, nl ? '\n' : ' ');
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

static int
util_slist_compare (const void *a, const void *b)
{
  return mu_c_strcasecmp (a, b);
}

void
util_slist_add (mu_list_t *plist, char *value)
{
  mu_list_t list;

  if (!*plist)
    {
      if (mu_list_create (&list))
	return;
      mu_list_set_destroy_item (list, mu_list_free_item);
      mu_list_set_comparator (list, util_slist_compare);
      *plist = list;
    }
  else
    list = *plist;
  mu_list_append (list, mu_strdup (value));
}

void
util_slist_remove (mu_list_t *list, char *value)
{
  mu_list_remove (*list, value);
}

void
util_slist_destroy (mu_list_t *list)
{
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

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&name);
      if (str && delim)
	util_strcat (&str, delim);
      util_strcat (&str, name);
    }
  mu_iterator_destroy (&itr);
  return str;
}

void
util_strcat (char **dest, const char *str)
{
  if (!*dest)
    *dest = mu_strdup (str);
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
      if (mailvar_get (&outfolder, "outfolder", mailvar_type_string, 0) == 0)
	{
	  char *ns = NULL;
	  mu_asprintf (&ns, "%s/%s", outfolder, str);
	  str = util_fullpath (ns);
	  free (ns);
	}
      break;

    }

  return mu_strdup (str);
}

/* Save an outgoing message. The SAVEFILE argument overrides the setting
   of the "record" variable. */
void
util_save_outgoing (mu_message_t msg, char *savefile)
{
  char *record;
  
  if (mailvar_get (&record, "record", mailvar_type_string, 0) == 0)
    {
      int rc;
      mu_mailbox_t outbox;
      char *filename = util_outfolder_name (savefile ? savefile : record);

      rc = mu_mailbox_create_default (&outbox, filename);
      if (rc)
	{
	  mu_error (_("Cannot create output mailbox `%s': %s"),
		      filename, strerror (rc));
	  free (filename);
	  return;
	}

      rc = mu_mailbox_open (outbox, MU_STREAM_WRITE | MU_STREAM_CREAT);
      if (rc)
	mu_error (_("Cannot open output mailbox `%s': %s"),
		    filename, strerror (rc));
      else
	{
	  rc = mu_mailbox_append_message (outbox, msg);
	  if (rc)
	    mu_error (_("Cannot append message to `%s': %s"),
			filename, strerror (rc));
	}

      mu_mailbox_close (outbox);
      mu_mailbox_destroy (&outbox);
      
      free (filename);
    }
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
      util_get_content_type (hdr, &type, NULL);
      if (mu_c_strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	{
	  if (mu_message_unencapsulate (mesg, &submsg, NULL))
	    {
	      mu_error (_("Cannot unencapsulate message/part"));
	      return 1;
	    }
	  mesg = submsg;
	}

      mu_message_get_num_parts (mesg, &nparts);
      if (nparts < msgset->msg_part[i])
	{
	  mu_error (_("No such (sub)part in the message: %lu"),
		      (unsigned long) msgset->msg_part[i]);
	  return 1;
	}

      if (mu_message_get_part (mesg, msgset->msg_part[i], &submsg))
	{
	  mu_error (_("Cannot get (sub)part from the message: %lu"),
		      (unsigned long) msgset->msg_part[i]);
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
	(*fun) (mesg, msgset, closure);
    }
}

int
util_get_content_type (mu_header_t hdr, char **value, char **args)
{
  char *type = NULL;
  util_get_hdr_value (hdr, MU_HEADER_CONTENT_TYPE, &type);
  if (type == NULL || *type == '\0')
    {
      if (type)
	free (type);
      type = mu_strdup ("text/plain"); /* Default.  */
    }
  else
    {
      char *p;
      p = strchr (type, ';');
      if (p)
	{
	  *p++ = 0;
	  if (args)
	    *args = p;
	}
    }
  *value = type;
  return 0;
}

int
util_get_hdr_value (mu_header_t hdr, const char *name, char **value)
{
  int status = mu_header_aget_value_unfold (hdr, name, value);
  if (status == 0)
    mu_rtrim_class (*value, MU_CTYPE_SPACE);
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
      char *val;
      
      rc = mu_address_aget_printable (addr, &val);
      if (rc == 0)
	{
	  free (*addr_str);
	  *addr_str = val;
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
  
  rc = mu_header_create (&hdr, "", 0);
  if (rc)
    {
      mu_error (_("Cannot create temporary header: %s"), mu_strerror (rc));
      return 1;
    }
      
  mu_header_get_field_count (*phdr, &nfields);
  for (i = 1; i <= nfields; i++)
    {
      const char *name, *value;
      
      if (mu_header_sget_field_name (*phdr, i, &name))
	continue;

      if (mu_header_sget_field_value (*phdr, i, &value))
	continue;
      
      if (is_address_field (name))
	{
	  const char *s;
	  mu_address_t addr = NULL;
	  struct mu_wordsplit ws;
	  size_t j;
	  
	  if (mu_header_sget_value (hdr, name, &s) == 0)
	    mu_address_create (&addr, s);

	  ws.ws_delim = ",";
	  if (mu_wordsplit (value, &ws,
			    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
			    MU_WRDSF_WS|
			    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
	    {
	      errcnt++;
	      mu_error (_("cannot split line `%s': %s"), value,
			mu_wordsplit_strerror (&ws));
	      break;
	    }

	  for (j = 0; j < ws.ws_wordc; j++)
	    {
	      const char *exp;
	      mu_address_t new_addr;
	      char *p = ws.ws_wordv[j];
	      
	      /* If inplacealiases was set, the value was already expanded */
	      if (mailvar_get (NULL, "inplacealiases",
			       mailvar_type_boolean, 0))
		exp = alias_expand (p);
	      else
		exp = p;
	      rc = mu_address_create (&new_addr, p);
	      if (rc)
		{
		  errcnt++;
		  if (exp)
		    mu_error (_("Cannot parse address `%s' (while expanding `%s'): %s"),
				exp, p, mu_strerror (rc));
		  else
		    mu_error (_("Cannot parse address `%s': %s"),
				p, mu_strerror (rc));
		}
	      
	      mu_address_union (&addr, new_addr);
	      mu_address_destroy (&new_addr);
	    }
	  
	  if (addr)
	    {
	      const char *newvalue;
	      
	      rc = mu_address_sget_printable (addr, &newvalue);
	      if (rc == 0)
		mu_header_set_value (hdr, name, newvalue, 1);
	      mu_address_destroy (&addr);
	    }
	}
      else
	mu_header_set_value (hdr, name, value, 0);
    }

  if (errcnt == 0)
    {
      mu_header_destroy (phdr);
      *phdr = hdr;
    }
  else
    mu_header_destroy (&hdr);

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
      mu_error (_("Cannot get message %lu: %s"),
		  (unsigned long) msgno, mu_strerror (status));
      return status;
    }

  return 0;
}

int
util_error_range (size_t msgno)
{
  mu_error (_("%lu: invalid message number"), (unsigned long) msgno);
  return 1;
}

void
util_noapp ()
{
  mu_error (_("No applicable messages"));
}

void
util_cache_command (mu_list_t *list, const char *fmt, ...)
{
  char *cmd = NULL;
  size_t size = 0;
  va_list ap;

  va_start (ap, fmt);
  mu_vasnprintf (&cmd, &size, fmt, ap);
  va_end (ap);

  if (!*list)
    mu_list_create (list);

  mu_list_append (*list, cmd);
}

static int
_run_and_free (void *item, void *data)
{
  util_do_command ("%s", (char *) item);
  free (item);
  return 0;
}

void
util_run_cached_commands (mu_list_t *list)
{
  mu_list_foreach (*list, _run_and_free, NULL);
  mu_list_destroy (list);
}

void
util_rfc2047_decode (char **value)
{
  char *charset = NULL;
  char *tmp;
  int rc;

  if (!*value || mailvar_get (&charset, "charset", mailvar_type_string, 0))
    return;

  if (mu_c_strcasecmp (charset, "auto") == 0)
    {
      static char *saved_charset;

      if (!saved_charset)
	{
	  /* Try to deduce the charset from LC_ALL or LANG variables */

	  tmp = getenv ("LC_ALL");
	  if (!tmp)
	    tmp = getenv ("LANG");

	  if (tmp)
	    {
	      struct mu_lc_all lc_all;
	      
	      if (mu_parse_lc_all (tmp, &lc_all, MU_LC_CSET) == 0)
		saved_charset = lc_all.charset;
	    }
	}
      charset = saved_charset; /* NOTE: a minor memory leak */
    }

  if (!charset)
    return;
  
  rc = mu_rfc2047_decode (charset, *value, &tmp);
  if (rc)
    {
      if (mailvar_get (NULL, "verbose", mailvar_type_boolean, 0) == 0)
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

mu_stream_t
open_pager (size_t lines)
{
  const char *pager;
  unsigned pagelines = util_get_crt ();
  mu_stream_t str;
  
  if (pagelines && lines > pagelines && (pager = getenv ("PAGER")))
    {
      int rc = mu_command_stream_create (&str, pager, MU_STREAM_WRITE);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_prog_stream_create",
			   pager, rc);
	  str = mu_strout;
	  mu_stream_ref (str);
	}
    }
  else
    {
      str = mu_strout;
      mu_stream_ref (str);
    }
  return str;
}
