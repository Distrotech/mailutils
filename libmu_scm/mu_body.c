/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2010-2012 Free Software
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#include "mu_scm.h"

static scm_t_bits body_tag;

struct mu_body
{
  mu_body_t body;             /* Message body */
  mu_stream_t stream;         /* Associated stream */
  char *buffer;               /* I/O buffer */
  size_t bufsize;             /* Size of allocated buffer */
  SCM msg;                    /* Message the body belongs to */		
};

/* Initial buffer size */
#define BUF_SIZE 64

/* SMOB functions: */
static SCM
mu_scm_body_mark (SCM body_smob)
{
  struct mu_body *mbp = (struct mu_body *) SCM_CDR (body_smob);
  return mbp->msg;
}

static scm_sizet
mu_scm_body_free (SCM body_smob)
{
  struct mu_body *mbp = (struct mu_body *) SCM_CDR (body_smob);
  if (mbp->buffer)
    free (mbp->buffer);
  mu_stream_unref (mbp->stream);
  free (mbp);
  return 0;
}

static int
mu_scm_body_print (SCM body_smob, SCM port, scm_print_state * pstate)
{
  struct mu_body *mbp = (struct mu_body *) SCM_CDR (body_smob);
  size_t b_size = 0, b_lines = 0;
  char buffer[512];

  mu_body_size (mbp->body, &b_size);
  mu_body_lines (mbp->body, &b_lines);

  scm_puts ("#<body ", port);
  snprintf (buffer, sizeof (buffer), "%3lu %-5lu",
	    (unsigned long) b_lines, (unsigned long) b_size);
  scm_puts (buffer, port);

  scm_puts (">", port);
  return 1;
}

/* Internal functions: */

int
mu_scm_is_body (SCM scm)
{
  return SCM_NIMP (scm) && (long) SCM_CAR (scm) == body_tag;
}

SCM
mu_scm_body_create (SCM msg, mu_body_t body)
{
  struct mu_body *mbp;

  mbp = scm_gc_malloc (sizeof (struct mu_body), "body");
  mbp->msg = msg;
  mbp->body = body;
  mbp->stream = NULL;
  mbp->buffer = NULL;
  mbp->bufsize = 0;
  SCM_RETURN_NEWSMOB (body_tag, mbp);
}

/* ************************************************************************* */
/* Guile primitives */

SCM_DEFINE_PUBLIC (scm_mu_body_p, "mu-body?", 1, 0, 0,
		   (SCM scm),
"Return @code{true} if @var{scm} is a Mailutils message body object.\n")
#define FUNC_NAME s_scm_mu_body_p
{
  return scm_from_bool (mu_scm_is_body (scm));
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_body_read_line, "mu-body-read-line", 1, 0, 0,
	    (SCM body), 
	    "Read next line from the @var{body}.")
#define FUNC_NAME s_scm_mu_body_read_line
{
  struct mu_body *mbp;
  size_t nread;
  int status;
  
  SCM_ASSERT (mu_scm_is_body (body), body, SCM_ARG1, FUNC_NAME);
  mbp = (struct mu_body *) SCM_CDR (body);

  if (!mbp->stream)
    {
      status = mu_body_get_streamref (mbp->body, &mbp->stream);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot get body stream",
		      SCM_BOOL_F);
    }

  if (!mbp->buffer)
    {
      mbp->bufsize = BUF_SIZE;
      mbp->buffer = malloc (mbp->bufsize);
      if (!mbp->buffer)
	mu_scm_error (FUNC_NAME, ENOMEM, "Cannot allocate memory", SCM_BOOL_F);
    }

  status = mu_stream_getline (mbp->stream, (char**)&mbp->buffer, &mbp->bufsize,
			      &nread);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Error reading from stream", SCM_BOOL_F);

  if (nread == 0)
    return SCM_EOF_VAL;

  return scm_from_locale_string (mbp->buffer);
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_body_write, "mu-body-write", 2, 0, 0,
	    (SCM body, SCM text),
	    "Append @var{text} to message @var{body}.")
#define FUNC_NAME s_scm_mu_body_write
{
  char *ptr;
  size_t len;
  struct mu_body *mbp;
  int status;
  
  SCM_ASSERT (mu_scm_is_body (body), body, SCM_ARG1, FUNC_NAME);
  mbp = (struct mu_body *) SCM_CDR (body);
  SCM_ASSERT (scm_is_string (text), text, SCM_ARG2, FUNC_NAME);
  
  if (!mbp->stream)
    {
      status = mu_body_get_streamref (mbp->body, &mbp->stream);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot get body stream", SCM_BOOL_F);
    }

  ptr = scm_to_locale_string (text);
  len = strlen (ptr);
  status = mu_stream_write (mbp->stream, ptr, len, NULL);
  free (ptr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Error writing to stream", SCM_BOOL_F);
  return SCM_BOOL_T;
}
#undef FUNC_NAME

/* Initialize the module */
void
mu_scm_body_init ()
{
  body_tag = scm_make_smob_type ("body", sizeof (struct mu_body));
  scm_set_smob_mark (body_tag, mu_scm_body_mark);
  scm_set_smob_free (body_tag, mu_scm_body_free);
  scm_set_smob_print (body_tag, mu_scm_body_print);

#include "mu_body.x"

}
