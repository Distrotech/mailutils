/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <mailbox0.h>
#include <registrar0.h>
#include <message0.h>
#include <attribute0.h>
#include <auth.h>
#include <locker.h>

#define HAVE_PTHREAD_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#else
# define flockfile(arg)
# define funlockfile(arg)
#endif
#include <string.h>
#include <ctype.h>
#include <limits.h>

static int mailbox_unix_init (mailbox_t *pmbox, const char *name);
static void mailbox_unix_destroy (mailbox_t *pmbox);

struct mailbox_registrar _mailbox_unix_registrar =
{
  "UNIX MBOX",
  mailbox_unix_init, mailbox_unix_destroy
};

/*
 * Keep the position of where the header and body starts
 * and ends. old_attr is the one that is part of the "Status:" message.
 */
typedef struct _mailbox_unix_message
{
  off_t header;
  off_t header_end;
  /* little hack to make things easier
   * when updating the attribute
   */
  off_t status;
  off_t status_end;
  off_t body;
  off_t body_end;
  attribute_t old_attr;
  attribute_t new_attr;
} *mailbox_unix_message_t;

typedef struct _mailbox_unix_data
{
  mailbox_unix_message_t messages;
  size_t messages_count;
  FILE *file;
  char *dirname;
  char *basename;
  int flags;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  time_t mtime;
  size_t size;
} *mailbox_unix_data_t;

static int mailbox_unix_open (mailbox_t mbox, int flag);
static int mailbox_unix_close (mailbox_t mbox);

static int mailbox_unix_append_message (mailbox_t, message_t msg);
static int mailbox_unix_messages_count (mailbox_t, size_t *msgno);
static int mailbox_unix_delete (mailbox_t, size_t msgno);
static int mailbox_unix_undelete (mailbox_t, size_t msgno);
static int mailbox_unix_is_deleted (mailbox_t, size_t msgno);
static int mailbox_unix_expunge (mailbox_t);
static int mailbox_unix_num_deleted (mailbox_t, size_t *);
static int mailbox_unix_get_size (mailbox_t, size_t msgno, size_t *header,
				  size_t *body);

static int mailbox_unix_parse (mailbox_t, size_t *msgs);
static int mailbox_unix_is_updated (mailbox_t);

static int mailbox_unix_size (mailbox_t, off_t *size);

static int mailbox_unix_get_attribute (mailbox_t mbox, size_t msgno,
				       attribute_t *pattribute);
static int mailbox_unix_set_attribute (mailbox_t mbox, size_t msgno,
				       attribute_t attribute);

static ssize_t mailbox_unix_get_header (mailbox_t, size_t msgno, char *h,
					size_t len, off_t off, int *err);
static ssize_t mailbox_unix_get_body (mailbox_t, size_t msgno, char *b,
				      size_t len, off_t off, int *err);


/* private stuff */
static int mailbox_unix_is_from (const char *);
static int mailbox_unix_readhdr (mailbox_t mbox, char *buf, size_t len,
				 off_t *content_length, size_t msgno);
static int mailbox_unix_lock (mailbox_t mbox, int flag);
static int mailbox_unix_unlock (mailbox_t mbox);
static int mailbox_unix_ilock (mailbox_t mbox, int flag);
static int mailbox_unix_iunlock (mailbox_t mbox);

/* We allocate the mailbox_t struct, but don't do any
 * parsing on the name or even thest for existence.
 * However we do strip any leading "unix:" part of
 * our name, this is suppose to be the protocol/scheme name.
 * Hopefully there will not be a mailbox name "unix:"
 */

