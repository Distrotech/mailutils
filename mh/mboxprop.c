/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

/* MH message sets. */

#include <mh.h>

/* Default value for missing "cur".  Valid values are 0 and 1. */
int mh_mailbox_cur_default = 0;

void
mh_mailbox_get_cur (mu_mailbox_t mbox, size_t *pcur)
{
  mu_property_t prop = NULL;
  const char *s;
  int rc = mu_mailbox_get_property (mbox, &prop);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_property", NULL, rc);
      exit (1);
    }
  rc = mu_property_sget_value (prop, "cur", &s);
  if (rc == MU_ERR_NOENT)
    *pcur = mh_mailbox_cur_default;
  else if (rc == 0)
    {
      char *p;
      *pcur = strtoul (s, &p, 10);
      if (*p)
        p = mu_str_skip_class (p, MU_CTYPE_SPACE);
      if (*p)
	{
	  mu_error (_("invalid \"cur\" value (%s)"), s);
	  *pcur = 1;
	}
    }
  else
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_sget_value", NULL, rc);
      exit (1);
    }
}
      
void
mh_mailbox_set_cur (mu_mailbox_t mbox, size_t cur)
{
  mu_property_t prop = NULL;
  int rc = mu_mailbox_get_property (mbox, &prop);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_property", NULL, rc);
      exit (1);
    }      
  rc = mu_property_set_value (prop, "cur", mu_umaxtostr (0, cur), 1);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_set_value", NULL, rc);
      exit (1);
    }
}

    
