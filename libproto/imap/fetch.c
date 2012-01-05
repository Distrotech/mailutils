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
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/address.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/assoc.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_fetch (mu_imap_t imap, int uid, mu_msgset_t msgset, const char *items)
{
  char const *argv[3];
  int i;
  static struct imap_command com;

  i = 0;
  if (uid)
    argv[i++] = "UID";
  argv[i++] = "FETCH";
  argv[i++] = "\\";
  
  com.session_state = MU_IMAP_SESSION_SELECTED;
  com.capa = NULL;
  com.rx_state = MU_IMAP_CLIENT_FETCH_RX;
  com.argc = i;
  com.argv = argv;
  com.extra = items;
  com.msgset = msgset;
  com.tagged_handler = NULL;
  com.untagged_handler = NULL;

  return mu_imap_gencom (imap, &com);
}

static void
_free_fetch_response (void *ptr)
{
  union mu_imap_fetch_response *resp = ptr;
  switch (resp->type)
    {
    case MU_IMAP_FETCH_BODY:
      free (resp->body.partv);
      free (resp->body.section);
      mu_list_destroy (&resp->body.fields);
      free (resp->body.text);
      break;
      
    case MU_IMAP_FETCH_BODYSTRUCTURE:
      mu_bodystructure_free (resp->bodystructure.bs);
      break;
      
    case MU_IMAP_FETCH_ENVELOPE:
      mu_message_imapenvelope_free (resp->envelope.imapenvelope);
      break;
      
    case MU_IMAP_FETCH_FLAGS:
    case MU_IMAP_FETCH_INTERNALDATE:
    case MU_IMAP_FETCH_RFC822_SIZE:
    case MU_IMAP_FETCH_UID:
      break;
    }
  free (resp);
}

static int
alloc_response (union mu_imap_fetch_response **resp, int type)
{
  static size_t sizetab[] = {
    sizeof (struct mu_imap_fetch_body),
    sizeof (struct mu_imap_fetch_bodystructure),
    sizeof (struct mu_imap_fetch_envelope),
    sizeof (struct mu_imap_fetch_flags),
    sizeof (struct mu_imap_fetch_internaldate),
    sizeof (struct mu_imap_fetch_rfc822_size),
    sizeof (struct mu_imap_fetch_uid)
  };
  union mu_imap_fetch_response *p;
  
  if (type < 0 || type >= MU_ARRAY_SIZE (sizetab))
    return EINVAL;
  p = calloc (1, sizetab[type]);
  if (!p)
    return ENOMEM;
  p->type = type;
  *resp = p;
  return 0;
}

enum parse_response_state
  {
    resp_kw,
    resp_val,
    resp_body,
    resp_body_section,
    resp_skip,
    resp_body_hlist,
    resp_body_end
  };

struct parse_response_env;

typedef int (*mapper_fn) (union mu_imap_fetch_response *resp,
			  struct imap_list_element *elt,
			  struct parse_response_env *env);

struct parse_response_env
{
  mu_list_t result;
  struct imap_list_element *elt;
  enum parse_response_state state;
  union mu_imap_fetch_response *resp;
  mapper_fn mapper;
  const char *section;
  mu_list_t hlist;
  int status;
};


static int
_uid_mapper (union mu_imap_fetch_response *resp,
	     struct imap_list_element *elt,
	     struct parse_response_env *parse_env)
{
  char *p;
  size_t uid;
  
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  uid = strtoul (elt->v.string, &p, 0);
  if (*p)
    return MU_ERR_FAILURE;
  resp->uid.uid = uid;
  return 0;
}
			 
static int
_size_mapper (union mu_imap_fetch_response *resp,
	      struct imap_list_element *elt,
	      struct parse_response_env *parse_env)
{
  char *p;
  size_t size;
  
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  size = strtoul (elt->v.string, &p, 0);
  if (*p)
    return MU_ERR_FAILURE;
  resp->rfc822_size.size = size;
  return 0;
}

static int
_body_mapper (union mu_imap_fetch_response *resp,
	      struct imap_list_element *elt,
	      struct parse_response_env *parse_env)
{
  const char *section, *p;
  size_t partc = 0;
  size_t *partv = NULL;
  
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;

