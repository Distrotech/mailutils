/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2006, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

static scm_t_bits mailbox_tag;

/* NOTE: Maybe will have to add some more members. That's why it is a
   struct, not just a typedef mu_mailbox_t */
struct mu_mailbox
{
  mu_mailbox_t mbox;       /* Mailbox */
  int noclose;
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
  if (!mum->noclose)
    {
      mu_mailbox_close (mum->mbox);
      mu_mailbox_destroy (&mum->mbox);
    }
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
  mu_url_t url = NULL;

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
      const char *p = mu_url_to_string (url);
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

/* There are two C interfaces for creating mailboxes in Scheme.
   The first one, mu_scm_mailbox_create0, allows to set `noclose'
   bit, which disables closing and releasing the underlying mu_mailbox_t
   after the hosting SCM object is freed. Use this, if this mailbox
   is referenced elsewhere.

   Another one, mu_scm_mailbox_create, always create an object that
   will cause closing the mu_mailbox_t object and releasing its memory
   after the hosting SCM object is swept away by GC. This is the only
   official one.

   The mu_scm_mailbox_create0 function is a kludge, needed because
   mu_mailbox_t objects don't have reference counters. When it is fixed in
   the library, the interface will be removed. */

SCM
mu_scm_mailbox_create0 (mu_mailbox_t mbox, int noclose)
{
  struct mu_mailbox *mum;

  mum = scm_gc_malloc (sizeof (struct mu_mailbox), "mailbox");
  mum->mbox = mbox;
  mum->noclose = noclose;
  SCM_RETURN_NEWSMOB (mailbox_tag, mum);
}

SCM
mu_scm_mailbox_create (mu_mailbox_t mbox)
{
  return mu_scm_mailbox_create0 (mbox, 0);
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
"Do not use this function. Use mu-user-mailbox-url instead.")
#define FUNC_NAME s_scm_mu_mail_directory
{
  mu_scm_error (FUNC_NAME, ENOSYS,
		"This function is deprecated. Use mu-user-mailbox-url instead.",
		  scm_list_1 (URL));
  return SCM_EOL;
}
#undef FUNC_NAME 

SCM_DEFINE (scm_mu_user_mailbox_url, "mu-user-mailbox-url", 1, 0, 0, 
	    (SCM USER),
	    "")
#define FUNC_NAME s_scm_mu_user_mailbox_url
{
  int rc;
  char *p, *str;
  SCM ret;
  
  SCM_ASSERT (scm_is_string (USER), USER, SCM_ARG1, FUNC_NAME);
  str = scm_to_locale_string (USER);
  rc = mu_construct_user_mailbox_url (&p, str);
  free (str);
  if (rc)
    mu_scm_error (FUNC_NAME, rc,
		  "Cannot construct mailbox URL for ~A",
		  scm_list_1 (USER));
  ret = scm_from_locale_string (p);
  free (p);
  return ret;
}
#undef FUNC_NAME 

SCM_DEFINE (scm_mu_folder_directory, "mu-folder-directory", 0, 1, 0,
	    (SCM URL), 
"If URL is given, sets it as a name of the user's folder directory.\n"
"Returns the current value of the folder directory.")
#define FUNC_NAME s_scm_mu_folder_directory
{
  if (!SCM_UNBNDP (URL))
    {
      char *s;

      SCM_ASSERT (scm_is_string (URL), URL, SCM_ARG1, FUNC_NAME);
      s = scm_to_locale_string (URL);
      mu_set_folder_directory (s);
      free (s);
    }
  return scm_from_locale_string (mu_folder_directory ());
}
#undef FUNC_NAME 

SCM_DEFINE (scm_mu_mailbox_open, "mu-mailbox-open", 2, 0, 0,
	    (SCM URL, SCM MODE), 
"Opens the mailbox specified by URL. MODE is a string, consisting of\n"
"the characters described below, giving the access mode for the mailbox\n"
"\n"
"@multitable @columnfractions 0.20 0.70\n"
"@headitem MODE @tab Meaning\n"
"@item r @tab Open for reading.\n"
"@item w @tab Open for writing.\n"
"@item a @tab Open for appending to the end of the mailbox.\n"
"@item c @tab Create the mailbox if it does not exist.\n"
"@end multitable\n"
)
#define FUNC_NAME s_scm_mu_mailbox_open
{
  mu_mailbox_t mbox = NULL;
  char *mode_str;
  int mode = 0;
  int status;
  SCM ret;
  
  SCM_ASSERT (scm_is_string (URL), URL, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (MODE), MODE, SCM_ARG2, FUNC_NAME);
  
  scm_dynwind_begin (0);
  
  mode_str = scm_to_locale_string (MODE);
  scm_dynwind_free (mode_str);
  for (; *mode_str; mode_str++)
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

  mode_str = scm_to_locale_string (URL);
  scm_dynwind_free (mode_str);
  
  status = mu_mailbox_create_default (&mbox, mode_str);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot create default mailbox ~A",
		  scm_list_1 (URL));


