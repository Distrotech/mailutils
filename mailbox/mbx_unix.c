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

#include <url_unix.h>
#include <mbx_unix.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

struct mailbox_type _mailbox_unix_type =
{
  "UNIX MBOX",
  (int)&_url_unix_type, &_url_unix_type,
  mailbox_unix_init, mailbox_unix_destroy
};

typedef struct _mailbox_unix_msg
{
  off_t header;
  off_t body;
  off_t end;
  int   deleted;
  int   is_read;
  int   is_seen;
} *mailbox_unix_msg_t;

typedef struct _mailbox_unix_data
{
  mailbox_unix_msg_t messages;
  FILE *file;
  char *dirname;
  char *basename;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  int lock;
  int ilock;
  time_t mtime;
  size_t size;
} *mailbox_unix_data_t;


/* forward prototypes */

static int mailbox_unix_open (mailbox_t mbox, int flag);
static int mailbox_unix_close (mailbox_t mbox);

static int mailbox_unix_get_name (mailbox_t, int *id, char *name,
				  size_t len, size_t *n);

/* passwd */

/* updating/deleting */
static int mailbox_unix_is_deleted (mailbox_t, size_t msgno);
static int mailbox_unix_delete (mailbox_t, size_t msgno);
static int mailbox_unix_undelete (mailbox_t, size_t msgno);
static int mailbox_unix_expunge (mailbox_t);
static int mailbox_unix_is_updated (mailbox_t);
static int mailbox_unix_scan (mailbox_t, size_t *msgs);


/* appending */
static int mailbox_unix_new_msg (mailbox_t, size_t *msgno);
static int mailbox_unix_set_header (mailbox_t, size_t msgno, const char *h,
				    size_t len, int replace);
static int mailbox_unix_set_body (mailbox_t, size_t msgno, const char *b,
				  size_t len, int replace);
static int mailbox_unix_append (mailbox_t, size_t msgno);
static int mailbox_unix_destroy_msg (mailbox_t, size_t msgno);

/* reading */
static int mailbox_unix_get_body (mailbox_t, size_t msgno, off_t off,
				  char *b, size_t len, size_t *n);
static int mailbox_unix_get_header (mailbox_t, size_t msgno, off_t off,
				    char *h, size_t len, size_t *n);
static int mailbox_unix_get_size (mailbox_t, size_t msgno, size_t *header,
				  size_t *body);

/* setting flags */
static int mailbox_unix_is_read (mailbox_t, size_t msgno);
static int mailbox_unix_set_read (mailbox_t, size_t msgno);
static int mailbox_unix_is_seen (mailbox_t, size_t msgno);
static int mailbox_unix_set_seen (mailbox_t, size_t msgno);

/* owner and group
static int mailbox_unix_set_owner (mailbox_t mbox, uid_t uid);
static int mailbox_unix_get_owner (mailbox_t mbox, uid_t *uid);
static int mailbox_unix_set_group (mailbox_t mbox, gid_t gid);
static int mailbox_unix_get_group (mailbox_t mbox, gid_t *gid);
*/

/* locking */
static int mailbox_unix_lock (mailbox_t, int flag);
static int mailbox_unix_unlock (mailbox_t);
static int mailbox_unix_ilock (mailbox_t, int flag);
static int mailbox_unix_iunlock (mailbox_t);

static int mailbox_unix_size (mailbox_t, off_t *size);

/* private stuff */
static int mailbox_unix_is_from (const char *);
static int mailbox_unix_readhdr (mailbox_t mbox, char *buf, size_t len,
				 off_t *content_length);
static int mailbox_unix_sigblock ();
static int mailbox_unix_sigunblock ();

/* Having a structure initialize at compiletime instead of runtime
   may speed thing a bit, but it is a real pain to maintain with
   the changing API */
static struct _mailbox unixmbox =
{
  /* Data */

  (char *)NULL, /* char *name ; */
  (uid_t)-1, /* uid_t owner; */
  (gid_t)-1, /* gid_t group; */
  0, /* messages */
  0, /* num_deleted */
  (off_t)0, /* size */
  0, /* lock */
  0, /* timeout; */
  0, /* refresh; */
  (int (*)())NULL, /* (*func) __P ((mailbox_t, void *arg)) */

