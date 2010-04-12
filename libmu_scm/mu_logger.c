/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2006, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

#include <syslog.h>

static char *log_tag;

SCM_DEFINE_PUBLIC (scm_mu_openlog, "mu-openlog", 3, 0, 0,
	   (SCM IDENT, SCM OPTION, SCM FACILITY),
"Opens a connection to the system logger for Guile program.\n"
"IDENT, OPTION and FACILITY have the same meaning as in openlog(3)")
#define FUNC_NAME s_scm_mu_openlog
{
  SCM_ASSERT (scm_is_string (IDENT), IDENT, SCM_ARG1, FUNC_NAME);
  if (log_tag)
    free (log_tag);
  log_tag = scm_to_locale_string(IDENT);
	
  SCM_ASSERT (scm_is_integer (OPTION), OPTION, SCM_ARG2, FUNC_NAME);
  SCM_ASSERT (scm_is_integer (FACILITY), FACILITY, SCM_ARG3, FUNC_NAME);
  openlog (log_tag, scm_to_int (OPTION), scm_to_int (FACILITY));
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_logger, "mu-logger", 2, 0, 0,
	   (SCM PRIO, SCM TEXT),
	   "Distributes TEXT via syslogd priority PRIO.")
#define FUNC_NAME s_scm_mu_logger
{
  int prio;
  char *str;

  SCM_ASSERT (scm_is_integer (PRIO), PRIO, SCM_ARG1, FUNC_NAME);
  prio = scm_to_int (PRIO);
  
  SCM_ASSERT (scm_is_string (TEXT), TEXT, SCM_ARG2, FUNC_NAME);
  str = scm_to_locale_string (TEXT);
  syslog (prio, "%s", str);
  free (str);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_closelog, "mu-closelog", 0, 0, 0,
	   (),
	   "Closes the channel to the system logger opened by @code{mu-openlog}.")
#define FUNC_NAME s_scm_mu_closelog
{
  closelog ();
  if (log_tag)
    {
      free (log_tag);
      log_tag = NULL;
    }
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


static struct
{
  char *name;
  int facility;
} syslog_kw[] = {
  { "LOG_USER",    LOG_USER },   
  { "LOG_DAEMON",  LOG_DAEMON },
  { "LOG_AUTH",	   LOG_AUTH },  
  { "LOG_LOCAL0",  LOG_LOCAL0 },
  { "LOG_LOCAL1",  LOG_LOCAL1 },
  { "LOG_LOCAL2",  LOG_LOCAL2 },
  { "LOG_LOCAL3",  LOG_LOCAL3 },
  { "LOG_LOCAL4",  LOG_LOCAL4 },
  { "LOG_LOCAL5",  LOG_LOCAL5 },
  { "LOG_LOCAL6",  LOG_LOCAL6 },
  { "LOG_LOCAL7",  LOG_LOCAL7 },
  /* severity */
  { "LOG_EMERG",   LOG_EMERG },    
  { "LOG_ALERT",   LOG_ALERT },   
  { "LOG_CRIT",	   LOG_CRIT },    
  { "LOG_ERR",	   LOG_ERR },     
  { "LOG_WARNING", LOG_WARNING }, 
  { "LOG_NOTICE",  LOG_NOTICE },  
  { "LOG_INFO",	   LOG_INFO },    
  { "LOG_DEBUG",   LOG_DEBUG },   
  /* options */
  { "LOG_CONS",    LOG_CONS },   
  { "LOG_NDELAY",  LOG_NDELAY }, 
  { "LOG_PID",     LOG_PID }
};

void
mu_scm_logger_init ()
{
  int i;
  
  for (i = 0; i < sizeof (syslog_kw)/sizeof (syslog_kw[0]); i++)
    {
      scm_c_define (syslog_kw[i].name, scm_from_int (syslog_kw[i].facility));
      scm_c_export (syslog_kw[i].name, NULL);
    }
#include <mu_logger.x>
}
