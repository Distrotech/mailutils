/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mail.local.h>

#ifdef USE_DBM

#define DEFRETVAL MQUOTA_UNLIMITED

size_t groupquota = 5*1024*1024UL;
static int get_size (char *, size_t *, char **);

int
get_size (char *str, size_t *size, char **endp)
{
  size_t s;

  s = strtol (str, &str, 0);
  switch (*str)
    {
    case 0:
      break;
    case 'k':
    case 'K':
      s *= 1024;
      break;
    case 'm':
    case 'M':
      s *= 1024*1024;
      break;
    default:
      *endp = str;
      return -1;
    }
  *size = s;
  return 0;
}

int
check_quota (char *name, size_t size, size_t *rest)
{
  DBM_FILE db;
  DBM_DATUM named, contentd;
  size_t quota;
  char buffer[64];
  int unlimited = 0;
  int rc;

  if (!quotadbname || mu_dbm_open (quotadbname, &db, MU_STREAM_READ, 0600)) 
    return DEFRETVAL;

  memset (&named, 0, sizeof named);
  memset (&contentd, 0, sizeof contentd);
  MU_DATUM_PTR (named) = name;
  MU_DATUM_SIZE (named) = strlen (name);
  rc = mu_dbm_fetch (db, named, &contentd);
  if (rc || !MU_DATUM_PTR (contentd))
    {
      /* User not in database, try default quota */
      memset (&named, 0, sizeof named);
      MU_DATUM_PTR (named) = "DEFAULT";
      MU_DATUM_SIZE (named) = strlen ("DEFAULT");
      rc = mu_dbm_fetch (db, named, &contentd);
      if (rc)
	{
	  /*mu_error (_("can't fetch data: %s"), strerror (rc));*/
	  return DEFRETVAL;
	}
      if (!MU_DATUM_PTR (contentd))
	return DEFRETVAL;
    }

  if (strncasecmp("none",
		       MU_DATUM_PTR (contentd),
		       MU_DATUM_SIZE (contentd)) == 0) 
      unlimited = 1;
  else if (MU_DATUM_SIZE (contentd) > sizeof(buffer)-1)
    {
      mu_error (_("mailbox quota for `%s' is too big: %d digits"),
		name, MU_DATUM_SIZE (contentd));
      quota = groupquota;
    }
  else
    {
      char *p;
		
      strncpy(buffer, MU_DATUM_PTR (contentd), MU_DATUM_SIZE (contentd));
      buffer[MU_DATUM_SIZE (contentd)] = 0;
      quota = strtoul (buffer, &p, 0);
      if (get_size (buffer, &quota, &p))
	{
	  mu_error (_("bogus mailbox quota for `%s' (near `%s')"), name, p);
	  quota = groupquota;
	}
    }

  mu_dbm_close (db);
  if (unlimited) 
    return MQUOTA_UNLIMITED;
  else if (quota < size)  /* Mailbox full */
    return MQUOTA_EXCEEDED;
	
  if (rest)
    *rest = quota - size;
	
  return MQUOTA_OK;
}

#endif