  /* type of mailbox */
  &_mailbox_unix_type, /*struct mailbox_type *mtype; */

  /* back pointer to the specific mailbox */
  (void *)NULL, /* void *data; */

  /* Functions */

  mailbox_unix_open, /* (*_open) */
  mailbox_unix_close, /* (*_close) */

  /* type */
  mailbox_unix_get_name, /* (*_get_name) */
  NULL, /* (*_get_mname) */

  /* passwd if needed */
  NULL, /* (*_get_passwd) */
  NULL, /* (*_get_mpasswd) */
  NULL, /* (*_set_passwd) */

  /* updating/deleting mesgs */
  mailbox_unix_is_deleted, /* (*_is_deleted) */
  mailbox_unix_delete, /* (*_delete) */
  mailbox_unix_undelete, /* (*_undelete) */
  mailbox_unix_expunge, /* (*_expunge) */
  mailbox_unix_is_updated, /* (*_is_updated) */
  mailbox_unix_scan, /* (*_scan) */

  /* appending messages */
  mailbox_unix_new_msg, /* (*_new_msg) */
  mailbox_unix_set_header, /* (*_set_header) */
  mailbox_unix_set_body, /* (*_set_body) */
  mailbox_unix_append, /* (*_append) */
  mailbox_unix_destroy_msg, /* (*_destroy_msg) */

  /* external locking */
  mailbox_unix_lock, /* (*_lock) */
  mailbox_unix_unlock, /* (*_unlock) */
  /* internal locking */
  mailbox_unix_ilock, /* (*_ilock) */
  mailbox_unix_iunlock, /* (*_iunlock) */

  /* reading mesgs */
  mailbox_unix_get_body, /* (*_get_body) */
  NULL, /* (*_get_mbody) */
  mailbox_unix_get_header, /* (*_get_header) */
  NULL, /* (*_get_mheader) */
  mailbox_unix_get_size, /* (*_get_msg_size) */

  /* setting flags */
  mailbox_unix_is_read, /* (*_msg_is_read) */
  mailbox_unix_set_read, /* (*_msg_set_read) */
  mailbox_unix_is_seen, /* (*_msg_is_seen) */
  mailbox_unix_set_seen, /* (*_msg_set_seen) */

  /* owner and group */
  NULL, /* (*_set_owner) */
  NULL, /* (*_get_owner) */
  NULL, /* (*_set_group) */
  NULL, /* (*_get_group) */

  mailbox_unix_size, /* (*_size) */
  NULL, /* (*_get_timeout) */
  NULL, /* (*_set_timeout) */
  NULL, /* (*_get_refresh) */
  NULL, /* (*_set_refresh) */
  NULL, /* (*_set_notification) */
};

int
mailbox_unix_init (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  size_t len;
  mailbox_unix_data_t mud;
  char *sep;


  /* sanity check */
  if (name == NULL || *name == '\0')
    {
      return EINVAL;
    }

  /* pass the url */
  sep = strstr (name, "unix://");
  if (sep)
    name += 7;

  /* allocate memory for mbox */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    {
      return ENOMEM; /* errno set by calloc() */
    }

  /* binary copy of the function */
  *mbox = unixmbox;

  /* specific unix mbox data */
  mud = mbox->data = calloc (1, sizeof (*mud));
  if (mbox->data == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }

  /* copy the name */
  len = strlen (name) + 1;
  mbox->name = calloc (len, sizeof (char));
  if (mbox->name == NULL)
    {
      mailbox_unix_destroy (&mbox);
      return ENOMEM;
    }
  memcpy (mbox->name, name, len);

  /* save the basename and dirname */
  sep = strrchr (name, '/');
  if (sep)
    {
      size_t baseln;
      mud->dirname = calloc (sep - name + 1, sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->dirname, name, sep - name);

      baseln = strlen (++sep) + 1;
      mud->basename = calloc (baseln, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, sep, baseln);
    }
  else
    {
      mud->dirname = calloc (2 , sizeof (char));
      if (mud->dirname == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      mud->dirname[0] = '.';

      mud->basename = calloc (len, sizeof (char));
      if (mud->basename == NULL)
	{
	  mailbox_unix_destroy (&mbox);
	  return ENOMEM;
	}
      memcpy (mud->basename, name, len);
    }

  /* mutex when accessing the structure fields */
  /* FIXME: should we use rdwr locks instead ?? */
#ifdef HAVE_PHTREAD_H
  pthread_mutex_init (mud->mutex, NULL);
#endif


  (*pmbox) = mbox;

  return 0; /* okdoke */
}

void
mailbox_unix_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      mailbox_unix_close (mbox);
      if (mbox->data)
	{
	  mailbox_unix_data_t mud = mbox->data;
	  if (mud->dirname)
	    free (mud->dirname);
	  if (mud->basename)
	    free (mud->basename);
	  if (mud->messages)
	    free (mud->messages);
	  free (mbox->data);
	}
      if (mbox->name)
	free (mbox->name);
      free (*pmbox);
      pmbox = NULL;
    }
}