static int
mailbox_unix_init (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  mailbox_unix_data_t mud;
  const char *sep;
  size_t name_len;
  int i;

  /* sanity check */
  if (name == NULL || *name == '\0')
    return EINVAL;

  name_len = strlen (name);

#define UNIX_SCHEME "unix:"
#define UNIX_SCHEME_LEN 5
#define SEPARATOR '/'

  /* sskip the url scheme */
  if (name_len > UNIX_SCHEME_LEN &&
      (name[0] == 'u' || name[0] == 'U') &&
      (name[1] == 'n' || name[1] == 'N') &&
      (name[2] == 'i' || name[2] == 'i') &&
      (name[3] == 'x' || name[3] == 'x' ) &&
      name[4] == ':')
    {
      name += UNIX_SCHEME_LEN;
      name_len -= UNIX_SCHEME_LEN;
    }

  /* allocate memory for mbox */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return ENOMEM;

  /* allocate specific unix mbox data */
  mud = mbox->data = calloc (1, sizeof (*mud));
  if (mbox->data == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }

  /* copy the name */
  mbox->name = calloc (name_len + 1, sizeof (char));
  if (mbox->name == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }
  memcpy (mbox->name, name, name_len);

  /* save the basename and dirname */
  /* FIXME: We may have to support imap "SELECT"
   * So we split the name.  But this should probably be
   * supported via "maildir:"
   */
  /* equivalent to strrchr (name, '/'); */
  for (i = name_len, sep = NULL; i >= 0; i--)
    {
      /* break on the first separator */
      if (name[i] == SEPARATOR)
	{
	  sep = &(name[i]);
	  break;
	}
    }

  if (sep)
    {
      /* split it into two */
      mud->dirname = calloc ((sep - name) + 1, sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->dirname, name, sep - name);

      ++sep;
      mud->basename = calloc (name_len - (sep - name) + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, sep, name_len - (sep - name));
    }
  else
    {
      /* use the relative directory "." */
      /* FIXME: should we call getcwd() instead ? */
      mud->dirname = calloc (2 , sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      mud->dirname[0] = '.';

      mud->basename = calloc (name_len + 1, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, name, name_len);
    }

#ifdef HAVE_PHTREAD_H
  /* mutex when accessing the structure fields */
  /* FIXME: should we use rdwr locks instead ?? */
  pthread_mutex_init (&(mud->mutex), NULL);
#endif

  mbox->_init = mailbox_unix_init;
  mbox->_destroy = mailbox_unix_destroy;

  mbox->_open = mailbox_unix_open;
  mbox->_close = mailbox_unix_close;

  /* messages */
  mbox->_append_message = mailbox_unix_append_message;
  mbox->_messages_count = mailbox_unix_messages_count;
  mbox->_delete = mailbox_unix_delete;
  mbox->_undelete = mailbox_unix_undelete;
  mbox->_is_deleted = mailbox_unix_is_deleted;
  mbox->_expunge = mailbox_unix_expunge;
  mbox->_num_deleted = mailbox_unix_num_deleted;
  mbox->_get_size = mailbox_unix_get_size;

  mbox->_is_updated = mailbox_unix_is_updated;

  mbox->_size = mailbox_unix_size;

  mbox->_get_attribute = mailbox_unix_get_attribute;
  mbox->_set_attribute = mailbox_unix_set_attribute;

  mbox->_get_header = mailbox_unix_get_header;
  mbox->_get_body = mailbox_unix_get_body;

  (*pmbox) = mbox;

  return 0; /* okdoke */
}

static void
mailbox_unix_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      mailbox_unix_close (mbox);
      if (mbox->data)
	{
	  size_t i;
	  mailbox_unix_data_t mud = mbox->data;
	  free (mud->dirname);
	  free (mud->basename);
	  for (i = 0; i < mud->messages_count; i++)
	    {
	      /* orphan the message */
	      mud->messages[i].old_attr->message = NULL;
	      mud->messages[i].new_attr->message = NULL;
	      attribute_destroy (&(mud->messages[i].old_attr));
	      attribute_destroy (&(mud->messages[i].new_attr));
	    }
	  free (mud->messages);
	  free (mbox->data);
	}
      free (mbox->name);
      if (mbox->messages)
	{
	  size_t i;
	  for (i = 0; i < mbox->messages_num; i++)
	    {
	      /* first orphan them */
	      mbox->messages[i]->mailbox = NULL;
	      message_destroy (&(mbox->messages[i]));
	    }
	}
      free (mbox->messages);
      free (*pmbox);
      *pmbox = NULL;
    }
}

