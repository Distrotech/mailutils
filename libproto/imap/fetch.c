/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_fetch (mu_imap_t imap, const char *msgset, const char *items)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;

  if (imap->session_state != MU_IMAP_SESSION_SELECTED)
    return MU_ERR_SEQ;
  
  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s FETCH %s %s\r\n",
				 imap->tag_str, msgset, items);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_FETCH_RX;

    case MU_IMAP_CLIENT_FETCH_RX:
      status = _mu_imap_response (imap, NULL, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->resp_code)
	{
	case MU_IMAP_OK:
	  status = 0;
	  break;

	case MU_IMAP_NO:
	  status = MU_ERR_FAILURE;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      imap->client_state = MU_IMAP_CLIENT_READY;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
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
      /* FIXME */
      break;
      
    case MU_IMAP_FETCH_ENVELOPE:
      free (resp->envelope.subject);
      mu_address_destroy (&resp->envelope.from);
      mu_address_destroy (&resp->envelope.sender);
      mu_address_destroy (&resp->envelope.reply_to);
      mu_address_destroy (&resp->envelope.to);
      mu_address_destroy (&resp->envelope.cc);
      mu_address_destroy (&resp->envelope.bcc);
      free (resp->envelope.in_reply_to);
      free (resp->envelope.message_id);
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
	  partv[i] = strtoul (p, &p, 10);
	  p++;
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

/* FIXME */
#define _bodystructure_mapper NULL

struct fill_env
{
  struct mu_imap_fetch_envelope *envelope;
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
  
  if (elt->type != imap_eltype_string)
    return EINVAL;
  if (mu_c_strcasecmp (elt->v.string, "NIL") == 0)
    p = NULL;
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
  if (arg && arg->type == imap_eltype_string && strcmp (arg->v.string, "NIL"))
    personal = arg->v.string;
  arg = _mu_imap_list_at (elt->v.list, 2);
  if (arg && arg->type == imap_eltype_string && strcmp (arg->v.string, "NIL"))
    local = arg->v.string;
  arg = _mu_imap_list_at (elt->v.list, 3);
  if (arg && arg->type == imap_eltype_string && strcmp (arg->v.string, "NIL"))
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
  if (elt->type != imap_eltype_list)
    {
      if (mu_c_strcasecmp (elt->v.string, "NIL") == 0)
	*paddr = NULL;
      else
	return EINVAL;
    }
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

  switch (env->n++)
    {
    case env_date:
      if (elt->type != imap_eltype_string)
	rc = MU_ERR_FAILURE;
      else
	{
	  if (mu_scan_datetime (elt->v.string,
				MU_DATETIME_SCAN_RFC822,
				&env->envelope->date,
				&env->envelope->tz, NULL))
	    rc = MU_ERR_FAILURE;
	  else
	    rc = 0;
	}
      break;
      
    case env_subject:
      rc = elt_to_string (elt, &env->envelope->subject);
      break;

    case env_from:
      rc = elt_to_address (elt, &env->envelope->from);
      break;
	
    case env_sender:
      rc = elt_to_address (elt, &env->envelope->sender);
      break;
      
    case env_reply_to:
      rc = elt_to_address (elt, &env->envelope->reply_to);
      break;
      
    case env_to:
      rc = elt_to_address (elt, &env->envelope->to);
      break;
      
    case env_cc:
      rc = elt_to_address (elt, &env->envelope->cc);
      break;
      
    case env_bcc:
      rc = elt_to_address (elt, &env->envelope->bcc);
      break;
      
    case env_in_reply_to:
      rc = elt_to_string (elt, &env->envelope->in_reply_to);
      break;
      
    case env_message_id:
      rc = elt_to_string (elt, &env->envelope->message_id);
      break;
    }
  return rc;
}
  
static int
_envelope_mapper (union mu_imap_fetch_response *resp,
		  struct imap_list_element *elt,
		  struct parse_response_env *parse_env)
{
  struct fill_env env;
  
  if (elt->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  env.envelope = &resp->envelope;
  env.n = 0;
  mu_list_foreach (elt->v.list, _fill_response, &env);
  return 0;
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
  { S("BODYSTRUCTURE"), },
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
  struct imap_list_element *elt = item;
  struct parse_response_env *env = data;

  switch (env->state)
    {
    case resp_kw:
      {
	int rc;
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
      
    case resp_body:
      if (_mu_imap_list_element_is_string (elt, "["))
	env->state = resp_body_section;
      else
	{
	  env->mapper = _bodystructure_mapper;
	  env->state = resp_val;
	}
      break;
      
    case resp_body_section:
      if (elt->type != imap_eltype_string)
	{
	  env->status = MU_ERR_PARSE;
	  return MU_ERR_FAILURE;
	}
      else if (strncmp (elt->v.string, "HEADER.FIELDS", 13) == 0)
	env->state = resp_body_hlist;
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
  return status;
}
