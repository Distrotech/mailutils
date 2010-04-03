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

/* See RFC 2045, 5.1.  Syntax of the Content-Type Header Field */
#define _ISSPECIAL(c) !!strchr ("()<>@,;:\\\"/[]?=", c)

/* _header_get_param - an auxiliary function to extract values from
   Content-Type, Content-Disposition and similar headers.

   Arguments:
   
   FIELD_BODY    Header value, complying to RFCs 2045, 2183, 2231.3;
   DISP          Disposition.  Unless it is NULL, the disposition part
                 of FIELD_BODY is compared with it.  If they differ,
		 the function returns MU_ERR_NOENT.
   PARAM         Name of the parameter to extract from FIELD_BODY;
   BUF           Where to extract the value to;
   BUFSZ         Size of BUF;
   PRET          Pointer to the memory location for the return buffer (see
                 below).
   PLEN          Pointer to the return size.

   The function parses FIELD_BODY and extracts the value of the parameter
   PARAM.

   If BUF is not NULL and BUFSZ is not 0, the extracted value is stored into
   BUF.  At most BUFSZ-1 bytes are copied.

   Otherwise, if PRET is not NULL, the function allocates enough memory to
   hold the extracted value, copies there the result, and stores the
   pointer to the allocated memory into the location pointed to by PRET.

   If PLEN is not NULL, the size of the extracted value (without terminating
   NUL character) is stored there.

   If BUF==NULL *and* PRET==NULL, no memory is allocated, but PLEN is
   honored anyway, i.e. unless it is NULL it receives size of the result.
   This can be used to estimate the needed buffer size.

   Return values:
     0             on success.
     MU_ERR_NOENT, requested parameter not found, or disposition does
                   not match DISP.
     MU_ERR_PARSE, if FIELD_BODY does not comply to any of the abovemntioned
                   RFCs.
     ENOMEM      , if unable to allocate memory.
*/
   