/* start of mbox Implementation */
static int
mailbox_unix_open (mailbox_t mbox, int flags)
{
  mailbox_unix_data_t mud;
  int fd = -1;
  int flg = 0;
  char *mode;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  if (flags & MU_MAILBOX_WRONLY)
    flg = O_WRONLY;
  else if (flags & MU_MAILBOX_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* FIXME: only if the file is open Write */
  if (flags & MU_MAILBOX_APPEND)
    flg |= O_APPEND;

  /* FIXME: does not really work, but local folders
   * should not block since it is local disk ???
   */
  if (flags & MU_MAILBOX_NONBLOCK)
    flg |= O_NONBLOCK;

  /* handle CREAT with care, not to follow symlinks */
  if (flags & MU_MAILBOX_CREAT)
    {
      /* Grab the lock for any race conditions */
      mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK);
      {
	/* first see if the file already exists */
	fd = open(mbox->name, flg);
	if (fd == -1)
	  {
	    /* oops bail out */
	    if (errno != ENOENT)
	      return errno;
	    /* Race condition here when creating the file ?? */
	    fd = open(mbox->name, flg|O_CREAT|O_EXCL, 0600);
	    if (fd < 0)
	      return errno;
	  }
	/*
	 * Set the owner ship, but should we return the errno
	 */
	if (mbox->auth)
	  {
	    auth_t auth = mbox->auth;
	    gid_t g ; uid_t u; mode_t m;
	    g = u = -1; m = 0600;
	    /* FIXME: what to do when they failed */
	    auth_get_owner (auth, &u);
	    auth_get_group (auth, &g);
	    auth_get_mode (auth, &m);
	    (void)fchown (fd, u, g);
	    (void)fchmod (fd, m);
	  }
      }
      mailbox_unix_unlock (mbox);
    }
  else
    {
      fd = open (mbox->name, flg);
      if (fd < 0)
	return errno;
    }

  /* we use FILE * object */
  if (flags & MU_MAILBOX_APPEND)
    mode = "a";
  else if (flags & MU_MAILBOX_RDWR || flags & MU_MAILBOX_WRONLY)
    mode = "r+";
  else /* default readonly*/
    mode = "r";

  /* clean up, make sure there was nothing before  */
  mailbox_unix_close (mbox);

  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  {
    mud->file = fdopen (fd, mode);
    if (mud->file == NULL)
      {
	/* what's up ?? */
	mailbox_unix_iunlock (mbox);
	return ENOMEM;
      }
    /* Is this necessary ?? */
    /* Check to make sure this is indeed a Unix Mail format */
    flockfile (mud->file);
    {
      char buf [6];
      if (fgets (buf, sizeof (buf), mud->file) == NULL)
	{
	  if (feof (mud->file))
	    clearerr (mud->file); /* the file maybe empty */
	  else if (ferror (mud->file))
	    {
	      mailbox_unix_iunlock (mbox);
	      return EIO;
	    }
	}
      else
	{
	  if (strncmp ("From ", buf, 5) != 0)
	    {
	      /* not a Unix mbox */
	      fclose (mud->file);
	      mailbox_unix_iunlock (mbox);
	      return EIO;
	    }
	}
      rewind (mud->file);
    }
    funlockfile (mud->file);
    mud->flags = flags;
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}

static int
mailbox_unix_close (mailbox_t mbox)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  {
    if (mud->file)
      fclose (mud->file);
    mud->file = NULL;
    /* make sure we do not hold any lock for that file */
    mailbox_unix_unlock (mbox);
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}


/* FIXME: a little weak, we should do full reconnaissance of the
 * the "From " header :
 *    From <user> <weekday> <month> <day> <hr:min:sec>
 *      [T1 [T2]] <year> [remote-list]
*/
static int
mailbox_unix_is_from (const char *from)
{
  const char *sep;

  /* From_ */
  if (strncmp (from, "From ", 5) != 0)
    return 0;
  from += 5;

  /* <user> */
  sep = strchr (from, ' ');
  if (sep == NULL)
    return 0;
  from = ++sep;

  /* weekday */
  sep = strchr (from, ' ');
  if (sep == NULL)
    return 0;
  from = ++sep;

  /* month */
  sep = strchr (from, ' ');
  if (sep == NULL)
    return 0;
  from = ++sep;

  /* day */
  sep = strchr (from, ' ');
  if (sep == NULL)
    return 0;
  from = ++sep;

  /* hr:min:sec */
  /* hr */
  sep = strchr (from, ':');
  if (sep == NULL)
    return 0;
  from = ++sep;
  /* min */
  sep = strchr (from, ':');
  if (sep == NULL)
    return 0;
  from = ++sep ;
  /* sec */
  sep = strchr (from, ' ');
  if (sep == NULL)
    return 0;

  /* FIXME PLEASE: the rest is getting more complex, finish it later */
  return 1;
}

/*
 * We skip over the rest of the header.  Scan for
 * Status: to set the attribute.  Hopefully the Content-Length
 * in there too.
 */
static int
mailbox_unix_readhdr (mailbox_t mbox, char *buf, size_t len,
		      off_t *content_length, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;
  mailbox_unix_message_t mum = &mud->messages[msgno];
  char *sep;

  /* skip over the remaining header */
  while (fgets (buf, len, mud->file))
    {
      /* FIXME: The heuristic is still weak
	 we will break and consider things not to be a header if:
	 1: an empty line
	 2: can not be a header
      */
      if (strcmp (buf, "\n") == 0 || strcmp (buf, "\r\n") == 0
	  || (isalpha (buf[0]) && (sep = strchr (buf, ':')) == NULL))
	break;
      /* get the the Content lenght of the body if possible */
      else if (strncmp (buf, "Content-Length:", 15) == 0)
	{
	  sep = strchr(buf, ':'); /* pass the ':' */
	  sep[strlen (sep) - 1] = '\0'; /* chop the newline */
	  /* FIXME: use xstrtol() if strtol() is not on the platform */
	  /* FIXME: what an awkward way of handling error
	     Some damage control can be done here.
	     for example just set content_length = -1;
	     and rely above to discover the body part */
	  errno = 0;
	  *content_length = strtol (sep + 1, NULL, 10);
	  if (*content_length == 0 ||
	      *content_length == LONG_MIN ||
	      *content_length == LONG_MAX)
	    {
	      if (errno != 0)
		return errno;
	    }
	}
      /* Set the attribute */
      else if (strncmp (buf, "Status:", 7) == 0)
        {
	  mum->status = ftell (mud->file);
	  mum->status_end = mum->status + strlen (buf);
          sep = strchr(buf, ':'); /* pass the ':' */
          if (strchr (sep, 'R') != NULL)
	    attribute_set_read (mum->old_attr);
          if (strchr (sep, 'O') != NULL)
	    attribute_set_seen (mum->old_attr);
          if (strchr (sep, 'A') != NULL)
	    attribute_set_answered (mum->old_attr);
          if (strchr (sep, 'F') != NULL)
	    attribute_set_flagged (mum->old_attr);
	  attribute_copy (mum->new_attr, mum->old_attr);
        }
    }
  /* check for any dubious conditions */
  if (feof (mud->file) || ferror (mud->file))
    return EIO;
  return 0;
}

/* Parsing.
 * This a bit fragile, I need to secure this.
 * The approach is to detect the "From " as start of a
 * new message, give the position of the header and scan
 * until "\n" then set header_end, set body position, if we have
 * a Content-Length field jump to the point if not
 * scan until we it another "From " and set body_end.
 */
static int
mailbox_unix_parse (mailbox_t mbox, size_t *msgs)
{
  char buf[BUFSIZ];
  int header = 1;
  int body = 0;
  off_t content_length = -1;
  size_t count = 0;
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  struct stat st;

  /* sanity */
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* FIXME:  I should also block signals and check cancelstate(Pthread)
     since We can not afford to be intr */
  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  mailbox_unix_lock (mbox, MU_LOCKER_RDLOCK);
  flockfile (mud->file);

  if (fstat (fileno (mud->file), &st) != 0)
    {
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      mailbox_unix_unlock (mbox);
      return EIO;
    }
  mud->mtime = st.st_mtime;
  mud->size = st.st_size;

  rewind (mud->file);

  while (fgets (buf, sizeof (buf), mud->file))
    {
      if (mailbox_unix_is_from (buf))
	{
	  if (body && count)
	    {
	      int over = strlen (buf);
              mum[count - 1].body_end = ftell (mud->file);
              mum[count - 1].body_end -= (over + 1);
	      body = 0;
	    }
	  header = 1;
	}

      /* header */
      if (header)
	{
	  /* FIXME: What happen if some mailer violates the rfc822 and the
	     "From " field contains a NULL byte */
	  int over = strlen (buf);

	  count++;
	  /* FIXME: This is a bad idea, we should not be allowed to
	   * jump out from the parsing this way.  We sit at the mercy
	   * of this function(_progress) waiting for disaster.
	   */
	  if (mbox->_progress)
	    {
	      /* FIXME: and its getting better, they can even bailout */
	      if (mbox->_progress (count, mbox->progress_arg) != 0)
		{
		  funlockfile (mud->file);
		  mailbox_unix_iunlock (mbox);
		  mailbox_unix_unlock (mbox);
		  return EINTR;
		}
	    }
	  /* allocate space for the new message */
	  if (count > mud->messages_count)
	    {
	      mum = realloc (mud->messages, count * sizeof (*mum));
	      if (mum == NULL)
		{
		  funlockfile (mud->file);
		  mailbox_unix_iunlock (mbox);
		  mailbox_unix_unlock (mbox);
		  return ENOMEM;
		}
	      mud->messages_count++;
	      memset (&mum[count - 1], 0, sizeof (*mum));
	      attribute_init (&(mum[count - 1].old_attr));
	      attribute_init (&(mum[count - 1].new_attr));
	    }
	  mud->messages = mum;
	  mum[count - 1].header = ftell (mud->file);
	  /* substract the overrun */
	  mum[count - 1].header -= over;

	  /* skip the remaining header and set the attributes */
	  if (mailbox_unix_readhdr (mbox, buf, sizeof (buf),
				    &content_length, count - 1) != 0)
	    {
	      funlockfile (mud->file);
	      mailbox_unix_iunlock (mbox);
	      mailbox_unix_unlock (mbox);
	      return EIO;
	    }
	  mum[count - 1].header_end = ftell (mud->file) - strlen(buf);
	  header = 0;
	  body = !header;
	} /* header */

      /* body */
      if (body)
	{
	  /* set the body position */
	  if (mum[count - 1].body == 0)
	    mum[count - 1].body = ftell (mud->file);

	  if (content_length >= 0)
	    {
	      /* ouf ! we got the lenght, jump */
	      mum[count - 1].body_end =  mum[count -1].body + content_length;
	      fseek (mud->file, content_length, SEEK_CUR);
	      content_length = -1;
	      body = 0;
	    }
	}
    } /* while */
  mum[count - 1].body_end = ftell (mud->file);

  if (feof (mud->file))
    clearerr (mud->file);
  else if (ferror (mud->file))
    {
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      mailbox_unix_unlock (mbox);
      return EIO;
    }
  rewind (mud->file);
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);
  mailbox_unix_unlock (mbox);
  if (msgs)
    *msgs = count;
  return 0;
}

