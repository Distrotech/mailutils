/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 
   2005 Free Software Foundation, Inc.

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

#include "mail.h"

/*
  FIXME:
 decode, this is temporary, until the API on how to present
 mime/attachements etc is less confusing.
 */

struct decode_closure
{
  int select_hdr;
};

static int print_stream __P ((stream_t, FILE *));
static int display_message __P ((message_t, msgset_t *msgset, void *closure));
static int display_message0 __P ((message_t, const msgset_t *, int));
static int get_content_encoding __P ((header_t hdr, char **value));
static void run_metamail __P((const char *mailcap, message_t mesg));

int
mail_decode (int argc, char **argv)
{
  msgset_t *msgset;
  struct decode_closure decode_closure;

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &msgset))
    return 1;

  decode_closure.select_hdr = islower (argv[0][0]);

  util_msgset_iterate (msgset, display_message, (void *)&decode_closure);

  msgset_free (msgset);
  return 0;
}

int
display_message (message_t mesg, msgset_t *msgset, void *arg)
{
  struct decode_closure *closure = arg;
  attribute_t attr = NULL;

  message_get_attribute (mesg, &attr);
  if (attribute_is_deleted (attr))
    return 1;

  display_message0 (mesg, msgset, closure->select_hdr);

  /* Mark enclosing message as read */
  if (mailbox_get_message (mbox, msgset->msg_part[0], &mesg) == 0)
    util_mark_read (mesg);

  return 0;
}

static void
display_headers (FILE *out, message_t mesg, const msgset_t *msgset ARG_UNUSED,
		 int select_hdr)
{
  header_t hdr = NULL;

  /* Print the selected headers only.  */
  if (select_hdr)
    {
      size_t num = 0;
      size_t i = 0;
      char buffer[512];

      message_get_header (mesg, &hdr);
      header_get_field_count (hdr, &num);
      for (i = 1; i <= num; i++)
	{
	  buffer[0] = '\0';
	  header_get_field_name (hdr, i, buffer, sizeof(buffer), NULL);
	  if (mail_header_is_visible (buffer))
	    {
	      fprintf (out, "%s: ", buffer);
	      header_get_field_value (hdr, i, buffer, sizeof(buffer), NULL);
	      fprintf (out, "%s\n", buffer);
	    }
	}
      fprintf (out, "\n");
    }
  else /* Print displays all headers.  */
    {
      stream_t stream = NULL;
      if (message_get_header (mesg, &hdr) == 0
	  && header_get_stream (hdr, &stream) == 0)
	print_stream (stream, out);
    }
}

static void
display_part_header (FILE *out, const msgset_t *msgset,
		     char *type, char *encoding)
{
  int size = util_screen_columns () - 3;
  unsigned int i;

  fputc ('+', out);
  for (i = 0; (int)i <= size; i++)
    fputc ('-', out);
  fputc ('+', out);
  fputc ('\n', out);
  fprintf (out, _("| Message=%d"), msgset->msg_part[0]);
  for (i = 1; i < msgset->npart; i++)
    fprintf (out, "[%d", msgset->msg_part[i]);
  for (i = 1; i < msgset->npart; i++)
    fprintf (out, "]");
  fprintf (out, "\n");

  fprintf (out, _("| Type=%s\n"), type);
  fprintf (out, _("| Encoding=%s\n"), encoding);
  fputc ('+', out);
  for (i = 0; (int)i <= size; i++)
    fputc ('-', out);
  fputc ('+', out);
  fputc ('\n', out);
}

static int
display_message0 (message_t mesg, const msgset_t *msgset,
		  int select_hdr)
{
  size_t nparts = 0;
  header_t hdr = NULL;
  const char *type;
  char *encoding;
  int ismime = 0;
  char *tmp;

  message_get_header (mesg, &hdr);
  util_get_content_type (hdr, &type);
  get_content_encoding (hdr, &encoding);

  message_is_multipart (mesg, &ismime);
  if (ismime)
    {
      unsigned int j;
      
      message_get_num_parts (mesg, &nparts);

      for (j = 1; j <= nparts; j++)
	{
	  message_t message = NULL;

	  if (message_get_part (mesg, j, &message) == 0)
	    {
	      msgset_t *set = msgset_expand (msgset_dup (msgset),
					     msgset_make_1 (j));
	      display_message0 (message, set, 0);
	      msgset_free (set);
	    }
	}
    }
  else if (strncasecmp (type, "message/rfc822", strlen (type)) == 0)
    {
      message_t submsg = NULL;

      if (message_unencapsulate (mesg, &submsg, NULL) == 0)
	display_message0 (submsg, msgset, select_hdr);
    }
  else if (util_getenv (&tmp, "metamail", Mail_env_string, 0) == 0)
    {
      run_metamail (tmp, mesg);
    }
  else
    {
      body_t body = NULL;
      stream_t b_stream = NULL;
      stream_t d_stream = NULL;
      stream_t stream = NULL;
      header_t hdr = NULL;
      char *no_ask = NULL;
      
      message_get_body (mesg, &body);
      message_get_header (mesg, &hdr);
      body_get_stream (body, &b_stream);

      /* Can we decode.  */
      if (filter_create(&d_stream, b_stream, encoding,
			MU_FILTER_DECODE, MU_STREAM_READ) == 0)
	stream = d_stream;
      else
	stream = b_stream;

      util_getenv (&no_ask, "mimenoask", Mail_env_string, 0);
      
      display_part_header (ofile, msgset, type, encoding);
      if (display_stream_mailcap (NULL, stream, hdr, no_ask, interactive,
				  0,
				  util_getenv (NULL, "verbose",
					       Mail_env_boolean, 0) ? 0 : 9))
	{
	  size_t lines = 0;
	  int pagelines = util_get_crt ();
	  FILE *out;
	  
	  message_lines (mesg, &lines);
	  if (pagelines && lines > pagelines)
	    out = popen (getenv ("PAGER"), "w");
	  else
	    out = ofile;

	  display_headers (out, mesg, msgset, select_hdr);

	  print_stream (stream, out);

	  if (out != ofile)
	    pclose (out);
	}
      if (d_stream)
	stream_destroy (&d_stream, NULL);
    }

  free (type);
  free (encoding);

  return 0;
}

