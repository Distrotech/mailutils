/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif
#ifndef MB_LEN_MAX
# define MB_LEN_MAX 4
#endif

#include <mbswidth.h>
#include <xalloc.h>

#ifdef HAVE_FRIBIDI_FRIBIDI_H
# include <fribidi/fribidi.h>
#endif

#include <mailutils/address.h>
#include <mailutils/argp.h>
#include <mailutils/attribute.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>
#include <mailutils/error.h>
#include <mailutils/mutil.h>
#include <mailutils/mime.h>

static char *show_field;   /* Show this header field instead of the default
			      `From: Subject:' pair. -f option */
static int show_to;        /* Additionally display To: field. -l option */ 
static int show_number;    /* Prefix each line with the message number. -n */
static int show_summary;   /* Summarize the number of messages by message
			      status in each mailbox. -S option */
static int be_quiet;       /* Quiet mode. -q option. */
static int show_query;     /* Additional flag toggled by -q to display 
			      a one-line summary for each mailbox */
static int align = 0;      /* Tidy mode. -t option. */
static int dbug;           /* Debug level. -d option.*/

#define IS_READ 0x001
#define IS_OLD  0x010
#define IS_NEW  0x100
static int select_attribute;
static int selected;

static int action (observer_t, size_t);
void init_output (size_t s);

const char *program_version = "frm (" PACKAGE_STRING ")";
static char doc[] = N_("GNU frm -- display From: lines");

static struct attr_tab {
  char *name;      /* Attribute name */
  int code;        /* Corresponding IS_.* flag */
  size_t len;      /* Minimum abbreviation length */
} attr_tab[] = {
  /* TRANSLATORS: See comment marked [frm status] below */
  { N_("new"), IS_NEW, 0 },
  /* TRANSLATORS: See comment marked [frm status] below */
  { N_("old"), IS_OLD, 0 },
  /* TRANSLATORS: See comment marked [frm status] below */
  { N_("unread"), IS_OLD, 0 },
  /* TRANSLATORS: See comment marked [frm status] below */
  { N_("read"), IS_READ, 0 },
  { NULL }
};

static char attr_help[] =
/* TRANSLATORS: [frm status]
   
   1) Please make sure the words "new", "unread", "old" and
   "read" are translated exactly as in four preceeding messages.
   
   2) If possible, select such translations for these words, that
   differ by the first letter.
*/
N_("Select messages with the specific attribute. STATUS is one \
of the following: new, unread, old (same as unread) or read. Any unambiguous \
abbreviation of those is also accepted.");


/* Attribute table handling */

/* Prepares the table for use. Computes minimum abbreviation lengths. */
static void
prepare_attrs (void)
{
  struct attr_tab *p, *q;

  for (p = attr_tab; p->name; p++)
    {
      const char *name = gettext (p->name);
      size_t len = strlen (name);
      size_t n = 1;

      for (q = attr_tab; q->name; q++)
	{
	  if (p != q)
	    {
	      const char *str = gettext (q->name);
	      size_t slen = strlen (str);

	      if (memcmp (name, str, n) == 0)
		{
		  for (n++;
		       memcmp (name, str, n) == 0
			 && n < len
			 && n < slen; n++)
		    ;
		  
		  q->len = n < slen ? n : slen;
		}
	    }
	}
      p->len = n < len ? n : len;
    }
}

/* Translates the textual status representation to the corresponding
   IS_.* flag */
static int
decode_attr (char *arg)
{
  struct attr_tab *p;
  int len = strlen (arg);
  int pretendents = 0;
  
  for (p = attr_tab; p->name; p++)
    {
      const char *str = gettext (p->name);
      if (str[0] == arg[0])
	{
	  if (len < p->len)
	    pretendents++;
	  else if (len > strlen (str))
	    continue;
	  if (memcmp (str, arg, p->len) == 0)
	    return p->code;
	}
    }
  if (pretendents)
    mu_error (_("%s: ambiguous abbreviation"), arg);
  else
    mu_error (_("%s: unknown attribute"), arg);
  return 0;
}



/* Get the number of columns on the screen
   First try an ioctl() call, not all shells set the COLUMNS environ.
   If ioctl does not succeed on stdout, try it on /dev/tty, as we
   may work via a pipe.
   
   This function was taken from mail/util.c. It should probably reside
   in the library */
