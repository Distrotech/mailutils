/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

typedef int (*address_aget_t) __PMT ((address_t addr, size_t no, char **buf));

static int
_get_address_part (void *item, void *data)
{
  sieve_runtime_tag_t *t = item;
  address_aget_t ret;
  
  if (strcmp (t->tag, "all") == 0)
    ret =  address_aget_email;
  else if (strcmp (t->tag, "domain") == 0)
    ret =  address_aget_domain;
  else if (strcmp (t->tag, "localpart") == 0)
    ret = address_aget_local_part;
  *(address_aget_t*)data = ret;
  return ret != NULL;
}

address_aget_t
sieve_get_address_part (list_t tags)
{
  address_aget_t ret = address_aget_email;
  list_do (tags, _get_address_part, &ret);
  return ret;
}

/* Structure shared between address and envelope tests */
struct address_closure
{
  address_aget_t aget;     /* appropriate address_aget_ function */
  void *data;              /* Either header_t or envelope_t */
  address_t addr;          /* Obtained address */
};  

int
retrieve_address (void *item, void *data, int idx, char **pval)
{
  struct address_closure *ap = data;
  char *val;
  int rc;
      
  if (!ap->addr)
    {
      if (header_aget_value ((header_t)ap->data, (char*)item, &val))
	return 1;
      rc = address_create (&ap->addr, val);
      free (val);
      if (rc)
	return rc;
    }

  rc = ap->aget (ap->addr, idx+1, pval);
  if (rc)
    address_destroy (&ap->addr);
  return rc;
}

int
sieve_test_address (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v;
  header_t header = NULL;
  sieve_comparator_t comp = sieve_get_comparator (mach, tags);
  struct address_closure clos;
  int rc;
  
  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "ADDRESS\n");

  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_error (mach, "address: can't get argument 1");
      sieve_abort (mach);
    }
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_error (mach, "address: can't get argument 2");
      sieve_abort (mach);
    }

  message_get_header (sieve_get_message (mach), &header);
  clos.data = header;
  clos.aget = sieve_get_address_part (tags);
  clos.addr = NULL;
  rc = sieve_vlist_compare (h, v, comp, retrieve_address, &clos);
  address_destroy (&clos.addr);
  return rc;
}

int
retrieve_header (void *item, void *data, int idx, char **pval)
{
  if (idx == 0)
    return header_aget_value ((header_t) data, (char*)item, pval);
  return 1;
}

int
sieve_test_header (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v;
  header_t header = NULL;
  sieve_comparator_t comp = sieve_get_comparator (mach, tags);
  
  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "HEADER\n");

  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_error (mach, "header: can't get argument 1");
      sieve_abort (mach);
    }
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_error (mach, "header: can't get argument 2");
      sieve_abort (mach);
    }

  if (sieve_tag_lookup (tags, "mime", NULL))
    {
      int ismime = 0;

      message_is_multipart (mach->msg, &ismime);
      if (ismime)
	{
	  size_t i, nparts = 0;
	  
	  message_get_num_parts (mach->msg, &nparts);
	  for (i = 1; i <= nparts; i++)
	    {
	      message_t message = NULL;

	      if (message_get_part (mach->msg, i, &message) == 0)
		{
		  message_get_header (message, &header);
		  if (sieve_vlist_compare (h, v, comp,
					   retrieve_header, header))
		    return 1;
		}
	    }
	}
    }
  message_get_header (mach->msg, &header);
  return sieve_vlist_compare (h, v, comp, retrieve_header, header);
}

int
retrieve_envelope (void *item, void *data, int idx, char **pval)
{
  struct address_closure *ap = data;
  int rc;
      
  if (!ap->addr)
    {
      char buf[512];
      size_t n;
      
      if (strcasecmp ((char*)item, "from") != 0)
	return 1;

      if (envelope_sender ((envelope_t)ap->data, buf, sizeof(buf), &n))
	return 1;

      rc = address_create (&ap->addr, buf);
      if (rc)
	return rc;
    }

  rc = ap->aget (ap->addr, idx+1, pval);
  if (rc)
    address_destroy (&ap->addr);
  return rc;
}

