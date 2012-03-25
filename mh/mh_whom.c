/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2007, 2009-2012 Free Software Foundation,
   Inc.

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

#include <mh.h>

struct recipient {
  char *addr;
  int isbcc;
};

static mu_list_t local_rcp;   /* Local recipients */
static mu_list_t network_rcp; /* Network recipients */

static void
addrcp (mu_list_t *list, char *addr, int isbcc)
{
  int rc;
  struct recipient *p = mu_alloc (sizeof (*p));
  p->addr = addr;
  p->isbcc = isbcc;
  if (!*list && (rc = mu_list_create (list)))
    {
      mu_error (_("cannot create list: %s"), mu_strerror (rc));
      exit (1);
    }
  mu_list_append (*list, p);
}

static int
ismydomain (char *p)
{
  const char *domain;
  if (!p)
    return 1;
  mu_get_user_email_domain (&domain);
  return mu_c_strcasecmp (domain, p + 1) == 0;
}

/* FIXME: incl is not used */
int
mh_alias_expand (const char *str, mu_address_t *paddr, int *incl)
{
  size_t i, count;
  mu_address_t addr;
  int status;
  
  if (!str || !*str)
    {
      *paddr = NULL;
      return 0;
    }

  if (incl)
    *incl = 0;
  status = mu_address_create_hint (&addr, str, NULL, 0);
  if (status)
    {
      mu_error (_("Bad address `%s': %s"), str, mu_strerror (status));
      return 1;
    }

  mu_address_get_count (addr, &count);
  for (i = 1; i <= count; i++)
    {
      mu_address_t subaddr = NULL;
      const char *key;

      if (mu_address_sget_domain (addr, i, &key) == 0 && key == NULL)
	{
	  if (mu_address_sget_local_part (addr, i, &key) ||
	      mh_alias_get_address (key, paddr, incl) == 0 ||
	      /* Recreating address from local part adds the default
		 domain to it: */
              mu_address_create (&subaddr, key))
            continue;
	}
      else 
        {
          status = mu_address_get_nth (addr, i, &subaddr);
          if (status)
	    {
	      mu_error (_("%s: cannot get address #%lu: %s"),
			str, (unsigned long) i, mu_strerror (status));
	      continue;
	    }
        }   
      mu_address_union (paddr, subaddr);
      mu_address_destroy (&subaddr);
    }
  return 0;
}


static void
scan_addrs (const char *str, int isbcc)
{
  mu_address_t addr = NULL;
  size_t i, count;
  char *buf;
  int rc;

  if (!str)
    return;

  mh_alias_expand (str, &addr, NULL);
    
  if (addr == NULL || mu_address_get_count (addr, &count))
    return;
    
  for (i = 1; i <= count; i++)
    {
      char *p;

      rc = mu_address_aget_email (addr, i, &buf);
      if (rc)
	{
	  mu_error ("mu_address_aget_email: %s", mu_strerror (rc));
	  continue;
	}

      p = strchr (buf, '@');
     
      if (ismydomain (p))
	addrcp (&local_rcp, buf, isbcc);
      else
	addrcp (&network_rcp, buf, isbcc);
    }
  mu_address_destroy (&addr);
}

static int
_destroy_recipient (void *item, void *unused_data)
{
  struct recipient *p = item;
  free (p->addr);
  free (p);
  return 0;
}

static void
destroy_addrs (mu_list_t *list)
{
  if (!*list)
    return;
  mu_list_foreach (*list, _destroy_recipient, NULL);
  mu_list_destroy (list);
}

/* Print an email in more readable form: localpart + "at" + domain */
static void
print_readable (char *email, int islocal)
{
  printf ("  ");
  for (; *email && *email != '@'; email++)
    putchar (*email);

  if (!*email || islocal)
    return;

  printf (_(" at %s"), email+1);
}

static int
_print_recipient (void *item, void *data)
{
  struct recipient *p = item;
  size_t *count = data;
  
  print_readable (p->addr, 0);
  if (p->isbcc)
    printf ("[BCC]");
  printf ("\n");
  (*count)++;
  return 0;
}

static int
_print_local_recipient (void *item, void *data)
{
  struct recipient *p = item;
  size_t *count = data;
  
  print_readable (p->addr, 1);
  if (p->isbcc)
    printf ("[BCC]");
  printf ("\n");
  (*count)++;
  return 0;
}

static mu_header_t
read_header (mu_stream_t stream)
{
  int rc;
  mu_stream_t flt;
  mu_off_t size;
  size_t total;
  char *blurb;
  mu_header_t hdr;
  
  rc = mu_stream_size (stream, &size);
  if (rc)
    {
      mu_error (_("cannot get stream size: %s"), mu_strerror (rc));
      exit (1);
    }
  
  rc = mu_filter_create (&flt, stream, "HEADER",
			 MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
      exit (1);
    }

  blurb = mu_alloc (size + 1);

  total = 0;
  while (1)
    {
      size_t n;
      
      rc = mu_stream_read (flt, blurb + total, size - total, &n);
      if (rc)
	break;
      if (n == 0)
	break;
      total += n;
    }

  mu_stream_destroy (&flt);
  if (rc)
    {
      free (blurb);
      mu_error (_("read error: %s"), mu_strerror (rc));
      exit (1);
    }

  rc = mu_header_create (&hdr, blurb, total);
  free (blurb);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_header_create", NULL, rc);
      exit (1);
    }
  return hdr;
}

int
mh_whom (const char *filename, int check)
{
  int rc = 0;

  if (access (filename, R_OK))
    {
      mu_error ("%s: %s", filename, mu_strerror (errno));
      rc = -1;
    }
  else
    {
      size_t count = 0;
      mu_header_t hdr;
      mu_stream_t str;
      int rc;
      const char *val;

      rc = mu_file_stream_create (&str, filename, MU_STREAM_READ);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create",
			   filename, rc);
	  exit (1);
	}
      hdr = read_header (str);
      mu_stream_unref (str);

      mh_read_aliases ();

      if (mu_header_sget_value (hdr, MU_HEADER_TO, &val) == 0)
	scan_addrs (val, 0);
      if (mu_header_sget_value (hdr, MU_HEADER_CC, &val) == 0)
	scan_addrs (val, 0);
      if (mu_header_sget_value (hdr, MU_HEADER_BCC, &val) == 0)
	scan_addrs (val, 1);

      if (local_rcp)
	{
	  printf ("  %s\n", _("-- Local Recipients --"));
	  mu_list_foreach (local_rcp, _print_local_recipient, &count);
	}

      if (network_rcp)
	{
	  printf ("  %s\n", _("-- Network Recipients --"));
	  mu_list_foreach (network_rcp, _print_recipient, &count);
	}

      if (count == 0)
	{
	  mu_error(_("no recipients"));
	  rc = -1;
	}
      mu_header_destroy (&hdr);
    }
  destroy_addrs (&network_rcp);
  destroy_addrs (&local_rcp);
  return rc;
}
