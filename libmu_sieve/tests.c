/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007-2012, 2014-2016 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sieve-priv.h>

typedef int (*address_aget_t) (mu_address_t addr, size_t no, char **buf);

address_aget_t
sieve_get_address_part (mu_sieve_machine_t mach)
{
  size_t i;
  
  for (i = 0; i < mach->tagcount; i++)
    {
      mu_sieve_value_t *t = mu_sieve_get_tag_n (mach, i);
      if (strcmp (t->tag, "all") == 0)
	return mu_address_aget_email;
      else if (strcmp (t->tag, "domain") == 0)
	return  mu_address_aget_domain;
      else if (strcmp (t->tag, "localpart") == 0)
	return mu_address_aget_local_part;
    }
  /* RFC 3028, 2.7.4.   Comparisons Against Addresses:
     If an optional address-part is omitted, the default is ":all". */
  return mu_address_aget_email;
}

/* Structure shared between address and envelope tests */
struct address_closure
{
  address_aget_t aget;     /* appropriate address_aget_ function */
  void *data;              /* Either mu_header_t or mu_envelope_t */
  mu_address_t addr;       /* Obtained address */
};  

static int
retrieve_address (void *item, void *data, size_t idx, char **pval)
{
  struct address_closure *ap = data;
  char *val;
  int rc;
      
  if (!ap->addr)
    {
      rc = mu_header_aget_value ((mu_header_t)ap->data, (char*)item, &val);
      if (rc)
	return rc;
      rc = mu_address_create (&ap->addr, val);
      free (val);
      if (rc)
	return rc;
    }

  rc = ap->aget (ap->addr, idx+1, pval);
  if (rc)
    mu_address_destroy (&ap->addr);
  return rc;
}

int
sieve_test_address (mu_sieve_machine_t mach)
{
  mu_sieve_value_t *h, *v;
  mu_header_t header = NULL;
  struct address_closure clos;
  int rc;
  
  h = mu_sieve_get_arg_untyped (mach, 0);
  v = mu_sieve_get_arg_untyped (mach, 1);

  mu_message_get_header (mu_sieve_get_message (mach), &header);
  clos.data = header;
  clos.aget = sieve_get_address_part (mach);
  clos.addr = NULL;
  rc = mu_sieve_vlist_compare (mach, h, v, retrieve_address, NULL, &clos);
  mu_address_destroy (&clos.addr);
  
  return rc;
}

struct header_closure
{
  mu_message_t message;
  size_t nparts;
  
  size_t part;
  mu_header_t header;
  size_t index;
};

int
retrieve_header (void *item, void *data, size_t idx, char **pval)
{
  struct header_closure *hc = data;
  char const *hname;
  int rc;
  
  if (idx == 0)
    {
      rc = mu_message_get_header (hc->message, &hc->header);
      if (rc)
	return rc;
      hc->index = 1;
      hc->part = 1;
    }
  
  do
    {
      if (!hc->header)
	{
	  if (hc->part <= hc->nparts)
	    {
	      mu_message_t msg;
	      rc = mu_message_get_part (hc->message, hc->part, &msg);
	      if (rc)
		return rc;
	      hc->part++;
	      rc = mu_message_get_header (msg, &hc->header);
	      if (rc)
		return rc;
	      hc->index = 1;
	    }
	  else
	    return 1;
	}
  
      while (!mu_header_sget_field_name (hc->header, hc->index, &hname))
	{
	  int i = hc->index++;
	  if (mu_c_strcasecmp (hname, (char*)item) == 0)
	    {
	      if (mu_header_aget_field_value_unfold (hc->header, i, pval))
		return -1;
	      return 0;
	    }
	}

      hc->header = NULL;
    }
  while (hc->part <= hc->nparts);
  
  return MU_ERR_NOENT;
}

int
sieve_test_header (mu_sieve_machine_t mach)
{
  mu_sieve_value_t *h, *v;
  int rc;
  struct header_closure clos;
  
  h = mu_sieve_get_arg_untyped (mach, 0);
  v = mu_sieve_get_arg_untyped (mach, 1);

  clos.message = mach->msg;
  
  if (mu_sieve_get_tag (mach, "mime", SVT_VOID, NULL))
    {
      int ismime = 0;

      mu_message_is_multipart (mach->msg, &ismime);
      if (ismime)
	mu_message_get_num_parts (mach->msg, &clos.nparts);
    }
  else
    clos.nparts = 0;
  
  rc = mu_sieve_vlist_compare (mach, h, v, retrieve_header, NULL, &clos);
  return rc;
}

int
retrieve_envelope (void *item, void *data, size_t idx, char **pval)
{
  struct address_closure *ap = data;
  int rc;
      
  if (!ap->addr)
    {
      const char *buf;
      
      if (mu_c_strcasecmp ((char*)item, "from") != 0)
	return MU_ERR_NOENT;

      rc = mu_envelope_sget_sender ((mu_envelope_t)ap->data, &buf);
      if (rc)
	return rc;

      rc = mu_address_create (&ap->addr, buf);
      if (rc)
	return rc;
    }

  rc = ap->aget (ap->addr, idx+1, pval);
  if (rc)
    mu_address_destroy (&ap->addr);
  return rc;
}

