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

#define TAG_LOCALPART   0
#define TAG_DOMAIN      1
#define TAG_ALL         2
#define TAG_COMPARATOR  3
#define TAG_IS          4
#define TAG_CONTAINS    5
#define TAG_MATCHES     6  
#define TAG_REGEX       7 
#define TAG_UNDER       8
#define TAG_OVER        9

int
sieve_test_address (sieve_machine_t mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_test_header (sieve_machine_t mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_test_envelope (sieve_machine_t mach, list_t args, list_t tags)
{
  return 0;
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
  else if (tag->tag == TAG_OVER)
    rc = size > val->v.number;
  else if (tag->tag == TAG_UNDER)
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

  return list_do (val->v.list, _test_exists, header) == 0;
}

#define ADDRESS_PART \
  { "localpart", TAG_LOCALPART, SVT_VOID },\
  { "domain", TAG_DOMAIN, SVT_VOID },\
  { "all", TAG_ALL, SVT_VOID }

#define MATCH_PART \
  { "is", TAG_IS, SVT_VOID },\
  { "contains", TAG_CONTAINS, SVT_VOID },\
  { "matches", TAG_MATCHES, SVT_VOID },\
  { "regex", TAG_REGEX, SVT_VOID }

#define COMP_PART \
  { "comparator", TAG_COMPARATOR, SVT_STRING }

#define SIZE_PART \
  { "under", TAG_UNDER, SVT_VOID },\
  { "over", TAG_OVER, SVT_VOID }
    
  
sieve_tag_def_t address_tags[] = {
  ADDRESS_PART,
  COMP_PART,
  MATCH_PART,
  { NULL }
};

sieve_data_type address_req_args[] = {
  SVT_STRING_LIST,
  SVT_STRING_LIST,
  SVT_VOID
};

sieve_tag_def_t size_tags[] = {
  SIZE_PART,
  { NULL }
};

sieve_data_type size_req_args[] = {
  SVT_NUMBER,
  SVT_VOID
};

sieve_tag_def_t envelope_tags[] = {
  COMP_PART,
  ADDRESS_PART,
  MATCH_PART,
  { NULL }
};

sieve_data_type exists_req_args[] = {
  SVT_STRING_LIST,
  SVT_VOID
};

sieve_tag_def_t header_tags[] = {
  COMP_PART,
  MATCH_PART,
  { NULL },
};

void
sieve_register_standard_tests ()
{
  sieve_register_test ("false", sieve_test_false, NULL, NULL, 1);
  sieve_register_test ("true", sieve_test_true, NULL, NULL, 1);
  sieve_register_test ("address", sieve_test_address,
		       address_req_args, address_tags, 1);
  sieve_register_test ("size", sieve_test_size,
		       size_req_args, size_tags, 1);
  sieve_register_test ("envelope", sieve_test_envelope,
		       address_req_args, envelope_tags, 1);
  sieve_register_test ("exists", sieve_test_exists,
		       exists_req_args, NULL, 1);
  sieve_register_test ("header", sieve_test_header,
		       address_req_args, header_tags, 1);
}
