/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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
#include <errno.h>
#include <mailutils/nls.h>
#include <mailutils/sys/imap.h>

const char *_mu_imap_state_name[] = {
  N_("initial"),
  N_("non-authenticated"),
  N_("authenticated"),
  N_("selected"),
  N_("logout")
};
int _mu_imap_state_count = MU_ARRAY_SIZE (_mu_imap_state_name);

int
mu_imap_state (mu_imap_t imap, int *pstate)
{
  if (imap == NULL || pstate == NULL)
    return EINVAL;
  *pstate = imap->imap_state;
  return 0;
}

int
mu_imap_state_str (int state, const char **pstr)
{
  if (state < 0 || state >= _mu_imap_state_count)
    return EINVAL;
  *pstr = gettext (_mu_imap_state_name[state]);
  return 0;
}
