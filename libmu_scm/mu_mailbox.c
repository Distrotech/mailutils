/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

long mailbox_tag;

/* NOTE: Maybe will have to add some more members. That's why it is a
   struct, not just a typedef mailbox_t */
struct mu_mailbox
{
  mailbox_t mbox;       /* Mailbox */
};

/* SMOB functions: */
static SCM
mu_scm_mailbox_mark (SCM mailbox_smob)
{
  return SCM_BOOL_F;
}

static scm_sizet
mu_scm_mailbox_free (SCM mailbox_smob)
{
  struct mu_mailbox *mum = (struct mu_mailbox *) SCM_CDR (mailbox_smob);
  mu_mailbox_close (mum->mbox);
  mu_mailbox_destroy (&mum->mbox);
  free (mum);
  /* NOTE: Currently there is no way for this function to return the
     amount of memory *actually freed* by mu_mailbox_destroy */
  return sizeof (struct mu_mailbox);
}

static int
mu_scm_mailbox_print (SCM mailbox_smob, SCM port, scm_print_state * pstate)
{
  struct mu_mailbox *mum = (struct mu_mailbox *) SCM_CDR (mailbox_smob);
  size_t count = 0;
  url_t url = NULL;

  mu_mailbox_messages_count (mum->mbox, &count);
  mu_mailbox_get_url (mum->mbox, &url);

  scm_puts ("#<mailbox ", port);

  if (mailbox_smob == SCM_BOOL_F)
    {
      /* mu_mailbox.* functions may return #f */
      scm_puts ("#f", port);
    }
  else
    {
      const char *p = url_to_string (url);
      if (p)
	{
	  char buf[64];
	  
	  scm_puts (p, port);
	  
	  snprintf (buf, sizeof (buf), " (%d)", count);
	  scm_puts (buf, port);
	}
      else
	scm_puts ("uninitialized", port);
    }
  scm_puts (">", port);

  return 1;
}

/* Internal functions */

SCM
mu_scm_mailbox_create (mailbox_t mbox)
{
  struct mu_mailbox *mum;

  mum = scm_must_malloc (sizeof (struct mu_mailbox), "mailbox");
  mum->mbox = mbox;
  SCM_RETURN_NEWSMOB (mailbox_tag, mum);
}

int
mu_scm_is_mailbox (SCM scm)
{
  return SCM_NIMP (scm) && (long) SCM_CAR (scm) == mailbox_tag;
}

/* ************************************************************************* */
/* Guile primitives */

SCM_DEFINE (scm_mu_mail_directory, "mu-mail-directory", 0, 1, 0,
	    (SCM URL), 
            "")
#define FUNC_NAME s_scm_mu_mail_directory
{
  if (!SCM_UNBNDP (URL))
    {
      SCM_ASSERT (SCM_NIMP (URL) && SCM_STRINGP (URL),
		  URL, SCM_ARG1, FUNC_NAME);
      mu_set_mail_directory (SCM_STRING_CHARS (URL));
    }
  return scm_makfrom0str (mu_mail_directory ());
}
#undef FUNC_NAME 

SCM_DEFINE (scm_mu_folder_directory, "mu-folder-directory", 0, 1, 0,
	    (SCM URL), 
            "")
#define FUNC_NAME s_scm_mu_folder_directory
{
  if (!SCM_UNBNDP (URL))
    {
      SCM_ASSERT (SCM_NIMP (URL) && SCM_STRINGP (URL),
		  URL, SCM_ARG1, FUNC_NAME);
      mu_set_folder_directory (SCM_STRING_CHARS (URL));
    }
  return scm_makfrom0str (mu_folder_directory ());
}
#undef FUNC_NAME 

SCM_DEFINE (scm_mu_mailbox_open, "mu-mailbox-open", 2, 0, 0,
	    (SCM URL, SCM MODE), 
            "Opens a mailbox specified by URL.")
