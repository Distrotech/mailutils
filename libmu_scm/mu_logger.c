/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

SCM_DEFINE(mu_openlog, "mu-openlog", 3, 0, 0,
	   (SCM IDENT, SCM OPTION, SCM FACILITY),
	   "Opens a connection to the system logger for Guile program.")
#define FUNC_NAME s_mu_openlog
{
  char *ident;
  int option, facility;

  if (IDENT == SCM_BOOL_F)
    ident = "libmu_scm";
  else
    {
      SCM_ASSERT (SCM_NIMP (IDENT) && SCM_STRINGP (IDENT),
		 IDENT, SCM_ARG1, FUNC_NAME);
      ident = SCM_STRING_CHARS (IDENT);
    }
	
  if (SCM_IMP (OPTION) && SCM_INUMP (OPTION))
    option = SCM_INUM (OPTION);
  else if (SCM_BIGP (OPTION)) 
    option = (int) scm_i_big2dbl (OPTION);
  else 
    SCM_ASSERT (0, OPTION, SCM_ARG2, FUNC_NAME);

  if (SCM_IMP (FACILITY) && SCM_INUMP (FACILITY)) 
    facility = SCM_INUM (FACILITY);
  else if (SCM_BIGP (FACILITY)) 
    facility = (int) scm_i_big2dbl (FACILITY);
  else
    SCM_ASSERT (0, FACILITY, SCM_ARG3, FUNC_NAME);

  openlog (ident, option, facility);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_logger, "mu-logger", 2, 0, 0,
	   (SCM PRIO, SCM TEXT),
	   "Generates a log message to be distributed via syslogd.")
#define FUNC_NAME s_mu_logger
{
  int prio;

  if (PRIO == SCM_BOOL_F) 
    prio = LOG_INFO;
  else if (SCM_IMP (PRIO) && SCM_INUMP (PRIO)) 
    prio = SCM_INUM (PRIO);
  else if (SCM_BIGP (PRIO)) 
    prio = (int) scm_i_big2dbl (PRIO);
  else
    SCM_ASSERT (0, PRIO, SCM_ARG1, FUNC_NAME);
  
  SCM_ASSERT (SCM_NIMP (TEXT) && SCM_STRINGP (TEXT),
	     TEXT, SCM_ARG2, FUNC_NAME);
  syslog (prio, "%s", SCM_STRING_CHARS (TEXT));
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_closelog, "mu-closelog", 0, 0, 0,
	   (),
	   "Closes the channel to the system logger open by mu-openlog.")
#define FUNC_NAME s_mu_closelog
{
  closelog ();
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
    scm_c_define (syslog_kw[i].name, SCM_MAKINUM (syslog_kw[i].facility));
#include <mu_logger.x>
}
