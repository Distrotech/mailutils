/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mu_scm.h"

#ifndef _PATH_SENDMAIL
# define _PATH_SENDMAIL "/usr/lib/sendmail"
#endif

SCM 
scm_makenum (unsigned long val)
{
  if (SCM_FIXABLE ((long) val))
    return SCM_MAKINUM (val);

#ifdef SCM_BIGDIG
  return scm_long2big (val);
#else /* SCM_BIGDIG */
  return scm_make_real ((double) val);
#endif /* SCM_BIGDIG */
}

SCM _mu_scm_mailer; /* STRING: Default mailer path. */
SCM _mu_scm_debug;  /* NUM: Default debug level. */

static struct
{
  char *name;
  int value;
} attr_kw[] = {
  { "MU-ATTRIBUTE-ANSWERED",  MU_ATTRIBUTE_ANSWERED },
  { "MU-ATTRIBUTE-FLAGGED",   MU_ATTRIBUTE_FLAGGED },
  { "MU-ATTRIBUTE-DELETED",   MU_ATTRIBUTE_DELETED }, 
  { "MU-ATTRIBUTE-DRAFT",     MU_ATTRIBUTE_DRAFT },   
  { "MU-ATTRIBUTE-SEEN",      MU_ATTRIBUTE_SEEN },    
  { "MU-ATTRIBUTE-READ",      MU_ATTRIBUTE_READ },    
  { "MU-ATTRIBUTE-MODIFIED",  MU_ATTRIBUTE_MODIFIED },
  { "MU-ATTRIBUTE-RECENT",    MU_ATTRIBUTE_RECENT },
  { NULL, 0 }
};
  

/* Initialize the library */
void
mu_scm_init ()
{
  SCM *scm_loc;
  char *defmailer;
  int i;
  
  asprintf (&defmailer, "sendmail:%s", _PATH_SENDMAIL);
  _mu_scm_mailer = scm_makfrom0str (defmailer);
  scm_loc = SCM_CDRLOC (scm_sysintern ("mu-mailer", SCM_EOL));
  *scm_loc = _mu_scm_mailer;

  _mu_scm_debug = scm_makenum(0);
  scm_loc = SCM_CDRLOC (scm_sysintern ("mu-debug", SCM_EOL));
  *scm_loc = _mu_scm_debug;

  /* Create MU- attribute names */
  for (i = 0; attr_kw[i].name; i++)
    scm_sysintern(attr_kw[i].name, SCM_MAKINUM(attr_kw[i].value));
  
  
  mu_scm_mailbox_init ();
  mu_scm_message_init ();
  mu_scm_body_init ();
}
