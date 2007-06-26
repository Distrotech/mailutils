/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2006, 2007 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

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
  struct recipient *p = xmalloc (sizeof (*p));
  p->addr = addr;
  p->isbcc = isbcc;
  if (!*list && (rc = mu_list_create (list)))
    {
      mu_error (_("Cannot create list: %s"), mu_strerror (rc));
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
  return strcasecmp (domain, p + 1) == 0;
}

int
mh_alias_expand (const char *str, mu_address_t *paddr, int *incl)
{
  size_t argc;
  char **argv;
  size_t i;
  char *buf;
  mu_address_t exaddr = NULL;

  if (incl)
    *incl = 0;
  mu_argcv_get (str, ",", NULL, &argc, &argv);
  for (i = 0; i < argc;)
    {
      if (i + 1 == argc)
	{
	  if (mh_alias_get_address (argv[i], &exaddr, incl) == 0)
	    {
	      free (argv[i]);
	      memcpy (&argv[i], &argv[i+1],
		      (argc - i + 1) * sizeof (argv[0]));
	      argc--;
	    }
	  else
	    i++;
	}
      else if (argv[i + 1][0] == ',')
	{
	  if (mh_alias_get_address (argv[i], &exaddr, incl) == 0)
	    {
	      free (argv[i]);
	      free (argv[i+1]);
	      memcpy (&argv[i], &argv[i+2],
		      (argc - i) * sizeof (argv[0]));
	      argc -= 2;
	    }
	  else
	    i += 2;
	}
      else
	{
	  for (; i < argc; i++)
	    if (argv[i][0] == ',')
	      {
		i++;
		break;
	      }
	}
    }

  if (argc)
    {
      int status;
      mu_argcv_string (argc, argv, &buf);
      if ((status = mu_address_create (paddr, buf)))
	mu_error (_("Bad address `%s': %s"), buf, mu_strerror (status));
      free (buf);
    }

  mu_argcv_free (argc, argv);
  
  mu_address_union (paddr, exaddr);
  mu_address_destroy (&exaddr);
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
  mu_list_do (*list, _destroy_recipient, NULL);
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
		  
int
mh_whom (const char *filename, int check)
{
  int rc = 0;
  mh_context_t *ctx;

  mh_read_aliases ();
  ctx = mh_context_create (filename, 1);
  if (mh_context_read (ctx))
    {
      mu_error (_("Malformed message"));
      rc = -1;
    }
  else
    {
      size_t count = 0;
      
      scan_addrs (mh_context_get_value (ctx, MU_HEADER_TO, NULL), 0);
      scan_addrs (mh_context_get_value (ctx, MU_HEADER_CC, NULL), 0);
      scan_addrs (mh_context_get_value (ctx, MU_HEADER_BCC, NULL), 1);

      if (local_rcp)
	{
	  printf ("  %s\n", _("-- Local Recipients --"));
	  mu_list_do (local_rcp, _print_local_recipient, &count);
	}

      if (network_rcp)
	{
	  printf ("  %s\n", _("-- Network Recipients --"));
	  mu_list_do (network_rcp, _print_recipient, &count);
	}

      if (count == 0)
	{
	  mu_error(_("No recipients"));
	  rc = -1;
	}
    }
  free (ctx);
  destroy_addrs (&network_rcp);
  destroy_addrs (&local_rcp);
  return rc;
}
