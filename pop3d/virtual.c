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


#include "pop3d.h"

#ifdef USE_VIRTUAL_DOMAINS

struct passwd *
getpwnam_ip_virtual (const char *u)
{
  struct sockaddr_in addr;
  struct passwd *pw = NULL;
  int len = sizeof (addr);
  if (getsockname (fileno (ifile), (struct sockaddr *)&addr, &len) == 0)
    {
      char *ip;
      char *user;
      ip = inet_ntoa (addr.sin_addr);
      user = malloc (strlen (ip) + strlen (u) + 2);
      if (user)
	{
	  sprintf (user, "%s!%s", u, ip);
	  pw = getpwnam_virtual (user);
	  free (user);
        }
    }
  return pw;
}

struct passwd *
getpwnam_host_virtual (const char *u)
{
  struct sockaddr_in addr;
  struct passwd *pw = NULL;
  int len = sizeof (addr);
  if (getsockname (fileno (ifile), (struct sockaddr *)&addr, &len) == 0)
    {
      struct hostent *info = gethostbyaddr ((char *)&addr.sin_addr,
					    4, AF_INET);
      if (info)
	{
	  char *user = malloc (strlen (info->h_name) + strlen (u) + 2);
	  if (user)
	    {
	      sprintf (user, "%s!%s", u, info->h_name);
	      pw = getpwnam_virtual (user);
	      free (user);
	    }
        }
    }
  return pw;
}

#endif
