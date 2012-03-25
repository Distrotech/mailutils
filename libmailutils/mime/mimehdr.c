/* Operations on RFC-2231-compliant mail headers fields.
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2009-2012 Free Software
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
   Public License along with this library.  If not,
   see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/errno.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/stream.h>
#include <mailutils/mime.h>
#include <mailutils/filter.h>
#include <mailutils/util.h>
#include <mailutils/wordsplit.h>
#include <mailutils/assoc.h>
#include <mailutils/iterator.h>
#include <mailutils/diag.h>
#include <mailutils/nls.h>

#define MU_MIMEHDR_MULTILINE 0x01  /* Parameter was multiline */
#define MU_MIMEHDR_CSINFO    0x02  /* Parameter contains charset/language
				      info */

/* Free members of struct mu_mime_param, but do not free it itself. */
static void
_mu_mime_param_free (struct mu_mime_param *p)
{
  free (p->lang);
  free (p->cset);
  free (p->value);
}

/* Treat ITEM as a pointer to struct mu_mime_param and reclaim all
   memory associated with it.

   This is intended for use as a destroy_item method of assoc tables. */
static void
_mu_mime_param_free_item (void *item)
{
  _mu_mime_param_free (item);
}

/* Recode a string between two charsets.
   
   Input:
     TEXT  - A string.
     ICS   - Charset of TEXT.
     OCS   - Charset to convert TEXT to.
   Output:
     PRESULT - On success, the pointer to the resulting string is stored here.
*/
static int
_recode_string (char *text, const char *ics, const char *ocs, char **presult)
{
  mu_stream_t istr, ostr, cvt;
  mu_off_t size;
  char *decoded;
  int rc;
  
  rc = mu_static_memory_stream_create (&istr, text, strlen (text));
  if (rc)
    return rc;
  rc = mu_memory_stream_create (&ostr, 0);
  if (rc)
    return rc;
  rc = mu_decode_filter (&cvt, istr, NULL, ics, ocs);
  mu_stream_unref (istr);
  if (rc)
    {
      mu_stream_unref (ostr);
      return rc;
    }
  rc = mu_stream_copy (ostr, cvt, 0, &size);
  mu_stream_unref (cvt);
  if (rc)
    {
      mu_stream_unref (ostr);
      return rc;
    }

  decoded = malloc (size + 1);
  if (!decoded)
    {
      mu_stream_unref (ostr);
      return ENOMEM;
    }

  mu_stream_seek (ostr, 0, MU_SEEK_SET, NULL);
  rc = mu_stream_read (ostr, decoded, size, NULL);
  mu_stream_unref (ostr);
  if (rc)
    free (decoded);
  else
    {
      decoded[size] = 0;
      *presult = decoded;
    }
  return rc;
}

/* Structure for composing continued parameters.
   See RFC 2231, Section 3, "Parameter Value Continuations" */
struct param_continuation
{
  char *param_name;    /* Parameter name */
  size_t param_length;        /* Length of param_name */
  mu_stream_t param_value;    /* Its value (memory stream) */ 
  int param_cind;             /* Expected continued parameter index. */
  /* Language/character set information */
  const char *param_lang;
  const char *param_cset;
};

static void
free_param_continuation (struct param_continuation *p)
{
  free (p->param_name);
  mu_stream_destroy (&p->param_value);
  /* param_lang and param_cset are handled separately */
  memset (p, 0, sizeof (*p));
}

/* Auxiliary function to store the data collected in CONT into ASSOC.
   If SUBSET is True, ASSOC is populated with empty mu_mime_param
   structures. In this case data will be stored only if CONT->param_name
   is already in ASSOC. If OUTCHARSET is not NULL, the value from
   CONT->param_value will be recoded to that charset before storing it. */
