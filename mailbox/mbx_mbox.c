/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* First draft by Alain Magloire */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef WITH_PTHREAD
# define _REENTRANT
# ifdef HAVE_PTHREAD_H
#  define _XOPEN_SOURCE  500
#  include <pthread.h>
# endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/message.h>
#include <mailutils/stream.h>
#include <mailutils/body.h>
#include <mailutils/header.h>
#include <mailutils/attribute.h>
#include <registrar0.h>
#include <mailbox0.h>

#define ATTRIBUTE_IS_DELETED(flag)        (flag & MU_ATTRIBUTE_DELETED)
#define ATTRIBUTE_IS_EQUAL(flag1, flag2)  (flag1 == flag2)

static void mbox_destroy (mailbox_t);

struct _mbox_message;
struct _mbox_data;

typedef struct _mbox_data* mbox_data_t;
typedef struct _mbox_message* mbox_message_t;

/* Below are the headers field-names that we are caching for speed, it is
   more or less the list of headers in ENVELOPE command from IMAP.  */
#define HDRSIZE        15

const char *fhdr_table[] =
{
#define H_BCC                       0
  "Bcc",
#define H_CC                        1
  "Cc",
#define H_CONTENT_LANGUAGE          2
  "Content-Language",
#define H_CONTENT_TRANSFER_ENCODING 3
  "Content-Transfer-Encoding",
#define H_CONTENT_TYPE              4
  "Content-Type",
#define H_DATE                      5
  "Date",
#define H_FROM                      6
  "From",
#define H_IN_REPLY_TO               7
  "In-Reply-To",
#define H_MESSAGE_ID                8
  "Message-ID",
#define H_REFERENCE                 9
  "Reply-To",
#define H_REPLY_TO                  10
  "Reply-To",
#define H_SENDER                    11
  "Sender",
#define H_SUBJECT                   12
  "Subject",
#define H_TO                        13
  "To",
#define H_X_UIDL                    14
  "X-UIDL"
};

/* Keep the file positions of where the headers and bodies start and end.
   attr_flags is the "Status:" message.  */
struct _mbox_message
{
  /* Offset of the messages in the mailbox.  */
  off_t header_from;
  off_t header_from_end;
  off_t body;
  off_t body_end;

  /* Fast header retrieve, we save here the most common headers. This will
     speed the header search.  The entire headers are copied, when modified,
     by the header_t object, we do not have to worry about updating them.  */
  char *fhdr[HDRSIZE];

  size_t uid; /* IMAP uid.  */

  int attr_flags; /* The attr_flags contains the "Status:" attribute  */

  size_t header_lines;
  size_t body_lines;

  message_t message; /* A message attach to it.  */
  mbox_data_t mud; /* Back pointer.  */
};

/* The umessages is an array of pointers that contains umessages_count of
   mbox_message_t*; umessages[umessages_count].  We do it this way because
   realloc() can move everything to a new memory region and invalidate all
   the pointers.  Thanks to <Dave Inglis> for pointing this out.  The
   messages_count is the count number of messages parsed so far.  */
struct _mbox_data
{
  mbox_message_t *umessages; /* Array.  */
  size_t umessages_count; /* How big is the umessages[].  */
  size_t messages_count; /* How many valid entry in umessages[].  */
  off_t size; /* Size of the mailbox.  */
  unsigned long uidvalidity;
  size_t uidnext;
  char *name;

  /* The variables below are use to hold the state when appending messages.  */
  enum mbox_state
  {
    MBOX_NO_STATE = 0,
    MBOX_STATE_APPEND_SENDER, MBOX_STATE_APPEND_DATE, MBOX_STATE_APPEND_HEADER,
    MBOX_STATE_APPEND_ATTRIBUTE, MBOX_STATE_APPEND_UID, MBOX_STATE_APPEND_BODY,
    MBOX_STATE_APPEND_MESSAGE
  } state ;
  char *sender;
  char *date;
  off_t off;
  mailbox_t mailbox; /* Back pointer. */
};

extern char *base_name __P ((char const *));

/* Mailbox concrete implementation.  */
static int mbox_open                  __P ((mailbox_t, int));
static int mbox_close                 __P ((mailbox_t));
static int mbox_get_message           __P ((mailbox_t, size_t, message_t *));
//static int mbox_get_message_by_uid    __P ((mailbox_t, size_t, message_t *));
static int mbox_append_message        __P ((mailbox_t, message_t));
static int mbox_messages_count        __P ((mailbox_t, size_t *));
static int mbox_messages_recent       __P ((mailbox_t, size_t *));
static int mbox_message_unseen        __P ((mailbox_t, size_t *));
static int mbox_expunge               __P ((mailbox_t));
static int mbox_uidvalidity           __P ((mailbox_t, unsigned long *));
static int mbox_uidnext               __P ((mailbox_t, size_t *));
static int mbox_scan                  __P ((mailbox_t, size_t, size_t *));
static int mbox_is_updated            __P ((mailbox_t));
static int mbox_get_size                  __P ((mailbox_t, off_t *));

/* private stuff */
static int mbox_scan0                 __P ((mailbox_t, size_t, size_t *, int));
static int mbox_append_message0       __P ((mailbox_t, message_t, off_t *,
					    int, int));
static int mbox_message_uid           __P ((message_t, size_t *));
static int mbox_header_fill           __P ((header_t, char *, size_t, off_t,
					    size_t *));
static int mbox_get_body_fd           __P ((stream_t, int *));
static int mbox_get_fd                __P ((mbox_message_t, int *));
static int mbox_get_attr_flags        __P ((attribute_t, int *));
static int mbox_set_attr_flags        __P ((attribute_t, int));
static int mbox_unset_attr_flags      __P ((attribute_t, int));
static int mbox_body_read             __P ((stream_t, char *, size_t, off_t,
					    size_t *));
static int mbox_body_readline         __P ((stream_t, char *, size_t, off_t,
					    size_t *));
static int mbox_readstream            __P ((mbox_message_t, char *, size_t,
					    off_t, size_t *, int, off_t,
					    off_t));

static int mbox_header_size           __P ((header_t, size_t *));
static int mbox_header_lines          __P ((header_t, size_t *));
static int mbox_body_size             __P ((body_t, size_t *));
static int mbox_body_lines            __P ((body_t, size_t *));
static int mbox_envelope_sender       __P ((envelope_t, char *, size_t,
					    size_t *));
