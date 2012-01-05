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
#include <mailutils/msgset.h>

/* Apply ACTION to each message number from MSGSET. */
int
mu_msgset_foreach_dir_msgno (mu_msgset_t msgset, int dir,
			     mu_msgset_msgno_action_t action,
			     void *data)
{
  return mu_msgset_foreach_num (msgset,
	       (dir ? MU_MSGSET_FOREACH_BACKWARD : MU_MSGSET_FOREACH_FORWARD)|
	       MU_MSGSET_NUM,
	       action, data);
}

int
mu_msgset_foreach_msgno (mu_msgset_t msgset,
			 mu_msgset_msgno_action_t action,
			 void *data)
{
  return mu_msgset_foreach_dir_msgno (msgset, 0, action, data);
}