/* start of Mbox Implementation */

static int
mailbox_unix_open (mailbox_t mbox, int flags)
{
  mailbox_unix_data_t mud = mbox->data;
  int fd = -1;
  int flg = 0;
  char * mode;

  /*
    FIXME: This is another problem, should we have a special set of flags
    MU_MB_{CREAT,APPEND,RDONLY,RDWR} or use the default open(2)
    and if RDONLY, should we have some security checks ?
  */
  if (flags & MU_MB_WRONLY)
    {
      flg = O_WRONLY;
    }
  else if (flags & MU_MB_RDWR)
    {
      flg = O_RDWR;
    }
  else /* default */
    {
      flg = O_RDONLY;
    }

  if (flags & MU_MB_APPEND)
    {
      flg |= O_APPEND;
    }

  /* handle CREAT with care, not to follow symlinks */
  if (flags & MU_MB_CREAT)
    {
      /* firts see if the file already exists */
      fd = open(mbox->name, flg);
      if (fd == -1)
	{
	  /* oops bail out */
	  if (errno != ENOENT)
	    {
	      return errno;
	    }
	  /* Create the file */
	  fd = open(mbox->name, flg|O_CREAT|O_EXCL, 0600);
	  if (fd < 0)
	    {
	      return errno;
	    }
	}
      /*
	FIXME:  How about owner(uid), to whom we set it to ?
	do We need a _set_owner(uid) to mailbox_t
      */
      (void)fchown (fd, mbox->owner, mbox->group);
      /* FIXME: should we have a mode field ? */
      (void)fchmod (fd, 0600);
    }
  else
    {
      fd = open (mbox->name, flg);
      if (fd < 0)
	{
	  return errno;
	}
    }

  /* we use FILE * object */
  if (flg & MU_MB_RDWR)
    {
      mode = "r+";
    }
  else if (flg & MU_MB_WRONLY)
    {
      mode = "w";
    }
  else if (flg & MU_MB_APPEND)
    {
      mode = "a";
    }
  else /* default readonly*/
    {
      mode = "r";
    }

  /* clean up */
  mailbox_unix_close (mbox);

  mailbox_unix_ilock (mbox, MU_MB_WRLOCK);
  {
    mud->file = fdopen (fd, mode);
    if (mud->file == NULL)
      {
	/* what's up ?? */
	mailbox_unix_iunlock (mbox);
	return ENOMEM;
      }
    flockfile (mud->file);
    {
      char buf [BUFSIZ];
      if (fgets (buf, sizeof (buf), mud->file) == NULL)
	{
	  if (feof (mud->file))
	    {
	      clearerr (mud->file); /* the file maybe empty */
	    }
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
	  else
	    {
	      rewind (mud->file);
	    }
	}
    }
    funlockfile (mud->file);
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}

static int
mailbox_unix_close (mailbox_t mbox)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;

  mailbox_unix_ilock (mbox, MU_MB_WRLOCK);
  {

    /* make sure we do not hold any lock for that file */

    mailbox_unix_unlock (mbox);
    if (mud->file)
      {
	fclose (mud->file);
      }
    mud->file = NULL;
  }
  mailbox_unix_iunlock (mbox);
  return 0;
}

static int
mailbox_unix_get_name (mailbox_t mbox, int *id, char *name,
		       size_t len, size_t *n)
{
  char *s = _mailbox_unix_type.name;
  size_t i = strlen (s);
  if (id)
    {
      *id = _mailbox_unix_type.id;
    }
  if (name && len > 0)
    {
      i = (len < i) ? len : i;
      strncpy (name, s, i - 1);
      name [i - 1] = 0;
    }
  if (n)
    {
      *n = i;
    }
  return 0;
}

