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

/*  This is an example on how to write extension tests for GNU sieve.
    It provides test "numaddr".

    Syntax:   numaddr [":over" / ":under"] <header-names: string-list>
              <limit: number>

    The "numaddr" test counts Internet addresses in structured headers
    that contain addresses.  It returns true if the total number of
    addresses satisfies the requested relation:

    If the argument is ":over" and the number of addresses is greater than
    the number provided, the test is true; otherwise, it is false.

    If the argument is ":under" and the number of addresses is less than
    the number provided, the test is true; otherwise, it is false.

    If the argument is empty, ":over" is assumed. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdlib.h>
#include <mailutils/libsieve.h>

struct val_ctr {
  header_t hdr;
  size_t limit;
  size_t count;
};

static int
_count_items (void *item, void *data)
{
  char *name = item;
  struct val_ctr *vp = data;
  char *val;
  address_t addr;
  size_t count = 0;
  
  if (header_aget_value (vp->hdr, name, &val))
    return 0;
  if (address_create (&addr, val) == 0)
    {
      address_get_count (addr, &count);
      address_destroy (&addr);
      vp->count += count;
    }
  free (val);
  return vp->count >= vp->limit;
}
  
static int
numaddr_test (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v;
  struct val_ctr vc;
  int rc;
  
  if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "NUMADDR\n");

  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_error (mach, "numaddr: can't get argument 1");
      sieve_abort (mach);
    }
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_error (mach, "numaddr: can't get argument 2");
      sieve_abort (mach);
    }

  message_get_header (sieve_get_message (mach), &vc.hdr);
  vc.count = 0;
  vc.limit = v->v.number;

  rc = sieve_vlist_do (h, _count_items, &vc);
  
  if (sieve_tag_lookup (tags, "under", NULL))
    rc = !rc;
  return rc;
}

static sieve_data_type numaddr_req_args[] = {
  SVT_STRING_LIST,
  SVT_NUMBER,
  SVT_VOID
};

static sieve_tag_def_t numaddr_tags[] = {
  { "over", SVT_VOID },
  { "under", SVT_VOID },
  { NULL }
};

static sieve_tag_group_t numaddr_tag_groups[] = {
  { numaddr_tags, NULL },
  { NULL }
};


int
SIEVE_EXPORT(numaddr,init) (sieve_machine_t mach)
{
  return sieve_register_test (mach, "numaddr", numaddr_test,
			      numaddr_req_args, numaddr_tag_groups, 1);
}
