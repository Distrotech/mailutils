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

static scm_t_bits mime_tag;

struct mu_mime
{
  mu_mime_t mime;
  SCM owner;
};

/* SMOB functions: */

static SCM
mu_scm_mime_mark (SCM mime_smob)
{
  struct mu_mime *mum = (struct mu_mime *) SCM_CDR (mime_smob);
  return mum->owner;
}

static scm_sizet
mu_scm_mime_free (SCM mime_smob)
{
  struct mu_mime *mum = (struct mu_mime *) SCM_CDR (mime_smob);
  mu_mime_destroy (&mum->mime);
  free (mum);
  return sizeof (struct mu_mime);
}

static int
mu_scm_mime_print (SCM mime_smob, SCM port, scm_print_state * pstate)
{
  struct mu_mime *mum = (struct mu_mime *) SCM_CDR (mime_smob);
  size_t nparts = 0;
  
  mu_mime_get_num_parts (mum->mime, &nparts);
  
  scm_puts ("#<mime ", port);
  scm_intprint (nparts, 10, port);
  scm_putc ('>', port);
  
  return 1;
}

/* Internal calls: */

SCM
mu_scm_mime_create (SCM owner, mu_mime_t mime)
{
  struct mu_mime *mum;

  mum = scm_gc_malloc (sizeof (struct mu_mime), "mime");
  mum->owner = owner;
  mum->mime = mime;
  SCM_RETURN_NEWSMOB (mime_tag, mum);
}

mu_mime_t
mu_scm_mime_get (SCM MIME)
{
  struct mu_mime *mum = (struct mu_mime *) SCM_CDR (MIME);
  return mum->mime;
}

int
mu_scm_is_mime (SCM scm)
{
  return SCM_NIMP (scm) && (long) SCM_CAR (scm) == mime_tag;
}

/* ************************************************************************* */
/* Guile primitives */

SCM_DEFINE_PUBLIC (scm_mu_mime_create, "mu-mime-create", 0, 2, 0,
	    (SCM FLAGS, SCM MESG),
"Creates a new @acronym{MIME} object.  Both arguments are optional.\n"
"FLAGS specifies the type of the object to create (@samp{0} is a reasonable\n"
"value).  MESG gives the message to create the @acronym{MIME} object from.")
#define FUNC_NAME s_scm_mu_mime_create
{
  mu_message_t msg = NULL;
  mu_mime_t mime;
  int flags;
  int status;
  
  if (scm_is_bool (FLAGS))
    {
      /*if (FLAGS == SCM_BOOL_F)*/
      flags = 0;
    }
  else
    {
      SCM_ASSERT (scm_is_integer (FLAGS), FLAGS, SCM_ARG1, FUNC_NAME);
      flags = scm_to_int32 (FLAGS);
    }
  
  if (!SCM_UNBNDP (MESG))
    {
      SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
      msg = mu_scm_message_get (MESG);
    }
  
  status = mu_mime_create (&mime, msg, flags);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot create MIME object", SCM_BOOL_F);
  
  return mu_scm_mime_create (MESG, mime);
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_mime_multipart_p, "mu-mime-multipart?", 1, 0, 0,
	    (SCM MIME),
"Returns @code{#t} if MIME is a multipart object.\n")
#define FUNC_NAME s_scm_mu_mime_multipart_p
{
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  return mu_mime_is_multipart (mu_scm_mime_get (MIME)) ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_mime_get_num_parts, "mu-mime-get-num-parts", 1, 0, 0,
	    (SCM MIME),
"Returns number of parts in the @sc{mime} object MIME.")
#define FUNC_NAME s_scm_mu_mime_get_num_parts
{
  mu_mime_t mime;
  size_t nparts;
  int status;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  status = mu_mime_get_num_parts (mime, &nparts);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot count MIME parts", SCM_BOOL_F);
  return scm_from_size_t (nparts);
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_mime_get_part, "mu-mime-get-part", 2, 0, 0,
	    (SCM MIME, SCM NUM),
	    "Returns NUMth part from the @sc{mime} object MIME.")
#define FUNC_NAME s_scm_mu_mime_get_part
{
  mu_message_t msg = NULL;
  int status;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_integer (NUM), NUM, SCM_ARG2, FUNC_NAME);
  
  status = mu_mime_get_part (mu_scm_mime_get (MIME),
			     scm_to_int32 (NUM), &msg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get part ~A from MIME object ~A",
		  scm_list_2 (NUM, MIME));
  
  return mu_scm_message_create (MIME, msg);
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_mime_add_part, "mu-mime-add-part", 2, 0, 0,
	    (SCM MIME, SCM MESG),
	    "Adds MESG to the @sc{mime} object MIME.")
#define FUNC_NAME s_scm_mu_mime_add_part
{
  mu_mime_t mime;
  mu_message_t msg;
  int status;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  msg = mu_scm_message_get (MESG);

  status = mu_mime_add_part (mime, msg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot add new part to MIME object ~A",
		  scm_list_1 (MIME));
  
  mu_scm_message_add_owner (MESG, MIME);
  
  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_mime_get_message, "mu-mime-get-message", 1, 0, 0,
	    (SCM MIME),
	    "Converts @sc{mime} object MIME to a message.\n")
#define FUNC_NAME s_scm_mu_mime_get_message
{
  mu_mime_t mime;
  mu_message_t msg;
  int status;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  status = mu_mime_get_message (mime, &msg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message from MIME object ~A",
		  scm_list_1 (MIME));

  return mu_scm_message_create (MIME, msg);
}
#undef FUNC_NAME

  
/* Initialize the module */

void
mu_scm_mime_init ()
{
  mime_tag = scm_make_smob_type ("mime", sizeof (struct mu_mime));
  scm_set_smob_mark (mime_tag, mu_scm_mime_mark);
  scm_set_smob_free (mime_tag, mu_scm_mime_free);
  scm_set_smob_print (mime_tag, mu_scm_mime_print);

#include "mu_mime.x"

}

