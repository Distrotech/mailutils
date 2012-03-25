/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/assoc.h>
#include <mailutils/list.h>
#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/header.h>
#include <mailutils/sys/message.h>
#include <mailutils/errno.h>
#include <mailutils/debug.h>
#include <mailutils/nls.h>
#include <mailutils/cstr.h>
#include <mailutils/body.h>

void
mu_list_free_bodystructure (void *item)
{
  mu_bodystructure_free (item);
}

void
mu_bodystructure_free (struct mu_bodystructure *bs)
{
  if (!bs)
    return;
  free (bs->body_type);
  free (bs->body_subtype);
  mu_assoc_destroy (&bs->body_param);
  free (bs->body_id);
  free (bs->body_descr);
  free (bs->body_encoding);
  free (bs->body_md5);
  free (bs->body_disposition);
  mu_assoc_destroy (&bs->body_disp_param);
  free (bs->body_language);
  free (bs->body_location);
  switch (bs->body_message_type)
    {
    case mu_message_other:
    case mu_message_text:
      break;
      
    case mu_message_rfc822:
      mu_message_imapenvelope_free (bs->v.rfc822.body_env);
      mu_bodystructure_free (bs->v.rfc822.body_struct);
      break;
      
    case mu_message_multipart:
      mu_list_destroy (&bs->v.multipart.body_parts);
    }

  free (bs);
}

static int bodystructure_fill (mu_message_t msg,
			       struct mu_bodystructure *bs);

static int
bodystructure_init (mu_message_t msg, struct mu_bodystructure **pbs)
{
  int rc;
  struct mu_bodystructure *bs = calloc (1, sizeof (*bs));
  if (!bs)
    return ENOMEM;
  rc = bodystructure_fill (msg, bs);
  if (rc)
    mu_bodystructure_free (bs);
  else
    *pbs = bs;
  return rc;
}