static int mbox_envelope_date         __P ((envelope_t, char *, size_t,
					    size_t *));
static int mbox_tmpfile               __P ((mailbox_t, char **pbox));
#ifdef WITH_PTHREAD
static void mbox_cleanup              __P ((void *));
#endif

/* Allocate the mbox_data_t struct(concrete mailbox), but don't do any
   parsing on the name or even test for existence.  However we do strip any
   leading "mbox:" part of the name, this is suppose to be the
   protocol/scheme name.  */
int
_mailbox_mbox_init (mailbox_t mailbox)
{
  mbox_data_t mud;
  size_t name_len;

  if (mailbox == NULL)
    return EINVAL;

  /* Allocate specific mbox data.  */
  mud = mailbox->data = calloc (1, sizeof (*mud));
  if (mailbox->data == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mud->mailbox = mailbox;

  /* Copy the name:
     We do not do any further interpretation after the scheme "mbox:"
     Because for example on distributed system like QnX4 a file name is
     //390/etc/passwd.  So the best approach is to let the OS handle it
     for example if we receive: "mbox:///var/mail/alain" the mailbox name
     will be "///var/mail/alain", we let open() do the right thing.
     So it will let things like this "mbox://390/var/mail/alain" where
     the "//" _is_ part of the filename, pass correctely.  */
  url_get_path (mailbox->url, NULL, 0, &name_len);
  mud->name = calloc (name_len + 1, sizeof (char));
  if (mud->name == NULL)
    {
      free (mud);
      mailbox->data = NULL;
      return ENOMEM;
    }
  url_get_path (mailbox->url, mud->name, name_len + 1, NULL);

  mud->state = MBOX_NO_STATE;

  /* Overloading the defaults.  */
  mailbox->_destroy = mbox_destroy;

  mailbox->_open = mbox_open;
  mailbox->_close = mbox_close;

  /* Overloading of the entire mailbox object methods.  */
  mailbox->_get_message = mbox_get_message;
  mailbox->_append_message = mbox_append_message;
  mailbox->_messages_count = mbox_messages_count;
  mailbox->_messages_recent = mbox_messages_recent;
  mailbox->_message_unseen = mbox_message_unseen;
  mailbox->_expunge = mbox_expunge;
  mailbox->_uidvalidity = mbox_uidvalidity;
  mailbox->_uidnext = mbox_uidnext;

  mailbox->_scan = mbox_scan;
  mailbox->_is_updated = mbox_is_updated;

  mailbox->_get_size = mbox_get_size;

  /* Set our properties.  */
  mailbox->properties = calloc (1, sizeof (*(mailbox->properties)));
  if (mailbox->properties == NULL)
    return ENOMEM;
  mailbox->properties_count = 1;
  mailbox->properties[0].key = strdup ("TYPE");
  mailbox->properties[0].value = strdup ("MBOX");

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_init(%s)\n", mud->name);
  return 0; /* okdoke */
}

/* Free all ressources associated with Unix concrete mailbox.  */
static void
mbox_destroy (mailbox_t mailbox)
{
  if (mailbox->data)
    {
      size_t i;
      mbox_data_t mud = mailbox->data;
      MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE,
		      "mbox_destroy (%s)\n", mud->name);
      monitor_wrlock (mailbox->monitor);
      for (i = 0; i < mud->umessages_count; i++)
	{
	  mbox_message_t mum = mud->umessages[i];
	  if (mum)
	    {
	      size_t j;
	      message_destroy (&(mum->message), mum);
	      for (j = 0; j < HDRSIZE; j++)
		if (mum->fhdr[j])
		  free (mum->fhdr[j]);
	      free (mum);
	    }
	}
      if (mud->umessages)
	free (mud->umessages);
      if (mud->name)
	free (mud->name);
      free (mud);
      mailbox->data = NULL;
      monitor_unlock (mailbox->monitor);
    }
}

/* Open the file.  For MU_STREAM_READ, the code tries mmap() first and fall
   back to normal file.  */
static int
mbox_open (mailbox_t mailbox, int flags)
{
  mbox_data_t mud = mailbox->data;
  int status = 0;

  if (mud == NULL)
    return EINVAL;

  mailbox->flags = flags;

  /* Get a stream.  */
  if (mailbox->stream == NULL)
    {
      /* We do not try to mmap for CREAT or APPEND, it is not supported.  */
      status = (flags & MU_STREAM_CREAT)
	|| (mailbox->flags & MU_STREAM_APPEND);

      /* Try to mmap () the file first.  */
      if (status == 0)
	{
	  status = mapfile_stream_create (&(mailbox->stream));
	  if (status == 0)
	    {
	      status = stream_open (mailbox->stream, mud->name, 0,
				    mailbox->flags);
	      if (status != 0)
		stream_destroy (&mailbox->stream, NULL);
	    }
	}

      /* Fall back to normal file if mmap() failed.  */
      if (status != 0)
	{
	  status = file_stream_create (&(mailbox->stream));
	  if (status != 0)
	    return status;
	  status = stream_open (mailbox->stream, mud->name, 0, mailbox->flags);
	}
      /* All failed, bail out.  */
      if (status != 0)
	return status;
      /* Even on top, of normal FILE *, lets agressively cache.  But this
	 may not be suitable for system tight on memory.  */
      stream_setbufsiz (mailbox->stream, BUFSIZ);
    }
  else
    {
      status = stream_open (mailbox->stream, mud->name, 0, mailbox->flags);
      if (status != 0)
	return status;
    }

  MAILBOX_DEBUG2 (mailbox, MU_DEBUG_TRACE, "mbox_open(%s, 0x%x)\n",
		  mud->name, mailbox->flags);

  /* Not of any use to try authenticate for a file mailbox.  Do it anyways.  */
  if (mailbox->authority)
    {
      status = authority_authenticate (mailbox->authority);
      if (status != 0)
	return status;
    }

  /* Give an appropriate way to file lock.  */
  /* FIXME: use dotlock external program: we may not be setgid.  */
  if (mailbox->locker == NULL)
    status = locker_create (&(mailbox->locker), mud->name, strlen (mud->name),
			    MU_LOCKER_PID | MU_LOCKER_FCNTL);
  return status;
}