int
util_getcols (void)
{
  struct winsize ws;
  
  ws.ws_col = ws.ws_row = 0;
  if (ioctl (1, TIOCGWINSZ, (char *) &ws) < 0)
    {
      int fd = open ("/dev/tty", O_RDWR);
      ioctl (fd, TIOCGWINSZ, (char *) &ws);
      close (fd);
    }
  if (ws.ws_row == 0)
    {
      const char *columns = getenv ("COLUMNS");
      if (columns)
	ws.ws_col = strtol (columns, NULL, 10);
    }
  return ws.ws_col;
}



static struct argp_option options[] = {
  {"debug",  'd', NULL,   0, N_("Enable debugging output"), 0},
  {"field",  'f', N_("NAME"), 0, N_("Header field to display"), 0},
  {"to",     'l', NULL,   0, N_("Include the To: information"), 0},
  {"number", 'n', NULL,   0, N_("Display message numbers"), 0},
  {"Quiet",  'Q', NULL,   0, N_("Very quiet"), 0},
  {"query",  'q', NULL,   0, N_("Print a message if the mailbox contains some unread mail"), 0},
  {"summary",'S', NULL,   0, N_("Print a summary of messages"), 0},
  {"status", 's', N_("STATUS"), 0, attr_help, 0},
  {"align",  't', NULL,   0, N_("Tidy mode: align subject lines"), 0},
  {0, 0, 0, 0}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      dbug++;
      break;

    case 'f':
      show_field = arg;
      align = 0;
      break;

    case 'l':
      show_to = 1;
      break;

    case 'n':
      show_number = 1;
      break;

    case 'Q':
      /* Very silent.  */
      be_quiet += 2;
      break;

    case 'q':
      be_quiet++;
      show_query = 1;
      break;

    case 'S':
      show_summary = 1;
      break;

    case 's':
      select_attribute = decode_attr (arg);
      break;
      
    case 't':
      align = 1;
      break;

    case ARGP_KEY_FINI:
      {
	size_t s;
	if (align && (s = util_getcols ()))
	  init_output (s);
	else
	  init_output (0);
      }
      break;
      
    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[URL ...]"),
  doc,
  NULL,
  NULL, NULL
};

static const char *frm_argp_capa[] = {
  "common",
  "license",
  "mailbox",
#ifdef WITH_TLS
  "tls",
#endif
  NULL
};


/* Charset magic */
static char *output_charset = NULL;

const char *
get_charset ()
{
  char *tmp;
  
  if (!output_charset)
    {
      char locale[32];
      
      memset (locale, 0, sizeof (locale));

      /* Try to deduce the charset from LC_ALL or LANG variables */

      tmp = getenv ("LC_ALL");
      if (!tmp)
	tmp = getenv ("LANG");

      if (tmp)
	{
	  char *sp = NULL;
	  char *lang;
	  char *terr;

	  strncpy (locale, tmp, sizeof (locale) - 1);
	  
	  lang = strtok_r (locale, "_", &sp);
	  terr = strtok_r (NULL, ".", &sp);
	  output_charset = strtok_r (NULL, "@", &sp);

	  if (output_charset)
	    output_charset = xstrdup (output_charset);
	  else
	    output_charset = mu_charset_lookup (lang, terr);

	  if (!output_charset)
	    output_charset = "ASCII";
	}
    }
  return output_charset;
}


/* BIDI support (will be moved to lib when it's ready) */
#ifdef HAVE_LIBFRIBIDI

static int fb_charset_num = -1;
FriBidiChar *logical;
char *outstring;
size_t logical_size;

void
alloc_logical (size_t size)
{
  logical = xmalloc (size * sizeof (logical[0]));
  logical_size = size;
  outstring = xmalloc (size);
}