/* passwd */
/* We don't care */

/* FIXME: a little weak, we should do full reconnaissance of the
   the "From " header :
   From email weekday month day time timezone year
*/
static int
mailbox_unix_is_from (const char *from)
{
  const char *sep;
  return (strncmp (from, "From ", 5) == 0
	  && (sep = strchr (from, ':')) != NULL
	  && strchr (sep, ':') != NULL);
}

static int
mailbox_unix_readhdr (mailbox_t mbox, char *buf, size_t len,
		      off_t *content_length)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;
  mailbox_unix_msg_t mum = mud->messages;
  char *sep;
  size_t count = mbox->messages;

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
	{
	  break;
	}
      /* get the the Content lenght of the body if possible */
      if (strncmp (buf, "Content-Length:", 15) == 0)
	{
	  sep = strchr(buf, ':'); /* pass the ':' */
	  sep[strlen (sep) - 1] = '\0'; /* chop the newline */
	  *content_length = strtol (sep + 1, NULL, 10);
	}
      if (strncmp (buf, "Status:", 7) == 0)
	{
	  sep = strchr(buf, ':'); /* pass the ':' */
	  if (strchr (sep, 'R') != NULL)
	    {
	      mum[count - 1].is_read = 1;
	    }
	  if (strchr (sep, 'O') != NULL)
	    {
	      mum[count - 1].is_seen = 1;
	    }
	}
    }
  if (feof (mud->file) || ferror (mud->file))
    {
      return EIO;
    }
  return 0;
}

  /* Updating/deleting */
static int
mailbox_unix_scan (mailbox_t mbox, size_t *msgs)
{
  char buf[BUFSIZ];
  int header = 1;
  int body = 0;
  off_t content_length = -1;
  size_t count = 0;
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  mailbox_unix_msg_t mum;
  struct stat st;

  mailbox_unix_ilock (mbox, MU_MB_WRLOCK);
  /* FIXME:  I should also block signals since
     We can affor to be intr */
  flockfile (mud->file);
  rewind (mud->file);

  if (fstat (fileno (mud->file), &st) != 0)
    {
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      return EIO;
    }
  mud->mtime = st.st_mtime;
  mud->size = st.st_size;

  while (fgets (buf, sizeof (buf), mud->file))
    {
      /* header */
      if ((header && mailbox_unix_is_from (buf)))
	{
	  /* FIXME: What happen if some mailer violates the rfc8222 and the
	     "From " field contains a NULL byte */
	  int over = strlen (buf);
	  count++;
	  if (count > mbox->messages)
	    {
	      mum = realloc (mud->messages, count * sizeof (*mum));
	      if (mum == NULL)
		{
		  funlockfile (mud->file);
		  mailbox_unix_iunlock (mbox);
		  return ENOMEM;
		}
	      mbox->messages++;
	      memset (&mum[count - 1], 0, sizeof (*mum));
	    }
	  mud->messages = mum;
	  mum[count - 1].header = ftell (mud->file);
	  mum[count - 1].header -= over;

	  /* skip over the remaining header */
	  if (mailbox_unix_readhdr (mbox, buf, sizeof (buf),
				    &content_length) != 0)
	    {
	      funlockfile (mud->file);
	      mailbox_unix_iunlock (mbox);
	      return EIO;
	    }
	  header = 0;
	  body = !header;
	} /* header */

      /* body */
      if (body && content_length >= 0)
	{
	  /* ouf ! we got the size */
	  mum[count - 1].body = ftell (mud->file) - strlen (buf);
	  fseek (mud->file, content_length, SEEK_CUR);
	  mum[count - 1].end = ftell (mud->file);
	  content_length = -1;
	  header = 1;
	  body = !header;
	}
      else if (body)
	{
	  /* oops! some heuristic since we do not know the size of the body */
	  if (mum[count - 1].body == 0)
	    {
	      mum[count - 1].body = ftell (mud->file) - strlen (buf);
	    }

	  if (mailbox_unix_is_from (buf))
	    {
	      int over = strlen (buf);
	      mum[count - 1].end = ftell (mud->file);
	      mum[count - 1].end -= over;
	      count++;
	      if (count > mbox->messages)
		{
		  mum = realloc (mud->messages, count * sizeof (*mum));
		  if (mum == NULL)
		    {
		      funlockfile (mud->file);
		      mailbox_unix_iunlock (mbox);
		      return ENOMEM;
		    }
		  mbox->messages++;
		  memset (&mum[count - 1], 0, sizeof (*mum));
		}
	      mud->messages = mum;
	      mum[count - 1].header = ftell (mud->file);
	      mum[count - 1].header -= over;

	      /* skip over the remaining header */
	      if (mailbox_unix_readhdr (mbox, buf, sizeof (buf),
					&content_length) != 0)
		{
		  funlockfile (mud->file);
		  mailbox_unix_iunlock (mbox);
		  return EIO;
		}

	      mum[count - 1].body = ftell (mud->file) - strlen (buf);
	      if (content_length >= 0)
		{
		  fseek (mud->file, content_length, SEEK_CUR);
		  mum[count - 1].end = ftell (mud->file);
		  content_length = -1;
		  header = 0;
		  body = !header;
		}
	    }
	} /* body */
    } /* while */
  mum[count - 1].end = ftell (mud->file);
  if (feof (mud->file))
    {
      clearerr (mud->file);
    }
  else if (ferror (mud->file))
    {
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      return EIO;
    }
  rewind (mud->file);
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);
  if (msgs)
    *msgs = count;
  return 0;
}

