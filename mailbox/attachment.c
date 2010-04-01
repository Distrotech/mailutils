/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007, 2009, 2010 Free
   Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/body.h>
#include <mailutils/filter.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/mutil.h>

#define BUF_SIZE	2048

struct _msg_info
{
  char *buf;
  size_t nbytes;
  char *header_buf;
  int header_len;
  int mu_header_size;
  mu_header_t hdr;
  mu_message_t msg;
  int ioffset;
  int ooffset;
  mu_stream_t stream;	/* output file/decoding stream for saving attachment */
  mu_stream_t fstream;	/* output file stream for saving attachment */
};

#define MSG_HDR "Content-Type: %s; name=%s\nContent-Transfer-Encoding: %s\nContent-Disposition: attachment; filename=%s\n\n"

int
mu_message_create_attachment (const char *content_type, const char *encoding,
			      const char *filename, mu_message_t *newmsg)
{
  mu_header_t hdr;
  mu_body_t body;
  mu_stream_t fstream = NULL, tstream = NULL;
  char *header = NULL, *name = NULL, *fname = NULL;
  int ret;

  if (newmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (filename == NULL)
    return EINVAL;

  if ((ret = mu_message_create (newmsg, NULL)) == 0)
    {
      if (content_type == NULL)
	content_type = "text/plain";
      if (encoding == NULL)
	encoding = "7bit";
      if ((fname = strdup (filename)) != NULL)
	{
	  name = strrchr (fname, '/');
	  if (name)
	    name++;
	  else
	    name = fname;
	  if ((header =
	       malloc (strlen (MSG_HDR) + strlen (content_type) +
		       strlen (name) * 2 + strlen (encoding) + 1)) == NULL)
	    ret = ENOMEM;
	  else
	    {
	      sprintf (header, MSG_HDR, content_type, name, encoding, name);
	      if ((ret =
		   mu_header_create (&hdr, header, strlen (header),
				     *newmsg)) == 0)
		{
		  mu_message_get_body (*newmsg, &body);
		  if ((ret =
		       mu_file_stream_create (&fstream, filename,
					      MU_STREAM_READ)) == 0)
		    {
		      if ((ret = mu_stream_open (fstream)) == 0)
			{
			  if ((ret =
			       mu_filter_create (&tstream, fstream, encoding,
						 MU_FILTER_ENCODE,
						 MU_STREAM_READ)) == 0)
			    {
			      mu_body_set_stream (body, tstream, *newmsg);
			      mu_message_set_header (*newmsg, hdr, NULL);
			    }
			}
		    }
		}
	      free (header);
	    }
	}
    }
  if (ret)
    {
      if (*newmsg)
	mu_message_destroy (newmsg, NULL);
      if (hdr)
	mu_header_destroy (&hdr, NULL);
      if (fstream)
	mu_stream_destroy (&fstream, NULL);
      if (fname)
	free (fname);
    }
  return ret;
}


static int
_attachment_setup (struct _msg_info **info, mu_message_t msg,
		   mu_stream_t *stream, void **data)
{
  int sfl, ret;
  mu_body_t body;

  if ((ret = mu_message_get_body (msg, &body)) != 0 ||
      (ret = mu_body_get_stream (body, stream)) != 0)
    return ret;
  mu_stream_get_flags (*stream, &sfl);
  if (data == NULL && (sfl & MU_STREAM_NONBLOCK))
    return EINVAL;
  if (data)
    *info = *data;
  if (*info == NULL)
    {
      if ((*info = calloc (1, sizeof (struct _msg_info))) == NULL)
	return ENOMEM;
    }
  if (((*info)->buf = malloc (BUF_SIZE)) == NULL)
    {
      free (*info);
      return ENOMEM;
    }
  return 0;
}

static void
_attachment_free (struct _msg_info *info, int free_message)
{
  if (info->buf)
    free (info->buf);
  if (info->header_buf)
    free (info->header_buf);
  if (free_message)
    {
      if (info->msg)
	mu_message_destroy (&(info->msg), NULL);
      else if (info->hdr)
	mu_header_destroy (&(info->hdr), NULL);
    }
  free (info);
}

#define _ISSPECIAL(c) ( \
    ((c) == '(') || ((c) == ')') || ((c) == '<') || ((c) == '>') \
    || ((c) == '@') || ((c) == ',') || ((c) == ';') || ((c) == ':') \
    || ((c) == '\\') || ((c) == '.') || ((c) == '[') \
    || ((c) == ']') )