static int
mbox_close (mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  size_t i;

  if (mud == NULL)
    return EINVAL;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE,  "mbox_close(%s)\n", mud->name);

  /* Make sure that we do not hold any file locking.  */
  locker_unlock (mailbox->locker);

  monitor_wrlock (mailbox->monitor);
  /* Before closing we need to remove all the messages
     - to reclaim the memory
     - to prepare for another scan.  */
  for (i = 0; i < mud->umessages_count; i++)
    {
      mbox_message_t mum = mud->umessages[i];
      /* Destroy the attach messages.  */
      if (mum)
	{
	  size_t j;
	  message_destroy (&(mum->message), mum);
	  for (j = 0; j < HDRSIZE; j++)
	    if (mum->fhdr[j])
	      free (mum->fhdr[j]);
	  free (mum);
	}
    }
  if (mud->umessages)
    free (mud->umessages);
  mud->umessages = NULL;
  mud->messages_count = mud->umessages_count = 0;
  mud->size = 0;
  mud->uidvalidity = 0;
  mud->uidnext = 0;
  monitor_unlock (mailbox->monitor);
  return stream_close (mailbox->stream);
}

/* Mailbox Parsing. Routing was way to ugly to put here.  */
#include "mbx_mboxscan.c"

/* Cover function that call the real thing, mbox_scan(), with
   notification set.  */
static int
mbox_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  size_t i;
  mbox_data_t mud = mailbox->data;
  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_scan(%s)\n", mud->name);
  if (! mbox_is_updated (mailbox))
    return mbox_scan0 (mailbox, msgno, pcount, 1);
  /* Since the mailbox is already updated fake the scan. */
  if (msgno > 0)
    msgno--; /* The fist message is number "1", decremente for the C array.  */
  for (i = msgno; i < mud->messages_count; i++)
    {
      if (observable_notify (mailbox->observable, MU_EVT_MESSAGE_ADD) != 0)
	break;
      if (((i +1) % 50) == 0)
	{
	  observable_notify (mailbox->observable, MU_EVT_MAILBOX_PROGRESS);
	}
    }
  return 0;
}

/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user start two
   browsers and deleted emails in one session.  My views is that we should
   scream bloody murder and hunt them with a machette. But for now just play
   dumb, but maybe the best approach is to pack our things and leave
   .i.e exit()/abort().  */
static int
mbox_is_updated (mailbox_t mailbox)
{
  off_t size = 0;
  mbox_data_t mud = mailbox->data;
  if (stream_size (mailbox->stream, &size) != 0)
    return 0;
  if (size < mud->size)
    {
      observable_notify (mailbox->observable, MU_EVT_MAILBOX_CORRUPT);
      /* And be verbose.  ? */
      fprintf (stderr, "* BAD : Mailbox corrupted, shrank size\n");
      /* FIXME: should I crash.  */
      return 1;
    }
  return (mud->size == size);
}

/* Try to create an uniq file, we no race conditions.   */
static int
mbox_tmpfile (mailbox_t mailbox, char **pbox)
{
  const char *tmpdir;
  int fd;
  const char *basename;
  mbox_data_t mud = mailbox->data;

  /*  P_tmpdir should be in <stdio.h>.  */
#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

  basename = base_name (mud->name);

  tmpdir =  getenv ("TMPDIR") ? getenv ("TMPDIR") : P_tmpdir;
  /* (separator + null) == 2 + XXXXXX == 6 + ... */
  *pbox = calloc (strlen (tmpdir) + /* '/' */ 1 + /*strlen ("MU_")*/ 3 +
		  strlen (basename) + /*strlen ("_XXXXXX")*/ 7 + /*null*/1,
		  sizeof (**pbox));
  if (*pbox == NULL)
    return -1;
  sprintf (*pbox, "%s/MU_%s_XXXXXX", tmpdir, basename);
#ifdef HAVE_MKSTEMP
  fd = mkstemp (*pbox);
#else
  /* Create the file.  It must not exist.  If it does exist, fail.  */
  if (mktemp (*pbox))
    {
      fd = open (*pbox, O_RDWR|O_CREAT|O_EXCL, 0600);
    }
  else
    {
      free (*pbox);
      fd = -1;
    }
#endif
  return fd;
}

/* For the expunge bits  we took a very cautionnary approach, meaning
   we create a temporary mailbox in the tmpdir copy all the message not mark
   deleted(Actually we copy all the message that may have been modified
   i.e new header values set; UIDL or UID or etc ....
   and skip the deleted ones, truncate the real mailbox to the desired size
   and overwrite with the tmp mailbox.  The approach to do everyting
   in core is tempting but require
   - to much memory, it is not rare now a day to have 30 Megs mailbox,
   - also there is danger for filesystems with quotas,
   - if we run out of disk space everything is lost.
   - or some program may not respect the advisory lock and decide to append
   a new message while your expunging etc ...
   The real downside to the approach we take is that when things go wrong
   the temporary file may be left in /tmp, which is not all that bad
   because at least, we have something to recuperate when failure.  */