static int
mailbox_unix_is_updated (mailbox_t mbox)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  struct stat st;
  if (mud == 0 || fstat (fileno (mud->file), &st) < 0)
    {
      return 0;
    }
  return (mud->mtime == st.st_mtime);
}

static int
mailbox_unix_is_valid (mailbox_t mbox, size_t msgno)
{
  /* valid ? */
  return (mbox->messages > 0 && msgno <= mbox->messages);
}

static int
mailbox_unix_is_deleted (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;

  /* Do we have a consistent view of the mbox ? */
  if (mud == NULL || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return 0;
    }
  return mud->messages[msgno].deleted;
}

static int
mailbox_unix_delete (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;

  /* oops out of range ? */
  /* if already deleted, noop */
  if (mud == NULL || ! mailbox_unix_is_valid (mbox, msgno)
      || mailbox_unix_is_deleted (mbox, msgno))
    {
      return 0;
    }

  /* Mark for deletion */
  mud->messages[msgno].deleted = 1;
  mbox->num_deleted++;
  return 0;
}

static int
mailbox_unix_undelete (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;

  /* oops out of range ? */
  /* if already undeleted, noop */
  if (mud == 0 || ! mailbox_unix_is_valid (mbox, msgno)
      || ! mailbox_unix_is_deleted (mbox, msgno))
    {
      return 0;
    }

  /* Mark undeletion */
  mud->messages[msgno].deleted = 0;
  mbox->num_deleted--;
  return 0;
}

static FILE *
mailbox_unix_tmpfile ()
{
  /*FIXME: racing conidtions, to correct, .i.e don;t use tmpfile*/
  //return tmpfile ();
  return fopen ("/tmp/mymail", "w");
}