int
sieve_test_envelope (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v;
  sieve_comparator_t comp = sieve_get_comparator (mach, tags);
  struct address_closure clos;
  int rc;
  
  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "HEADER\n");

  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_error (mach, "header: can't get argument 1");
      sieve_abort (mach);
    }
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_error (mach, "header: can't get argument 2");
      sieve_abort (mach);
    }

  message_get_envelope (sieve_get_message (mach), (envelope_t*)&clos.data);
  clos.aget = sieve_get_address_part (tags);
  clos.addr = NULL;
  rc = sieve_vlist_compare (h, v, comp, retrieve_envelope, &clos);
  address_destroy (&clos.addr);
  return rc;
}

int
sieve_test_size (sieve_machine_t mach, list_t args, list_t tags)
{
  int rc;
  sieve_runtime_tag_t *tag = NULL;
  size_t size;
  
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, "size: can't get argument!");
      sieve_abort (mach);
    }

  message_size (sieve_get_message (mach), &size);
  list_get (tags, 0, (void **)&tag);
  if (!tag)
    rc = size == val->v.number;
  else if (strcmp (tag->tag, "over") == 0)
    rc = size > val->v.number;
  else if (strcmp (tag->tag, "under") == 0)
    rc = size < val->v.number;

  return rc;
}

int
sieve_test_true (sieve_machine_t mach, list_t args, list_t tags)
{
  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "TRUE\n");
  return 1;
}

int
sieve_test_false (sieve_machine_t mach, list_t args, list_t tags)
{
  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "FALSE\n");
  return 0;
}

int
_test_exists (void *item, void *data)
{
  header_t hdr = data;
  size_t n;

  return header_get_value (hdr, (char*)item, NULL, 0, &n);
}
  
int
sieve_test_exists (sieve_machine_t mach, list_t args, list_t tags)
{
  header_t header = NULL;
  sieve_value_t *val;   

  if (mach->debug_level & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "EXISTS\n");

  message_get_header (sieve_get_message (mach), &header);
  val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, "exists: can't get argument!");
      sieve_abort (mach);
    }

  return sieve_vlist_do (val, _test_exists, header) == 0;
}

static sieve_tag_def_t address_part_tags[] = {
  { "localpart", SVT_VOID },
  { "domain", SVT_VOID },
  { "all", SVT_VOID },
  { NULL }
};

static sieve_tag_def_t match_part_tags[] = {
  { "is", SVT_VOID },
  { "contains", SVT_VOID },
  { "matches", SVT_VOID },
  { "regex", SVT_VOID },
  { "comparator", SVT_STRING },
  { NULL }
};

static sieve_tag_def_t size_tags[] = {
  { "over", SVT_VOID },
  { "under", SVT_VOID },
  { NULL }
};

static sieve_tag_def_t mime_tags[] = {
  { "mime", SVT_VOID },
  { NULL }
};

#define ADDRESS_PART_GROUP \
  { address_part_tags, NULL }

#define MATCH_PART_GROUP \
  { match_part_tags, sieve_match_part_checker }

#define SIZE_GROUP { size_tags, NULL }

#define MIME_GROUP \
  { mime_tags, NULL }

sieve_tag_group_t address_tag_groups[] = {
  ADDRESS_PART_GROUP,
  MATCH_PART_GROUP,
  { NULL }
};

sieve_data_type address_req_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

sieve_tag_group_t size_tag_groups[] = {
  SIZE_GROUP,
  { NULL }
};

sieve_data_type size_req_args[] = {
  SVT_NUMBER,
  SVT_VOID
};

sieve_tag_group_t envelope_tag_groups[] = {
  ADDRESS_PART_GROUP,
  MATCH_PART_GROUP,
  { NULL }
};

sieve_data_type exists_req_args[] = {
  SVT_STRING_LIST,
  SVT_VOID
};

sieve_tag_group_t header_tag_groups[] = {
  MATCH_PART_GROUP,
  MIME_GROUP,
  { NULL }
};

void
sieve_register_standard_tests (sieve_machine_t mach)
{
  sieve_register_test (mach, "false", sieve_test_false, NULL, NULL, 1);
  sieve_register_test (mach, "true", sieve_test_true, NULL, NULL, 1);
  sieve_register_test (mach, "address", sieve_test_address,
		       address_req_args, address_tag_groups, 1);
  sieve_register_test (mach, "size", sieve_test_size,
		       size_req_args, size_tag_groups, 1);
  sieve_register_test (mach, "envelope", sieve_test_envelope,
		       address_req_args, envelope_tag_groups, 1);
  sieve_register_test (mach, "exists", sieve_test_exists,
		       exists_req_args, NULL, 1);
  sieve_register_test (mach, "header", sieve_test_header,
		       address_req_args, header_tag_groups, 1);
}