static int
mbox_expunge (mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  int status = 0;
  sigset_t signalset;
  int tempfile;
  size_t i, j, dirty;  /* dirty will indicate the first modified message.  */
  off_t marker = 0;   /* marker will be the position to truncate.  */
  off_t total = 0;
  char *tmpmboxname = NULL;
  mailbox_t tmpmailbox = NULL;
  size_t save_imapbase = 0;  /* uidvalidity is save in the first message.  */
#ifdef WITH_PTHREAD
  int state;
#endif

  if (mud == NULL)
    return EINVAL;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_expunge (%s)\n", mud->name);

  /* Noop.  */
  if (mud->messages_count == 0)
    return 0;

  /* Find the first dirty(modified) message.  */
  for (dirty = 0; dirty < mud->messages_count; dirty++)
    {
      mum = mud->umessages[dirty];
      /* Message may have been tampered, break here.  */
      if ((mum->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mum->message && message_is_modified (mum->message)))
	break;
    }

  /* Did something change ?  */
  if (dirty == mud->messages_count)
    return 0; /* Nothing change, bail out.  */

  /* Create a temporary file.  */
  tempfile = mbox_tmpfile (mailbox, &tmpmboxname);
  if (tempfile == -1)
    {
      if (tmpmboxname)
	free (tmpmboxname);
      fprintf (stderr, "Failed to create temporary file when expunging.\n");
      return errno;
    }

  /* This is redundant, we go to the loop again.  But it's more secure here
     since we don't want to be disturb when expunging.  Destroy all the
     messages mark for deletion.  */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && mum->message && ATTRIBUTE_IS_DELETED (mum->attr_flags))
	message_destroy (&(mum->message), mum);
    }

  /* Create temporary mailbox_t.  */
  {
    mbox_data_t tmp_mud;
    char *m = alloca (5 + strlen (tmpmboxname) + 1);
    /* Try via the mbox: protocol.  */
    sprintf (m, "mbox:%s", tmpmboxname);
    status = mailbox_create (&tmpmailbox, m);
    if (status != 0)
      {
	/* Do not give up just yet, maybe they register the path_record.  */
	sprintf (m, "%s", tmpmboxname);
	status = mailbox_create (&tmpmailbox, m);
	if (status != 0)
	  {
	    /* Ok give up.  */
	    close (tempfile);
	    remove (tmpmboxname);
	    free (tmpmboxname);
	    return status;
	  }
      }

    /* Must be flag CREATE if not the mailbox_open will try to mmap()
       the file.  */
    status = mailbox_open (tmpmailbox, MU_STREAM_CREAT | MU_STREAM_RDWR);
    if (status != 0)
      {
	close (tempfile);
	remove (tmpmboxname);
	free (tmpmboxname);
	return status;
      }
    close (tempfile); /* This one is useless the mailbox have its own.  */
    tmp_mud = tmpmailbox->data;
    /* May need when appending.  */
    tmp_mud->uidvalidity = mud->uidvalidity;
    tmp_mud->uidnext = mud->uidnext;
  }

  /* Get the File lock.  */
  if (locker_lock (mailbox->locker, MU_LOCKER_WRLOCK) != 0)
    {
      mailbox_close (tmpmailbox);
      mailbox_destroy (&tmpmailbox);
      remove (tmpmboxname);
      free (tmpmboxname);
      fprintf (stderr, "Failed to grab the lock\n");
      return ENOLCK;
    }

  /* Critical section, we can not allowed signal here.  */
#ifdef WITH_PTHREAD
  pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);
#endif
  sigemptyset (&signalset);
  sigaddset (&signalset, SIGTERM);
  sigaddset (&signalset, SIGHUP);
  sigaddset (&signalset, SIGTSTP);
  sigaddset (&signalset, SIGINT);
  sigaddset (&signalset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &signalset, 0);

  /* Set the marker position.  */
  marker = mud->umessages[dirty]->header_from;
  total = 0;

  /* Copy to the temporary mailbox emails not mark deleted.  */
  for (i = dirty; i < mud->messages_count; i++)
    {
      mum = mud->umessages[i];

      /* Skip it, if mark for deletion.  */
      if (ATTRIBUTE_IS_DELETED (mum->attr_flags))
	{
	  /* We save the uidvalidity in the first message, if it is being
	     deleted we need to move the uidvalidity to the first available
	     (non-deleted) message.  */
	  if (i == save_imapbase)
	    {
	      save_imapbase = i + 1;
	      if (save_imapbase < mud->messages_count)
		(mud->umessages[save_imapbase])->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	    }
	  continue;
	}

      /* Do the expensive mbox_append_message0() only if mark dirty.  */
      if ((mum->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mum->message && message_is_modified (mum->message)))
	{
	  /* The message was not instanciated, probably the dirty flag was
	     set by mbox_scan(), create one here.  */
	  if (mum->message == 0)
	    {
	      message_t msg;
	      status = mbox_get_message (mailbox, i, &msg);
	      if (status != 0)
		{
		  fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			   strerror (status));
		  goto bailout0;
		}
	    }
	  status = mbox_append_message0 (tmpmailbox, mum->message,
					 &total, 1, (i == save_imapbase));
	  if (status != 0)
	    {
	      fprintf (stderr, "Error expunge:%d: %s", __LINE__,
		       strerror (status));
	      goto bailout0;
	    }
	  /* Clear the dirty bit.  */
	  mum->attr_flags &= ~MU_ATTRIBUTE_MODIFIED;
	  message_clear_modified (mum->message);
	}
      else
	{
	  /* Nothing changed copy the message straight.  */
	  char buffer [1024];
	  size_t n;
	  off_t offset = mum->header_from;
	  size_t len = mum->body_end - mum->header_from;
	  while (len > 0)
	    {
	      n = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if ((status = stream_read (mailbox->stream, buffer, n, offset,
					 &n) != 0)
		  || (status = stream_write (tmpmailbox->stream, buffer, n,
					     total, &n) != 0))
		{
		  fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			   strerror (status));
		  goto bailout0;
		}
	      len -= n;
	      total += n;
	      offset += n;
	    }
	  /* Add the newline separator.  */
	  status = stream_write (tmpmailbox->stream, "\n", 1, total, &n);
	  if (status != 0)
	    {
	      fprintf (stderr, "Error expunge:%d: %s", __LINE__,
		       strerror (status));
	      goto bailout0;
	    }
	  total++;
	}
    } /* for (;;) */

  /* Caution: before ftruncate()ing the file see
     - if we've receive new mails.  Some programs may not respect the lock,
     or the lock was held for too long.
     - The mailbox may not have been properly updated before expunging.  */
  {
    off_t size = 0;
    if (stream_size (mailbox->stream, &size) == 0)
      {
	off_t len = size - mud->size;
	off_t offset = mud->size;
	char buffer [1024];
	size_t n = 0;
	if (len > 0 )
	  {
	    while ((status = stream_read (mailbox->stream, buffer,
					  sizeof (buffer), offset, &n)) == 0
		   && n > 0)
	      {
		status = stream_write (tmpmailbox->stream, buffer, n,
				       total, &n);
		if (status != 0)
		  {
		    fprintf (stderr, "Error expunge:%d: %s", __LINE__,
			     strerror (status));
		    goto bailout0;
		  }
		total += n;
		offset += n;
	      }
	  }
	else if (len < 0)
	  {
	    /* Corrupted mailbox.  */
	    fprintf (stderr, "Error expunge:%d: %s", __LINE__,
		     strerror (status));
	    goto bailout0;
	  }
      }
  } /* End of precaution.  */

  /* Seek and rewrite it.  */
  if (total > 0)
    {
      char buffer [1024];
      size_t n = 0;
      off_t off = 0;
      off_t offset = marker;
      while ((status = stream_read (tmpmailbox->stream, buffer,
				    sizeof (buffer), off, &n)) == 0
	     && n > 0)
	{
	  status = stream_write (mailbox->stream, buffer, n, offset, &n);
	  if (status != 0)
	    {
	      fprintf (stderr, "Error expunge:%d: %s\n", __LINE__,
		       strerror (status));
	      goto bailout;
	    }
	  off += n;
	  offset += n;
	}
    }

  /* Flush/truncation. Need to flush before truncate.  */
  stream_flush (mailbox->stream);
  status = stream_truncate (mailbox->stream, total + marker);
  if (status != 0)
    {
      fprintf (stderr, "Error expunging:%d: %s\n", __LINE__,
	       strerror (status));
      goto bailout;
    }

  /* Don't remove the tmp mbox in case of errors, when writing back.  */
 bailout0:
  remove (tmpmboxname);

 bailout:

  free (tmpmboxname);
  /* Release the File lock.  */
  locker_unlock (mailbox->locker);
  mailbox_close (tmpmailbox);
  mailbox_destroy (&tmpmailbox);