/*
  FIXME: the use of tmpfile() on some system can lead to
  race condition, We should use a safer approach.
  We take a very naive approach to this, it involves unfortunately
  two copies.
*/
static int
mailbox_unix_expunge (mailbox_t mbox)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t)mbox->data;
  mailbox_unix_msg_t mum;
  sigset_t sigset;
  FILE *tmpfile;
  size_t i;
  off_t total = 0;
  char buffer [BUFSIZ];

  if (mud == NULL)
    {
      return EINVAL;
    }
  if (mbox->messages == 0 || mbox->num_deleted <= 0)
    {
      /* noop */
      return 0;
    }

  tmpfile = mailbox_unix_tmpfile ();
  if (tmpfile == NULL)
    {
      return errno;
    }

  /* Get the lock */
  mailbox_unix_ilock (mbox, MU_MB_RDLOCK);
  flockfile (mud->file);
  if (mailbox_unix_lock (mbox, MU_MB_WRLOCK) < 0)
    {
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      fclose (tmpfile);
      return EINVAL;
    }

  /* Do we have a consistent view of the mbox */
  if (! mailbox_unix_is_updated (mbox))
    {
      /* things change, flag an error */
      mailbox_unix_unlock (mbox);
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      fclose (tmpfile);
      return EAGAIN;
    }


  rewind (mud->file);
  /* copy to temp file emails not mark deleted emails */
  for (i = 1; i <= mbox->messages; i++)
    {
      mum = &mud->messages[i];
      if ( ! mum->deleted)
	{
	  size_t len = mum->body - mum->header;
	  size_t nread;
	  /* copy the header */
	  if (fseek (mud->file, mum->header, SEEK_SET) == -1)
	    {
	      mailbox_unix_unlock (mbox);
	      funlockfile (mud->file);
	      mailbox_unix_iunlock (mbox);
	      fclose (tmpfile);
	      return EIO;
	    }
	  while (len > 0)
	    {
	      nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
		  || fwrite(buffer, sizeof(*buffer), nread, tmpfile) != nread)
		{
		  mailbox_unix_unlock (mbox);
		  funlockfile (mud->file);
		  mailbox_unix_iunlock (mbox);
		  fclose (tmpfile);
		  return EIO;
		}
	      len -= nread;
	      total += nread;
	    }

	  /* FIXME: We should set the seen flag also here */

	  /* copy the body */
	  len = mum->end - mum->body;
	  if (fseek (mud->file, mum->body, SEEK_SET) < 0)
	    {
	      mailbox_unix_unlock (mbox);
	      funlockfile (mud->file);
	      mailbox_unix_iunlock (mbox);
	      fclose (tmpfile);
	      return EIO;
	    }
	  while (len > 0)
	    {
	      nread = (len < sizeof (buffer)) ? len : sizeof (buffer);
	      if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
		  || fwrite(buffer, sizeof(*buffer), nread, tmpfile) != nread)
		{
		  mailbox_unix_unlock (mbox);
		  fclose (tmpfile);
		  mailbox_unix_iunlock (mbox);
		  funlockfile (mud->file);
		  return EIO;
		}
	      len -= nread;
	      total += nread;
	    }
	}
    }

  /* Critical section, we can not allowed signal here */
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGTERM);
  sigaddset (&sigset, SIGHUP);
  sigaddset (&sigset, SIGTSTP);
  sigaddset (&sigset, SIGINT);
  sigaddset (&sigset, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigset, 0);

  /* truncate the mailbox and rewrite it */
  rewind (mud->file);
  rewind (tmpfile);
  if (ftruncate (fileno(mud->file), total) < 0)
    {
      mailbox_unix_unlock (mbox);
      funlockfile (mud->file);
      mailbox_unix_iunlock (mbox);
      fclose (tmpfile);
      sigprocmask (SIG_UNBLOCK, &sigset, 0);
      return EIO;
    }

  while (total > 0)
    {
      size_t nread = ((unsigned)total < sizeof (buffer)) ? total : sizeof (buffer);
      if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread
	  || fwrite (buffer, sizeof (*buffer), nread, tmpfile) != nread)
	{
	  mailbox_unix_unlock (mbox);
	  funlockfile (mud->file);
	  mailbox_unix_iunlock (mbox);
	  fclose (tmpfile);
	  sigprocmask (SIG_UNBLOCK, &sigset, 0);
	  return EIO;
	}
      total -= nread;
    }

  fflush (mud->file);

  /* Release the lock */
  mailbox_unix_unlock (mbox);
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);
  fclose (tmpfile);
  sigprocmask (SIG_UNBLOCK, &sigset, 0);
  return 0;
}

/* appending */
static int
mailbox_unix_new_msg (mailbox_t mbox, size_t *msgno)
{
  return ENOSYS;
}
static int
mailbox_unix_set_header (mailbox_t mbox, size_t msgno, const char *h,
			 size_t len, int replace)
{
  return ENOSYS;
}
static int
mailbox_unix_set_body (mailbox_t mbox, size_t msgno, const char *b,
		       size_t len, int replace)
{
  return ENOSYS;
}
static int
mailbox_unix_append (mailbox_t mbox, size_t msgno)
{
  return ENOSYS;
}
static int
mailbox_unix_destroy_msg (mailbox_t mbox, size_t msgno)
{
  return ENOSYS;
}

