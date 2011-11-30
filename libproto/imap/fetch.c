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
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;

  if (imap->imap_state != MU_IMAP_STATE_SELECTED)
    return MU_ERR_SEQ;
  
  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s FETCH %s %s\r\n",
				 imap->tag_str, msgset, items);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_FETCH_RX;

    case MU_IMAP_FETCH_RX:
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
      imap->state = MU_IMAP_CONNECTED;
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
      free (resp->body.key);
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

static int
_uid_mapper (struct imap_list_element **elt,
	     union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  char *p;
  size_t uid;
  
  if (elt[1]->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  uid = strtoul (elt[1]->v.string, &p, 0);
  if (*p)
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_UID);
  if (rc)
    return rc;
  resp->uid.uid = uid;
  *return_resp = resp;
  return 0;
}
			 
static int
_size_mapper (struct imap_list_element **elt,
	      union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  char *p;
  size_t size;
  
  if (elt[1]->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  size = strtoul (elt[1]->v.string, &p, 0);
  if (*p)
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_RFC822_SIZE);
  if (rc)
    return rc;
  resp->rfc822_size.size = size;
  *return_resp = resp;
  return 0;
}

static int
_body_mapper (struct imap_list_element **elt,
	      union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  char *section, *p;
  size_t partc = 0;
  size_t *partv = NULL;
  
  if (elt[1]->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_BODY);
  if (rc)
    return rc;

  section = strchr (elt[0]->v.string, '[');
  if (section)
    {
      p = section;
      while (1)
	{
	  p = strchr (p, '.');
	  if (*p)
	    {
	      p++;
	      if (mu_isdigit (p[1]))
		{
		  partc++;
		  continue;
		}
	    }
	  
	  break;
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

  if (p)
    {
      size_t len = strlen (p);
      resp->body.key = malloc (len);
      if (!resp->body.key)
	{
	  free (resp->body.partv);
	  free (resp);
	  return ENOMEM;
	}
      len--;
      memcpy (resp->body.key, p, len);
      resp->body.key[len] = 0;
    }
  
  resp->body.text = strdup (elt[1]->v.string);
  if (!resp->body.text)
    {
      free (resp->body.key);
      free (resp->body.partv);
      free (resp);
      return ENOMEM;
    }
  *return_resp = resp;
  return 0;
}

static int
_rfc822_mapper (const char *key, struct imap_list_element *elt,
		union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  
  if (elt->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_BODY);
  if (rc)
    return rc;

  resp->body.partc = 0;
  resp->body.partv = NULL;

  resp->body.key = strdup (key);
  if (!resp->body.key)
    {
      free (resp);
      return ENOMEM;
    }
  
  resp->body.text = strdup (elt->v.string);
  if (!resp->body.text)
    {
      free (resp->body.key);
      free (resp->body.partv);
      free (resp);
      return ENOMEM;
    }
  *return_resp = resp;
  return 0;
}

static int
_rfc822_header_mapper (struct imap_list_element **elt,
		       union mu_imap_fetch_response **return_resp)
{
  return _rfc822_mapper ("HEADER", elt[1], return_resp);
}

static int
_rfc822_text_mapper (struct imap_list_element **elt,
		     union mu_imap_fetch_response **return_resp)
{
  return _rfc822_mapper ("TEXT", elt[1], return_resp);
}

static int
_flags_mapper (struct imap_list_element **elt,
	       union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  int flags;
  
  if (elt[1]->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  if (_mu_imap_collect_flags (elt[1], &flags))
    return MU_ERR_FAILURE;

  rc = alloc_response (&resp, MU_IMAP_FETCH_FLAGS);
  if (rc)
    return rc;
  resp->flags.flags = flags;
  *return_resp = resp;
  return 0;
}

static int
_date_mapper (struct imap_list_element **elt,
	      union mu_imap_fetch_response **return_resp)
{
  struct mu_imap_fetch_internaldate idate;
  union mu_imap_fetch_response *resp;
  int rc;
  const char *p;
  
  if (elt[1]->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  p = elt[1]->v.string;
  if (mu_parse_imap_date_time (&p, &idate.tm, &idate.tz))
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_INTERNALDATE);
  if (rc)
    return rc;
  resp->internaldate = idate;
  *return_resp = resp;
  return 0;
}

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
	  char const *p = elt->v.string;
	  if (mu_parse_imap_date_time (&p,
				       &env->envelope->date,
				       &env->envelope->tz))
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
_envelope_mapper (struct imap_list_element **elt,
		  union mu_imap_fetch_response **return_resp)
{
  union mu_imap_fetch_response *resp;
  int rc;
  struct fill_env env;
  
  if (elt[1]->type != imap_eltype_list)
    return MU_ERR_FAILURE;
  rc = alloc_response (&resp, MU_IMAP_FETCH_ENVELOPE);
  if (rc)
    return rc;
  env.envelope = &resp->envelope;
  env.n = 0;
  mu_list_foreach (elt[1]->v.list, _fill_response, &env);
  
  *return_resp = resp;
  return 0;
}

struct mapper_tab
{
  char *name;
  size_t size;
  int prefix;
  int (*mapper) (struct imap_list_element **, union mu_imap_fetch_response **);
};
  
static struct mapper_tab mapper_tab[] = {
#define S(s) s, (sizeof (s) - 1)
  { S("BODYSTRUCTURE"), 0, },
  { S("BODY["),         1, _body_mapper },
  { S("BODY"),          0, _body_mapper },
  { S("ENVELOPE"),      0, _envelope_mapper },
  { S("FLAGS"),         0, _flags_mapper },
  { S("INTERNALDATE"),  0, _date_mapper },
  { S("RFC822"),        0, _body_mapper},
  { S("RFC822.HEADER"), 0, _rfc822_header_mapper }, 
  { S("RFC822.SIZE"),   0, _size_mapper },
  { S("RFC822.TEXT"),   0, _rfc822_text_mapper },
  { S("UID"),           0, _uid_mapper },
#undef S
  { NULL }
};
  
static int
_fetch_mapper (void **itmv, size_t itmc, void *call_data)
{
  int *status = call_data;
  struct mapper_tab *mt;
  struct imap_list_element *elt[2];
  char *kw;
  size_t kwlen;
  union mu_imap_fetch_response *resp;

  elt[0] = itmv[0];
  elt[1] = itmv[1];
  
  if (elt[0]->type != imap_eltype_string)
    {
      *status = MU_ERR_FAILURE;
      return MU_LIST_MAP_STOP|MU_LIST_MAP_SKIP;
    }
  kw = elt[0]->v.string;
  kwlen = strlen (kw);
  for (mt = mapper_tab; mt->name; mt++)
    {
      if (mt->prefix)
	{
	  if (mt->size >= kwlen && memcmp (mt->name, kw, kwlen) == 0)
	    break;
	}
      else if (mt->size == kwlen && memcmp (mt->name, kw, kwlen) == 0)
	break;
    }

  if (!mt->name)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE9,
		("ignoring unknown FETCH item '%s'", kw));
      return MU_LIST_MAP_SKIP;
    }

  if (mt->mapper)
    {
      int rc = mt->mapper (elt, &resp);
      if (rc == 0)
	{
	  itmv[0] = resp;
	  return MU_LIST_MAP_OK;
	}
      else
	{
	  *status = rc;
	  return MU_LIST_MAP_STOP|MU_LIST_MAP_SKIP;
	}
    }
      
  return MU_LIST_MAP_SKIP;
}

int
_mu_imap_parse_fetch_response (mu_list_t input, mu_list_t *result_list)
{
  mu_list_t result;
  int status;

  status = mu_list_create (&result);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("mu_list_create: %s", mu_strerror (status)));
      return 1;
    }
  mu_list_set_destroy_item (result, _free_fetch_response);
  mu_list_map (input, _fetch_mapper, &status, 2, &result);
  if (status)
    mu_list_destroy (&result);
  else
    *result_list = result;
  
  return status;
}