#ifdef WITH_PTHREAD
  pthread_setcancelstate (state, &state);
#endif
  sigprocmask (SIG_UNBLOCK, &signalset, 0);

  /* We need to readjust the pointers.  */
  if (status == 0)
    {
      size_t dlast;
      monitor_wrlock (mailbox->monitor);
      for (j = dirty, dlast = mud->messages_count - 1;
	   j <= dlast; j++)
	{
	  /* Clear all the references, any attach messages been already
	     destroy above.  */
	  mum = mud->umessages[j];
	  if (ATTRIBUTE_IS_DELETED (mum->attr_flags))
	    {
	      if ((j + 1) <= dlast)
		{
		  /* Move all the pointers up.  So the message pointer
		     part of mum will be at the right position.  */
		  memmove (mud->umessages + j, mud->umessages + j + 1,
			   (dlast - j) * sizeof (mum));
		  //mum->header_from = mum->header_from_end = 0;
		  //mum->body = mum->body_end = 0;
		  //mum->header_lines = mum->body_lines = 0;
		  for (i = 0; i < HDRSIZE; i++)
		    if (mum->fhdr[i])
		      {
			free (mum->fhdr[i]);
			mum->fhdr[i] = NULL;
		      }
		  memset (mum, 0, sizeof (*mum));
		  /* We are not free()ing the useless mum, but instead
		     we put it back in the pool, to be reuse.  */
		  mud->umessages[dlast] = mum;
		  dlast--;
		  /* Set mum to the new value after the memmove so it
		     gets cleared to.  */
		  mum = mud->umessages[j];
		}
	      else
		{
		  for (i = 0; i < HDRSIZE; i++)
		    if (mum->fhdr[i])
		      {
			free (mum->fhdr[i]);
			mum->fhdr[i] = NULL;
		      }
		  memset (mum, 0, sizeof (*mum));
		}
	    }
	  mum->header_from = mum->header_from_end = 0;
	  mum->body = mum->body_end = 0;
	  mum->header_lines = mum->body_lines = 0;
	  for (i = 0; i < HDRSIZE; i++)
	    if (mum->fhdr[i])
	      {
		free (mum->fhdr[i]);
		mum->fhdr[i] = NULL;
	      }
	}
      monitor_unlock (mailbox->monitor);
      /* This is should reset the messages_count, the last argument 0 means
	 not to send event notification.  */
      mbox_scan0 (mailbox, dirty, NULL, 0);
    }
  return status;
}

static int
mbox_message_uid (message_t msg, size_t *puid)
{
  mbox_message_t mum = message_get_owner (msg);
  if (puid)
    *puid = mum->uid;
  return 0;
}

static int
mbox_get_body_fd (stream_t is, int *pfd)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  return mbox_get_fd (mum, pfd);
}

static int
mbox_get_fd (mbox_message_t mum, int *pfd)
{
  int status;
  if (mum == NULL)
    return EINVAL;
  status = stream_get_fd (mum->mud->mailbox->stream, pfd);
  return status;
}

static int
mbox_get_attr_flags (attribute_t attr, int *pflags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  if (pflags)
    *pflags = mum->attr_flags;
  return 0;
}

static int
mbox_set_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  mum->attr_flags |= flags;
  return 0;
}

static int
mbox_unset_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  mbox_message_t mum = message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  mum->attr_flags &= ~flags;
  return 0;
}

static int
mbox_body_readline (stream_t is, char *buffer, size_t buflen,
		    off_t off, size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);

  return mbox_readstream (mum, buffer, buflen, off, pnread, 1,
			  mum->body, mum->body_end);
}

static int
mbox_body_read (stream_t is, char *buffer, size_t buflen,
		off_t off, size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  return mbox_readstream (mum, buffer, buflen, off, pnread, 0,
			  mum->body, mum->body_end);
}

static int
mbox_readstream (mbox_message_t mum, char *buffer, size_t buflen,
		 off_t off, size_t *pnread, int isreadline,
		 off_t start, off_t end)
{
  size_t nread = 0;
  int status = 0;

  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = nread;
      return 0;
    }

  monitor_rdlock (mum->mud->mailbox->monitor);
#ifdef WITH_PTHREAD
  /* read() is cancellation point since we're doing a potentially
     long operation.  Lets make sure we clean the state.  */
  pthread_cleanup_push (mbox_cleanup, (void *)mum->mud->mailbox);
#endif
  {
    off_t ln = end - (start + off);
    if (ln > 0)
      {
	/* Position the file pointer and the buffer.  */
	nread = ((size_t)ln < buflen) ? ln : buflen;
	if (isreadline)
	  status = stream_readline (mum->mud->mailbox->stream, buffer, buflen,
				    start + off, &nread);
	else
	  status = stream_read (mum->mud->mailbox->stream, buffer, nread,
				start + off, &nread);
      }
  }
  monitor_unlock (mum->mud->mailbox->monitor);
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif

  if (pnread)
    *pnread = nread;
  return status;
}