/* reading */
static int
mailbox_unix_get_body (mailbox_t mbox, size_t msgno, off_t off,
		       char *buffer, size_t len, size_t *n)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  size_t nread;

  /* check if valid */
  if (len < 1 || mud == NULL || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return EINVAL;
    }

  mailbox_unix_ilock (mbox, MU_MB_RDLOCK);
  flockfile (mud->file);
  {
    mailbox_unix_msg_t mum = &(mud->messages[msgno]);
    off_t ln = mum->end - mum->body + off;
    if (ln < 0)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    nread = (ln < len) ? ln : len;
    /* position the file pointer and the buffer */
    if (fseek (mud->file, mum->body + off, SEEK_SET) < 0)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    buffer[nread - 1] = '\0';
  }
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);

  if (n)
    *n = nread;
  return 0;
}
static int
mailbox_unix_get_header (mailbox_t mbox, size_t msgno, off_t off,
			 char *buffer, size_t len, size_t *n)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  size_t nread = 0;

  /* check if valid */
  if ( len < 1 || mud == NULL || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return EINVAL;
    }

  mailbox_unix_ilock (mbox, MU_MB_RDLOCK);
  flockfile (mud->file);
  {
    mailbox_unix_msg_t mum = &(mud->messages[msgno]);
    off_t ln = mum->body - mum->header + off;
    if (ln < 0)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    nread = (ln < len) ? ln : len;
    /* position the file pointer and the buffer */
    if (fseek (mud->file, mum->header + off, SEEK_SET) < 0)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    if (fread (buffer, sizeof (*buffer), nread, mud->file) != nread)
      {
	funlockfile (mud->file);
	mailbox_unix_iunlock (mbox);
	return EIO;
      }
    buffer[nread - 1] = '\0';
  }
  funlockfile (mud->file);
  mailbox_unix_iunlock (mbox);

  if (n)
    *n = nread;
  return 0;
}

/* setting flags */
static int
mailbox_unix_is_read (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  if (mud == 0 || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return 0;
    }
  return mud->messages[msgno].is_read;
}

static int
mailbox_unix_set_read (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  if (mud == 0 || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return EINVAL;
    }
  mud->messages[msgno].is_read = 1;;
  return 0;
}
static int
mailbox_unix_is_seen (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  if (mud == 0 || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return 0;
    }
  return mud->messages[msgno].is_seen;
}

static int
mailbox_unix_set_seen (mailbox_t mbox, size_t msgno)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  if (mud == 0 || ! mailbox_unix_is_valid (mbox, msgno))
    {
      return EINVAL;
    }
  mud->messages[msgno].is_seen = 1;
  return 0;
}

/* locking */
static int
mailbox_unix_ilock (mailbox_t mbox, int flag)
{
#ifdef HAVE_PTHREAD_H
  pthread_mutex_lock (&mutex)
#endif
  return 0;
}

static int
mailbox_unix_iunlock (mailbox_t mbox)
{
#ifdef HAVE_PTHREAD_H
  pthread_mutex_unlock (&mutex)
#endif
  return 0;
}

static int
mailbox_unix_lock (mailbox_t mbox, int flag)
{
#ifdef HAVE_MAILOCK_H
#endif
  return 0;
}
static int
mailbox_unix_unlock (mailbox_t mbox)
{
#ifdef HAVE_MAILOCK_H
#endif
  return 0;
}

/* miscellany */
static int
mailbox_unix_size (mailbox_t mbox, off_t *size)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  struct stat st;
  int fd;

  if (mud == NULL || mud->file == NULL)
    {
      /* maybe was not open yet ?? */
      return EIO;
    }
  fd = fileno (mud->file);
  if (fstat (fd, &st) != 0)
    {
      /* oops !! */
      /* errno set by fstat () */
      return EIO;
    }
  *size = st.st_size;
  return 0;
}

static int
mailbox_unix_get_size (mailbox_t mbox, size_t msgno, size_t *h, size_t *b)
{
  mailbox_unix_data_t mud = (mailbox_unix_data_t) mbox->data;
  mailbox_unix_msg_t mum;

  if (mailbox_unix_is_valid (mbox, msgno))
    {
      return EINVAL;
    }
  mum = &(mud->messages[msgno]);
  if (h)
    *h = mum->header - mum->body;
  if (b)
    *b = mum->body - mum->header;
  return 0;
}