static int
bodystructure_fill (mu_message_t msg, struct mu_bodystructure *bs)
{
  mu_header_t header = NULL;
  const char *buffer = NULL;
  mu_body_t body = NULL;
  int rc;
  int is_multipart = 0;

  rc = mu_message_get_header (msg, &header);
  if (rc)
    return rc;
  
  if (mu_header_sget_value (header, MU_HEADER_CONTENT_TYPE, &buffer) == 0)
    {
      char *value;
      char *p;
      size_t len;
      
      rc = mu_mime_header_parse (buffer, "UTF-8", &value, &bs->body_param);
      if (rc)
	return rc;

      len = strcspn (value, "/");

      if (mu_c_strcasecmp (value, "MESSAGE/RFC822") == 0)
        bs->body_message_type = mu_message_rfc822;
      else if (mu_c_strncasecmp (value, "TEXT", len) == 0)
        bs->body_message_type = mu_message_text;

      p = malloc (len + 1);
      if (!p)
	return ENOMEM;
      memcpy (p, value, len);
      p[len] = 0;
      
      bs->body_type = p;
      mu_strupper (bs->body_type);
      if (value[len])
	{
	  bs->body_subtype = strdup (value + len + 1);
	  if (!bs->body_subtype)
	    return ENOMEM;
	  mu_strupper (bs->body_subtype);
	}
      
      /* body parameter parenthesized list: Content-type attributes */

      rc = mu_message_is_multipart (msg, &is_multipart);
      if (rc)
	return rc;
      if (is_multipart)
	bs->body_message_type = mu_message_multipart;
    }
  else
    {
      struct mu_mime_param param;
      
      /* Default? If Content-Type is not present consider as text/plain.  */
      bs->body_type = strdup ("TEXT");
      if (!bs->body_type)
	return ENOMEM;
      bs->body_subtype = strdup ("PLAIN");
      if (!bs->body_subtype)
        {
          free (bs->body_type);
	  return ENOMEM;
	}
      rc = mu_mime_param_assoc_create (&bs->body_param);
      if (rc)
	return rc;
      memset (&param, 0, sizeof (param));
      param.value = strdup ("US-ASCII");
      if (!param.value)
        {
          free (bs->body_type);
          free (bs->body_subtype);
          return ENOMEM;
        }
      rc = mu_assoc_install (bs->body_param, "CHARSET", &param);
      if (rc)
	{
	  free (param.value);
	  return rc;
	}
      bs->body_message_type = mu_message_text;
    }

  if (is_multipart)
    {
      size_t i, nparts;

      rc = mu_message_get_num_parts (msg, &nparts);
      if (rc)
	return rc;

      rc = mu_list_create (&bs->v.multipart.body_parts);
      if (rc)
	return rc;

      mu_list_set_destroy_item (bs->v.multipart.body_parts,
				mu_list_free_bodystructure);
      
      for (i = 1; i <= nparts; i++)
        {
          mu_message_t partmsg;
	  struct mu_bodystructure *partbs;

	  rc = mu_message_get_part (msg, i, &partmsg);
	  if (rc)
	    return rc;

	  rc = bodystructure_init (partmsg, &partbs);
	  if (rc)
	    return rc;

	  rc = mu_list_append (bs->v.multipart.body_parts, partbs);
	  if (rc)
	    {
	      mu_bodystructure_free (partbs);
	      return rc;
	    }
	}
    }
  else
    {
      /* body id: Content-ID. */
      rc = mu_header_aget_value_unfold (header, MU_HEADER_CONTENT_ID,
					&bs->body_id);
      if (rc && rc != MU_ERR_NOENT)
	return rc;
      /* body description: Content-Description. */
      rc = mu_header_aget_value_unfold (header, MU_HEADER_CONTENT_DESCRIPTION,
					&bs->body_descr);
      if (rc && rc != MU_ERR_NOENT)
	return rc;
      
      /* body encoding: Content-Transfer-Encoding. */
      rc = mu_header_aget_value_unfold (header,
					MU_HEADER_CONTENT_TRANSFER_ENCODING,
					&bs->body_encoding);
      if (rc == MU_ERR_NOENT)
	{
	  bs->body_encoding = strdup ("7BIT");
	  if (!bs->body_encoding)
	    return ENOMEM;
	}
      else if (rc)
	return rc;

      /* body size RFC822 format.  */
      rc = mu_message_get_body (msg, &body);
      if (rc)
	return rc;
      rc = mu_body_size (body, &bs->body_size);
      if (rc)
	return rc;
      
      /* If the mime type was text.  */
      if (bs->body_message_type == mu_message_text)
	{
	  rc = mu_body_lines (body, &bs->v.text.body_lines);
	  if (rc)
	    return rc;
	}
      else if (bs->body_message_type == mu_message_rfc822)
	{
	  mu_message_t emsg = NULL;

	  /* Add envelope structure of the encapsulated message.  */
	  rc = mu_message_unencapsulate  (msg, &emsg, NULL);
	  if (rc)
	    return rc;
	  rc = mu_message_get_imapenvelope (emsg, &bs->v.rfc822.body_env);
	  if (rc)
	    return rc;
	  /* Add body structure of the encapsulated message.  */
	  rc = bodystructure_init (emsg, &bs->v.rfc822.body_struct);
	  if (rc)
	    return rc;
	  /* Size in text lines of the encapsulated message.  */
	  rc = mu_message_lines (emsg, &bs->v.rfc822.body_lines);
	  mu_message_destroy (&emsg, NULL);
	}
    }
  
  /* body MD5: Content-MD5.  */
  rc = mu_header_aget_value_unfold (header, MU_HEADER_CONTENT_MD5,
				    &bs->body_md5);
  if (rc && rc != MU_ERR_NOENT)
    return rc;
  
  /* body disposition: Content-Disposition.  */
  rc = mu_header_sget_value (header, MU_HEADER_CONTENT_DISPOSITION,
			     &buffer);
  if (rc == 0)
    {
      rc = mu_mime_header_parse (buffer, "UTF-8", &bs->body_disposition,
				 &bs->body_disp_param);
      if (rc)
	return rc;
    }
  else if (rc != MU_ERR_NOENT)
    return rc;
  /* body language: Content-Language.  */
  rc = mu_header_aget_value_unfold (header, MU_HEADER_CONTENT_LANGUAGE,
				    &bs->body_language);
  if (rc && rc != MU_ERR_NOENT)
    return rc;
  rc = mu_header_aget_value_unfold (header, MU_HEADER_CONTENT_LOCATION,
				    &bs->body_location);
  if (rc && rc != MU_ERR_NOENT)
    return rc;

  return 0;
}

int
mu_message_get_bodystructure (mu_message_t msg,
			      struct mu_bodystructure **pbs)
{
  if (msg == NULL)
    return EINVAL;
  if (pbs == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (msg->_bodystructure)
    return msg->_bodystructure (msg, pbs);
  return bodystructure_init (msg, pbs);
}

int
mu_message_set_bodystructure (mu_message_t msg,
      int (*_bodystructure) (mu_message_t, struct mu_bodystructure **),
      void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_bodystructure = _bodystructure;
  return 0;
}
