/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

/* First draft by Sergey Poznyakoff */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#ifdef WITH_PTHREAD
# ifdef HAVE_PTHREAD_H
#  ifndef _XOPEN_SOURCE
#   define _XOPEN_SOURCE  500
#  endif
#  include <pthread.h>
# endif
#endif

#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/envelope.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/locker.h>
#include <mailutils/message.h>
#include <mailutils/util.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/observer.h>
#include <mailutils/io.h>
#include <mailutils/cctype.h>
#include <mailutils/mh.h>
#include <mailutils/sys/mailbox.h>
#include <mailutils/sys/registrar.h>
#include <mailutils/sys/amd.h>

struct _mh_message
{
  struct _amd_message amd_message;
  size_t seq_number;        /* message sequence number */
};

static int
mh_message_cmp (struct _amd_message *a, struct _amd_message *b)
{
  struct _mh_message *ma = (struct _mh_message *) a;
  struct _mh_message *mb = (struct _mh_message *) b;
  if (ma->seq_number < mb->seq_number)
    return -1;
  else if (ma->seq_number > mb->seq_number)
    return 1;
  return 0;
}

static size_t
_mh_next_seq (struct _amd_data *amd)
{
  struct _mh_message *msg = (struct _mh_message *)
                              _amd_get_message (amd, amd->msg_count);
  return (msg ? msg->seq_number : 0) + 1;
}

/* Return current filename for the message.
   NOTE: Allocates memory. */
static int
_mh_cur_message_name (struct _amd_message *amsg, char **pname)
{
  int status = 0;
  struct _mh_message *mhm = (struct _mh_message *) amsg;
  char *filename;
  char *pnum;
  size_t len;

  status = mu_asprintf (&pnum, "%lu", (unsigned long) mhm->seq_number);
  if (status)
    return status;
  len = strlen (amsg->amd->name) + 1 + strlen (pnum) + 1;
  filename = malloc (len);
  if (filename)
    {
      strcpy (filename, amsg->amd->name);
      strcat (filename, "/");
      strcat (filename, pnum);
      *pname = filename;
    }
  else
    status = ENOMEM;
  free (pnum);
  return status;
}

/* Return newfilename for the message.
   NOTE: Allocates memory. */
static int
_mh_new_message_name (struct _amd_message *amsg, int flags,
		      int expunge MU_ARG_UNUSED,
		      char **pname)
{
  int status = 0;
  struct _mh_message *mhm = (struct _mh_message *) amsg;
  char *filename;
  char *pnum;
  size_t len;

  status = mu_asprintf (&pnum, "%lu", (unsigned long) mhm->seq_number);
  if (status)
    return status;
  len = strlen (amsg->amd->name) + 1 +
               ((flags & MU_ATTRIBUTE_DELETED) ? 1 : 0) + strlen (pnum) + 1;
  filename = malloc (len);
  if (filename)
    {
      strcpy (filename, amsg->amd->name);
      strcat (filename, "/");
      if (flags & MU_ATTRIBUTE_DELETED)
	strcat (filename, ",");
      strcat (filename, pnum);
      *pname = filename;
    }
  else
    status = ENOMEM;
  free (pnum);
  return status;
}

/* Find the message with the given sequence number */
static struct _mh_message *
_mh_get_message_seq (struct _amd_data *amd, size_t seq)
{
  struct _mh_message msg;
  size_t index;
  
  msg.seq_number = seq;
  if (amd_msg_lookup (amd, (struct _amd_message*) &msg, &index))
    return NULL;

  return (struct _mh_message *) _amd_get_message (amd, index);
}

/* Scan the mailbox */
static int
mh_scan0 (mu_mailbox_t mailbox, size_t msgno MU_ARG_UNUSED, size_t *pcount, 
          int do_notify)
{
  struct _amd_data *amd = mailbox->data;
  struct _mh_message *msg;
  DIR *dir;
  struct dirent *entry;
  int status = 0;
  struct stat st;
  int need_sort = 0;

  if (amd == NULL)
    return EINVAL;

  dir = opendir (amd->name);
  if (!dir)
    return errno;

  mu_monitor_wrlock (mailbox->monitor);

#ifdef WITH_PTHREAD
  pthread_cleanup_push (amd_cleanup, (void *)mailbox);
#endif

  mu_locker_lock (mailbox->locker);

  /* Do actual work. */

  while ((entry = readdir (dir)))
    {
      char *namep;
      int attr_flags;
      size_t num;

      attr_flags = 0;
      switch (entry->d_name[0])
	{
	case '.':
	  /* FIXME: .mh_sequences */
	  continue;
	case ',':
	  continue;
#if 0
	  attr_flags |= MU_ATTRIBUTE_DELETED;
	  namep = entry->d_name+1;
	  break;
#endif
	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
	  namep = entry->d_name;
	  break;
	default:
	  /*FIXME: Invalid entry. Report? */
	  continue;
	}

      num = strtoul (namep, &namep, 10);
      if (namep[0])
	continue;

      msg = _mh_get_message_seq (amd, num);
      if (!msg)
	{
	  msg = calloc (1, sizeof(*msg));
	  status = _amd_message_append (amd, (struct _amd_message *) msg);
	  if (status)
	    {
	      free (msg);
	      break;
	    }

	  msg->seq_number = num;
	  msg->amd_message.attr_flags = attr_flags;
	  msg->amd_message.orig_flags = msg->amd_message.attr_flags;

	  need_sort = 1;
	}
      else
	{
	  msg->amd_message.attr_flags = attr_flags;
	  msg->amd_message.orig_flags = msg->amd_message.attr_flags;
	}
    }

  closedir (dir);

  if (need_sort)
    amd_sort (amd);

  if (status == 0)
    {
      if (do_notify)
	{
	  size_t i;

	  for (i = 0; i < amd->msg_count; i++)
	    {
	      DISPATCH_ADD_MSG (mailbox, amd, i);
	    }
	}
  
      if (stat (amd->name, &st) == 0)
	amd->mtime = st.st_mtime;

      if (pcount)
	*pcount = amd->msg_count;
    }
  /* Clean up the things */

  amd_cleanup (mailbox);
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif
  return status;
}

