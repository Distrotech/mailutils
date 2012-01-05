/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mailutils/mailutils.h>
#include "mu.h"

int
get_bool (const char *str, int *pb)
{
  if (mu_c_strcasecmp (str, "yes") == 0
      || mu_c_strcasecmp (str, "on") == 0
      || mu_c_strcasecmp (str, "true") == 0)
    *pb = 1;
  else if (mu_c_strcasecmp (str, "no") == 0
      || mu_c_strcasecmp (str, "off") == 0
      || mu_c_strcasecmp (str, "false") == 0)
    *pb = 0;
  else
    return 1;

  return 0;
}

int
get_port (const char *port_str, int *pn)
{
  short port_num;
  long num;
  char *p;
  
  num = port_num = strtol (port_str, &p, 0);
  if (*p == 0)
    {
      if (num != port_num)
	{
	  mu_error ("bad port number: %s", port_str);
	  return 1;
	}
    }
  else
    {
      struct servent *sp = getservbyname (port_str, "tcp");
      if (!sp)
	{
	  mu_error ("unknown port name");
	  return 1;
	}
      port_num = ntohs (sp->s_port);
    }
  *pn = port_num;
  return 0;
}