/* FIXME:  How to handle a shrink ? meaning, the &^$^@%#@^& user
 * start two browsers and delete files in one.  My views
 * is that we should scream bloody murder and hunt them with
 * a machette.
 */
static int
mailbox_unix_is_updated (mailbox_t mbox)
{
  mailbox_unix_data_t mud;
  struct stat st;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      fstat (fileno (mud->file), &st) < 0)
    return 0;
  return (mud->mtime == st.st_mtime);
}

static int
mailbox_unix_is_valid (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return 0;
  /* valid ? */
  return (mud->messages_count > 0 && msgno <= mud->messages_count);
}

static int
mailbox_unix_is_deleted (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      ! mailbox_unix_is_valid (mbox, msgno))
    return 0;
  return attribute_is_deleted (mud->messages[msgno].new_attr);
}

static int
mailbox_unix_delete (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;

  /* oops out of range ? */
  /* if already deleted, noop */
  if (! mailbox_unix_is_valid (mbox, msgno) ||
      mailbox_unix_is_deleted (mbox, msgno))
    return 0;

  /* Mark for deletion */
  attribute_set_deleted (mud->messages[msgno].new_attr);
  return 0;
}

static int
mailbox_unix_num_deleted (mailbox_t mbox, size_t *num)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  size_t i, total;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;
  for (i = total = 0; i < mud->messages_count; i++)
    {
      mum = &mud->messages[i];
      if (attribute_is_deleted (mum->new_attr))
	total++;
    }

  if (num)
    *num = total;
  return 0;
}