static int
flush_param (struct param_continuation *cont, mu_assoc_t assoc, int subset,
	     const char *outcharset)
{
  int rc;
  struct mu_mime_param param, *param_slot = NULL;
  mu_off_t size;
  
  if (subset)
    {
      param_slot = mu_assoc_ref (assoc, cont->param_name);
      if (!param_slot)
	return 0;
    }
  
  if (cont->param_lang)
    {
      param.lang = strdup (cont->param_lang);
      if (!param.lang)
	return ENOMEM;
    }
  else
    param.lang = NULL;

  if (outcharset || cont->param_cset)
    {
      param.cset = strdup (outcharset ? outcharset : cont->param_cset);
      if (!param.cset)
	{
	  free (param.lang);
	  return ENOMEM;
	}
    }
  
  rc = mu_stream_size (cont->param_value, &size);
  if (rc == 0)
    {
      param.value = malloc (size + 1);
      if (!param.value)
	rc = ENOMEM;
    }

  if (rc == 0)
    {
      rc = mu_stream_seek (cont->param_value, 0, MU_SEEK_SET, NULL);
      if (rc == 0)
	rc = mu_stream_read (cont->param_value, param.value, size, NULL);
      param.value[size] = 0;
    }
  
  if (rc)
    {
      free (param.lang);
      free (param.cset);
      return rc;
    }

  if (cont->param_cset && outcharset &&
      mu_c_strcasecmp (cont->param_cset, outcharset))
    {
      char *tmp;
      rc = _recode_string (param.value, cont->param_cset, outcharset, &tmp);
      free (param.value);
      if (rc)
	{
	  free (param.lang);
	  free (param.cset);
	  return rc;
	}
      param.value = tmp;
    }
	
  if (param_slot)
    {
      *param_slot = param;
    }
  else
    {
      rc = mu_assoc_install (assoc, cont->param_name, &param);
      if (rc)
	_mu_mime_param_free (&param);
    }
  
  return rc;
}

/* Create and initialize an empty associative array for parameters. */
int
mu_mime_param_assoc_create (mu_assoc_t *paramtab)
{
  mu_assoc_t assoc;
  int rc = mu_assoc_create (&assoc, sizeof (struct mu_mime_param),
			    MU_ASSOC_ICASE);
  if (rc == 0)
    mu_assoc_set_free (assoc, _mu_mime_param_free_item);
  *paramtab = assoc;
  return rc;
}

/* Add an empty structure for the slot NAME in ASSOC. */
int
mu_mime_param_assoc_add (mu_assoc_t assoc, const char *name)
{
  struct mu_mime_param param;

  memset (&param, 0, sizeof param);
  return mu_assoc_install (assoc, name, &param);
}

/* See FIXME near the end of _mime_header_parse, below. */
static int
_remove_entry (void *item, void *data)
{
  struct mu_mime_param *p = item;
  mu_assoc_t assoc = data;
  mu_assoc_remove_ref (assoc, p);
  return 0;
}

