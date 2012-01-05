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

/* Remove range R from the set S.

   There are following basic cases:

   1. S does not contain R.

      Return imeediately, there is nothing to do here.

   2. S contains a range X that matches exactly R.

      Action: Remove X entirely and return.

   3. There is a range X in S such that R falls entirely within it.

      Action: Split X into two ranges:

         X.msg_beg   : R.msg_beg-1
	 R.msg_end+1 : X.msg_end

      and return.
      In border cases one of the new ranges may be empty.

   4. There is a range X which contains an initial subrange of R.

      Action: Remove the subrange from X. If the resulting range is
      empty, remove it. Continue iteration with X.msg_end:R.msg_end
      in place of R.

   5. There is a range X such that its initial subrange is contained
      in R.

      Action: Remove X.msg_beg:R.msg_end and return.

   6. There is a range X that is contained within R.

      Action: Remove X and continue.

There are two special cases.  First is when R.msg_end is 0, meaning the last
available message number in the mailbox.  In this case the problem is reduced
to either 2 or a combination of 4 and zero or more 6.  It is handled by the
sub_msgno_last function below.

Yet another special case is when the last element of S has msg_end == 0.  If
so, the problem is reduced to either 2 or 4.
*/

static int
sub_msgno_last (mu_msgset_t mset, size_t beg)
{
  int rc;
  struct mu_msgrange *range;

  if (beg == 1)
    mu_list_clear (mset->list);
  else
    {
      mu_iterator_t itr;
      
      rc = mu_list_get_iterator (mset->list, &itr);
      if (rc)
	return rc;
      rc = 1;
      rc = mu_iterator_ctl (itr, mu_itrctl_set_direction, &rc);
      if (rc)
	{
	  mu_iterator_destroy (&itr);
	  return rc;
	}
	  
      for (mu_iterator_first (itr); rc == 0 && !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  mu_iterator_current (itr, (void **)&range);
	  if (range->msg_beg > beg)
	    rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	  else if (range->msg_beg == beg)
	    {
	      rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	      break;
	    }
	  else
	    break;
	}
      mu_iterator_destroy (&itr);
    }
  return 0;
}

int
mu_msgset_sub_range (mu_msgset_t mset, size_t beg, size_t end, int mode)
{
  int rc;
  mu_iterator_t itr;
  struct mu_msgrange *mr;

  if (!mset)
    return EINVAL;
  if (mu_list_is_empty (mset->list))
    return MU_ERR_NOENT;
  if (end && beg > end)
    {
      size_t t = end;
      end = beg;
      beg = t;
    }

  rc = _mu_msgset_translate_pair (mset, mode, &beg, &end);
  if (rc == MU_ERR_NOENT)
    return 0;
  else if (rc)
    return rc;
  
  rc = mu_msgset_aggregate (mset);
  if (rc)
    return rc;

  if (end == MU_MSGNO_LAST)
    return sub_msgno_last (mset, beg);
  
  /* Test border cases */
  rc = mu_list_head (mset->list, (void**)&mr);
  if (rc)
    return rc;
  if (end < mr->msg_beg)
    return 0;
  if (beg < mr->msg_beg)
    beg = mr->msg_beg;
  
  rc = mu_list_tail (mset->list, (void**) &mr);
  if (mr->msg_end != MU_MSGNO_LAST)
    {
      if (beg > mr->msg_end)
	return 0;
      if (end > mr->msg_end)
	end = mr->msg_end;
    }

  rc = mu_list_get_iterator (mset->list, &itr);
  if (rc)
    return rc;
  for (mu_iterator_first (itr); rc == 0 && !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&mr);

      if (mr->msg_end == MU_MSGNO_LAST)
	{
	  /* This is the last element in list. */
	  if (mr->msg_beg == beg)
	    rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	  else if (mr->msg_beg > beg)
	    mr->msg_beg = end + 1;
	  break;
	}
      
      if (mr->msg_beg == beg && mr->msg_end == end) /* See case 2 above */
	{
	  rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	  break;
	}
      else if (mr->msg_beg <= beg && beg <= mr->msg_end) 
	{ 
	  if (mr->msg_beg <= end && end <= mr->msg_end) /* Case 3 */
	    {
	      /* Split the range */
	      if (end != mr->msg_end)
		{
		  struct mu_msgrange *newrange = calloc (1,
							 sizeof (*newrange));
		  if (!newrange)
		    {
		      rc = ENOMEM;
		      break;
		    }
		  newrange->msg_beg = end + 1;
		  newrange->msg_end = mr->msg_end;
		  rc = mu_iterator_ctl (itr, mu_itrctl_insert, newrange);
		  if (rc)
		    {
		      free (newrange);
		      break;
		    }
		}
	      
	      if (mr->msg_beg == beg)
		rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	      else
		mr->msg_end = beg - 1;
	      break;
	    }
	  else if (mr->msg_beg == beg)      /* Case 4 */
	    {
	      beg = mr->msg_end;
	      rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	    }
	  else
	    {
	      size_t n = mr->msg_end;
	      mr->msg_end = beg - 1;
	      beg = n;
	    }
	}
      else if (mr->msg_beg <= end && end <= mr->msg_end) /* Case 5 */
	{
	  mr->msg_beg = end + 1;
	  if (mr->msg_beg >= mr->msg_end)
	    rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	  break;
	}
      else if (beg <= mr->msg_beg && mr->msg_beg <= end)
	{
	  rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
	}
    }
  mu_iterator_destroy (&itr);
  
  return rc;
}
      