int
sieve_test_envelope (mu_sieve_machine_t mach)
{
  mu_sieve_value_t *h, *v;
  struct address_closure clos;
  int rc;
  
  h = mu_sieve_get_arg_untyped (mach, 0);
  v = mu_sieve_get_arg_untyped (mach, 1);

  mu_message_get_envelope (mu_sieve_get_message (mach),
			   (mu_envelope_t*)&clos.data);
  clos.aget = sieve_get_address_part (mach);
  clos.addr = NULL;
  rc = mu_sieve_vlist_compare (mach, h, v, retrieve_envelope, NULL, &clos);
  mu_address_destroy (&clos.addr);
  return rc;
}

int
sieve_test_size (mu_sieve_machine_t mach)
{
  int rc = 1;
  size_t size;
  size_t arg;
  
  mu_sieve_get_arg (mach, 0, SVT_NUMBER, &arg);
  mu_message_size (mu_sieve_get_message (mach), &size);
  if (mach->tagcount)
    {
      mu_sieve_value_t *tag = mu_sieve_get_tag_n (mach, 0);
      if (strcmp (tag->tag, "over") == 0)
	rc = size > arg;
      else if (strcmp (tag->tag, "under") == 0)
	rc = size < arg;
      else
	abort ();
    }
  else
    rc = size == arg;

  return rc;
}

int
_test_exists (void *item, void *data)
{
  mu_header_t hdr = data;
  size_t n;

  return mu_header_get_value (hdr, (char*)item, NULL, 0, &n);
}
  
int
sieve_test_exists (mu_sieve_machine_t mach)
{
  mu_header_t header = NULL;
  mu_sieve_value_t *val;   

  mu_message_get_header (mu_sieve_get_message (mach), &header);
  val = mu_sieve_get_arg_untyped (mach, 0);
  return mu_sieve_vlist_do (mach, val, _test_exists, header) == 0;
}

static mu_sieve_tag_def_t address_part_tags[] = {
  { "localpart", SVT_VOID },
  { "domain", SVT_VOID },
  { "all", SVT_VOID },
  { NULL }
};

mu_sieve_tag_def_t mu_sieve_match_part_tags[] = {
  { "is", SVT_VOID },
  { "contains", SVT_VOID },
  { "matches", SVT_VOID },
  { "regex", SVT_VOID },
  { "count", SVT_STRING },
  { "value", SVT_STRING },
  { "comparator", SVT_STRING },
  { NULL }
};

static mu_sieve_tag_def_t size_tags[] = {
  { "over", SVT_VOID },
  { "under", SVT_VOID },
  { NULL }
};

static mu_sieve_tag_def_t mime_tags[] = {
  { "mime", SVT_VOID },
  { NULL }
};

#define ADDRESS_PART_GROUP \
  { address_part_tags, NULL }

#define MATCH_PART_GROUP \
  { mu_sieve_match_part_tags, mu_sieve_match_part_checker }

#define SIZE_GROUP { size_tags, NULL }

#define MIME_GROUP \
  { mime_tags, NULL }

mu_sieve_tag_group_t address_tag_groups[] = {
  ADDRESS_PART_GROUP,
  MATCH_PART_GROUP,
  { NULL }
};

mu_sieve_data_type address_req_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

mu_sieve_tag_group_t size_tag_groups[] = {
  SIZE_GROUP,
  { NULL }
};

mu_sieve_data_type size_req_args[] = {
  SVT_NUMBER,
  SVT_VOID
};

mu_sieve_tag_group_t envelope_tag_groups[] = {
  ADDRESS_PART_GROUP,
  MATCH_PART_GROUP,
  { NULL }
};

mu_sieve_data_type exists_req_args[] = {
  SVT_STRING_LIST,
  SVT_VOID
};

mu_sieve_tag_group_t header_tag_groups[] = {
  MATCH_PART_GROUP,
  MIME_GROUP,
  { NULL }
};

void
mu_i_sv_register_standard_tests (mu_sieve_machine_t mach)
{
  /* true and false are built-ins */
  mu_sieve_register_test (mach, "address", sieve_test_address,
			  address_req_args, address_tag_groups, 1);
  mu_sieve_register_test (mach, "size", sieve_test_size,
			  size_req_args, size_tag_groups, 1);
  mu_sieve_register_test (mach, "envelope", sieve_test_envelope,
			  address_req_args, envelope_tag_groups, 1);
  mu_sieve_register_test (mach, "exists", sieve_test_exists,
			  exists_req_args, NULL, 1);
  mu_sieve_register_test (mach, "header", sieve_test_header,
			  address_req_args, header_tag_groups, 1);
}
