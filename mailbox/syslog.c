/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program. If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <syslog.h>
#include <string.h>

struct kw_int
{
  char *name;
  int tok;
};

static int
syslog_to_n (struct kw_int *kw, char *str, int *pint)
{
  int i;

  if (strncasecmp (str, "LOG_", 4) == 0)
    str += 4;

  for (; kw->name; kw++)
    if (strcasecmp (kw->name, str) == 0)
      {
	*pint = kw->tok;
	return 0;
      }
  return 1;
}

static const char *
n_to_syslog (struct kw_int *kw, int n)
{
  for (; kw->name; kw++)
    if (kw->tok == n)
      return kw->name;
  return NULL;
}

static struct kw_int kw_facility[] = {
  { "USER",    LOG_USER },   
  { "DAEMON",  LOG_DAEMON },
  { "AUTH",    LOG_AUTH },
  { "AUTHPRIV",LOG_AUTHPRIV },
  { "MAIL",    LOG_MAIL },
  { "CRON",    LOG_CRON },
  { "LOCAL0",  LOG_LOCAL0 },
  { "LOCAL1",  LOG_LOCAL1 },
  { "LOCAL2",  LOG_LOCAL2 },
  { "LOCAL3",  LOG_LOCAL3 },
  { "LOCAL4",  LOG_LOCAL4 },
  { "LOCAL5",  LOG_LOCAL5 },
  { "LOCAL6",  LOG_LOCAL6 },
  { "LOCAL7",  LOG_LOCAL7 },
  { NULL }
};

int
mu_string_to_syslog_facility (char *str, int *pfacility)
{
  return syslog_to_n (kw_facility, str, pfacility);
}

const char *
mu_syslog_facility_to_string (int n)
{
  return n_to_syslog (kw_facility, n);
}

static struct kw_int kw_prio[] = {
  { "EMERG", LOG_EMERG },
  { "ALERT", LOG_ALERT },
  { "CRIT", LOG_CRIT },
  { "ERR", LOG_ERR },
  { "WARNING", LOG_WARNING },
  { "NOTICE", LOG_NOTICE },
  { "INFO", LOG_INFO },
  { "DEBUG", LOG_DEBUG },
  { NULL }
};

int
mu_string_to_syslog_priority (char *str, int *pprio)
{
  return syslog_to_n (kw_prio, str, pprio);
}

const char *
mu_syslog_priority_to_string (int n)
{
  return n_to_syslog (kw_prio, n);
}


int log_facility;