void
puts_bidi (char *string)
{
  if (fb_charset_num == -1)
    {
      fb_charset_num = fribidi_parse_charset (get_charset ());
      if (fb_charset_num && dbug)
	mu_error (_("fribidi failed to recognize charset `%s'"),
		  get_charset ());
    }
  
  if (fb_charset_num == 0)
    puts (string);
  else
    {
      FriBidiStrIndex len;
      FriBidiCharType base = FRIBIDI_TYPE_ON;
      fribidi_boolean log2vis;
      
      static FriBidiChar *visual;
      static size_t visual_size;
      
      
      len = fribidi_charset_to_unicode (fb_charset_num,
					string, strlen (string),
					logical);

      if (len + 1 > visual_size)
	{
	  visual_size = len + 1;
	  visual = xrealloc (visual, visual_size * sizeof *visual);
	}
      
      /* Create a bidi string. */
      log2vis = fribidi_log2vis (logical, len, &base,
				 /* output */
				 visual, NULL, NULL, NULL);

      if (log2vis)
	{
	  FriBidiStrIndex idx, st;
	  FriBidiStrIndex new_len;
	  
	  for (idx = 0; idx < len;)
	    {
	      FriBidiStrIndex wid, inlen;
	      
	      wid = 3 * logical_size;
	      st = idx;

	      if (fb_charset_num != FRIBIDI_CHARSET_CAP_RTL)
		{
		  while (wid > 0 && idx < len)
		    wid -= fribidi_wcwidth (visual[idx++]);
		}
	      else
		{
		  while (wid > 0 && idx < len)
		    {
		      wid--;
		      idx++;
		    }
		}
	      
	      if (wid < 0 && idx > st + 1)
		idx--;
	      inlen = idx - st;

	      new_len = fribidi_unicode_to_charset (fb_charset_num,
						    visual + st, inlen,
						    outstring);
	      printf ("%s", outstring);
	    }
	  putchar ('\n');
	}
      else
	{
	  /* Print the string as is */
	  puts (string);
	}
    }
}
#else
# define alloc_logical(s)
# define puts_bidi puts
#endif


/* Output functions */

/* Number of columns in output:

     Maximum     4     message number, to, from, subject   -ln
     Default     2     from, subject                       [none]
     Minimum     1     FIELD                               -f FIELD
*/

static int numfields;      /* Number of output fields */
static int fieldwidth[4];  /* Field start positions */
static char *linebuf;      /* Output line buffer */
static size_t linemax;     /* Size of linebuf */
static size_t linepos;     /* Position in the output line buffer */
static int curfield;       /* Current output field */
static int nextstart;      /* Start position of the next field */
static int curcol;         /* Current output column */

typedef void (*fmt_formatter) (const char *fmt, ...);

static fmt_formatter format_field;

void
print_line ()
{
  if (linebuf)
    {
      puts_bidi (linebuf);
      linebuf[0] = 0;
      linepos = 0;
      curcol = nextstart = 0;
    }
  else
    putchar ('\n');
  curfield = 0;
}

void
format_field_simple (const char *fmt, ...)
{
  va_list ap;
  if (curfield++)
    putchar ('\t');
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}

void
format_field_align (const char *fmt, ...)
{
  size_t n, width;
  va_list ap;

  va_start (ap, fmt);
  if (nextstart != 0)
    {
      if (curcol >= nextstart)
	{
	  if (curfield == numfields - 1)
	    {
	      puts_bidi (linebuf);
	      linepos = 0;
	      printf ("%*s", nextstart, "");
	    }
	  else
	    {
	      linebuf[linepos++] = ' ';
	      curcol++;
	    }
	}
      else if (nextstart != curcol)
	{
	  /* align to field start */
	  n = snprintf (linebuf + linepos, linemax - linepos,
			"%*s", nextstart - curcol, "");
	  linepos += n;
	  curcol = nextstart;
	}
    }

  n = vsnprintf (linebuf + linepos, linemax - linepos, fmt, ap);
  va_end (ap);

  /* Compute output width */
  if (curfield == numfields - 1)
    {
      for ( ; n > 0; n--)
	{
	  int c = linebuf[linepos + n];
	  linebuf[linepos + n] = 0;
	  width = mbswidth (linebuf + linepos, 0);
	  if (width <= fieldwidth[curfield])
	    break;
	  linebuf[linepos + n] = c;
	}
    }
  else
    width = mbswidth (linebuf + linepos, 0);

  /* Increment counters */
  linepos += n;
  curcol += width;
  nextstart += fieldwidth[curfield++];
}

void
init_output (size_t s)
{
  int i;
  size_t width = 0;

  if (s == 0)
    {
      format_field = format_field_simple;
      return;
    }
  
  format_field = format_field_align;
	  
  /* Allocate the line buffer */
  linemax = s * MB_LEN_MAX + 1;
  linebuf = xmalloc (linemax);
  alloc_logical (s);
	  
  /* Set up column widths */
  if (show_number)
    fieldwidth[numfields++] = 5;
  
  if (show_to)
    fieldwidth[numfields++] = 20;
  
  if (show_field)
    fieldwidth[numfields++] = 0;
  else
    {
      fieldwidth[numfields++] = 20;
      fieldwidth[numfields++] = 0;
    }
  
  for (i = 0; i < numfields; i++)
    width += fieldwidth[i];
  
  fieldwidth[numfields-1] = util_getcols () - width;
}


