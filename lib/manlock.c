/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/diag.h>
#include <mailutils/locker.h>
#include <mailutils/mailbox.h>
#include <mailutils/util.h>
#include <mailutils/url.h>
#include <mailutils/stream.h>
#include <mailutils/registrar.h>
#include <mailutils/nls.h>
#include <mailutils/cfg.h>
#include "muaux.h"

/* This file implements mandatory locking for pop3d and imap4d.  Mandatory
   locking applies to all mailbox formats.  It is enabled by the following
   configuration clause:

   mandatory-locking {
     enable yes;
   }

   If the underlying mailbox mechanism does not provide a locker (as is
   the case, e.g., for maildir or MH mailboxes), a new locker object is
   created and attached to the mailbox.  This new locker complies with
   the "logging" configuration settings.  The name of the corresponding
   lock file is created based on the mailbox URL.  It is created in
   a "lock directory", which is configured by `lock-directory' clause in
   the `mandatory-locking' block.  
   For example, a lock file for "/var/spool/mail/g/r/gray" would
   be "/var/lock/subsys/mail/%2Fvar%2Fspool%2Fmail%2Fg%2Fr%2Fgray.lock".
*/

int manlock_mandatory_locking;
char *manlock_lock_dir = "/var/lock/subsys/mail";

static char *
make_locker_file_name (const char *urlstr)
{
  char *fname;
  char *buf;
  size_t size;
  const char *p;
  char *q;
  static const char escapable_chars[] = "/%";
  static const char xchr[] = "0123456789ABCDEF";
	  
  for (p = urlstr, size = 0; *p; p++, size++)
    {
      if (strchr (escapable_chars, *p))
	size += 2;
    }
  
  buf = malloc (size + 1);
  if (!buf)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "malloc", NULL, errno);
      return NULL;
    }
	  
  for (p = urlstr, q = buf; *p; p++)
    {
      if (strchr (escapable_chars, *p))
	{
	  unsigned char c = *p;
	  *q++ = '%';
	  *q++ = xchr[c >> 4];
	  *q++ = xchr[c & 0xf];
	}
      else
	*q++ = *p;
    }
  *q = 0;
  
  fname = mu_make_file_name_suf (manlock_lock_dir, buf, NULL);
  if (!fname)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_make_file_name_suf", buf, errno);
  free (buf);
  return fname;
}

static int
mailbox_open_and_lock (mu_mailbox_t mbox, int flags)
{
  mu_url_t url;
  int status;
  const char *urlstr;
  mu_locker_t lock;
  
  mu_mailbox_get_url (mbox, &url);
  urlstr = mu_url_to_string (url);
  
  if ((status = mu_mailbox_open (mbox, flags)) != 0)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open", urlstr, status);
      return MU_ERR_FAILURE;
    }
  /* This is a mild race condition: mailbox needs to be opened in order to
     obtain its locker.  Other process can acquire the lock in the meantime,
     but that's not critical, because manlock_lock below will fail if unable
     to lock the mailbox anyway. */
  status = mu_mailbox_get_locker (mbox, &lock);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_locker", urlstr, status);
      return MU_ERR_FAILURE;
    }

  if (!lock && manlock_mandatory_locking && manlock_lock_dir)
    {
      char *fname = NULL;
      int res;

      if (mu_registrar_test_local_url (url, &res) == 0 && res)
	{
	  const char *path;
	  status = mu_url_sget_path (url, &path);
	  if (status == 0)
	    fname = make_locker_file_name (path);
	}
      if (!fname)
	fname = make_locker_file_name (urlstr);

      if (!fname)
	return MU_ERR_FAILURE;
	  
      status = mu_locker_create (&lock, fname, 0);
      if (status)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_locker_create", fname, status);
	  free (fname);
	  return MU_ERR_FAILURE;
	}
      mu_mailbox_set_locker (mbox, lock);
    }

  return manlock_lock (mbox);
}

int
manlock_open_mailbox (mu_mailbox_t *pmbox, const char *mailbox_name, int def,
		      int flags)
{
  mu_mailbox_t mbox;
  int status;

  status = (def ? mu_mailbox_create_default : mu_mailbox_create)
               (&mbox, mailbox_name);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_create", mailbox_name,
		       status);
      return 1;
    }

  status = mailbox_open_and_lock (mbox, flags);
  
  if (status == 0)
    *pmbox = mbox;
  else
    mu_mailbox_destroy (&mbox);
  
  return status;
}


struct mu_cfg_param manlock_param[] = {
  { "enable", mu_cfg_bool, &manlock_mandatory_locking, 0, NULL,
    N_("Enable mandatory locking.") },
  { "lock-directory", mu_cfg_string, &manlock_lock_dir, 0, NULL,
    N_("Set directory for mandatory lock files.") },
  { NULL }
};

void
manlock_cfg_init ()
{
  struct mu_cfg_section *section;
  mu_create_canned_section ("mandatory-locking", &section);
  mu_cfg_section_add_params (section, manlock_param);
}



  

int
manlock_lock (mu_mailbox_t mbox)
{
  mu_url_t url = NULL;
  mu_locker_t lock = NULL;
  const char *name;
  int status;

  if (!manlock_mandatory_locking)
    return 0;
  mu_mailbox_get_url (mbox, &url);
  name = mu_url_to_string (url);
  mu_mailbox_get_locker (mbox, &lock);  
  mu_locker_mod_flags (lock, MU_LOCKER_PID, mu_locker_set_bit);
  if ((status = mu_locker_lock (lock)))
    {
      mu_diag_output (MU_DIAG_NOTICE, _("locking mailbox `%s' failed: %s"),
	      name ? name : "?", mu_strerror(status));
      return status;
    }
  return 0;
}

int
manlock_touchlock (mu_mailbox_t mbox)
{
  if (manlock_mandatory_locking)
    {
      mu_locker_t lock = NULL;
      mu_mailbox_get_locker (mbox, &lock);
      mu_locker_touchlock (lock);
    }
  return 0;
}

int
manlock_unlock (mu_mailbox_t mbox)
{
  if (manlock_mandatory_locking)
    {
      mu_locker_t lock = NULL;
      mu_mailbox_get_locker (mbox, &lock);
      mu_locker_unlock (lock);
    }
  return 0;
}