static int
print_stream (stream_t stream, FILE *out)
{
  char buffer[512];
  off_t off = 0;
  size_t n = 0;

  while (stream_read (stream, buffer, sizeof (buffer) - 1, off, &n) == 0
	 && n != 0)
    {
      if (ml_got_interrupt())
	{
	  util_error(_("\nInterrupt"));
	  break;
	}
      buffer[n] = '\0';
      fprintf (out, "%s", buffer);
      off += n;
    }
  return 0;
}

static int
get_content_encoding (header_t hdr, char **value)
{
  char *encoding = NULL;
  util_get_hdr_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, &encoding);
  if (encoding == NULL || *encoding == '\0')
    {
      if (encoding)
	free (encoding);
      encoding = strdup ("7bit"); /* Default.  */
    }
  *value = encoding;
  return 0;
}

/* Run `metamail' program MAILCAP_CMD on the message MESG */
static void
run_metamail (const char *mailcap_cmd, message_t mesg)
{
  pid_t pid;
  struct sigaction ignore;
  struct sigaction saveintr;
  struct sigaction savequit;
  sigset_t chldmask;
  sigset_t savemask;
  
  ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
  ignore.sa_flags = 0;
  sigemptyset (&ignore.sa_mask);
  if (sigaction (SIGINT, &ignore, &saveintr) < 0)
    {
      util_error ("sigaction: %s", strerror (errno));
      return;
    }      
  if (sigaction (SIGQUIT, &ignore, &savequit) < 0)
    {
      util_error ("sigaction: %s", strerror (errno));
      sigaction (SIGINT, &saveintr, NULL);
      return;
    }      
      
  sigemptyset (&chldmask);	/* now block SIGCHLD */
  sigaddset (&chldmask, SIGCHLD);

  if (sigprocmask (SIG_BLOCK, &chldmask, &savemask) < 0)
    {
      sigaction (SIGINT, &saveintr, NULL);
      sigaction (SIGQUIT, &savequit, NULL);
      return;
    }
  
  pid = fork ();
  if (pid < 0)
    {
      util_error ("fork: %s", strerror (errno));
    }
  else if (pid == 0)
    {
      /* Child process */
      int status;
      stream_t stream;

      do /* Fake loop to avoid gotos */
	{
	  stream_t pstr;
	  char buffer[512];
	  size_t n;
	  char *no_ask;
	  
	  setenv ("METAMAIL_PAGER", getenv ("PAGER"), 0);
	  if (util_getenv (&no_ask, "mimenoask", Mail_env_string, 0))
	    setenv ("MM_NOASK", no_ask, 1);
	  
	  status = message_get_stream (mesg, &stream);
	  if (status)
	    {
	      mu_error ("message_get_stream: %s", mu_strerror (status));
	      break;
	    }

	  status = prog_stream_create (&pstr, mailcap_cmd, MU_STREAM_WRITE);
	  if (status)
	    {
	      mu_error ("prog_stream_create: %s", mu_strerror (status));
	      break;
	    }

	  status = stream_open (pstr);
	  if (status)
	    {
	      mu_error ("stream_open: %s", mu_strerror (status));
	      break;
	    }
	  
	  while (stream_sequential_read (stream,
					 buffer, sizeof buffer - 1,
					 &n) == 0
		 && n > 0)
	    stream_sequential_write (pstr, buffer, n);

	  stream_close (pstr);
	  stream_destroy (&pstr, stream_get_owner (pstr));
	  exit (0);
	}
      while (0);
      
      abort ();
    }
  else
    {
      int status;
      /* Master process */
      
      while (waitpid (pid, &status, 0) < 0)
	if (errno != EINTR)
	  break;
    }

  /* Restore the signal handlers */
  sigaction (SIGINT, &saveintr, NULL);
  sigaction (SIGQUIT, &savequit, NULL);
  sigprocmask (SIG_SETMASK, &savemask, NULL);
}
