/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

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

struct imap4d_sizeinfo
{
  mu_off_t size;
  mu_off_t nfiles;
  mu_off_t ndirs;
  mu_off_t nerrs;
};
  
static int
addsize (mu_folder_t folder, struct mu_list_response *resp, void *data)
{
  struct imap4d_sizeinfo *si = data;

  if (resp->type & MU_FOLDER_ATTRIBUTE_DIRECTORY)
    si->ndirs++;
  
  if (resp->type & MU_FOLDER_ATTRIBUTE_FILE)
    {
      mu_off_t size;
      mu_mailbox_t mbox;
      int rc;
      
      si->nfiles++;

      rc = mu_mailbox_create_from_record (&mbox, resp->format, resp->name);

      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_create_from_record",
			   resp->name, rc);
	  si->nerrs++;
	  return 0;
	}

      rc = mu_mailbox_open (mbox, MU_STREAM_READ);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open",
			   resp->name, rc);
	  si->nerrs++;
	  mu_mailbox_destroy (&mbox);
	  return 0;
	}

      rc = mu_mailbox_get_size (mbox, &size);
      mu_mailbox_destroy (&mbox);

      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open",
			   resp->name, rc);
	  si->nerrs++;
	  return 0;
	}
      si->size += size;
    }
  return 0;
}
  
void
directory_size (const char *dirname, mu_off_t *size)
{
  mu_folder_t folder;
  struct imap4d_sizeinfo sizeinfo;
  int status;
  
  status = mu_folder_create (&folder, dirname);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_create", dirname, status);
      return;
    }

  status = mu_folder_open (folder, MU_STREAM_READ);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_open", dirname, status);
      mu_folder_destroy (&folder);
      return;
    }

  memset (&sizeinfo, 0, sizeof (sizeinfo));
  status = mu_folder_enumerate (folder, NULL, "*", 0, 0, NULL,
				addsize, &sizeinfo);
  if (status)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_folder_enumerate", dirname, status);
  else
    {
      mu_diag_output (MU_DIAG_INFO,
		      _("%s statistics: size=%lu, ndirs=%lu, nfiles=%lu, nerrs=%lu"),
		      dirname,
		      (unsigned long)sizeinfo.size,
		      (unsigned long)sizeinfo.ndirs,
		      (unsigned long)sizeinfo.nfiles,
		      (unsigned long)sizeinfo.nerrs);
    }
  *size = sizeinfo.size;
}


mu_off_t used_size;

void
quota_setup ()
{
  directory_size (imap4d_homedir, &used_size);
}

int
quota_check (mu_off_t size)
{
  char *mailbox_name;
  mu_mailbox_t mbox;
  mu_off_t total;
  int rc;
  
  if (auth_data->quota == 0)
    return RESP_OK;

  total = used_size;

  mailbox_name = namespace_getfullpath ("INBOX", NULL);
  rc = mu_mailbox_create (&mbox, mailbox_name);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_create", mailbox_name, rc);
      free (mailbox_name);
    }
  else
    {
      do
	{
	  mu_off_t mbsize;
	  
	  rc = mu_mailbox_open (mbox, MU_STREAM_READ);
	  if (rc)
	    {
	      if (rc != ENOENT)
		mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open",
				 mailbox_name, rc);
	      break;
	    }
	  
	  rc = mu_mailbox_get_size (mbox, &mbsize);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_size",
			       mailbox_name, rc);
	      mu_mailbox_close (mbox);
	      break;
	    }
	  total += mbsize;
	  mu_mailbox_close (mbox);
	}
      while (0);
      mu_mailbox_destroy (&mbox);
    }
  free (mailbox_name);

  if (rc)
    return RESP_BAD;
  
  if (total > auth_data->quota)
    {
      mu_diag_output (MU_DIAG_NOTICE,
		      _("user %s is out of mailbox quota"),
		      auth_data->name);
      return RESP_NO;
    }
  else if (total + size > auth_data->quota)
    {
      mu_diag_output (MU_DIAG_NOTICE,
		      _("user %s: adding %lu bytes would exceed mailbox quota"),
		      auth_data->name, (unsigned long) size);
      return RESP_NO;
    }
  return RESP_OK;
}

void
quota_update (mu_off_t size)
{
  used_size += size;
}