#define FUNC_NAME s_scm_mu_mailbox_open
{
  mailbox_t mbox = NULL;
  char *mode_str;
  int mode = 0;

  SCM_ASSERT (SCM_NIMP (URL) && SCM_STRINGP (URL), URL, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (SCM_NIMP (MODE) && SCM_STRINGP (MODE),
	      MODE, SCM_ARG2, FUNC_NAME);

  for (mode_str = SCM_STRING_CHARS (MODE); *mode_str; mode_str++)
    switch (*mode_str)
      {
      case 'r':
	mode |= MU_STREAM_READ;
	break;
      case 'w':
	mode |= MU_STREAM_WRITE;
	break;
      case 'a':
	mode |= MU_STREAM_APPEND;
	break;
      case 'c':
	mode |= MU_STREAM_CREAT;
	break;
      }

  if (mode & MU_STREAM_READ && mode & MU_STREAM_WRITE)
    mode = (mode & ~(MU_STREAM_READ | MU_STREAM_WRITE)) | MU_STREAM_RDWR;

  if (mu_mailbox_create_default (&mbox, SCM_STRING_CHARS (URL)) != 0)
    return SCM_BOOL_F;

  if (mu_mailbox_open (mbox, mode) != 0)
    {
      mu_mailbox_destroy (&mbox);
      return SCM_BOOL_F;
    }

  return mu_scm_mailbox_create (mbox);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_close, "mu-mailbox-close", 1, 0, 0,
	    (SCM MBOX), "Closes mailbox MBOX")
#define FUNC_NAME s_scm_mu_mailbox_close
{
  struct mu_mailbox *mum;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  mu_mailbox_close (mum->mbox);
  mu_mailbox_destroy (&mum->mbox);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_get_url, "mu-mailbox-get-url", 1, 0, 0,
	    (SCM MBOX), 
            "Returns the URL of the mailbox.")
#define FUNC_NAME s_scm_mu_mailbox_get_url
{
  struct mu_mailbox *mum;
  url_t url;
  
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  mu_mailbox_get_url (mum->mbox, &url);
  return scm_makfrom0str (url_to_string (url));
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_get_port, "mu-mailbox-get-port", 2, 0, 0,
	    (SCM MBOX, SCM MODE),
	    "Returns a port associated with the contents of the MBOX.\n"
	    "MODE is a string defining operation mode of the stream. It may\n"
	    "contain any of the two characters: \"r\" for reading, \"w\" for\n"
	    "writing.\n")
#define FUNC_NAME s_scm_mu_mailbox_get_port
{
  struct mu_mailbox *mum;
  stream_t stream;
  
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (SCM_NIMP (MODE) && SCM_STRINGP (MODE),
	      MODE, SCM_ARG2, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  if (mu_mailbox_get_stream (mum->mbox, &stream))
    return SCM_BOOL_F;
  return mu_port_make_from_stream (MBOX, stream,
				   scm_mode_bits (SCM_STRING_CHARS (MODE)));    
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_get_message, "mu-mailbox-get-message", 2, 0, 0,
	    (SCM MBOX, SCM MSGNO), "Retrieve from MBOX message # MSGNO.")
#define FUNC_NAME s_scm_mu_mailbox_get_message
{
  size_t msgno;
  struct mu_mailbox *mum;
  message_t msg;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT ((SCM_IMP (MSGNO) && SCM_INUMP (MSGNO)),
	      MSGNO, SCM_ARG2, FUNC_NAME);

  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  msgno = SCM_INUM (MSGNO);

  if (mu_mailbox_get_message (mum->mbox, msgno, &msg))
    return SCM_BOOL_F;

  return mu_scm_message_create (MBOX, msg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_messages_count, "mu-mailbox-messages-count", 1, 0, 0,
	    (SCM MBOX), "Returns number of messages in the mailbox.")
#define FUNC_NAME s_scm_mu_mailbox_messages_count
{
  struct mu_mailbox *mum;
  size_t nmesg;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);

  if (mu_mailbox_messages_count (mum->mbox, &nmesg))
    return SCM_BOOL_F;
  return mu_scm_makenum (nmesg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_expunge, "mu-mailbox-expunge", 1, 0, 0,
	    (SCM MBOX), "Expunges deleted messages from the mailbox.")
#define FUNC_NAME s_scm_mu_mailbox_expunge
{
  struct mu_mailbox *mum;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  if (mu_mailbox_expunge (mum->mbox))
    return SCM_BOOL_F;
  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_url, "mu-mailbox-url", 1, 0, 0,
	    (SCM MBOX), "Returns the URL of the mailbox")
#define FUNC_NAME s_scm_mu_mailbox_url
{
  struct mu_mailbox *mum;
  url_t url;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  mu_mailbox_get_url (mum->mbox, &url);
  return scm_makfrom0str (url_to_string (url));
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_append_message, "mu-mailbox-append-message", 2, 0, 0,
	    (SCM MBOX, SCM MESG), "Appends the message to the mailbox")
#define FUNC_NAME s_scm_mu_mailbox_append_message
{
  struct mu_mailbox *mum;
  message_t msg;

  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  msg = mu_scm_message_get (MESG);
  if (mu_mailbox_append_message (mum->mbox, msg))
    return SCM_BOOL_F;
  return SCM_BOOL_T;
}
#undef FUNC_NAME

/* Initialize the module */
void
mu_scm_mailbox_init ()
{
  mailbox_tag = scm_make_smob_type ("mailbox", sizeof (struct mu_mailbox));
  scm_set_smob_mark (mailbox_tag, mu_scm_mailbox_mark);
  scm_set_smob_free (mailbox_tag, mu_scm_mailbox_free);
  scm_set_smob_print (mailbox_tag, mu_scm_mailbox_print);

#include "mu_mailbox.x"

}