static int
mbox_header_fill (header_t header, char *buffer, size_t len,
		  off_t off, size_t *pnread)
{
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  size_t j;
  /* Since we are filling the header there is no need for the cache headers
     discard them.  */
  for (j = 0; j < HDRSIZE; j++)
    {
      if (mum->fhdr[j])
	{
	  free (mum->fhdr[j]);
	  mum->fhdr[j] = NULL;
	}
    }
  return mbox_readstream (mum, buffer, len, off, pnread, 0,
			  mum->header_from_end, mum->body);
}

static int
mbox_header_get_fvalue (header_t header, const char *name, char *buffer,
		       size_t buflen, size_t *pnread)
{
  size_t i, fv_len = 0;
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  int err = ENOENT;
  for (i = 0; i < HDRSIZE; i++)
    {
      if (*name == *(fhdr_table[i]) && strcasecmp (fhdr_table[i], name) == 0)
	{
	  if (mum->fhdr[i])
	    {
	      fv_len = strlen (mum->fhdr[i]);
	      if (buffer && buflen > 0)
		{
		  /* For the null.  */
		  buflen--;
		  fv_len = (fv_len < buflen) ? fv_len : buflen;
		  memcpy (buffer, mum->fhdr[i], fv_len);
		  buffer[fv_len] = '\0';
		}
	      err = 0;
	    }
	  else
	    err = ENOENT;
	  break;
	}
    }

  if (pnread)
    *pnread = fv_len;
  return err;
}

static int
mbox_header_size (header_t header, size_t *psize)
{
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body - mum->header_from_end;
  return 0;
}

static int
mbox_header_lines (header_t header, size_t *plines)
{
  message_t msg = header_get_owner (header);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (plines)
    *plines = mum->header_lines;
  return 0;
}

static int
mbox_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body_end - mum->body;
  return 0;
}

static int
mbox_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  mbox_message_t mum = message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (plines)
    *plines = mum->body_lines;
  return 0;
}

static int
mbox_envelope_date (envelope_t envelope, char *buf, size_t len,
		    size_t *pnwrite)
{
  message_t msg = envelope_get_owner (envelope);
  mbox_message_t mum = message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mum == NULL)
    return EINVAL;

  status = stream_readline (mum->mud->mailbox->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  if (status != 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      return status;
    }

  /* Format:  "From [sender] [date]" */
  /* strlen ("From ") == 5 */
  if (n > 5 && (s = strchr (buffer + 5, ' ')) != NULL)
    {
      if (buf && len > 0)
	{
	  len--; /* Leave space for the null.  */
	  strncpy (buf, s + 1, len)[len] = '\0';
	  len = strlen (buf);
	}
      else
	len = strlen (s + 1);
    }
  else
    len = 0;

  if (pnwrite)
    *pnwrite = len;
  return 0;
}

static int
mbox_envelope_sender (envelope_t envelope, char *buf, size_t len,
		    size_t *pnwrite)
{
  message_t msg = envelope_get_owner (envelope);
  mbox_message_t mum = message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mum == NULL)
    return EINVAL;

  status = stream_readline (mum->mud->mailbox->stream, buffer, sizeof(buffer),
			    mum->header_from, &n);
  if (status != 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      return status;
    }

  /* Format:  "From [sender] [date]" */
  /* strlen ("From ") == 5 */
  if (n > 5 && (s = strchr (buffer + 5, ' ')) != NULL)
    {
      /* Put a NULL to isolate the sender string, make a C string.  */
      *s = '\0';
      if (buf && len > 0)
	{
	  len--; /* leave space for the null */
	  strncpy (buf, buffer + 5, len)[len] = '\0';
	  len = strlen (buf);
	}
      else
	len = strlen (buffer + 5);
    }
  else
    len = 0;

  if (pnwrite)
    *pnwrite = len;
  return 0;
}

static int
mbox_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  int status;
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  message_t msg = NULL;

  /* Sanity checks.  */
  if (pmsg == NULL || mud == NULL)
    return EINVAL;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  /* Second sanity: check the message number.  */
  if (!(mud->messages_count > 0
	&& msgno > 0
	&& msgno <= mud->messages_count))
    return EINVAL;

  mum = mud->umessages[msgno - 1];

  /* Check if we already have it.  */
  if (mum->message)
    {
      if (pmsg)
	*pmsg = mum->message;
      return 0;
    }

  MAILBOX_DEBUG2 (mailbox, MU_DEBUG_TRACE, "mbox_get_message(%s, %d)\n",
		  mud->name, msgno);

  /* Get an empty message struct.  */
  status = message_create (&msg, mum);
  if (status != 0)
    return status;

  /* Set the header.  */
  {
    header_t header = NULL;
    status = header_create (&header, NULL, 0, msg);
    if (status != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    header_set_fill (header, mbox_header_fill, msg);
    header_set_get_fvalue (header, mbox_header_get_fvalue, msg);
    header_set_size (header, mbox_header_size, msg);
    header_set_lines (header, mbox_header_lines, msg);
    message_set_header (msg, header, mum);
  }

  /* Set the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, msg);
    if (status != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    attribute_set_get_flags (attribute, mbox_get_attr_flags, msg);
    attribute_set_set_flags (attribute, mbox_set_attr_flags, msg);
    attribute_set_unset_flags (attribute, mbox_unset_attr_flags, msg);
    message_set_attribute (msg, attribute, mum);
  }

  /* Prepare the body.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
	|| (status = stream_create (&stream,
				    mailbox->flags | MU_STREAM_SEEKABLE,
				    body)) != 0)
      {
	body_destroy (&body, msg);
	stream_destroy (&stream, body);
	message_destroy (&msg, mum);
	return status;
      }
    stream_set_read (stream, mbox_body_read, body);
    stream_set_readline (stream, mbox_body_readline, body);
    stream_set_fd (stream, mbox_get_body_fd, body);
    body_set_stream (body, stream, msg);
    body_set_size (body, mbox_body_size, msg);
    body_set_lines (body, mbox_body_lines, msg);
    message_set_body (msg, body, mum);
  }

  /* Set the envelope.  */
  {
    envelope_t envelope= NULL;
    status = envelope_create (&envelope, msg);
    if (status != 0)
      {
	message_destroy (&msg, mum);
	return status;
      }
    envelope_set_sender (envelope, mbox_envelope_sender, msg);
    envelope_set_date (envelope, mbox_envelope_date, msg);
    message_set_envelope (msg, envelope, mum);
  }

  /* Set the UID.  */
  message_set_uid (msg, mbox_message_uid, mum);

  /* Attach the message to the mailbox mbox data.  */
  mum->message = msg;
  message_set_mailbox (msg, mailbox, mum);

  *pmsg = msg;
  return 0;
}

