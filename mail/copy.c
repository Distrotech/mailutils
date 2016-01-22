/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2003, 2005, 2007, 2009-2012, 2014-2016 Free
   Software Foundation, Inc.

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
#include <mailutils/locker.h>

/*
 * c[opy] [file]
 * c[opy] [msglist] file
 * C[opy] [msglist]
 */

struct append_stat
{
  size_t size;
  size_t lines;
};

static int
append_to_mailbox (char const *filename, msgset_t *msglist, int mark,
		   struct append_stat *totals)
{
  int status;
  mu_mailbox_t mbx;
  msgset_t *mp;
  size_t size;
  mu_message_t msg;
  
  if ((status = mu_mailbox_create_default (&mbx, filename)) != 0)
    {
      mu_error (_("Cannot create mailbox %s: %s"), filename, 
                   mu_strerror (status));
      return 1;
    }
  if ((status = mu_mailbox_open (mbx, MU_STREAM_WRITE | MU_STREAM_CREAT)) != 0)
    {
      mu_error (_("Cannot open mailbox %s: %s"), filename, 
                   mu_strerror (status));
      mu_mailbox_destroy (&mbx);
      return 1;
    }

  for (mp = msglist; mp; mp = mp->next)
    {
      status = util_get_message (mbox, mp->msg_part[0], &msg);
      if (status)
        break;

      status = mu_mailbox_append_message (mbx, msg);
      if (status)
	{
	  mu_error (_("Cannot append message: %s"), mu_strerror (status));
	  break;
	}
      
      mu_message_size (msg, &size);
      totals->size += size;
      mu_message_lines (msg, &size);
      totals->lines += size;

      if (mark)
 	{
	  mu_attribute_t attr;
	  mu_message_get_attribute (msg, &attr);
	  mu_attribute_set_userflag (attr, MAIL_ATTRIBUTE_SAVED);
	}
    }
  
  mu_mailbox_close (mbx);
  mu_mailbox_destroy (&mbx);
  return 0;
}

static int
append_to_file (char const *filename, msgset_t *msglist, int mark,
		struct append_stat *totals)
{
  int status;
  msgset_t *mp;
  mu_stream_t ostr, mstr;
  mu_off_t size;
  size_t lines;
  mu_message_t msg;
  mu_locker_t locker;
  
  status = mu_file_stream_create (&ostr, filename,
				  MU_STREAM_CREAT|MU_STREAM_APPEND);
  if (status)
    {
      mu_error (_("Cannot open output file %s: %s"),
		filename, mu_strerror (status));
      return 1;
    }
  
  status = mu_locker_create (&locker, filename,
			     MU_LOCKER_KERNEL|MU_LOCKER_RETRY);
  if (status)
    {
      mu_error (_("Cannot create locker %s: %s"),
		filename, mu_strerror (status));
      mu_stream_unref (ostr);
      return 1;
    }
  mu_locker_lock_mode (locker, mu_lck_exc);

  status = mu_locker_lock (locker);
  if (status)
    {
      mu_error (_("Cannot lock %s: %s"),
		filename, mu_strerror (status));
      mu_locker_destroy (&locker);
      mu_stream_unref (ostr);
      return 1;
    }
  
  for (mp = msglist; mp; mp = mp->next)
    {
      mu_envelope_t env;
      const char *s, *d;
      int n;
      
      status = util_get_message (mbox, mp->msg_part[0], &msg);
      if (status)
        break;

      status = mu_message_get_envelope (msg, &env);
      if (status)
	{
	  mu_error (_("Cannot get envelope: %s"), mu_strerror (status));
	  break;
	}

      status = mu_envelope_sget_sender (env, &s);
      if (status)
	{
	  mu_error (_("Cannot get envelope sender: %s"), mu_strerror (status));
	  break;
	}
      
      status = mu_envelope_sget_date (env, &d);
      if (status)
	{
	  mu_error (_("Cannot get envelope date: %s"), mu_strerror (status));
	  break;
	}
      
      status = mu_stream_printf (ostr, "From %s %s\n%n", s, d, &n);
      if (status)
	{
	  mu_error (_("Write error: %s"), mu_strerror (status));
	  break;
	}
      
      totals->lines++;
      totals->size += n;
      
      status = mu_message_get_streamref (msg, &mstr);
      if (status)
	{
	  mu_error (_("Cannot get message: %s"), mu_strerror (status));
	  break;
	}

      status = mu_stream_copy (ostr, mstr, 0, &size);
      if (status)
	{
	  mu_error (_("Cannot append message: %s"), mu_strerror (status));
	  break;
	}
      
      mu_stream_unref (mstr);

      mu_stream_write (ostr, "\n", 1, NULL);
      
      totals->size += size + 1;
      mu_message_lines (msg, &lines);
      totals->lines += lines + 1;

      if (mark)
 	{
	  mu_attribute_t attr;
	  mu_message_get_attribute (msg, &attr);
	  mu_attribute_set_userflag (attr, MAIL_ATTRIBUTE_SAVED);
	}
    }

  mu_stream_close (ostr);
  mu_stream_unref (ostr);

  mu_locker_unlock (locker);
  mu_locker_destroy (&locker);
  
  return 0;
}

/*
 * mail_copy0() is shared between mail_copy() and mail_save().
 * argc, argv -- argument count & vector
 * mark -- whether we should mark the message as saved.
 */
int
mail_copy0 (int argc, char **argv, int mark)
{
  char *filename = NULL;
  msgset_t *msglist = NULL;
  int sender = 0;
  struct append_stat totals = { 0, 0 };
  int status;

  if (mu_isupper (argv[0][0]))
    sender = 1;
  else if (argc >= 2)
    filename = mail_expand_name (argv[--argc]);
  else
    filename = mu_strdup ("mbox");

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &msglist))
    {
      if (filename)
	free (filename);
      return 1;
    }

  if (sender)
    filename = util_outfolder_name (util_get_sender (msglist->msg_part[0], 1));

  if (!filename)
    {
      msgset_free (msglist);
      return 1;
    }

  if (mu_is_proto (filename))
    status = append_to_mailbox (filename, msglist, mark, &totals);
  else
    status = append_to_file (filename, msglist, mark, &totals);
  
  if (status == 0)
    mu_printf ("\"%s\" %3lu/%-5lu\n", filename,
	     (unsigned long) totals.lines, (unsigned long) totals.size);

  free (filename);
  msgset_free (msglist);
  return 0;
}

int
mail_copy (int argc, char **argv)
{
  return mail_copy0 (argc, argv, 0);
}