/* A working horse of this module.  Parses input string, which should
   be a header field value complying to RFCs 2045, 2183, 2231.3.

   Input:
    TEXT   - The string.
    ASSOC  - Associative array of parameters indexed by their names.
    SUBSET - If true, store only those parameters that are already
             in ASSOC.
   Output:
    PVALUE - Unless NULL, a pointer to the field value is stored here on
             success.
    ASSOC  - Unless NULL, parameters are stored here.	     

   Either PVALUE or ASSOC (but not both) can be NULL, meaning that the
   corresponding data are of no interest to the caller.
*/
static int
_mime_header_parse (const char *text, char **pvalue,
		    mu_assoc_t assoc, const char *outcharset, int subset)
{
  int rc = 0;
  struct mu_wordsplit ws;
  struct param_continuation cont;
  size_t i;

  ws.ws_delim = " \t\r\n;";
  ws.ws_escape = "\\\"";
  if (mu_wordsplit (text, &ws,
		    MU_WRDSF_DELIM | MU_WRDSF_ESCAPE |
		    MU_WRDSF_NOVAR | MU_WRDSF_NOCMD |
		    MU_WRDSF_DQUOTE | MU_WRDSF_SQUEEZE_DELIMS |
		    MU_WRDSF_RETURN_DELIMS | MU_WRDSF_WS))
    {
      mu_debug (MU_DEBCAT_MIME, MU_DEBUG_ERROR,
		(_("wordsplit: %s"), mu_wordsplit_strerror (&ws)));
      return MU_ERR_PARSE;
    }

  if (!assoc)
    {
      if (!pvalue)
	return MU_ERR_OUT_PTR_NULL;
      *pvalue = strdup (ws.ws_wordv[0]);
      mu_wordsplit_free (&ws);
      if (!*pvalue)
	return ENOMEM;
      return 0;
    }
    
  memset (&cont, 0, sizeof (cont));
  for (i = 1; i < ws.ws_wordc; i++)
    {
      size_t klen;
      char *key;
      char *val;
      const char *lang = NULL;
      const char *cset = NULL;
      char *langp = NULL;
      char *csetp = NULL;
      char *p;
      char *decoded;
      int flags = 0;
      struct mu_mime_param param;

      key = ws.ws_wordv[i];
      if (key[0] == ';')
	/* Reportedly, some MUAs insert several semicolons */
	continue;
      p = strchr (key, '=');
      if (!p)
	val = "";
      else
	{
	  *p++ = 0;
	  val = p;
	}

      klen = strlen (key);
      if (klen == 0)
	continue;

      p = strchr (key, '*');
      if (p)
	{
	  /* It is a parameter value continuation (RFC 2231, Section 3)
	     or parameter value character set and language information
	     (ibid., Section 4). */
	  klen = p - key;
	  if (p[1])
	    {
	      if (mu_isdigit (p[1]))
		{
		  char *q;
		  unsigned long n = strtoul (p + 1, &q, 10);

		  if (*q && *q != '*')
		    {
		      mu_debug (MU_DEBCAT_MIME, MU_DEBUG_TRACE0,
				(_("malformed parameter name %s: skipping"),
				 key));
		      continue;
		    }

		  if (n != cont.param_cind)
		    {
		      mu_debug (MU_DEBCAT_MIME, MU_DEBUG_TRACE0,
				(_("continuation index out of sequence in %s: "
				   "skipping"),
				 key));
		      /* Ignore this parameter. Another possibility would be
			 to drop the continuation assembled so far. That makes
			 little difference, because the string is malformed
			 anyway.

			 We try to continue just to gather as many information
			 as possible from this mess.
		      */
		      continue;
		    }

		  if (n == 0)
		    {
		      cont.param_name = malloc (klen + 1);
		      if (!cont.param_name)
			{
			  rc = ENOMEM;
			  break;
			}
		      cont.param_length = klen;
		      memcpy (cont.param_name, key, klen);
		      cont.param_name[klen] = 0;

		      rc = mu_memory_stream_create (&cont.param_value,
						    MU_STREAM_RDWR);
		      if (rc)
			break;
		    }
		  else if (cont.param_length != klen ||
			   memcmp (cont.param_name, key, klen))
		    {
		      mu_debug (MU_DEBCAT_MIME, MU_DEBUG_TRACE0,
				(_("continuation name mismatch: %s: "
				   "skipping"),
				 key));
		      continue;
		    }
		      
		  if (*q == '*')
		    flags |= MU_MIMEHDR_CSINFO;
		      
		  cont.param_cind++;
		  flags |= MU_MIMEHDR_MULTILINE;
		}
	    }
	  else
	    {
	      flags |= MU_MIMEHDR_CSINFO;
	      *p = 0;
	    }
	}
      else if (cont.param_name)
	{
	  rc = flush_param (&cont, assoc, subset, outcharset);
	  free_param_continuation (&cont);
	  if (rc)
	    break;
	}
      
      if (flags & MU_MIMEHDR_CSINFO)
	{
	  p = strchr (val, '\'');
	  if (p)
	    {
	      char *q = strchr (p + 1, '\'');
	      if (q)
		{
		  cset = val;
		  *p++ = 0;
		  lang = p;
		  *q++ = 0;
		  val = q;
		}
	    }

	  if ((flags & MU_MIMEHDR_MULTILINE) && cont.param_cind == 1)
	    {
	      cont.param_lang = lang;
	      cont.param_cset = cset;
	    }
	}

      if (flags & MU_MIMEHDR_CSINFO)
	{
	  char *tmp;
	  
	  rc = mu_str_url_decode (&tmp, val);
	  if (rc)
	    break;
	  if (!(flags & MU_MIMEHDR_MULTILINE))
	    {
	      if (!outcharset || mu_c_strcasecmp (cset, outcharset) == 0)
		decoded = tmp;
	      else
		{
		  rc = _recode_string (tmp, cset, outcharset, &decoded);
		  free (tmp);
		}
	      if (rc)
		break;
	    }
	  else
	    decoded = tmp;
	}
      else
	{
	  rc = mu_rfc2047_decode_param (outcharset, val, &param);
	  if (rc)
	    return rc;
	  cset = csetp = param.cset;
	  lang = langp = param.lang;
	  decoded = param.value;
	}
      val = decoded;
      
      if (flags & MU_MIMEHDR_MULTILINE)
	{
	  rc = mu_stream_write (cont.param_value, val, strlen (val),
				NULL);
	  free (decoded);
	  free (csetp);
	  free (langp);
	  if (rc)
	    break;
	  continue;
	}

      memset (&param, 0, sizeof (param));
      if (lang)
	{
	  param.lang = strdup (lang);
	  if (!param.lang)
	    rc = ENOMEM;
	  else
	    {
	      param.cset = strdup (cset);
	      if (!param.cset)
		{
		  free (param.lang);
		  rc = ENOMEM;
		}
	    }
	}

      free (csetp);
      free (langp);
      
      if (rc)
	{
	  free (decoded);
	  break;
	}
      
      param.value = strdup (val);
      free (decoded);
      if (!param.value)
	{
	  _mu_mime_param_free (&param);
	  rc = ENOMEM;
	  break;
	}

      if (subset)
	{
	  struct mu_mime_param *p = mu_assoc_ref (assoc, key);
	  if (p)
	    *p = param;
	}
      else
	{
	  rc = mu_assoc_install (assoc, key, &param);
	  if (rc)
	    {
	      _mu_mime_param_free (&param);
	      break;
	    }
	}
    }

  if (rc == 0 && cont.param_name)
    rc = flush_param (&cont, assoc, subset, outcharset);
  free_param_continuation (&cont);
  if (rc == 0)
    {
      if (pvalue)
	{
	  *pvalue = strdup (ws.ws_wordv[0]);
	  if (!*pvalue)
	    rc = ENOMEM;
	}
    }

  mu_wordsplit_free (&ws);

  if (subset)
    {
      /* Eliminate empty elements.
	 
	 FIXME: What I wanted to do initially is commented out, because
	 unfortunately iterator_ctl is not defined for assoc tables...
      */
#if 0
      mu_iterator_t itr;

      rc = mu_assoc_get_iterator (assoc, &itr);
      if (rc == 0)
	{
	  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	       mu_iterator_next (itr))
	    {
	      const char *name;
	      struct mu_mime_param *p;
	      
	      mu_iterator_current_kv (itr, (const void **)&name, (void**)&p);
	      if (!p->value)
		mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	    }
	  mu_iterator_destroy (&itr);
	}
#else
  /* ... Instead, the following kludgy approach is taken: */
      mu_iterator_t itr;
      mu_list_t elist;

      rc = mu_list_create (&elist);
      if (rc)
	return rc;
      rc = mu_assoc_get_iterator (assoc, &itr);
      if (rc == 0)
	{
	  for (mu_iterator_first (itr); rc == 0 && !mu_iterator_is_done (itr);
	       mu_iterator_next (itr))
	    {
	      const char *name;
	      struct mu_mime_param *p;
	      
	      mu_iterator_current_kv (itr, (const void **)&name, (void**)&p);
	      if (!p->value)
		rc = mu_list_append (elist, p);
	    }
	  mu_iterator_destroy (&itr);
	}
      if (rc == 0)
	mu_list_foreach (elist, _remove_entry, assoc);
      mu_list_destroy (&elist);
#endif
    }
  
  return rc;
}