  section = parse_env->section;
  if (section)
    {
      p = section;
      while (mu_isdigit (*p))
	{
	  partc++;
	  p = strchr (p, '.');
	  if (p)
	    {
	      p++;
	      continue;
	    }
	  
	  break;
	}
    }
  else
    p = NULL;

  if (p)
    {
      resp->body.section = strdup (p);
      if (!resp->body.section)
	{
	  free (resp);
	  return ENOMEM;
	}
    }

  if (partc)
    {
      size_t i;
      
      partv = calloc (partc, sizeof (partv[0]));
      for (i = 0, p = section; i < partc; i++)
	{
	  char *q;
	  partv[i] = strtoul (p, &q, 10);
	  p = q + 1;
	}
    }

  resp->body.partc = partc;
  resp->body.partv = partv;

  resp->body.fields = parse_env->hlist;
  parse_env->hlist = NULL;
  
  resp->body.text = strdup (elt->v.string);
  if (!resp->body.text)
    {
      free (resp->body.section);
      free (resp->body.partv);
      free (resp);
      return ENOMEM;
    }
  return 0;
}

static int
_rfc822_mapper (union mu_imap_fetch_response *resp,
		struct imap_list_element *elt,
		struct parse_response_env *parse_env)
{
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;

  resp->body.partc = 0;
  resp->body.partv = NULL;

  resp->body.section = strdup (parse_env->section);
  if (!resp->body.section)
    {
      free (resp);
      return ENOMEM;
    }
  
  resp->body.text = strdup (elt->v.string);
  if (!resp->body.text)
    {
      free (resp->body.section);
      free (resp->body.partv);
      free (resp);
      return ENOMEM;
    }
  return 0;
}

static int
_rfc822_header_mapper (union mu_imap_fetch_response *resp,
		       struct imap_list_element *elt,
		       struct parse_response_env *parse_env)
{
  parse_env->section = "HEADER";
  return _rfc822_mapper (resp, elt, parse_env);
}

static int
_rfc822_text_mapper (union mu_imap_fetch_response *resp,
		     struct imap_list_element *elt,
		     struct parse_response_env *parse_env)
{
  parse_env->section = "TEXT";
  return _rfc822_mapper (resp, elt, parse_env);
}

static int
_flags_mapper (union mu_imap_fetch_response *resp,
	       struct imap_list_element *elt,
	       struct parse_response_env *parse_env)
{
  if (elt->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  if (_mu_imap_collect_flags (elt, &resp->flags.flags))
    return MU_ERR_FAILURE;
  return 0;
}

static int
_date_mapper (union mu_imap_fetch_response *resp,
	      struct imap_list_element *elt,
	      struct parse_response_env *parse_env)
{
  struct tm tm;
  struct mu_timezone tz;
  
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  if (mu_scan_datetime (elt->v.string, MU_DATETIME_INTERNALDATE, &tm, &tz,
			NULL))
    return MU_ERR_FAILURE;
  resp->internaldate.tm = tm;
  resp->internaldate.tz = tz;
  return 0;
}

static int parse_bodystructure (struct imap_list_element *elt,
				struct mu_bodystructure **pbs);

struct body_field_map
{
  size_t offset; /* Offset of the target member of mu_bodystructure */
  int (*mapper) (struct imap_list_element *, void *);
};

static int
parse_bs_list (struct imap_list_element *elt,
	       struct mu_bodystructure *bs,
	       struct body_field_map *map)
{
  int rc;
  mu_iterator_t itr;