static char *
_header_get_param (char *field_body, const char *param, size_t *len)
{
  char *str, *p, *v, *e;
  int quoted = 0, was_quoted = 0;

  if (len == NULL || (str = field_body) == NULL)
    return NULL;

  p = strchr (str, ';');
  while (p)
    {
      p++;
      while (mu_isspace (*p))	/* walk upto start of param */
	p++;
      if ((v = strchr (p, '=')) == NULL)
	break;
      *len = 0;
      v = e = v + 1;
      while (*e && (quoted || (!_ISSPECIAL (*e) && !mu_isspace (*e))))
	{			/* skip pass value and calc len */
	  if (*e == '\"')
	    quoted = ~quoted, was_quoted = 1;
	  else
	    (*len)++;
	  e++;
	}
      if (mu_c_strncasecmp (p, param, strlen (param)))
	{			/* no match jump to next */
	  p = strchr (e, ';');
	  continue;
	}
      else
	return was_quoted ? v + 1 : v;	/* return unquoted value */
    }
  return NULL;
}

int
mu_message_aget_attachment_name (mu_message_t msg, char **name)
{
  size_t sz = 0;
  int ret = 0;

  if (name == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if ((ret = mu_message_get_attachment_name (msg, NULL, 0, &sz)) != 0)
    return ret;

  *name = malloc (sz + 1);
  if (!*name)
    return ENOMEM;
  
  if ((ret = mu_message_get_attachment_name (msg, *name, sz + 1, NULL)) != 0)
    {
      free (*name);
      *name = NULL;
    }

  return ret;
}

int
mu_message_get_attachment_name (mu_message_t msg, char *buf, size_t bufsz,
				size_t *sz)
{
  int ret = EINVAL;
  mu_header_t hdr;
  char *value = NULL;
  char *name = NULL;
  size_t namesz = 0;

  if (!msg)
    return ret;

  if ((ret = mu_message_get_header (msg, &hdr)) != 0)
    return ret;

  ret = mu_header_aget_value (hdr, "Content-Disposition", &value);

  /* If the header wasn't there, we'll fall back to Content-Type, but
     other errors are fatal. */
  if (ret != 0 && ret != MU_ERR_NOENT)
    return ret;

  if (ret == 0 && value != NULL)
    {
      /* FIXME: this is cheezy, it should check the value of the
         Content-Disposition field, not strstr it. */

      if (strstr (value, "attachment") != NULL)
	name = _header_get_param (value, "filename", &namesz);
    }

  /* If we didn't get the name, we fall back on the Content-Type name
     parameter. */

  if (name == NULL)
    {
      if (value)
	free (value);

      ret = mu_header_aget_value (hdr, "Content-Type", &value);
      name = _header_get_param (value, "name", &namesz);
    }

  if (name)
    {
      ret = 0;

      name[namesz] = '\0';

      if (sz)
	*sz = namesz;

      if (buf)
	strncpy (buf, name, bufsz);
    }
  else
    ret = MU_ERR_NOENT;

  return ret;
}

int
mu_message_save_attachment (mu_message_t msg, const char *filename,
			    void **data)
{
  mu_stream_t istream;
  struct _msg_info *info = NULL;
  int ret;
  size_t size;
  size_t nbytes;
  mu_header_t hdr;
  const char *fname = NULL;
  char *partname = NULL;

  if (msg == NULL)
    return EINVAL;

  if ((ret = _attachment_setup (&info, msg, &istream, data)) != 0)
    return ret;

  if (ret == 0 && (ret = mu_message_get_header (msg, &hdr)) == 0)
    {
      if (filename == NULL)
	{
	  ret = mu_message_aget_attachment_name (msg, &partname);
	  if (partname)
	    fname = partname;
	}
      else
	fname = filename;
      if (fname
	  && (ret =
	      mu_file_stream_create (&info->fstream, fname,
				     MU_STREAM_WRITE | MU_STREAM_CREAT)) == 0)
	{
	  if ((ret = mu_stream_open (info->fstream)) == 0)
	    {
	      char *content_encoding;
	      char *content_encoding_mem = NULL;

	      mu_header_get_value (hdr, "Content-Transfer-Encoding", NULL, 0,
				   &size);
	      if (size)
		{
		  content_encoding_mem = malloc (size + 1);
		  if (content_encoding_mem == NULL)
		    ret = ENOMEM;
		  content_encoding = content_encoding_mem;
		  mu_header_get_value (hdr, "Content-Transfer-Encoding",
				       content_encoding, size + 1, 0);
		}
	      else
		content_encoding = "7bit";
	      ret =
		mu_filter_create (&info->stream, istream, content_encoding,
				  MU_FILTER_DECODE, MU_STREAM_READ);
	      free (content_encoding_mem);
	    }
	}
    }
  if (info->stream && istream)
    {
      if (info->nbytes)
	memmove (info->buf, info->buf + (BUF_SIZE - info->nbytes),
		 info->nbytes);
      while ((ret == 0 && info->nbytes)
	     ||
	     ((ret =
	       mu_stream_read (info->stream, info->buf, BUF_SIZE,
			       info->ioffset, &info->nbytes)) == 0
	      && info->nbytes))
	{
	  info->ioffset += info->nbytes;
	  while (info->nbytes)
	    {
	      if ((ret =
		   mu_stream_write (info->fstream, info->buf, info->nbytes,
				    info->ooffset, &nbytes)) != 0)
		break;
	      info->nbytes -= nbytes;
	      info->ooffset += nbytes;
	    }
	}
    }
  if (ret != EAGAIN && info)
    {
      mu_stream_close (info->fstream);
      mu_stream_destroy (&info->stream, NULL);
      mu_stream_destroy (&info->fstream, NULL);
      _attachment_free (info, ret);
    }

  /* Free fname if we allocated it. */
  if (partname)
    free (partname);

  return ret;
}

int
mu_message_encapsulate (mu_message_t msg, mu_message_t *newmsg, void **data)
{
  mu_stream_t istream, ostream;
  const char *header;
  struct _msg_info *info = NULL;
  int ret = 0;
  size_t nbytes;
  mu_body_t body;

  if (msg == NULL)
    return EINVAL;
  if (newmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if ((ret = _attachment_setup (&info, msg, &ostream, data)) != 0)
    return ret;

  if (info->msg == NULL
      && (ret = mu_message_create (&(info->msg), NULL)) == 0)
    {
      header =
	"Content-Type: message/rfc822\nContent-Transfer-Encoding: 7bit\n\n";
      if ((ret =
	   mu_header_create (&(info->hdr), header, strlen (header),
			     msg)) == 0)
	ret = mu_message_set_header (info->msg, info->hdr, NULL);
    }
  if (ret == 0 && (ret = mu_message_get_stream (msg, &istream)) == 0)
    {
      if ((ret = mu_message_get_body (info->msg, &body)) == 0 &&
	  (ret = mu_body_get_stream (body, &ostream)) == 0)
	{
	  if (info->nbytes)
	    memmove (info->buf, info->buf + (BUF_SIZE - info->nbytes),
		     info->nbytes);
	  while ((ret == 0 && info->nbytes)
		 ||
		 ((ret =
		   mu_stream_read (istream, info->buf, BUF_SIZE,
				   info->ioffset, &info->nbytes)) == 0
		  && info->nbytes))
	    {
	      info->ioffset += info->nbytes;
	      while (info->nbytes)
		{
		  if ((ret =
		       mu_stream_write (ostream, info->buf, info->nbytes,
					info->ooffset, &nbytes)) != 0)
		    break;
		  info->nbytes -= nbytes;
		  info->ooffset += nbytes;
		}
	    }
	}
    }
  if (ret == 0)
    *newmsg = info->msg;
  if (ret != EAGAIN && info)
    _attachment_free (info, ret);
  return ret;
}

int
mu_message_unencapsulate (mu_message_t msg, mu_message_t *newmsg,
			  void **data)
{
  size_t size, nbytes;
  int ret = 0;
  mu_header_t hdr;
  mu_stream_t istream, ostream;
  struct _msg_info *info = NULL;

  if (msg == NULL)
    return EINVAL;
  if (newmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if ((data == NULL || *data == NULL)
      && (ret = mu_message_get_header (msg, &hdr)) == 0)
    {
      mu_header_get_value (hdr, "Content-Type", NULL, 0, &size);
      if (size)
	{
	  char *content_type;
	  if ((content_type = malloc (size + 1)) == NULL)
	    return ENOMEM;
	  mu_header_get_value (hdr, "Content-Type", content_type, size + 1,
			       0);
	  ret =
	    mu_c_strncasecmp (content_type, "message/rfc822",
			      strlen ("message/rfc822"));
	  free (content_type);
	  if (ret != 0)
	    return EINVAL;
	}
      else
	return EINVAL;
    }
  if ((ret = _attachment_setup (&info, msg, &istream, data)) != 0)
    return ret;
  if (info->msg == NULL)
    ret = mu_message_create (&(info->msg), NULL);
  if (ret == 0)
    {
      mu_message_get_stream (info->msg, &ostream);
      if (info->nbytes)
	memmove (info->buf, info->buf + (BUF_SIZE - info->nbytes),
		 info->nbytes);
      while ((ret == 0 && info->nbytes)
	     ||
	     ((ret =
	       mu_stream_read (istream, info->buf, BUF_SIZE, info->ioffset,
			       &info->nbytes)) == 0 && info->nbytes))
	{
	  info->ioffset += info->nbytes;
	  while (info->nbytes)
	    {
	      if ((ret =
		   mu_stream_write (ostream, info->buf, info->nbytes,
				    info->ooffset, &nbytes)) != 0)
		break;
	      info->nbytes -= nbytes;
	      info->ooffset += nbytes;
	    }
	}
    }
  if (ret == 0)
    *newmsg = info->msg;
  if (ret != EAGAIN && info)
    _attachment_free (info, ret);
  return ret;
}