/* Parse header value from TEXT and return its value and a subset of
   parameters.

   Input:
     TEXT   - Header value.
     CSET   - Output charset.  Can be NULL, in which case no conversions
              take place.
     ASSOC  - Parameter array initialized with empty slots for those
              parameters, which are wanted on output.  It should be
	      created using mu_mime_param_assoc_create and populated
	      using mu_mime_param_assoc_add.
   Output:
     PVALUE - A pointer to the field value is stored here on success.
     ASSOC  - Receives available parameters matching the input subset.

   Either PVALUE or ASSOC (but not both) can be NULL, meaning that the
   corresponding data are of no interest to the caller.
*/
int
mu_mime_header_parse_subset (const char *text, const char *cset,
			     char **pvalue, mu_assoc_t assoc)
{
  return _mime_header_parse (text, pvalue, assoc, cset, 1);
}

/* Parse header value from TEXT and return its value and parameters.

   Input:
     TEXT   - Header value.
     CSET   - Output charset.  Can be NULL, in which case no conversions
              take place.
   Output:
     PVALUE - A pointer to the field value is stored here on success.
     PASSOC - Receives an associative array of parameters.

   Either PVALUE or PASSOC (but not both) can be NULL, meaning that the
   corresponding data are of no interest to the caller.
*/
int
mu_mime_header_parse (const char *text, char *cset, char **pvalue,
		      mu_assoc_t *passoc)
{
  int rc;
  mu_assoc_t assoc;
  
  rc = mu_mime_param_assoc_create (&assoc);
  if (rc == 0)
    {
      rc = _mime_header_parse (text, pvalue, assoc, cset, 0);
      if (rc || !passoc)
	mu_assoc_destroy (&assoc);
      else
	*passoc = assoc;
    }
  
  return rc;
}  