  rc = mu_list_get_iterator (elt->v.list, &itr);
  if (rc)
    return rc;
  for (mu_iterator_first (itr);
       map->mapper && !mu_iterator_is_done (itr);
       mu_iterator_next (itr), map++)
    {
      struct imap_list_element *tok;
      mu_iterator_current (itr, (void**)&tok);
      rc = map->mapper (tok, (char*)bs + map->offset);
      if (rc)
	break;
    }
  mu_iterator_destroy (&itr);
  return rc;
}

static int
_map_body_param (void **itmv, size_t itmc, void *call_data)
{
  mu_assoc_t assoc = call_data;
  struct mu_mime_param param;
  struct imap_list_element *key, *val;
  int rc;

  if (itmc != 2)
    return MU_ERR_PARSE;

  key = itmv[0];
  val = itmv[1];
  if (key->type != imap_eltype_string || val->type != imap_eltype_string)
    return MU_ERR_PARSE;
  
  rc = mu_rfc2047_decode_param ("UTF-8", val->v.string, &param);
  if (rc)
    {
      param.lang = param.cset = NULL;
      param.value = strdup (val->v.string);
      if (!param.value)
	return ENOMEM;
    }
  return mu_assoc_install (assoc, key->v.string, &param);
}

static int
_body_field_text_mapper (struct imap_list_element *tok, void *ptr)
{
  char *s;
  
  if (_mu_imap_list_element_is_nil (tok))
    s = NULL;
  else if (tok->type != imap_eltype_string)
    return MU_ERR_PARSE;
  else if (!(s = strdup (tok->v.string)))
    return ENOMEM;
  *(char**) ptr = s;
  return 0;
}

static int
_body_field_size_mapper (struct imap_list_element *tok, void *ptr)
{
  unsigned long n;
  
  if (_mu_imap_list_element_is_nil (tok))
    n = 0;
  else if (tok->type != imap_eltype_string)
    return MU_ERR_PARSE;
  else
    {
      char *s;

      errno = 0;
      n = strtoul (tok->v.string, &s, 10);
      if (*s || errno)
	return MU_ERR_PARSE;
    }
  *(size_t*) ptr = n;
  return 0;
}

static int
_body_field_param_mapper (struct imap_list_element *tok, void *ptr)
{
  mu_assoc_t param;
  int rc = mu_mime_param_assoc_create (&param);
  if (rc)
    return rc;
  *(mu_assoc_t*) ptr = param;
  if (_mu_imap_list_element_is_nil (tok))
    return 0;
  if (tok->type != imap_eltype_list)
    return MU_ERR_PARSE;
  return mu_list_gmap (tok->v.list, _map_body_param, 2, param);
}  

static int
_body_field_disposition_mapper (struct imap_list_element *tok, void *ptr)
{
  int rc;
  struct mu_bodystructure *bs = ptr;
  struct imap_list_element *elt;
  
  if (_mu_imap_list_element_is_nil (tok))
    return 0;
  if (tok->type != imap_eltype_list)
    return MU_ERR_PARSE;
  elt = _mu_imap_list_at (tok->v.list, 0);
  if (_mu_imap_list_element_is_nil (elt))
    bs->body_disposition = NULL;
  else if (elt->type != imap_eltype_string)
    return MU_ERR_PARSE;
  else if ((bs->body_disposition = strdup (elt->v.string)) == NULL)
    return ENOMEM;

  rc = mu_mime_param_assoc_create (&bs->body_disp_param);
  if (rc)
    return rc;
  
  elt = _mu_imap_list_at (tok->v.list, 1);
  if (_mu_imap_list_element_is_nil (elt))
    return 0;
  else if (elt->type != imap_eltype_list)
    return MU_ERR_PARSE;
  return mu_list_gmap (elt->v.list, _map_body_param, 2, bs->body_disp_param);
}

static int parse_envelope (struct imap_list_element *elt,
			   struct mu_imapenvelope **penv);

static int
_body_field_imapenvelope_mapper (struct imap_list_element *tok, void *ptr)
{
  return parse_envelope (tok, ptr);
}

static int
_body_field_bodystructure_mapper (struct imap_list_element *tok, void *ptr)
{
  return parse_bodystructure (tok, ptr);
}

/* Simple text or message/rfc822 body.

   Sample TEXT body structure:

   ("TEXT" "PLAIN" ("CHARSET" "US-ASCII" "NAME" "cc.diff")
    "<960723163407.20117h@cac.washington.edu>" "Compiler diff"
    "BASE64" 4554 73)

    Elements:

    0        "TEXT"            body_type
    1        "PLAIN"           body_subtype
    2        (...)             body_param
    3        "<9607...>"       body_id
    4        "Compiler diff"   body_descr
    5        "BASE64"          body_encoding
    6        4554              body_size
    7        73                v.text.body_lines
    [Optional]
    8                          body_md5
    9                          body_disposition;
    10                         body_language;
    11                         body_location;
*/

struct body_field_map base_field_map[] = {
  { mu_offsetof (struct mu_bodystructure, body_type),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_subtype),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_param),
    _body_field_param_mapper },
  { mu_offsetof (struct mu_bodystructure, body_id),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_descr),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_encoding),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_size),
    _body_field_size_mapper },
  { mu_offsetof (struct mu_bodystructure, body_md5),
    _body_field_text_mapper },
  { 0, _body_field_disposition_mapper },
  { mu_offsetof (struct mu_bodystructure, body_language),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_location),
    _body_field_text_mapper },
  { 0, NULL }
};