static int
mbox_append_message (mailbox_t mailbox, message_t msg)
{
  mbox_data_t mud = mailbox->data;
  if (msg == NULL || mud == NULL)
    return EINVAL;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mbox_append_message (%s)\n",
		  mud->name);

  switch (mud->state)
    {
    case MBOX_NO_STATE:
      if (locker_lock (mailbox->locker, MU_LOCKER_WRLOCK) != 0)
	{
	  MAILBOX_DEBUG0 (mailbox, MU_DEBUG_TRACE,
			  "mbox_append_message:lock failed\n");
	  return ENOLCK;
	}

    default:
      {
	off_t size;
	int status;
	/* Move to the end of the file, not necesary if _APPEND mode.  */
	if ((status = stream_size (mailbox->stream, &size)) != 0
	    || (status = mbox_append_message0 (mailbox, msg, &size, 0, 0)) != 0)
	  {
	    if (status != EAGAIN)
	      locker_unlock (mailbox->locker);
	    return status;
	  }
      }
    }
  locker_unlock (mailbox->locker);
  return 0;
}

/* FIXME: We need to escape body line that begins with "From ", this
   will required to read the body by line instead of by chuncks hurting
   perfomance big time when expunging.  But should not this be the
   responsability of the client ?  */
static int
mbox_append_message0 (mailbox_t mailbox, message_t msg, off_t *psize,
		      int is_expunging, int first)
{
  mbox_data_t mud = mailbox->data;
  int status = 0;
  size_t n = 0;
  char nl = '\n';

  switch (mud->state)
    {
    case MBOX_NO_STATE:
      /* Allocate memory for the sender/date buffers.  */
      mud->sender = calloc (128, sizeof (char));
      if (mud->sender == NULL)
	return ENOMEM;
      mud->date = calloc (128, sizeof (char));
      if (mud->date == NULL)
	{
	  free (mud->sender);
	  mud->sender = NULL;
	  return ENOMEM;
	}
      mud->off = 0;
      mud->state = MBOX_STATE_APPEND_SENDER;

    case MBOX_STATE_APPEND_SENDER:
      /* Generate the sender for the "From " separator.  */
      {
	char *s;
	size_t len = 0;
	envelope_t envelope = NULL;
	message_get_envelope (msg, &envelope);
	status = envelope_sender (envelope, mud->sender, 128, &len);
	if (status != 0)
	  {
	    if (status != EAGAIN)
	      {
		free (mud->sender);
		free (mud->date);
		mud->date = mud->sender = NULL;
		mud->state = MBOX_NO_STATE;
	      }
	    return status;
	  }

	/* Nuke trailing newline.  */
	s = memchr (mud->sender, nl, len);
	if (s)
	  *s = '\0';
	mud->state = MBOX_STATE_APPEND_DATE;
      }

    case MBOX_STATE_APPEND_DATE:
      /* Generate a date for the "From "  separator.  */
      {
	char *s;
	size_t len = 0;
	envelope_t envelope = NULL;
	char buffer[1024];
	message_get_envelope (msg, &envelope);
	status = envelope_date (envelope, mud->date, 128, &len);
	if (status != 0)
	  {
	    if (status != EAGAIN)
	      {
		free (mud->sender);
		free (mud->date);
		mud->date = mud->sender = NULL;
		mud->state = MBOX_NO_STATE;
	      }
	    return status;
	  }

	/* Nuke trailing newline.  */
	s = memchr (mud->date, nl, len);
	if (s)
	  *s = '\0';

	/* Write the separator to the mailbox.  */
	n = snprintf (buffer, sizeof (buffer), "From %s %s",
		      mud->sender, mud->date);
	stream_write (mailbox->stream, buffer, n, *psize, &n);
	*psize += n;

	/* Add the newline, the above may be truncated.  */
	stream_write (mailbox->stream, &nl , 1, *psize, &n);
	*psize += n;

	free (mud->sender);
	free (mud->date);
	mud->sender = mud->date = NULL;
	/* If we are not expunging get the message in one block via the stream
	   message instead of the header/body.  This is good for POP where
	   there is no separation between header and body(RETR).  */
	if (! is_expunging)
	  {
	    mud->state = MBOX_STATE_APPEND_MESSAGE;
	    break;
	  }
	mud->state = MBOX_STATE_APPEND_HEADER;
      }

    case MBOX_STATE_APPEND_HEADER:
      /* Append the Header.  */
      {
	char buffer[1024];
	size_t nread = 0;
	stream_t is = NULL;
	header_t header = NULL;
	message_get_header (msg, &header);
	header_get_stream (header, &is);
	do
	  {
	    status = stream_readline (is, buffer, sizeof (buffer), mud->off,
				      &nread);
	    if (status != 0)
	      {
		if (status != EAGAIN)
		  {
		    mud->state = MBOX_NO_STATE;
		    mud->off = 0;
		  }
		return status;
	      }
	    mud->off += nread;
	    if (*buffer == '\n')
	      break;

	    /* We do not copy the Status since it is rewritten by the
	       attribute code below. Ditto for X-UID and X-IMAPBase.
	       FIXME:
	       - We have a problem here the header may not fit the buffer.
	       - Should  we skip the IMAP "X-Status"? */
	    if ((strncasecmp (buffer, "Status", 6) == 0)
		|| (strncasecmp (buffer, "X-IMAPbase", 10) == 0)
		|| (strncasecmp (buffer, "X-UID", 4) == 0
		    && (buffer[5] == ':' || buffer[5] == ' '
			|| buffer[5] == '\t')))
	      continue;

	    status = stream_write (mailbox->stream, buffer, nread,
				    *psize, &n);
	    if (status != 0)
	      break;
	    *psize += n;
	  }
	while (nread > 0);
	mud->off = 0;

	/* Rewrite the X-IMAPbase marker. */
	if (first && is_expunging)
	  {
	    n = sprintf (buffer, "X-IMAPbase: %lu %u\n",
			 mud->uidvalidity, mud->uidnext);
	    stream_write (mailbox->stream, buffer, n, *psize, &n);
	    *psize += n;
	  }
	mud->state = MBOX_STATE_APPEND_ATTRIBUTE;
      }

      case MBOX_STATE_APPEND_ATTRIBUTE:
      /* Put the new attributes.  */
      {
	char abuf[64];
	size_t na = 0;
	attribute_t attr = NULL;
	abuf[0] = '\0';
	message_get_attribute (msg, &attr);
	attribute_to_string (attr, abuf, sizeof(abuf), &na);

	status = stream_write (mailbox->stream, abuf, na, *psize, &n);
	if (status != 0)
	  break;
	*psize += n;

	mud->state = MBOX_STATE_APPEND_UID;
      }

    case MBOX_STATE_APPEND_UID:
      /* The new X-UID. */
      {
	char suid[64];
	size_t uid = 0;
	if (is_expunging)
	  {
	    status = message_get_uid (msg, &uid);
	    if (status == EAGAIN)
	      return status;
	  }
	else
	  uid = mud->uidnext++;

	if (status == 0 || uid != 0)
	  {
	    n = sprintf (suid, "X-UID: %d\n", uid);
	    /* Put the UID.  */
	    status = stream_write (mailbox->stream, suid, n, *psize, &n);
	    if (status != 0)
	      break;
	    *psize += n;
	  }

	/* New line separator of the Header.  */
	status = stream_write (mailbox->stream, &nl , 1, *psize, &n);
	if (status != 0)
	  break;
	*psize += n;
	mud->state = MBOX_STATE_APPEND_BODY;
      }

    case MBOX_STATE_APPEND_BODY:
      /* Append the Body.  */
      {
	char buffer[1024];
	size_t nread = 0;
	stream_t is = NULL;
	body_t body = NULL;
	message_get_body (msg, &body);
	body_get_stream (body, &is);
	do
	  {
	    status = stream_read (is, buffer, sizeof (buffer), mud->off,
				  &nread);
	    if (status != 0)
	      {
		if (status != EAGAIN)
		  {
		    mud->state = MBOX_NO_STATE;
		    mud->off = 0;
		  }
		return status;
	      }
	    mud->off += nread;
	    status = stream_write (mailbox->stream, buffer, nread, *psize, &n);
	    if (status != 0)
	      break;
	    *psize += n;
	  }
	while (nread > 0);
	mud->off = 0;
	n = 0;
	stream_write (mailbox->stream, &nl, 1, *psize, &n);
	*psize += n;
      }

    default:
      break;
    }

  /* If not expunging we are taking the stream message.  */
  if (!is_expunging)
    {
      switch (mud->state)
        {
	case MBOX_STATE_APPEND_MESSAGE:
	  {
	    /* Append the Message.  */
	    char buffer[1024];
	    size_t nread = 0;
	    stream_t is = NULL;
	    message_get_stream (msg, &is);
	    do
	      {
		status = stream_read (is, buffer, sizeof (buffer), mud->off,
				      &nread);
		if (status != 0)
		  {
		    if (status != EAGAIN)
		      {
			mud->state = MBOX_NO_STATE;
			mud->off = 0;
		      }
		    stream_flush (mailbox->stream);
		    return status;
		  }
		stream_write (mailbox->stream, buffer, nread, *psize, &n);
		mud->off += nread;
		*psize += n;
	      }
	    while (nread > 0);
	    n = 0;
	    stream_write (mailbox->stream, &nl, 1, *psize, &n);
	    *psize += n;
	  }

	default:
	  break;
	}
    } /* is_expunging */
  mud->state = MBOX_NO_STATE;
  stream_flush (mailbox->stream);
  return status;
}