int
_header_get_param (char *field_body,
		   const char *disp,
		   const char *param,
		   char *buf, size_t bufsz,
		   char **pret, size_t *plen)
{
  int res = MU_ERR_NOENT;            /* Return value, pessimistic default */
  size_t param_len = strlen (param);
  char *p;
  char *mem = NULL;                  /* Allocated memory storage */
  size_t retlen = 0;                 /* Total number of bytes copied */
  unsigned long cind = 0;            /* Expected continued parameter index.
					See RFC 2231, Section 3,
					"Parameter Value Continuations" */
  
  if (field_body == NULL)
    return EINVAL;

  if (bufsz == 0) /* Make sure buf value is meaningful */
    buf = NULL;
  
  p = strchr (field_body, ';');
  if (!p)
    return MU_ERR_NOENT;
  if (disp && mu_c_strncasecmp (field_body, disp, p - field_body))
    return MU_ERR_NOENT;
      
  while (p && *p)
    {
      char *v, *e;
      size_t len, escaped_chars = 0;

      if (*p != ';')
	{
	  res = MU_ERR_PARSE;
	  break;
	}
      
      /* walk upto start of param */      
      p = mu_str_skip_class (p + 1, MU_CTYPE_SPACE);
      if ((v = strchr (p, '=')) == NULL)
	break;
      v++;
      /* Find end of the parameter */
      if (*v == '"')
	{
	  /* Quoted string */
	  for (e = ++v; *e != '"'; e++)
	    {
	      if (*e == 0) /* Malformed header */
		{
		  res = MU_ERR_PARSE;
		  break;
		}
	      if (*e == '\\')
		{
		  if (*++e == 0)
		    {
		      res = MU_ERR_PARSE;
		      break;
		    }
		  escaped_chars++;
		}
	    }
	  if (res == MU_ERR_PARSE)
	    break;
	  len = e - v;
	  e++;
	}
      else
	{
	  for (e = v + 1; !(_ISSPECIAL (*e) || mu_isspace (*e)); e++)
	    ;
	  len = e - v;
	}

      /* Is it our parameter? */
      if (mu_c_strncasecmp (p, param, param_len))
	{			/* nope, jump to next */
	  p = strchr (e, ';');
	  continue;
	}
	
      res = 0; /* Indicate success */
      
      if (p[param_len] == '*')
	{
	  /* Parameter value continuation (RFC 2231, Section 3).
	     See if the index is OK */
	  char *end;
	  unsigned long n = strtoul (p + param_len + 1, &end, 10);
	  if (*end != '=' || n != cind)
	    {
	      res = MU_ERR_PARSE;
	      break;
	    }
	  /* Everything OK, increase the estimation */
	  cind++; 
	}
      
      /* Prepare P for the next iteration */
      p = e;

      /* Escape characters that appear in quoted-pairs are
	 semantically "invisible" (RFC 2822, Section 3.2.2,
	 "Quoted characters") */
      len -= escaped_chars;

      /* Adjust len if nearing end of the buffer */
      if (bufsz && len >= bufsz)
	len = bufsz - 1;

      if (pret)
	{
	  /* The caller wants us to allocate the memory */
	  if (!buf && !mem)
	    {
	      mem = malloc (len + 1);
	      if (!mem)
		{
		  res = ENOMEM;
		  break;
		}
	      buf = mem;
	    }
	  else if (mem)
	    {
	      /* If we got here, it means we are iterating over
		 a parameter value continuation, and cind=0 has
		 already been passed.  Reallocate the memory to
		 accomodate next chunk of data. */
	      char *newmem = realloc (mem, retlen + len + 1);
	      if (!newmem)
		{
		  res = ENOMEM;
		  break;
		}
	      mem = newmem;
	    }
	}

      if (buf)
	{
	  /* Actually copy the data.  Buf is not NULL either because
	     the user passed it as an argument, or because we allocated
	     memory for it. */
	  if (escaped_chars)
	    {
	      int i;
	      for (i = 0; i < len; i++)
		{
		  if (*v == '\\')
		    ++v;
		  buf[retlen + i] = *v++;
		}
	    }
	  else
	    memcpy (buf + retlen, v, len);
	}
      /* Adjust total result size ... */
      retlen += len;
      /* ... and remaining buffer size, if necessary */
      if (bufsz)
	{
	  bufsz -= len;
	  if (bufsz == 0)
	    break;
	}
    }

  if (res == 0)
    {
      /* Everything OK, prepare the returned data. */
      if (buf)
	buf[retlen] = 0;
      if (plen)
	*plen = retlen;
      if (pret)
	*pret = mem;
    }
  else if (mem)
    free (mem);
  return res;
}

/* Get the attachment name from MSG.  See _header_get_param, for a
   description of the rest of arguments. */
static int
_get_attachment_name (mu_message_t msg, char *buf, size_t bufsz,
		      char **pbuf, size_t *sz)
{
  int ret = EINVAL;
  mu_header_t hdr;
  char *value = NULL;

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
      ret = _header_get_param (value, "attachment",
			       "filename", buf, bufsz, pbuf, sz);
      free (value);
      value = NULL;
      if (ret == 0 || ret != MU_ERR_NOENT)
	return ret;
    }

  /* If we didn't get the name, we fall back on the Content-Type name
     parameter. */

  free (value);
  ret = mu_header_aget_value (hdr, "Content-Type", &value);
  if (ret == 0)
    ret = _header_get_param (value, NULL, "name", buf, bufsz, pbuf, sz);
  free (value);

  return ret;
}

int
mu_message_aget_attachment_name (mu_message_t msg, char **name)
{
  if (name == NULL)
    return MU_ERR_OUT_PTR_NULL;
  return _get_attachment_name (msg, NULL, 0, name, NULL);
}

int
mu_message_get_attachment_name (mu_message_t msg, char *buf, size_t bufsz,
				size_t *sz)
{
  return _get_attachment_name (msg, buf, bufsz, NULL, sz);
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