struct body_field_map text_field_map[] = {
  { mu_offsetof (struct mu_bodystructure, body_type),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_subtype),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_param),
    _body_field_param_mapper },
  { mu_offsetof (struct mu_bodystructure, body_id),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_descr),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_encoding),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_size),
    _body_field_size_mapper },
  { mu_offsetof (struct mu_bodystructure, v.text.body_lines),
    _body_field_size_mapper },
  { mu_offsetof (struct mu_bodystructure, body_md5),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_disposition),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_language),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_location),
    _body_field_text_mapper },
  { 0, NULL }
};

struct body_field_map message_field_map[] = {
  { mu_offsetof (struct mu_bodystructure, body_type),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_subtype),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_param),
    _body_field_param_mapper },
  { mu_offsetof (struct mu_bodystructure, body_id),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_descr),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_encoding),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_size),
    _body_field_size_mapper },
  { mu_offsetof (struct mu_bodystructure, v.rfc822.body_env),
    _body_field_imapenvelope_mapper },
  { mu_offsetof (struct mu_bodystructure, v.rfc822.body_struct),
    _body_field_bodystructure_mapper },
  { mu_offsetof (struct mu_bodystructure, v.rfc822.body_lines),
    _body_field_size_mapper },
  { mu_offsetof (struct mu_bodystructure, body_md5),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_disposition),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_language),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_location),
    _body_field_text_mapper },
  { 0, NULL }
};

struct body_field_map multipart_field_map[] = {
  /* Body type is processed separately */
  { mu_offsetof (struct mu_bodystructure, body_subtype),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_param),
    _body_field_param_mapper },
  { 0, _body_field_disposition_mapper },
  { mu_offsetof (struct mu_bodystructure, body_language),
    _body_field_text_mapper },
  { mu_offsetof (struct mu_bodystructure, body_location),
    _body_field_text_mapper },
  { 0, NULL }
};

#define BSTOK_BODY_TYPE    0
#define BSTOK_BODY_SUBTYPE 1

static int
_parse_bodystructure_simple (struct imap_list_element *elt,
			     struct mu_bodystructure *bs)
{
  size_t n;
  struct imap_list_element *tok, *subtype;
  struct body_field_map *map;
  
  mu_list_count (elt->v.list, &n);
  if (n < 7)
    return MU_ERR_PARSE;

  tok = _mu_imap_list_at (elt->v.list, BSTOK_BODY_TYPE);
  if (!tok || tok->type != imap_eltype_string)
    return MU_ERR_PARSE;
  subtype = _mu_imap_list_at (elt->v.list, BSTOK_BODY_SUBTYPE);
  if (!subtype || subtype->type != imap_eltype_string)
    return MU_ERR_PARSE;
  
  if (mu_c_strcasecmp (tok->v.string, "TEXT") == 0)
    {
      bs->body_message_type = mu_message_text;
      map = text_field_map;
    }
  else if (mu_c_strcasecmp (tok->v.string, "MESSAGE") == 0 &&
	   mu_c_strcasecmp (subtype->v.string, "RFC822") == 0)
    {
      bs->body_message_type = mu_message_rfc822;
      map = message_field_map;
    }
  else
    {
      bs->body_message_type = mu_message_other;
      map = base_field_map;
    }
  
  return parse_bs_list (elt, bs, map);
}

/* Example multipart:
        (("TEXT" "PLAIN" ("CHARSET" "US-ASCII") NIL NIL "7BIT" 1152
         23)
	 ("TEXT" "PLAIN" ("CHARSET" "US-ASCII" "NAME" "cc.diff")
         "<960723163407.20117h@cac.washington.edu>" "Compiler diff"
         "BASE64" 4554 73)
	 "MIXED")

*/
static int
_parse_bodystructure_mixed (struct imap_list_element *elt,
			    struct mu_bodystructure *bs)
{
  int rc;
  struct imap_list_element *tok;
  mu_iterator_t itr;
  struct body_field_map *map = multipart_field_map;
  
