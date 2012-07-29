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

int
make_interdir (const char *name, int delim, int perms)
{
  int rc;
  size_t i;
  struct mu_wordsplit ws;
  char *namebuf;
  size_t namelen = 0;
  char delimbuf[2];
  
  namebuf = mu_alloc (strlen (name) + 1);
  if (name[0] == '/')
    namebuf[namelen++] = name[0];

  delimbuf[0] = delim;
  delimbuf[1] = 0;
  ws.ws_delim = delimbuf;
  if (mu_wordsplit (name, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    {
      mu_error (_("cannot split line `%s': %s"), name,
		mu_wordsplit_strerror (&ws));
      free (namebuf);
      return 1;
    }

  rc = 0;
  for (i = 0; rc == 0 && i < ws.ws_wordc - 1; i++)
    {
      struct stat st;
      
      strcpy (namebuf + namelen, ws.ws_wordv[i]);
      namelen += strlen (ws.ws_wordv[i]);

      if (stat (namebuf, &st))
	{
	  if (errno == ENOENT)
	    {
	      if (mkdir (namebuf, perms))
		{
		  mu_error (_("cannot create directory %s: %s"), namebuf,
			    mu_strerror (errno));
		  rc = 1;
		}
	    }
	  else
	    {
	      mu_error (_("cannot stat file %s: %s"),
			namebuf, mu_strerror (errno));
	      rc = 1;
	    }
	}
      else if (!S_ISDIR (st.st_mode))
	{
	  mu_error (_("component %s is not a directory"), namebuf);
	  rc = 1;
	}
      namebuf[namelen++] = '/';
    }
  
  mu_wordsplit_free (&ws);
  free (namebuf);
  return rc;
}

/*
6.3.5.  RENAME Command

   Arguments:  existing mailbox name
               new mailbox name

   Responses:  no specific responses for this command

   Result:     OK - rename completed
               NO - rename failure: can't rename mailbox with that name,
                    can't rename to mailbox with that name
               BAD - command unknown or arguments invalid
*/  
/*
  FIXME: Renaming a mailbox we must change the UIDVALIDITY
  of the mailbox.  */

int
imap4d_rename (struct imap4d_session *session,
               struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  char *oldname;
  char *newname;
  int rc = RESP_OK;
  const char *msg = "Completed";
  struct stat newst;
  int ns;
  
  if (imap4d_tokbuf_argc (tok) != 4)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  
  oldname = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);
  newname = imap4d_tokbuf_getarg (tok, IMAP4_ARG_2);

  if (mu_c_strcasecmp (newname, "INBOX") == 0)
    return io_completion_response (command, RESP_NO, "Name Inbox is reservered");

  /* Allocates memory.  */
  newname = namespace_getfullpath (newname, &ns);
  if (!newname)
    return io_completion_response (command, RESP_NO, "Permission denied");

  /* It is an error to attempt to rename from a mailbox name that already
     exist.  */
  if (stat (newname, &newst) == 0)
    {
      /* FIXME: What if it's a maildir?!? */
      if (!S_ISDIR (newst.st_mode))
	{
	  free (newname);
	  return io_completion_response (command, RESP_NO,
	                                 "Already exist, delete first");
	}
    }

  if (make_interdir (newname, MU_HIERARCHY_DELIMITER, MKDIR_PERMISSIONS))
    {
      free (newname);
      return io_completion_response (command, RESP_NO, "Cannot rename");
    }
  
  /* Renaming INBOX is permitted, and has special behavior.  It moves
     all messages in INBOX to a new mailbox with the given name,
     leaving INBOX empty.  */
  if (mu_c_strcasecmp (oldname, "INBOX") == 0)
    {
      mu_mailbox_t newmbox = NULL;
      mu_mailbox_t inbox = NULL;

      if (S_ISDIR (newst.st_mode))
	{
	  free (newname);
	  return io_completion_response (command, RESP_NO, 
	                                 "Cannot be a directory");
	}
      if (mu_mailbox_create (&newmbox, newname) != 0
	  || mu_mailbox_open (newmbox,
			      MU_STREAM_CREAT | MU_STREAM_RDWR
			        | mailbox_mode[ns]) != 0)
	{
	  free (newname);
	  return io_completion_response (command, RESP_NO,
	                                 "Cannot create new mailbox");
	}
      free (newname);

      if (mu_mailbox_create_default (&inbox, auth_data->name) == 0 &&
	  mu_mailbox_open (inbox, MU_STREAM_RDWR) == 0)
	{
	  size_t no;
	  size_t total = 0;
	  mu_mailbox_messages_count (inbox, &total);
	  for (no = 1; no <= total; no++)
	    {
	      mu_message_t message;
	      if (mu_mailbox_get_message (inbox, no, &message) == 0)
		{
		  mu_attribute_t attr = NULL;

		  imap4d_enter_critical ();
		  mu_mailbox_append_message (newmbox, message);
		  imap4d_leave_critical ();
		  mu_message_get_attribute (message, &attr);
		  mu_attribute_set_deleted (attr);
		}
	    }
	  imap4d_enter_critical ();
	  mu_mailbox_expunge (inbox);
	  imap4d_leave_critical ();
	  mu_mailbox_close (inbox);
	  mu_mailbox_destroy (&inbox);
	}
      mu_mailbox_close (newmbox);
      mu_mailbox_destroy (&newmbox);
      return io_completion_response (command, RESP_OK, "Rename successful");
    }

  oldname = namespace_getfullpath (oldname, NULL);

  /* It must exist.  */
  /* FIXME: 1. What if odlname or newname is a remote mailbox?
            2. If newname is local and is in another namespace, its
  	       permissions must be fixed.
            3. All in all, it would perhaps be better to use the same
	       algorithm as for INBOX, and delete source mailbox afterwards.
  */
  if (!oldname)
    {
      rc = RESP_NO;
      msg = "Failed";
    }
  else
    {
      if (rename (oldname, newname) != 0)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "rename", oldname, errno);
          rc = RESP_NO;
          msg = "Failed";
	}
      free (oldname);
    }
  free (newname);
  return io_completion_response (command, rc, "%s", msg);
}
