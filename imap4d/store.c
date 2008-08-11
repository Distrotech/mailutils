/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007, 2008 Free Software Foundation, Inc.

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

#include "imap4d.h"

/*
 * Now you're messing with a sumbitch
 */

int
imap4d_store (struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *err_text;
  
  rc = imap4d_store0 (tok, 0, &err_text);
  return util_finish (command, rc, "%s", err_text);
}

struct parsebuf
{
  char *token;
  imap4d_tokbuf_t tok;
  int arg;
  jmp_buf errjmp;
  char *err_text;
};

static void
parsebuf_exit (struct parsebuf *p, char *text)
{
  p->err_text = text;
  longjmp (p->errjmp, 1);
}

static char *
parsebuf_next (struct parsebuf *p, int req)
{
  p->token = imap4d_tokbuf_getarg (p->tok, p->arg++);
  if (!p->token && req)
    parsebuf_exit (p, "Too few arguments");
  return p->token;
}

int
imap4d_store0 (imap4d_tokbuf_t tok, int isuid, char **ptext)
{
  char *msgset;
  int status;
  int ack = 1;
  size_t i;
  int n = 0;
  size_t *set = NULL;
  enum value_type { STORE_SET, STORE_ADD, STORE_UNSET } how;
  struct parsebuf pb;
  char *data;
  int type = 0;
  
  pb.tok = tok;
  pb.arg = IMAP4_ARG_1 + !!isuid;
  pb.err_text = NULL;
  if (setjmp (pb.errjmp))
    {
      *ptext = pb.err_text;
      free (set);
      return RESP_BAD;
    }
      
  msgset = parsebuf_next (&pb, 1);
  data = parsebuf_next (&pb, 1);

  if (*data == '+')
    {
      how = STORE_ADD;
      data++;
    }
  else if (*data == '-')
    {
      how = STORE_UNSET;
      data++;
    }
  else
    how = STORE_SET;
  
  if (strcasecmp (data, "FLAGS"))
    parsebuf_exit (&pb, "Bogus data item");
  data = parsebuf_next (&pb, 1);

  if (*data == '.')
    {
      data = parsebuf_next (&pb, 1);
      if (strcasecmp (data, "SILENT") == 0)
	{
	  ack = 0;
	  parsebuf_next (&pb, 1);
	}
      else
	parsebuf_exit (&pb, "Bogus data suffix");
    }

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, isuid);
  if (status != 0)
    parsebuf_exit (&pb, "Bogus number set");

  if (pb.token[0] != '(')
    parsebuf_exit (&pb, "Syntax error");
  parsebuf_next (&pb, 1);
  
  do
    {
      int t;
      if (!util_attribute_to_type (pb.token, &t))
	type |= t;
    }
  while (parsebuf_next (&pb, 1) && pb.token[0] != ')');

  for (i = 0; i < n; i++)
    {
      mu_message_t msg = NULL;
      mu_attribute_t attr = NULL;
      size_t msgno = isuid ? uid_to_msgno (set[i]) : set[i];
      
      if (msgno)
	{
	  mu_mailbox_get_message (mbox, msgno, &msg);
	  mu_message_get_attribute (msg, &attr);

	  switch (how)
	    {
	    case STORE_ADD:
	      mu_attribute_set_flags (attr, type);
	      break;
      
	    case STORE_UNSET:
	      mu_attribute_unset_flags (attr, type);
	      break;
      
	    case STORE_SET:
	      mu_attribute_unset_flags (attr, 0xffffffff); /* FIXME */
	      mu_attribute_set_flags (attr, type);
	    }
	}

      if (ack)
	{
	  util_send ("* %d FETCH (", msgno);

	  if (isuid)
	    util_send ("UID %lu ", (unsigned long) msgno);
	  util_send ("FLAGS (");
	  util_print_flags (attr);
	  util_send ("))\r\n");
	}
      /* Update the flags of uid table.  */
      imap4d_sync_flags (set[i]);
    }
  free (set);
  *ptext = "Completed";
  return RESP_OK;
}