  bs->body_message_type = mu_message_multipart;
  if (!(bs->body_type = strdup ("MULTIPART")))
    return ENOMEM;

  rc = mu_list_create (&bs->v.multipart.body_parts);
  if (rc)
    return rc;

  mu_list_set_destroy_item (bs->v.multipart.body_parts,
			    mu_list_free_bodystructure);

  rc = mu_list_get_iterator (elt->v.list, &itr);
  if (rc)
    return rc;
  for (mu_iterator_first (itr);
       !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      struct mu_bodystructure *bspart;

      mu_iterator_current (itr, (void**) &tok);
      if (!tok)
	return MU_ERR_PARSE;
      if (tok->type != imap_eltype_list)
	break;
      rc = parse_bodystructure (tok, &bspart);
      if (rc)
	return rc;
      rc = mu_list_append (bs->v.multipart.body_parts, bspart);
      if (rc)
	{
	  mu_bodystructure_free (bspart);
	  return rc;
	}
    }

  if (mu_iterator_is_done (itr))
    return MU_ERR_PARSE;
  
  for (; map->mapper && !mu_iterator_is_done (itr);
       mu_iterator_next (itr), map++)
    {
      struct imap_list_element *tok;
      mu_iterator_current (itr, (void**)&tok);
      rc = map->mapper (tok, (char*)bs + map->offset);
      if (rc)
	return rc;
    }
  mu_iterator_destroy (&itr);
  return 0;
}

static int
parse_bodystructure (struct imap_list_element *elt,
		     struct mu_bodystructure **pbs)
{
  int rc;
  struct mu_bodystructure *bs;
  struct imap_list_element *tok;
  