/*
  FIXME: Generalize this function and move it
  to `mailbox/locale.c'. Do the same with the one
  from `from/from.c' and `mail/util.c'...
*/
static char *
rfc2047_decode_wrapper (char *buf, size_t buflen)
{
  int rc;
  char *tmp;
  const char *charset = get_charset ();
  
  if (strcmp (charset, "ASCII") == 0)
    return strdup (buf);

  rc = rfc2047_decode (charset, buf, &tmp);
  if (rc)
    {
      if (dbug)
	mu_error (_("Cannot decode line `%s': %s"),
		  buf, mu_strerror (rc));
      return strdup (buf);
    }

  return tmp;
}

/* Retrieve the Personal Name from the header To: or From:  */
static int
get_personal (header_t hdr, const char *field, char **personal)
{
  char *hfield;
  int status;

  status = header_aget_value_unfold (hdr, field, &hfield);
  if (status == 0)
    {
      address_t address = NULL;
      char *s;
      
      address_create (&address, hfield);
      
      address_aget_personal (address, 1, &s);
      address_destroy (&address);
      if (s == NULL)
	s = hfield;
      else
	free (hfield);

      *personal = rfc2047_decode_wrapper (s, strlen (s));
      free (s);
    }
  return status;
}

static struct
{
  size_t index;
  size_t new;
  size_t read;
  size_t unread;
} counter;

/* Observable action is being called on discovery of each message. */
/* FIXME: The format of the display is poorly done, please correct.  */
static int
action (observer_t o, size_t type)
{
  int status;

  switch (type)
    {
    case MU_EVT_MESSAGE_ADD:
      {
	mailbox_t mbox = observer_get_owner (o);
	message_t msg = NULL;
	header_t hdr = NULL;
	attribute_t attr = NULL;

	counter.index++;

	mailbox_get_message (mbox, counter.index, &msg);

	message_get_attribute (msg, &attr);
	message_get_header (msg, &hdr);

	if (attribute_is_read (attr))
	  counter.read++;
	else if (attribute_is_seen (attr))
	  counter.unread++;
	else if (attribute_is_recent (attr))
	  counter.new++;
	
	if (((select_attribute & IS_READ) && (!attribute_is_read (attr)))
	    || ((select_attribute & IS_NEW) && (!attribute_is_recent (attr)))
	    || ((select_attribute & IS_OLD) && (!attribute_is_seen (attr))))
	  break;

	if (select_attribute)
	  selected = 1;

	if (be_quiet)
	  break;
	
	if (show_number)
	  format_field ("%4lu:", (u_long) counter.index);

	if (show_to)
	  {
	    char *hto;
	    status = get_personal (hdr, MU_HEADER_TO, &hto);

	    if (status == 0)
	      {
		format_field ("(%s)", hto);
		free (hto);
	      }
	    else
	      format_field ("(none)");
	  }

	if (show_field) /* FIXME: This should be also rfc2047_decode. */
	  {
	    char *hfield;
	    status = header_aget_value_unfold (hdr, show_field, &hfield);
	    if (status == 0)
	      {
		format_field ("%s", hfield);
		free (hfield);
	      }
	    else
	      format_field ("");
	  }
	else
	  {
	    char *tmp;
	    status = get_personal (hdr, MU_HEADER_FROM, &tmp);
	    if (status == 0)
	      {
		format_field ("%s", tmp);
		free (tmp);
	      }
	    else
	      format_field ("");

	    status = header_aget_value_unfold (hdr, MU_HEADER_SUBJECT,
					       &tmp);
	    if (status == 0)
	      {
		char *s = rfc2047_decode_wrapper (tmp, strlen (tmp));
		format_field ("%s", s);
		free (s);
		free (tmp);
	      }
	  }
	print_line ();
	break;
      }

    case MU_EVT_MAILBOX_PROGRESS:
      /* Noop.  */
      break;
    }
  return 0;
}

static void
frm_abort (mailbox_t *mbox)
{
  int status;
  
  if ((status = mailbox_close (*mbox)) != 0)
    {
      url_t url;
      
      mu_error (_("Could not close <%s>: %s."),
		url_to_string (url), mu_strerror (status));
      exit (3);
    }
  
  mailbox_destroy (mbox);
  exit (3);
}