/* TEXT is a value of a structured MIME header, e.g. Content-Type.
   This function returns the `disposition part' of it.  In other
   words, it returns disposition, if TEXT is a Content-Disposition
   value, and `type/subtype' part, if it is a Content-Type value.
*/
int
mu_mimehdr_get_disp (const char *text, char *buf, size_t bufsz, size_t *retsz)
{
  int rc;
  char *value;
  
  rc = mu_mime_header_parse (text, NULL, &value, NULL);
  if (rc == 0)
    {
      size_t size = strlen (value);
      if (size > bufsz)
	size = bufsz;
      if (buf)
	size = mu_cpystr (buf, value, size);
      if (retsz)
	*retsz = size;
    }
  free (value);
  return 0;
}

/* Same as mu_mimehdr_get_disp, but allocates memory */
int
mu_mimehdr_aget_disp (const char *text, char **pvalue)
{
  return mu_mime_header_parse (text, NULL, pvalue, NULL);
}

/* Get the value of parameter NAME from STR, which must be
   a value of a structured MIME header.
   At most BUFSZ-1 of data are stored in BUF.  A terminating NUL
   character is appended to it.
   
   Unless NULL, RETSZ is filled with the actual length of the
   returned data (not including the NUL terminator).

   BUF may be NULL, in which case the function will only fill
   RETSZ, as described above. */
int
mu_mimehdr_get_param (const char *str, const char *name,
		      char *buf, size_t bufsz, size_t *retsz)
{
  int rc;
  char *value;
  
  rc = mu_mimehdr_aget_param (str, name, &value);
  if (rc == 0)
    {
      size_t size = strlen (value);
      if (size > bufsz)
	size = bufsz;
      if (buf)
	size = mu_cpystr (buf, value, size);
      if (retsz)
	*retsz = size;
    }
  free (value);
  return rc;
}

/* Same as mu_mimehdr_get_param, but allocates memory. */
int
mu_mimehdr_aget_param (const char *str, const char *name, char **pval)
{
  return mu_mimehdr_aget_decoded_param (str, name, NULL, pval, NULL);
}


/* Similar to mu_mimehdr_aget_param, but the returned value is decoded
   according to the CHARSET.  Unless PLANG is NULL, it receives malloc'ed
   language name from STR.  If there was no language name, *PLANG is set
   to NULL. 
*/
int
mu_mimehdr_aget_decoded_param (const char *str, const char *name,
			       const char *charset, 
			       char **pval, char **plang)
{
  mu_assoc_t assoc;
  int rc;

  rc = mu_mime_param_assoc_create (&assoc);
  if (rc == 0)
    {
      rc = mu_mime_param_assoc_add (assoc, name);
      if (rc == 0)
	{
	  rc = mu_mime_header_parse_subset (str, charset, NULL, assoc);
	  if (rc == 0)
	    {
	      struct mu_mime_param *param = mu_assoc_ref (assoc, name);
	      if (!param)
		rc = MU_ERR_NOENT;
	      else
		{
		  *pval = param->value;
		  if (plang)
		    {
		      *plang = param->lang;
		      param->lang = NULL;
		    }
		  param->value = NULL;
		}
	    }
	}
      mu_assoc_destroy (&assoc);
    }
  return rc;
}
  
