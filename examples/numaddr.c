/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils; if not, write to the Free Software
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

struct val_ctr {  /* Data passed to the counter function */
  header_t hdr;   /* Headers of the current message */
  size_t limit;   /* Limit for the number of addresses */
  size_t count;   /* Number of addresses counted so far */
};

/* Count addresses in a single header value.

   Input:
     ITEM is the name of the header to scan.
     DATA is a pointer to the val_ctr structure 
   Return value:
     non-zero if the limit on the number of addresses has been reached. */
     
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

/* Handler for the numaddr test */
static int
numaddr_test (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *h, *v;
  struct val_ctr vc;
  int rc;
  
  if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "NUMADDR\n");

  /* Retrieve required arguments: */
  /* First argument: list of header names */
  h = sieve_value_get (args, 0);
  if (!h)
    {
      sieve_error (mach, "numaddr: can't get argument 1");
      sieve_abort (mach);
    }
  /* Second argument: Limit on the number of addresses */
  v = sieve_value_get (args, 1);
  if (!v)
    {
      sieve_error (mach, "numaddr: can't get argument 2");
      sieve_abort (mach);
    }

  /* Fill in the val_ctr structure */
  message_get_header (sieve_get_message (mach), &vc.hdr);
  vc.count = 0;
  vc.limit = v->v.number;

  /* Count the addresses */
  rc = sieve_vlist_do (h, _count_items, &vc);

  /* Here rc >= 1 iff the counted number of addresses is greater or equal
     to vc.limit. If `:under' tag was given we reverse the return value */
  if (sieve_tag_lookup (tags, "under", NULL))
    rc = !rc;
  return rc;
}

/* Syntactic definitions for the numaddr test */

/* Required arguments: */
static sieve_data_type numaddr_req_args[] = {
  SVT_STRING_LIST,
  SVT_NUMBER,
  SVT_VOID
};

/* Tagged arguments: */
static sieve_tag_def_t numaddr_tags[] = {
  { "over", SVT_VOID },
  { "under", SVT_VOID },
  { NULL }
};

static sieve_tag_group_t numaddr_tag_groups[] = {
  { numaddr_tags, NULL },
  { NULL }
};

/* Initialization function. It is the only function exported from this
   module. */
int
SIEVE_EXPORT(numaddr,init) (sieve_machine_t mach)
{
  return sieve_register_test (mach, "numaddr", numaddr_test,
                              numaddr_req_args, numaddr_tag_groups, 1);
}
