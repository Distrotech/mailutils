/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

static list_t local_rcp;   /* Local recipients */
static list_t network_rcp; /* Network recipients */

static void
addrcp (list_t *list, char *addr, int isbcc)
{
  int rc;
  struct recipient *p = xmalloc (sizeof (*p));
  p->addr = addr;
  p->isbcc = isbcc;
  if (!*list && (rc = list_create (list)))
    {
      mh_error (_("Cannot create list: %s"), mu_strerror (rc));
      exit (1);
    }
  list_append (*list, p);
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
mh_alias_expand (char *str, address_t *paddr, int *incl)
{
  size_t argc;
  char **argv;
  size_t i;
  char *buf;
  address_t exaddr = NULL;
  
  argcv_get (str, ",", NULL, &argc, &argv);
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
      argcv_string (argc, argv, &buf);
      if (status = address_create (paddr, buf))
	mh_error (_("Bad address `%s': %s"), buf, mu_strerror (status));
      free (buf);
    }

  argcv_free (argc, argv);
  
  address_union (paddr, exaddr);
  address_destroy (&exaddr);
  return 0;
}


static void
scan_addrs (char *str, int isbcc)
{
  address_t addr = NULL;
  size_t i, count;
  char *buf;
  int rc;

  if (!str)
    return;

  mh_alias_expand (str, &addr, NULL);
    
  if (addr == NULL || address_get_count (addr, &count))
    return;
    
  for (i = 1; i <= count; i++)
    {
      char *p;

      rc = address_aget_email (addr, i, &buf);
      if (rc)
	{
	  mh_error ("address_aget_email: %s", mu_strerror (rc));
	  continue;
	}

      p = strchr (buf, '@');
     
      if (ismydomain (p))
	addrcp (&local_rcp, buf, isbcc);
      else
	addrcp (&network_rcp, buf, isbcc);
    }
  address_destroy (&addr);
  free (str); /* FIXME: This will disappear. Note comment to
		 mh_context_get_value! */
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
destroy_addrs (list_t *list)
{
  if (!*list)
    return;
  list_do (*list, _destroy_recipient, NULL);
  list_destroy (list);
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
mh_whom (char *filename, int check)
{
  int rc = 0;
  mh_context_t *ctx;

  mh_read_aliases ();
  ctx = mh_context_create (filename, 1);
  if (mh_context_read (ctx))
    {
      mh_error (_("Malformed message"));
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
	  list_do (local_rcp, _print_local_recipient, &count);
	}

      if (network_rcp)
	{
	  printf ("  %s\n", _("-- Network Recipients --"));
	  list_do (network_rcp, _print_recipient, &count);
	}

      if (count == 0)
	{
	  mh_error(_("No recipients"));
	  rc = -1;
	}
    }
  free (ctx);
  destroy_addrs (&network_rcp);
  destroy_addrs (&local_rcp);
  return rc;
}
