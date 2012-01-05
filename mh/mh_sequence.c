/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2009-2012 Free Software Foundation,
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
#include <mailutils/sys/msgset.h>

static char *
private_sequence_name (const char *name)
{
  char *p;
  char *mbox_dir = mh_expand_name (NULL, mh_current_folder (), 0);
  mu_asprintf (&p, "atr-%s-%s", name, mbox_dir);
  free (mbox_dir);
  return p;
}

const char *
mh_seq_read (mu_mailbox_t mbox, const char *name, int flags)
{
  const char *value;

  if (flags & SEQ_PRIVATE)
    {
      char *p = private_sequence_name (name);
      value = mh_global_context_get (p, NULL);
      free (p);
    }
  else
    value = mh_global_sequences_get (mbox, name, NULL);
  return value;
}

static void
write_sequence (mu_mailbox_t mbox, const char *name, char *value, int private)
{
  if (private)
    {
      char *p = private_sequence_name (name);
      mh_global_context_set (p, value);
      free (p);
    }
  else
    mh_global_sequences_set (mbox, name, value);
}

static void
delete_sequence (mu_mailbox_t mbox, const char *name, int private)
{
  write_sequence (mbox, name, NULL, private);
}

struct format_closure
{
  mu_stream_t stream;
  mu_mailbox_t mailbox;
};

static int
format_sequence (void *item, void *data)
{
  struct mu_msgrange *r = item;
  struct format_closure *clos = data;
  int rc;
  size_t beg, end;

  if (clos->mailbox)
    {
      rc = mu_mailbox_translate (clos->mailbox,
				 MU_MAILBOX_MSGNO_TO_UID,
				 r->msg_beg, &beg);
      if (rc)
	return rc;
    }
  else
    beg = r->msg_beg;
  if (r->msg_beg == r->msg_end)
    rc = mu_stream_printf (clos->stream, " %lu", (unsigned long) beg);
  else
    {
      if (clos->mailbox)
	{
	  rc = mu_mailbox_translate (clos->mailbox,
				     MU_MAILBOX_MSGNO_TO_UID,
				     r->msg_end, &end);
	  if (rc)
	    return rc;
	}
      else
	end = r->msg_end;
      if (beg + 1 == end)
	rc = mu_stream_printf (clos->stream, " %lu %lu",
			       (unsigned long) beg,
			       (unsigned long) end);
      else
	rc = mu_stream_printf (clos->stream, " %lu-%lu",
			       (unsigned long) beg,
			       (unsigned long) end);
    }
  return rc;
}

static void
save_sequence (mu_mailbox_t mbox, const char *name, mu_msgset_t mset,
	       int flags)
{
  mu_list_t list;
  
  mu_msgset_get_list (mset, &list);
  if (mu_list_is_empty (list))
    write_sequence (mset->mbox, name, NULL, flags & SEQ_PRIVATE);
  else
    {
      struct format_closure clos;
      int rc;
      mu_transport_t trans[2];
      
      rc = mu_memory_stream_create (&clos.stream, MU_STREAM_RDWR);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_memory_stream_create", NULL, rc);
	  exit (1);
	}
      
      clos.mailbox = mset->mbox;
      rc = mu_list_foreach (list, format_sequence, &clos);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_list_foreach", NULL, rc);
	  exit (1);
	}
      mu_stream_write (clos.stream, "", 1, NULL);
      mu_stream_ioctl (clos.stream, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET,
		       trans);
      write_sequence (mbox, name, (char*)trans[0], flags & SEQ_PRIVATE);
      mu_stream_unref (clos.stream);
    }
}

void
mh_seq_add (mu_mailbox_t mbox, const char *name, mu_msgset_t mset, int flags)
{
  const char *value = mh_seq_read (mbox, name, flags);
  
  delete_sequence (mbox, name, !(flags & SEQ_PRIVATE));
  if (value && !(flags & SEQ_ZERO))
    {
      mu_msgset_t oldset;
      mh_msgset_parse_string (&oldset, mbox, value, "cur");
      mu_msgset_add (oldset, mset);
      save_sequence (mbox, name, oldset, flags);
      mu_msgset_free (oldset);
    }
  else
    save_sequence (mbox, name, mset, flags);
}

int
mh_seq_delete (mu_mailbox_t mbox, const char *name,
	       mu_msgset_t mset, int flags)
{
  const char *value = mh_seq_read (mbox, name, flags);
  mu_msgset_t oldset;

  if (!value)
    return 0;
  mh_msgset_parse_string (&oldset, mbox, value, "cur");
  mu_msgset_sub (oldset, mset);
  save_sequence (mbox, name, oldset, flags);
  mu_msgset_free (oldset);
  return 0;
}