static int
mailbox_unix_undelete (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;
  /* oops out of range ? */
  /* if already undeleted, noop */
  if (! mailbox_unix_is_valid (mbox, msgno) ||
      ! mailbox_unix_is_deleted (mbox, msgno))
    return 0;

  /* Mark undeletion */
  attribute_unset_deleted (mud->messages[msgno].new_attr);
  return 0;
}

/*
  FIXME: the use of tmpfile() on some system can lead to
  race condition, We should use a safer approach.
  We take a very naive approach for this, it involves unfortunately
  two copies.
*/
static FILE *
mailbox_unix_tmpfile ()
{
  /*FIXME: racing conditions, to correct, .i.e don;t use tmpfile*/
  //return tmpfile ();
  return fopen ("/tmp/mymail", "w+");
}

static int
mailbox_unix_expunge (mailbox_t mbox)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;
  int status = 0;
  sigset_t sigset;
  FILE *tmpfile;
  size_t nread;
  size_t i, j, first;
  off_t marker = 0;
  off_t total = 0;
  char buffer [BUFSIZ];

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* noop */
  if (mud->messages_count == 0)
    return 0;

  tmpfile = mailbox_unix_tmpfile ();
  if (tmpfile == NULL)
    return errno;

  /* Critical section, we can not allowed signal here */
  /* FIXME: If NONBLOCKING is set we should unset reset to
   * later, we can not afford to luxury here
   */
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGTERM);
  sigaddset (&sigset, SIGHUP);
  sigaddset (&sigset, SIGTSTP);
  sigaddset (&sigset, SIGINT);
  sigaddset (&sigset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigset, 0);

  /* Get the lock */
  if (mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK) < 0)
    {
      status = EIO;
      goto bailout;
    }
  mailbox_unix_ilock (mbox, MU_LOCKER_RDLOCK);
  flockfile (mud->file);

  /* Do we have a consistent view of the mailbox */
  /* FIXME: this is not enough we can do better
   * - by checking the file size and scream bloody murder
   * if it has shrink.
   * - if its bigger we should be able to handle it
   * before the ftruncate() by copying back the new messages.
   */
  if (! mailbox_unix_is_updated (mbox))
    {
      status = EAGAIN;
      goto bailout;
    }

  rewind (mud->file);

  /* Go to the first mail with an attribute change */
  for (j = 0; j < mud->messages_count; j++)
    {
      mum = &mud->messages[j];
      if (! attribute_is_equal (mum->old_attr, mum->new_attr))
	break;
    }

  /* did something change ? */
  if ((j + 1) == mud->messages_count)
    {
      /* nothing change, bail out */
      status = 0;
      goto bailout;
    }

  /* set the marker position */
  total = marker = mud->messages[j].header;

  /* copy to tempfile emails not mark changed */
  for (first = 1, i = j; i < mud->messages_count; i++)
    {
      size_t len;
      off_t current;
      mum = &mud->messages[i];

      /* skip it, if mark for deletion */
      if (attribute_is_deleted (mum->new_attr))
	continue;

      /* add a NL separtor between messages */
      if (first)
	first = 0;
      else
	{
	  fputc ('\n', tmpfile);
	  total++;
	}
      /* copy the header */
      if (fseek (mud->file, mum->header, SEEK_SET) == -1)
	{
	  status = EIO;
	  goto bailout;
	}
      /* attribute change ? */
      if (! attribute_is_equal (mum->old_attr, mum->new_attr) &&
	  mum->status > mum->header)
	{
	  len = mum->status - mum->header;
	  current = mum->status_end;
	  while (len > 0)
	    {
	      nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
		  || fwrite(buffer, sizeof(*buffer), nread, tmpfile) != nread)
		{
		  status = EIO;
		  goto bailout;
		}
	      len -= nread;
	      total += nread;
	    }
	  /* put the new attributes */
	  {
	    attribute_t attr = mum->new_attr;
	    fputs ("Status: ", tmpfile);
	    total += 8;
	    if (attribute_is_seen (attr))
	      {
		fputc ('R', tmpfile);
		total++;
	      }
	    if (attribute_is_answered (attr))
	      {
		fputc ('A', tmpfile);
		total++;
	      }
	    if (attribute_is_flagged (attr))
	      {
		fputc ('F', tmpfile);
		total++;
	      }
	    if (attribute_is_read (attr))
	      {
		fputc ('O', tmpfile);
		total++;
	      }
	    fputc ('\n', tmpfile);
	    total++;
	  }
	}
      else /* attribute did not change */
	{
	  current = mum->header;
	}
      len = mum->header_end - current;
      while (len > 0)
	{
	  nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	  if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
	      || fwrite(buffer, sizeof(*buffer), nread, tmpfile) != nread)
	    {
	      status = EIO;
	      goto bailout;
	    }
	  len -= nread;
	  total += nread;
	}

      /* Separate the header from body */
      fputc ('\n', tmpfile);
      total++;

      /* copy the body */
      if (fseek (mud->file, mum->body, SEEK_SET) < 0)
	{
	  status = EIO;
	  goto bailout;
	}
      len = mum->body_end - mum->body;
      while (len > 0)
	{
	  nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	  if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
	      || fwrite(buffer, sizeof(*buffer), nread, tmpfile) != nread)
	    {
	      status = EIO;
	      goto bailout;
	    }
	  len -= nread;
	  total += nread;
	}
    }

  /* truncate the mailbox and rewrite it */
  if (total <= 0 || fseek (mud->file, marker, SEEK_SET) < 0 ||
      ftruncate (fileno(mud->file), total) < 0)
    {
      status = EIO;
      goto bailout;
    }
  rewind (tmpfile);

  while ((nread = fread (buffer, sizeof (*buffer), sizeof (buffer), tmpfile)) != 0)
    fwrite (buffer, sizeof (*buffer), nread, mud->file);

  /* how can I handle error here ?? */
  clearerr (tmpfile);
  clearerr (mud->file);
  fflush (mud->file);

