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

long mime_tag;

struct mu_mime
{
  mime_t mime;
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
  mime_destroy (&mum->mime);
  free (mum);
  return sizeof (struct mu_mime);
}

static int
mu_scm_mime_print (SCM mime_smob, SCM port, scm_print_state * pstate)
{
  struct mu_mime *mum = (struct mu_mime *) SCM_CDR (mime_smob);
  size_t nparts = 0;
  
  mime_get_num_parts (mum->mime, &nparts);
  
  scm_puts ("#<mime ", port);
  scm_intprint (nparts, 10, port);
  scm_putc ('>', port);
  
  return 1;
}

/* Internal calls: */

SCM
mu_scm_mime_create (SCM owner, mime_t mime)
{
  struct mu_mime *mum;

  mum = scm_must_malloc (sizeof (struct mu_mime), "mime");
  mum->owner = owner;
  mum->mime = mime;
  SCM_RETURN_NEWSMOB (mime_tag, mum);
}

const mime_t
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

SCM_DEFINE (scm_mu_mime_create, "mu-mime-create", 0, 2, 0,
	    (SCM FLAGS, SCM MESG),
	    "Creates a new MIME object.")
#define FUNC_NAME s_scm_mu_mime_create
{
  message_t msg = NULL;
  mime_t mime;
  int flags;

  if (SCM_IMP (FLAGS) && SCM_BOOLP (FLAGS))
    {
      /*if (FLAGS == SCM_BOOL_F)*/
      flags = 0;
    }
  else
    {
      SCM_ASSERT (SCM_IMP (FLAGS) && SCM_INUMP (FLAGS),
		  FLAGS, SCM_ARG1, FUNC_NAME);
      flags = SCM_INUM (FLAGS);
    }
  
  if (!SCM_UNBNDP (MESG))
    {
      SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
      msg = mu_scm_message_get (MESG);
    }
  
  if (mime_create (&mime, msg, flags))
    return SCM_BOOL_F;
  
  return mu_scm_mime_create (MESG, mime);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mime_multipart_p, "mu-mime-multipart?", 1, 0, 0,
	    (SCM MIME),
	    "Returns #t if MIME is a multipart object.\n")
#define FUNC_NAME s_scm_mu_mime_multipart_p
{
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  return mime_is_multipart (mu_scm_mime_get (MIME)) ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mime_get_num_parts, "mu-mime-get-num-parts", 1, 0, 0,
	    (SCM MIME),
	    "Returns number of parts in a MIME object.")
#define FUNC_NAME s_scm_mu_mime_get_num_parts
{
  mime_t mime;
  size_t nparts;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  if (mime_get_num_parts (mime, &nparts))
    return SCM_BOOL_F;
  return mu_scm_makenum (nparts);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mime_get_part, "mu-mime-get-part", 2, 0, 0,
	    (SCM MIME, SCM PART),
	    "Returns part number PART from a MIME object.")
#define FUNC_NAME s_scm_mu_mime_get_part
{
  message_t msg = NULL;

  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (SCM_IMP (PART) && SCM_INUMP (PART),
	      PART, SCM_ARG2, FUNC_NAME);
  
  if (mime_get_part (mu_scm_mime_get (MIME), SCM_INUM (PART), &msg))
    return SCM_BOOL_F;
  return mu_scm_message_create (MIME, msg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mime_add_part, "mu-mime-add-part", 2, 0, 0,
	    (SCM MIME, SCM MESG),
	    "Adds MESG to the MIME object.")
#define FUNC_NAME s_scm_mu_mime_add_part
{
  mime_t mime;
  message_t msg;

  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG2, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  msg = mu_scm_message_get (MESG);

  if (mime_add_part (mime, msg))
    return SCM_BOOL_F;

  mu_scm_message_add_owner (MESG, MIME);
  
  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_mime_get_message, "mu-mime-get-message", 1, 0, 0,
	    (SCM MIME),
	    "Converts MIME object to a message.\n")
#define FUNC_NAME s_scm_mu_mime_get_message
{
  mime_t mime;
  message_t msg;
  
  SCM_ASSERT (mu_scm_is_mime (MIME), MIME, SCM_ARG1, FUNC_NAME);
  mime = mu_scm_mime_get (MIME);
  if (mime_get_message (mime, &msg))
    return SCM_BOOL_F;
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