static int
mh_qfetch (struct _amd_data *amd, mu_message_qid_t qid)
{
  char *p;
  size_t num = 0;
  int attr_flags = 0;
  struct _mh_message *msg;

  p = qid + strlen (qid) - 1;
  if (!mu_isdigit (*p))
    return EINVAL;
  
  for (p--; p >= qid && mu_isdigit (*p); p--)
    ;

  if (p == qid)
    return EINVAL;

  num = strtoul (p + 1, NULL, 10);
  
  if (*p == ',')
    {
      attr_flags |= MU_ATTRIBUTE_DELETED;
      p--;
    }

  if (*p != '/')
    return EINVAL;
  
  msg = calloc (1, sizeof (*msg));
  msg->seq_number = num;
  msg->amd_message.attr_flags = attr_flags;
  msg->amd_message.orig_flags = msg->amd_message.attr_flags;
  _amd_message_insert (amd, (struct _amd_message*) msg);
  return 0;
}


/* Note: In this particular implementation the message sequence number
   serves also as its UID. This allows to avoid many problems related
   to keeping the uids in the headers of the messages. */

static int
mh_message_uid (mu_message_t msg, size_t *puid)
{
  struct _mh_message *mhm = mu_message_get_owner (msg);
  if (puid)
    *puid = mhm->seq_number;
  return 0;
}

static int
_mh_msg_init (struct _amd_data *amd, struct _amd_message *amm)
{
  struct _mh_message *mhm = (struct _mh_message *) amm;
  mhm->seq_number = _mh_next_seq (amd);
  return 0;
}

static int
_mh_msg_delete (struct _amd_data *amd, struct _amd_message *amm)
{
  int rc, status;
  char *name;
  char *argv[3];
  /* FIXME: Cache this value */
  const char *proc = mu_mhprop_get_value (mu_mh_profile, "rmmproc", NULL);
  if (!proc)
    return ENOSYS;

  rc = amd->cur_msg_file_name (amm, &name);
  if (rc)
    return rc;
  
  if (proc[0] == 0)
    {
      rc = unlink (name);
      if (rc)
	rc = errno;
    }
  else
    {
      argv[0] = (char*) proc;
      argv[1] = name;
      argv[2] = NULL;
      rc = mu_spawnvp (proc, argv, &status);
    }
  free (name);
  return rc;
}


static int
mh_remove (struct _amd_data *amd)
{
  return amd_remove_dir (amd->name);
}


static int
mh_get_property (mu_mailbox_t mailbox, mu_property_t *pprop)
{
  struct _amd_data *amd = mailbox->data;
  mu_property_t property = NULL;
  struct mu_mh_prop *mhprop;
  const char *p;
  
  mhprop = calloc (1, sizeof (mhprop[0]));
  if (!mhprop)
    return ENOMEM;
  p = mu_mhprop_get_value (mu_mh_profile, "mh-sequences",
			   MU_MH_SEQUENCES_FILE);
  mhprop->filename = mu_make_file_name (amd->name, p);
  mu_property_create_init (&property, mu_mh_property_init, mhprop);
  mu_mailbox_set_property (mailbox, property);
      
  /*FIXME mu_property_set_value (property, "TYPE", "MH", 1);*/
  *pprop = property;
  return 0;
}



static int
mh_translate (mu_mailbox_t mbox, int cmd, size_t from, size_t *to)
{
  struct _amd_data *amd = mbox->data;
  struct _mh_message msg, *mp;
  size_t n;

  /* Make sure the mailbox has been scanned */ 
  mu_mailbox_messages_count (mbox, &n);
    
  switch (cmd)
    {
    case MU_MAILBOX_UID_TO_MSGNO:
      msg.seq_number = from;
      if (amd_msg_lookup (amd, (struct _amd_message*) &msg, &n))
	return MU_ERR_NOENT;
      *to = n;
      break;

    case MU_MAILBOX_MSGNO_TO_UID:
      mp = (struct _mh_message *) _amd_get_message (amd, from);
      if (!mp)
	return MU_ERR_NOENT;
      *to = mp->seq_number;
      break;

    default:
      return ENOSYS;
    }
  return 0;
}



int
_mailbox_mh_init (mu_mailbox_t mailbox)
{
  int rc;
  struct _amd_data *amd;

  rc = amd_init_mailbox (mailbox, sizeof (struct _amd_data), &amd);
  if (rc)
    return rc;

  amd->msg_size = sizeof (struct _mh_message);
  amd->msg_free = NULL;
  amd->msg_init_delivery = _mh_msg_init;
  amd->msg_finish_delivery = NULL;
  amd->cur_msg_file_name = _mh_cur_message_name;
  amd->new_msg_file_name = _mh_new_message_name;
  amd->scan0 = mh_scan0;
  amd->qfetch = mh_qfetch;
  amd->msg_cmp = mh_message_cmp;
  amd->message_uid = mh_message_uid;
  amd->next_uid = _mh_next_seq;
  amd->remove = mh_remove;

  mailbox->_get_property = mh_get_property;
  mailbox->_translate = mh_translate;

  if (mu_mhprop_get_value (mu_mh_profile, "rmmproc", NULL))
    amd->delete_msg = _mh_msg_delete;
    
  
  return 0;
}