  status = mu_mailbox_open (mbox, mode);
  if (status)
    {
      mu_mailbox_destroy (&mbox);
      mu_scm_error (FUNC_NAME, status,
		    "Cannot open default mailbox ~A",
		    scm_list_1 (URL));
    }
  ret = mu_scm_mailbox_create (mbox);
  scm_dynwind_end ();
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_close, "mu-mailbox-close", 1, 0, 0,
	    (SCM MBOX), "Closes mailbox MBOX.")
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
            "Returns url of the mailbox MBOX.")
#define FUNC_NAME s_scm_mu_mailbox_get_url
{
  struct mu_mailbox *mum;
  mu_url_t url;
  int status;
 
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  status = mu_mailbox_get_url (mum->mbox, &url);
  if (status)
    mu_scm_error (FUNC_NAME, status,
                  "Cannot get mailbox url",
                  SCM_BOOL_F);

  return scm_from_locale_string (mu_url_to_string (url));
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_get_port, "mu-mailbox-get-port", 2, 0, 0,
	    (SCM MBOX, SCM MODE),
"Returns a port associated with the contents of the MBOX.\n"
"MODE is a string defining operation mode of the stream. It may\n"
"contain any of the two characters: @samp{r} for reading, @samp{w} for\n"
"writing.\n")
#define FUNC_NAME s_scm_mu_mailbox_get_port
{
  struct mu_mailbox *mum;
  mu_stream_t stream;
  int status;
  char *s;
  SCM ret;
  
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (MODE), MODE, SCM_ARG2, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  status = mu_mailbox_get_stream (mum->mbox, &stream);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get mailbox stream",
		  scm_list_1 (MBOX));
  s = scm_to_locale_string (MODE);
  ret = mu_port_make_from_stream (MBOX, stream, scm_mode_bits (s));
  free (s);
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_get_message, "mu-mailbox-get-message", 2, 0, 0,
            (SCM MBOX, SCM MSGNO), 
"Retrieve from message #MSGNO from the mailbox MBOX.")
#define FUNC_NAME s_scm_mu_mailbox_get_message
{
  size_t msgno;
  struct mu_mailbox *mum;
  mu_message_t msg;
  int status;
    
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_integer (MSGNO), MSGNO, SCM_ARG2, FUNC_NAME);

  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  msgno = scm_to_int32 (MSGNO);

  status = mu_mailbox_get_message (mum->mbox, msgno, &msg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message ~A from mailbox ~A",
		  scm_list_2 (MSGNO, MBOX));
    
  return mu_scm_message_create (MBOX, msg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_messages_count, "mu-mailbox-messages-count", 1, 0, 0,
	    (SCM MBOX), 
"Returns number of messages in the mailbox MBOX.")
#define FUNC_NAME s_scm_mu_mailbox_messages_count
{
  struct mu_mailbox *mum;
  size_t nmesg;
  int status;
  
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);

  status = mu_mailbox_messages_count (mum->mbox, &nmesg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot count messages in mailbox ~A",
		  scm_list_1 (MBOX));
  return scm_from_size_t (nmesg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_expunge, "mu-mailbox-expunge", 1, 0, 0,
	    (SCM MBOX), 
"Expunges deleted messages from the mailbox MBOX.")
#define FUNC_NAME s_scm_mu_mailbox_expunge
{
  struct mu_mailbox *mum;
  int status;
    
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  status = mu_mailbox_expunge (mum->mbox);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot expunge messages in mailbox ~A",
		  scm_list_1 (MBOX));
  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mailbox_append_message, "mu-mailbox-append-message", 2, 0, 0,
	    (SCM MBOX, SCM MESG),
"Appends message MESG to the mailbox MBOX.")
#define FUNC_NAME s_scm_mu_mailbox_append_message
{
  struct mu_mailbox *mum;
  mu_message_t msg;
  int status;
    
  SCM_ASSERT (mu_scm_is_mailbox (MBOX), MBOX, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
  mum = (struct mu_mailbox *) SCM_CDR (MBOX);
  msg = mu_scm_message_get (MESG);
  status = mu_mailbox_append_message (mum->mbox, msg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot append message ~A to mailbox ~A",
		  scm_list_2 (MESG, MBOX));
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