static int
mbox_get_size (mailbox_t mailbox, off_t *psize)
{
  off_t size;
  int status;

  /* Maybe was not open yet ??  */
  status  = stream_size (mailbox->stream, &size);
  if (status != 0)
    return status;
  if (psize)
    *psize = size;
  return 0;
}

static int
mbox_messages_count (mailbox_t mailbox, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;
  if (mud == NULL)
    return EINVAL;

  if (! mbox_is_updated (mailbox))
    return mbox_scan0 (mailbox,  mud->messages_count, pcount, 1);

  if (pcount)
    *pcount = mud->messages_count;

  return 0;
}

static int
mbox_messages_recent (mailbox_t mailbox, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  size_t j, recent;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      int status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  for (recent = j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && ((mum->attr_flags == 0) ||
		  ! ((mum->attr_flags & MU_ATTRIBUTE_SEEN)
		     && (mum->attr_flags & MU_ATTRIBUTE_READ))))
	recent++;
    }
  *pcount = recent;
  return 0;
}

static int
mbox_message_unseen (mailbox_t mailbox, size_t *pmsgno)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  size_t j, unseen;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      int status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  for (unseen = j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && ((mum->attr_flags == 0) ||
		  ! ((mum->attr_flags & MU_ATTRIBUTE_SEEN)
		     && (mum->attr_flags & MU_ATTRIBUTE_READ))))
	{
	  unseen = j + 1;
	  break;
	}
    }
  *pmsgno = unseen;
  return 0;
}

static int
mbox_uidvalidity (mailbox_t mailbox, unsigned long *puidvalidity)
{
  mbox_data_t mud = mailbox->data;
  int status = mbox_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidvalidity)
    *puidvalidity = mud->uidvalidity;
  return 0;
}

static int
mbox_uidnext (mailbox_t mailbox, size_t *puidnext)
{
  mbox_data_t mud = mailbox->data;
  int status = mbox_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidnext)
    *puidnext = mud->uidnext;
  return 0;
}

#ifdef WITH_PTHREAD
static void
mbox_cleanup (void *arg)
{
  mailbox_t mailbox = arg;
  monitor_unlock (mailbox->monitor);
  locker_unlock (mailbox->locker);
}
#endif