  if (elt->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  bs = calloc (1, sizeof (*bs));
  if (!bs)
    return ENOMEM;
  tok = _mu_imap_list_at (elt->v.list, 0);
  if (tok->type == imap_eltype_string)
    rc = _parse_bodystructure_simple (elt, bs);
  else
    rc = _parse_bodystructure_mixed (elt, bs);

  if (rc)
    mu_bodystructure_free (bs);
  else
    *pbs = bs;
  return rc;
}

static int
_bodystructure_mapper (union mu_imap_fetch_response *resp,
		       struct imap_list_element *elt,
		       struct parse_response_env *parse_env)
{
  return parse_bodystructure (elt, &resp->bodystructure.bs);
}

struct fill_env
{
  struct mu_imapenvelope *imapenvelope;
  size_t n;
};

enum env_index
  {
    env_date,
    env_subject,
    env_from,
    env_sender,
    env_reply_to,
    env_to,
    env_cc,
    env_bcc,
    env_in_reply_to,
    env_message_id
  };

static int
elt_to_string (struct imap_list_element *elt, char **pstr)
{
  char *p;

  if (_mu_imap_list_element_is_nil (elt))
    p = NULL;
  else if (elt->type != imap_eltype_string)
    return EINVAL;
  else
    {
      p = strdup (elt->v.string);
      if (!p)
	return ENOMEM;
    }
  *pstr = p;
  return 0;
}

struct addr_env
{
  mu_address_t addr;
  size_t n;
};

static int
_fill_subaddr (void *item, void *data)
{
  struct addr_env *addr_env = data;
  struct imap_list_element *elt = item, *arg;
  const char *domain = NULL, *local = NULL, *personal = NULL;
  
  if (elt->type != imap_eltype_list)
    return 0;
  
  arg = _mu_imap_list_at (elt->v.list, 0);
  if (arg && arg->type == imap_eltype_string)
    personal = arg->v.string;
  arg = _mu_imap_list_at (elt->v.list, 2);
  if (arg && arg->type == imap_eltype_string)
    local = arg->v.string;
  arg = _mu_imap_list_at (elt->v.list, 3);
  if (arg && arg->type == imap_eltype_string)
    domain = arg->v.string;

  if (domain && local)
    {
      if (!addr_env->addr)
	{
	  int rc = mu_address_create_null (&addr_env->addr);
	  if (rc)
	    return rc;
	}
      mu_address_set_local_part (addr_env->addr, addr_env->n, local);
      mu_address_set_domain (addr_env->addr, addr_env->n, domain);
      mu_address_set_personal (addr_env->addr, addr_env->n, personal);
      addr_env->n++;
    }
  return 0;
}

static int
elt_to_address (struct imap_list_element *elt, mu_address_t *paddr)
{
  if (_mu_imap_list_element_is_nil (elt))
    *paddr = NULL;
  else if (elt->type != imap_eltype_list)
    return EINVAL;
  else
    {
      struct addr_env addr_env;
      addr_env.addr = NULL;
      addr_env.n = 1;
      mu_list_foreach (elt->v.list, _fill_subaddr, &addr_env);
      *paddr = addr_env.addr;
    }
  return 0;
}

static int
_fill_response (void *item, void *data)
{
  int rc;
  struct imap_list_element *elt = item;
  struct fill_env *env = data;
  struct mu_imapenvelope *imapenvelope = env->imapenvelope;
  
  switch (env->n++)
    {
    case env_date:
      if (elt->type != imap_eltype_string)
	rc = MU_ERR_FAILURE;
      else
	{
	  if (mu_scan_datetime (elt->v.string,
				MU_DATETIME_SCAN_RFC822,
				&imapenvelope->date,
				&imapenvelope->tz, NULL))
	    rc = MU_ERR_FAILURE;
	  else
	    rc = 0;
	}
      break;
      
    case env_subject:
      rc = elt_to_string (elt, &imapenvelope->subject);
      break;

    case env_from:
      rc = elt_to_address (elt, &imapenvelope->from);
      break;
	
    case env_sender:
      rc = elt_to_address (elt, &imapenvelope->sender);
      break;
      
    case env_reply_to:
      rc = elt_to_address (elt, &imapenvelope->reply_to);
      break;
      
    case env_to:
      rc = elt_to_address (elt, &imapenvelope->to);
      break;
      
    case env_cc:
      rc = elt_to_address (elt, &imapenvelope->cc);
      break;
      
    case env_bcc:
      rc = elt_to_address (elt, &imapenvelope->bcc);
      break;
      
    case env_in_reply_to:
      rc = elt_to_string (elt, &imapenvelope->in_reply_to);
      break;
      
    case env_message_id:
      rc = elt_to_string (elt, &imapenvelope->message_id);
      break;
    }
  return rc;
}

static int
parse_envelope (struct imap_list_element *elt, struct mu_imapenvelope **penv)
{
  struct fill_env env;
  
  if (elt->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  env.imapenvelope = calloc (1, sizeof (*env.imapenvelope));
  if (!env.imapenvelope)
    return ENOMEM;
  env.n = 0;
  mu_list_foreach (elt->v.list, _fill_response, &env);
  *penv = env.imapenvelope;
  return 0;
}

static int
_envelope_mapper (union mu_imap_fetch_response *resp,
		  struct imap_list_element *elt,
		  struct parse_response_env *parse_env)
{
  return parse_envelope (elt, &resp->envelope.imapenvelope);
}

struct mapper_tab
{
  char *name;
  size_t size;
  int type;
  mapper_fn mapper;
};
  
static struct mapper_tab mapper_tab[] = {
#define S(s) s, (sizeof (s) - 1)
  { S("BODYSTRUCTURE"), MU_IMAP_FETCH_BODYSTRUCTURE, _bodystructure_mapper },
  { S("BODY"),          MU_IMAP_FETCH_BODY,        _body_mapper },
  { S("ENVELOPE"),      MU_IMAP_FETCH_ENVELOPE,    _envelope_mapper },
  { S("FLAGS"),         MU_IMAP_FETCH_FLAGS,       _flags_mapper },
  { S("INTERNALDATE"),  MU_IMAP_FETCH_INTERNALDATE, _date_mapper },
  { S("RFC822"),        MU_IMAP_FETCH_BODY,        _body_mapper},
  { S("RFC822.HEADER"), MU_IMAP_FETCH_BODY,        _rfc822_header_mapper }, 
  { S("RFC822.SIZE"),   MU_IMAP_FETCH_RFC822_SIZE, _size_mapper },
  { S("RFC822.TEXT"),   MU_IMAP_FETCH_BODY,        _rfc822_text_mapper },
  { S("UID"),           MU_IMAP_FETCH_UID,         _uid_mapper },
#undef S
  { NULL }
};

static int
_extract_string (void **itmv, size_t itmc, void *call_data)
{
  struct imap_list_element *elt = itmv[0];
  if (elt->type != imap_eltype_string)
    return MU_LIST_MAP_SKIP;
  itmv[0] = elt->v.string;
  return 0;
}

static int
_fetch_fold (void *item, void *data)
{
  int rc;
  struct imap_list_element *elt = item;
  struct parse_response_env *env = data;

  switch (env->state)
    {
    case resp_kw:
      {
	char *kw;
	size_t kwlen;
	struct mapper_tab *mt;

	if (elt->type != imap_eltype_string)
	  {
	    env->status = MU_ERR_FAILURE;
	    return MU_ERR_FAILURE;
	  }
	kw = elt->v.string;
	kwlen = strlen (kw);
	for (mt = mapper_tab; mt->name; mt++)
	  {
	    if (mt->size == kwlen && memcmp (mt->name, kw, kwlen) == 0)
	      break;
	  }

	if (!mt->name)
	  {
	    mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE9,
		      ("ignoring unknown FETCH item '%s'", kw));
	    env->state = resp_skip;
	    return 0;
	  }

	env->mapper = mt->mapper;
	rc = alloc_response (&env->resp, mt->type);
	if (rc)
	  {
	    env->status = rc;
	    return MU_ERR_FAILURE;
	  }
	env->state = mt->type == MU_IMAP_FETCH_BODY ? resp_body : resp_val;
	break;
      }
      
    case resp_body:
      if (_mu_imap_list_element_is_string (elt, "["))
	{
	  env->state = resp_body_section;
	  break;
	}
      else
	{
	  env->mapper = _bodystructure_mapper;
	  _free_fetch_response (env->resp);
	  rc = alloc_response (&env->resp, MU_IMAP_FETCH_BODYSTRUCTURE);
	  if (rc)
	    {
	      env->status = rc;
	      return MU_ERR_FAILURE;
	    }
	  env->state = resp_val;
	}
      /* fall through */
      
    case resp_val:
      if (env->mapper)
	{
	  int rc = env->mapper (env->resp, elt, env);
	  if (rc)
	    _free_fetch_response (env->resp);
	  else
	    mu_list_append (env->result, env->resp);
	}
      env->resp = NULL;
      mu_list_destroy (&env->hlist);
      env->state = resp_kw;
      break;
      
    case resp_body_section:
      if (elt->type != imap_eltype_string)
	{
	  env->status = MU_ERR_PARSE;
	  return MU_ERR_FAILURE;
	}
      else if (strncmp (elt->v.string, "HEADER.FIELDS", 13) == 0)
	env->state = resp_body_hlist;
      else if (strcmp (elt->v.string, "]") == 0)
	{
	  env->section = NULL;
	  env->state = resp_val;
	  break;
	}
      else
	env->state = resp_body_end;
      env->section = elt->v.string;
      break;
      
    case resp_skip:
      mu_list_destroy (&env->hlist);
      env->state = resp_kw;
      break;
      
    case resp_body_hlist:
      if (elt->type != imap_eltype_list)
	{
	  env->status = MU_ERR_PARSE;
	  return MU_ERR_FAILURE;
	}
      mu_list_map (elt->v.list, _extract_string, NULL, 1, &env->hlist);
      env->state = resp_body_end;
      break;
      
    case resp_body_end:
      if (_mu_imap_list_element_is_string (elt, "]"))
	env->state = resp_val;
      else
	{
	  env->status = MU_ERR_PARSE;
	  return MU_ERR_FAILURE;
	}
    }
  
  return 0;
}

int
_mu_imap_parse_fetch_response (mu_list_t input, mu_list_t *result_list)
{
  mu_list_t result;
  int status;
  struct parse_response_env env;
  
  status = mu_list_create (&result);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("mu_list_create: %s", mu_strerror (status)));
      return 1;
    }
  mu_list_set_destroy_item (result, _free_fetch_response);
  memset (&env, 0, sizeof (env));

  env.result = result;
  mu_list_foreach (input, _fetch_fold, &env);
  if (env.status)
    mu_list_destroy (&result);
  else
    *result_list = result;
  mu_list_destroy (&env.hlist);
  return env.status;
}