bailout:
  /* Release the lock */
  mailbox_unix_unlock (mbox);
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);
  fclose (tmpfile);
  sigprocmask (SIG_UNBLOCK, &sigset, 0);
  return status;
}

/* reading */
static ssize_t
mailbox_unix_get_body (mailbox_t mbox, size_t msgno, char *buffer,
		       size_t len, off_t off, int *err)
{
  mailbox_unix_data_t mud;
  size_t nread = 0;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    {
      if (err)
	*err = EINVAL;
      return -1;
    }

  /* check if valid */
  if (len < 1 || mud == NULL || ! mailbox_unix_is_valid (mbox, msgno))
    {
      if (err)
	*err = EINVAL;
      return -1;
    }

  mailbox_unix_ilock (mbox, MU_LOCKER_RDLOCK);
  flockfile (mud->file);
  {
    mailbox_unix_message_t mum = &(mud->messages[msgno]);
    off_t ln = mum->body_end - (mum->body + off);
    if (ln > 0)
      {
	nread = ((size_t)ln < len) ? ln : len;
	/* position the file pointer and the buffer */
	if (fseek (mud->file, mum->body + off, SEEK_SET) < 0)
	  {
	    funlockfile (mud->file);
	    mailbox_unix_iunlock (mbox);
	    if (err)
	      *err = EIO;
	    return -1;
	  }
	if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread)
	  {
	    funlockfile (mud->file);
	    mailbox_unix_iunlock (mbox);
	    if (err)
	      *err = EIO;
	    return -1;
	  }
      }
    else
      nread = 0;
  }
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);

  return nread;
}

