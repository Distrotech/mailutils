/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

/* Comparator function for sorting the message set list. */
static int
compare_msgrange (const void *a, const void *b)
{
  struct mu_msgrange const *sa = a;
  struct mu_msgrange const *sb = b;
  
  if (sa->msg_end != sb->msg_end)
    {
      if (sa->msg_end == MU_MSGNO_LAST)
	return 1;
      if (sb->msg_end == MU_MSGNO_LAST)
	return -1;
    }
  
  if (sa->msg_beg < sb->msg_beg)
    return -1;
  if (sa->msg_beg > sb->msg_beg)
    return 1;

  if (sa->msg_end == sb->msg_end)
    return 0;

  if (sa->msg_end < sb->msg_end)
    return -1;
  return 1;
}

int
mu_msgset_aggregate (mu_msgset_t mset)
{
  int rc;
  mu_iterator_t itr;
  size_t count;
  struct mu_msgrange *prev = NULL, *mr;
  int dir;
  
  if (!mset)
    return EINVAL;

  if (mset->flags & _MU_MSGSET_AGGREGATED)
    return 0; /* nothing to do */
  
  rc = mu_list_count (mset->list, &count);
  if (rc)
    return rc;
  if (count < 2)
    return 0;
  
  mu_list_sort (mset->list, compare_msgrange);
  
  rc = mu_list_get_iterator (mset->list, &itr);
  if (rc)
    return rc;
  /* Set backward direction */
  dir = 1;
  rc = mu_iterator_ctl (itr, mu_itrctl_set_direction, &dir);
  if (rc)
    {
      mu_iterator_destroy (&itr);
      return rc;
    }

  mu_iterator_first (itr);
  mu_iterator_current (itr, (void **)&mr);
  
  if (mr->msg_end == MU_MSGNO_LAST)
    {
      struct mu_msgrange *last = mr;
      
      for (mu_iterator_next (itr);
	   rc == 0 && !mu_iterator_is_done (itr); mu_iterator_next (itr))
	{
	  mu_iterator_current (itr, (void **)&mr);
	  if (mr->msg_end == MU_MSGNO_LAST)
	    {
	      last->msg_beg = mr->msg_beg;
	      rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	    }
	  else if (mr->msg_beg >= last->msg_beg)
	    rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	  else if (mr->msg_end + 1 >= last->msg_beg)
	    {
	      last->msg_beg = mr->msg_beg;
	      rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	    }
	  else
	    break;
	}
    }

  if (rc == 0)
    {
      dir = 0;
      rc = mu_iterator_ctl (itr, mu_itrctl_set_direction, &dir);
      if (rc)
	{
	  mu_iterator_destroy (&itr);
	  return rc;
	}

      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  mu_iterator_current (itr, (void **)&mr);
	  if (mr->msg_end == MU_MSGNO_LAST)
	    break;
	  if (prev)
	    {
	      if (prev->msg_beg <= mr->msg_beg && mr->msg_beg <= prev->msg_end)
		{
		  /* Possible cases:
		     1. mr lies fully within prev:
		     mr->msg_end <= prev->msg_end
		     Action: mr deleted
		     2. mr overlaps with prev:
		     mr->msg_end > prev->msg_end
		     Action: prev->msg_end is set to mr->msg_end; mr deleted
		  */
		  if (mr->msg_end > prev->msg_end)
		    prev->msg_end = mr->msg_end;
		  rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
		  if (rc)
		    break;
		  continue;
		}
	      else if (prev->msg_end + 1 == mr->msg_beg)
		{
		  prev->msg_end = mr->msg_end;
		  rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
		  if (rc)
		    break;
		  continue;
		}
	    }
	  prev = mr;
	}
    }
  mu_iterator_destroy (&itr);

  if (rc == 0)
    mset->flags |= _MU_MSGSET_AGGREGATED;
  return rc;
}
