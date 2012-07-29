/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007-2012 Free Software Foundation,
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

#include "imap4d.h"

enum value_type { STORE_SET, STORE_ADD, STORE_UNSET };

struct store_parse_closure
{
  enum value_type how;
  int ack;
  int type;
  int isuid;
  mu_msgset_t msgset;
};
  
static int
store_thunk (imap4d_parsebuf_t p)
{
  struct store_parse_closure *pclos = imap4d_parsebuf_data (p);
  char *mstr;
  char *data;
  int status;
  char *end;
  
  mstr = imap4d_parsebuf_next (p, 1);
  data = imap4d_parsebuf_next (p, 1);

  if (*data == '+')
    {
      pclos->how = STORE_ADD;
      data++;
    }
  else if (*data == '-')
    {
      pclos->how = STORE_UNSET;
      data++;
    }
  else
    pclos->how = STORE_SET;
  
  if (mu_c_strcasecmp (data, "FLAGS"))
    imap4d_parsebuf_exit (p, "Bogus data item");
  data = imap4d_parsebuf_next (p, 1);

  if (*data == '.')
    {
      data = imap4d_parsebuf_next (p, 1);
      if (mu_c_strcasecmp (data, "SILENT") == 0)
	{
	  pclos->ack = 0;
	  imap4d_parsebuf_next (p, 1);
	}
      else
	imap4d_parsebuf_exit (p, "Bogus data suffix");
    }

  status = mu_msgset_create (&pclos->msgset, mbox, MU_MSGSET_NUM);
  if (status)
    imap4d_parsebuf_exit (p, "Software error");
  
  /* Get the message numbers in set[].  */
  status = mu_msgset_parse_imap (pclos->msgset,
				 pclos->isuid ? MU_MSGSET_UID : MU_MSGSET_NUM,
				 mstr, &end);
  if (status)
    imap4d_parsebuf_exit (p, "Failed to parse message set");

  if (strcmp (p->token, "NIL"))
    {
      if (p->token[0] != '(')
	imap4d_parsebuf_exit (p, "Syntax error");
      while (imap4d_parsebuf_next (p, 1) && p->token[0] != ')')
	if (mu_imap_flag_to_attribute (p->token, &pclos->type))
	  imap4d_parsebuf_exit (p, "Unrecognized flag");
    }
  return RESP_OK;
}

static int
_do_store (size_t msgno, mu_message_t msg, void *data)
{
  struct store_parse_closure *pclos = data;
  mu_attribute_t attr = NULL;
      
  mu_message_get_attribute (msg, &attr);
	      
  switch (pclos->how)
    {
    case STORE_ADD:
      mu_attribute_set_flags (attr, pclos->type);
      break;
      
    case STORE_UNSET:
      mu_attribute_unset_flags (attr, pclos->type);
      break;
      
    case STORE_SET:
      mu_attribute_unset_flags (attr, 0xffffffff); /* FIXME */
      mu_attribute_set_flags (attr, pclos->type);
    }

	  
  if (pclos->ack)
    {
      io_sendf ("* %lu FETCH (", (unsigned long) msgno);
      
      if (pclos->isuid)
	{
	  size_t uid;
	  int rc = mu_mailbox_translate (mbox, MU_MAILBOX_UID_TO_MSGNO,
					 msgno, &uid);
	  if (rc == 0)
	    io_sendf ("UID %lu ", (unsigned long) uid);
	}
      io_sendf ("FLAGS (");
      util_print_flags (attr);
      io_sendf ("))\n");
    }
  /* Update the flags of uid table.  */
  imap4d_sync_flags (msgno);
  return 0;
}

int
imap4d_store0 (imap4d_tokbuf_t tok, int isuid, char **ptext)
{
  int rc;
  struct store_parse_closure pclos;
  
  memset (&pclos, 0, sizeof pclos);
  pclos.ack = 1;
  pclos.isuid = isuid;
  
  rc = imap4d_with_parsebuf (tok,
			     IMAP4_ARG_1 + !!isuid,
			     ".",
			     store_thunk, &pclos,
			     ptext);
  if (rc == RESP_OK)
    {
      mu_msgset_foreach_message (pclos.msgset, _do_store, &pclos);
    
      *ptext = "Completed";
    }
  
  mu_msgset_free (pclos.msgset);
  
  return rc;
}

int
imap4d_store (struct imap4d_session *session,
              struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *err_text = NULL;
  
  rc = imap4d_store0 (tok, 0, &err_text);
  return io_completion_response (command, rc, "%s", err_text ? err_text : "");
}