static ssize_t
mailbox_unix_get_header (mailbox_t mbox, size_t msgno, char *buffer,
			 size_t len, off_t off, int *err)
{
  mailbox_unix_data_t mud;
  size_t nread = 0;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    {
      if (err)
	*err = EINVAL;
      return -1;
    }

  /* check if valid */
  if (len < 1 || ! mailbox_unix_is_valid (mbox, msgno))
    {
      if (err)
	*err = EINVAL;
      return -1;
    }

  mailbox_unix_ilock (mbox, MU_LOCKER_RDLOCK);
  flockfile (mud->file);
  {
    mailbox_unix_message_t mum = &(mud->messages[msgno]);
    off_t ln = mum->header_end - (mum->header + off);
    if (ln > 0)
      {
	nread = ((size_t)ln < len) ? ln : len;
	/* position the file pointer and the buffer */
	if (fseek (mud->file, mum->header + off, SEEK_SET) < 0)
	  {
	    funlockfile (mud->file);
	    mailbox_unix_iunlock (mbox);
	    if (err)
	      *err = EIO;
	    return -1;
	  }
	if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread)
	  {
	    funlockfile (mud->file);
	    mailbox_unix_iunlock (mbox);
	    if (err)
	      *err = EIO;
	    return -1;
	  }
      }
    else
      nread = 0;
  }
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);

  return nread;
}