/* Get the attachment name from a message.

   Input:
     MSG     - The input message.
     CHARSET - Character set to recode output values to.  Can be NULL.
   Output:
     PBUF    - Output value.
     PSZ     - Its size in bytes, not counting the terminating zero.
     PLANG   - Language the name is written in, if provided in the header.

   Either PSZ or PLAN (or both) can be NULL.
*/
static int
_get_attachment_name (mu_message_t msg, const char *charset,
		      char **pbuf, size_t *psz, char **plang)
{
  int ret = EINVAL;
  mu_header_t hdr;
  char *value = NULL;
  mu_assoc_t assoc;

  if (!msg)
    return ret;

  if ((ret = mu_message_get_header (msg, &hdr)) != 0)
    return ret;

  ret = mu_header_aget_value_unfold (hdr, MU_HEADER_CONTENT_DISPOSITION,
				     &value);

  /* If the header wasn't there, we'll fall back to Content-Type, but
     other errors are fatal. */
  if (ret != 0 && ret != MU_ERR_NOENT)
    return ret;

  if (ret == 0 && value != NULL)
    {
      ret = mu_mime_param_assoc_create (&assoc);
      if (ret)
	return ret;
      ret = mu_mime_param_assoc_add (assoc, "filename");
      if (ret == 0)
	{
	  char *disp;
	  
	  ret = mu_mime_header_parse_subset (value, charset, &disp, assoc);
	  if (ret == 0)
	    {
	      struct mu_mime_param *param;
	      if (mu_c_strcasecmp (disp, "attachment") == 0 &&
		  (param = mu_assoc_ref (assoc, "filename")))
		{
		  *pbuf = param->value;
		  if (psz)
		    *psz = strlen (*pbuf);
		  param->value = NULL;
		  if (plang)
		    {
		      *plang = param->lang;
		      param->lang = NULL;
		    }
		}
	      else
		ret = MU_ERR_NOENT;
	      free (disp);
	      mu_assoc_destroy (&assoc);
	    }
	}
    }

  free (value);

  if (ret == 0)
    return ret;

  /* If we didn't get the name, we fall back on the Content-Type name
     parameter. */

  ret = mu_header_aget_value_unfold (hdr, MU_HEADER_CONTENT_TYPE, &value);
  if (ret == 0)
    {
      ret = mu_mime_param_assoc_create (&assoc);
      if (ret)
	return ret;
      ret = mu_mime_param_assoc_add (assoc, "name");
      if (ret == 0)
	{
	  ret = mu_mime_header_parse_subset (value, charset, NULL, assoc);
	  if (ret == 0)
	    {
	      struct mu_mime_param *param;
	      if ((param = mu_assoc_ref (assoc, "name")))
		{
		  *pbuf = param->value;
		  if (psz)
		    *psz = strlen (*pbuf);
		  param->value = NULL;
		  if (plang)
		    {
		      *plang = param->lang;
		      param->lang = NULL;
		    }
		}
	      else
		ret = MU_ERR_NOENT;
	    }
	}
      free (value);
    }

  return ret;
}

int
mu_message_aget_attachment_name (mu_message_t msg, char **name)
{
  if (name == NULL)
    return MU_ERR_OUT_PTR_NULL;
  return _get_attachment_name (msg, NULL, name, NULL, NULL);
}

int
mu_message_aget_decoded_attachment_name (mu_message_t msg,
					 const char *charset,
					 char **pval,
					 char **plang)
{
  if (pval == NULL)
    return MU_ERR_OUT_PTR_NULL;
  return _get_attachment_name (msg, charset, pval, NULL, plang);
}

int
mu_message_get_attachment_name (mu_message_t msg, char *buf, size_t bufsz,
				size_t *sz)
{
  char *tmp;
  size_t size;
  int rc = _get_attachment_name (msg, NULL, &tmp, &size, NULL);
  if (rc == 0)
    {
      if (size > bufsz)
	size = bufsz;
      if (buf)
	size = mu_cpystr (buf, tmp, size);
      if (sz)
	*sz = size;
    }
  free (tmp);
  return rc;
}