/* This is a clone of the elm program call "frm".  It is a good example on
   how to use the observable(callback) of libmailbox.  "frm" has to
   be very interactive, it is not possible to call mailbox_messages_count()
   and wait for the scanning to finish before displaying.  As soon as the scan
   finds a new message we want to know about it. This is done by registering
   an observable type MU_MAILBOX_MSG_ADD. The rest is formatting code.  */

int
frm (char *mailbox_name)
{
  int status;
  mailbox_t mbox;
  url_t url = NULL;
  size_t total = 0;

  status = mailbox_create_default (&mbox, mailbox_name);
  if (status != 0)
    {
      mu_error (_("Could not create mailbox <%s>: %s."),
		mailbox_name ? mailbox_name : _("default"),
		mu_strerror (status));
      exit (3);
    }

  if (dbug)
    {
      mu_debug_t debug;
      mailbox_get_debug (mbox, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  mailbox_get_url (mbox, &url);

  status = mailbox_open (mbox, MU_STREAM_READ);
  if (status == ENOENT)
    /* nothing to do */;
  else if (status != 0)
    {
      mu_error (_("Could not open mailbox <%s>: %s."),
		url_to_string (url), mu_strerror (status));
      frm_abort (&mbox);
    }
  else
    {
      observer_t observer;
      observable_t observable;
      
      observer_create (&observer, mbox);
      observer_set_action (observer, action, mbox);
      mailbox_get_observable (mbox, &observable);
      observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);

      memset (&counter, 0, sizeof counter);
      
      status = mailbox_scan (mbox, 1, &total);
      if (status != 0)
	{
	  mu_error (_("Could not scan mailbox <%s>: %s."),
		    url_to_string (url), mu_strerror (status));
	  frm_abort (&mbox);
	}
      
      observable_detach (observable, observer);
      observer_destroy (&observer, mbox);
      
      if ((status = mailbox_close (mbox)) != 0)
	{
	  mu_error (_("Could not close <%s>: %s."),
		    url_to_string (url), mu_strerror (status));
	  exit (3);
	}
    }
  mailbox_destroy (&mbox);
  
  if (show_summary)
    {
      if (total == 0)
	printf (_("Folder contains no messages."));
      else
	{
	  char *delim = "";
      	  
	  printf (_("Folder contains "));
	
	  if (counter.new)
	    {
	      printf (ngettext ("%lu new message",
				"%lu new messages",
				counter.new),
		      (u_long) counter.new);
	      delim = ", ";
	    }
	  
	  if (counter.unread)
	    {
	      printf ("%s", delim);
	      
	      printf (ngettext ("%lu unread message",
				"%lu unread messages",
				counter.unread),
		      (u_long) counter.unread);
	      delim = ", ";
	    }
	  
	  if (counter.read)
	    {
	      printf ("%s", delim);
	      
	      printf (ngettext ("%lu read message",
				"%lu read messages",
				counter.read),
		      (u_long) counter.read);
	    }
	  /* TRANSLATORS: This dot finishes the sentence

              "Folder contains XXX messages."

	      Leave it as it is unless your language requires to reorder
	      the parts of speach in the message
	  */
	  printf (_("."));
	}
      printf ("\n");
    }
  else if (show_query)
    {
      if (total > 0)
	printf (_("There are messages in that folder.\n"));
      else
	printf (_("No messages in that folder!\n"));
    }
  
  /* EXIT STATUS
     Frm returns a zero status ("true") if messages matching `status' are
     present.  Frm returns 1 if no messages matching `status' are present,
     but there are some messages, returns 2 if there are no messages at
     all, or returns 3 if an error occurred. */
  
  if (selected)
    status = 0;
  else if (total > 0)
    status = 1;
  else
    status = 2;

  return status;
}

int
main (int argc, char **argv)
{
  int c;
  int status = 0;

  /* Native Language Support */
  mu_init_nls ();

  prepare_attrs ();
  
  /* register the formats.  */
  mu_register_all_mbox_formats ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, frm_argp_capa, &c, NULL);

  /* have an argument */
  if (c == argc)
    status = frm (NULL);
  else if (c + 1 == argc)
    status = frm (argv[c]);
  else
    for (; c < argc; c++)
      {
	printf ("%s:\n", argv[c]);
	status = frm (argv[c]);
      }

  return status;
}