static int
mailbox_unix_append_message (mailbox_t mbox, message_t msg)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL || msg == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;
  if (! ((mud->flags & MU_MAILBOX_APPEND) ||
	 (mud->flags & MU_MAILBOX_RDWR) ||
	 (mud->flags & MU_MAILBOX_WRONLY)))
    return EACCES;

  mailbox_unix_lock (mbox, MU_LOCKER_WRLOCK);
  flockfile (mud->file);
  {
    struct stat st;
    int fd;
    char buffer[BUFSIZ];
    header_t hdr;
    off_t off = 0;
    int err;
    size_t nread;

    fd = fileno (mud->file);
    if (fstat (fd, &st) != 0)
      {
	funlockfile (mud->file);
	mailbox_unix_unlock (mbox);
	return errno;
      }
    if (fseek(mud->file, st.st_size, SEEK_SET) != 0)
      {
	funlockfile (mud->file);
	mailbox_unix_unlock (mbox);
	return errno;
      }
    message_get_header (msg, &hdr);
    if (st.st_size != 0)
      fputc ('\n', mud->file);
    while ((nread = header_get_data (hdr, buffer, sizeof (buffer),
				     off, &err)) > 0)
      {
	fwrite (buffer, sizeof (*buffer), nread, mud->file);
	off += nread;
      }
    fputc ('\n', mud->file);
    off = 0;
    while ((nread = message_get_content (msg, buffer, sizeof (buffer),
					off, &err)) > 0)
      {
	fwrite (buffer, sizeof (*buffer), nread, mud->file);
	off += nread;
      }
  }
  fflush(mud->file);
  funlockfile (mud->file);
  mailbox_unix_unlock (mbox);
  return 0;
}

static int
mailbox_unix_size (mailbox_t mbox, off_t *size)
{
  mailbox_unix_data_t mud;
  struct stat st;
  int fd;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* maybe was not open yet ?? */
  if (mud->file == NULL)
      return EIO;
  fd = fileno (mud->file);
  /* oops !! */
  if (fstat (fd, &st) != 0)
    return errno;
  *size = st.st_size;
  return 0;
}

static int
mailbox_unix_get_size (mailbox_t mbox, size_t msgno, size_t *h, size_t *b)
{
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum;

  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      !mailbox_unix_is_valid (mbox, msgno))
    return EINVAL;

  mum = &(mud->messages[msgno]);
  if (h)
    *h = mum->header_end - mum->header;
  if (b)
    *b = mum->body_end - mum->body;
  return 0;
}

static int
mailbox_unix_messages_count (mailbox_t mbox, size_t *count)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL || (mud = (mailbox_unix_data_t) mbox->data) == NULL)
    return EINVAL;

  if (! mailbox_unix_is_updated (mbox))
    return mailbox_unix_parse (mbox, count);

  if (count)
    *count = mud->messages_count;

  return 0;
}

static int
mailbox_unix_get_attribute (mailbox_t mbox, size_t msgno,
			    attribute_t *pattribute)
{
  mailbox_unix_data_t mud;

  if (mbox == NULL || pattribute == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      ! mailbox_unix_is_valid (mbox, msgno))
    return EINVAL;

  *pattribute = mud->messages[msgno].new_attr;
  return 0;
}

static int
mailbox_unix_set_attribute (mailbox_t mbox, size_t msgno,
			    attribute_t attribute)
{
  mailbox_unix_data_t mud;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL ||
      ! mailbox_unix_is_valid (mbox, msgno))
    return EINVAL;
  attribute_copy (mud->messages[msgno].new_attr, attribute);
  return 0;
}

/* locking */
static int
mailbox_unix_lock (mailbox_t mbox, int flag)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_lock (locker, flag);
    }
  return 0;
}

static int
mailbox_unix_unlock (mailbox_t mbox)
{
  if (mbox && mbox->locker != NULL)
    {
      locker_t locker = mbox->locker;
      locker_unlock (locker);
    }
  return 0;
}

static int
mailbox_unix_ilock (mailbox_t mbox, int flag)
{
#ifdef HAVE_PTHREAD_H
  mailbox_unix_data_t mud;
  (void)flag; /* we should use rwlocks for more concurency */
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;
  return pthread_mutex_lock (&(mud->mutex));
#else
  (void)mbox; (void)flag;
  return 0;
#endif
}

static int
mailbox_unix_iunlock (mailbox_t mbox)
{
#ifdef HAVE_PTHREAD_H
  mailbox_unix_data_t mud;
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;
  return pthread_mutex_unlock (&(mud->mutex));
#else
  (void)mbox;
  return 0;
#endif
}
