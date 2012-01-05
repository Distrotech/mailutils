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
#include <mailutils/msgset.h>
#include <mailutils/mailbox.h>
#include <mailutils/sys/msgset.h>

int
_mu_msgset_translate_pair (mu_msgset_t mset, int mode,
			   size_t *pbeg, size_t *pend)
{
  if (mode != mset->flags && mset->mbox)
    {
      int cmd, rc;
      size_t n;
      size_t beg = *pbeg;
      size_t end = *pend;
      
      switch (mset->flags)
	{
	case MU_MSGSET_NUM:
	  cmd = MU_MAILBOX_UID_TO_MSGNO;
	  break;

	case MU_MSGSET_UID:
	  cmd = MU_MAILBOX_MSGNO_TO_UID;
	  break;

	default:
	  return EINVAL;
	}

      rc = mu_mailbox_translate (mset->mbox, cmd, beg, &n);
      if (rc)
	{
	  if (rc == MU_ERR_NOENT && cmd == MU_MAILBOX_UID_TO_MSGNO)
	    {
	      size_t x;
	      
	      if (end == MU_MSGNO_LAST)
		{
		  rc = mu_mailbox_uidnext (mset->mbox, &x);
		  if (rc)
		    return rc;
		}
	      else
		x = end;
	      for (; rc == MU_ERR_NOENT && beg < x; beg++)
		rc = mu_mailbox_translate (mset->mbox, cmd, beg, &n);
	    }
	  if (rc)
	    return rc;
	}

      *pbeg = n;
      
      if (beg == end)
	*pend = n;
      else if (end != MU_MSGNO_LAST)
	{
	  rc = mu_mailbox_translate (mset->mbox, cmd, end, &n);
	  if (rc == MU_ERR_NOENT && cmd == MU_MAILBOX_UID_TO_MSGNO)
	    {
	      for (; rc == MU_ERR_NOENT && beg < end; end--)
		rc = mu_mailbox_translate (mset->mbox, cmd, end, &n);
	      
	    }
	  if (rc)
	    return rc;
	  *pend = n;
	}
    }
  return 0;
}

int
_mu_msgset_translate_range (mu_msgset_t mset, int mode, struct mu_msgrange *r)
{
  return _mu_msgset_translate_pair (mset, mode, &r->msg_beg, &r->msg_end);
}

