/* This file is part of GNU Mailutils.
   Copyright (C) 1998, 2001, 2002, 2005, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; see the file COPYING.  If not, write
   to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301 USA. */

#include "comsat.h"

typedef struct netdef netdef_t;

struct netdef
{
  netdef_t *next;
  unsigned int ipaddr;
  unsigned int netmask;
};

#define ACT_ALLOW  0
#define ACT_DENY   1

typedef struct acl acl_t;

struct acl
{
  acl_t *next;
  netdef_t *netlist;
  int action;
};


acl_t *acl_head, *acl_tail;

#define DOTTED_QUAD_LEN 16

static int
read_address (char **line_ptr, char *ptr)
{
  char *startp = *line_ptr;
  char *endp;
  int dotcount = 0;

  for (endp = startp; *endp; endp++, ptr++)
    if (!(isdigit (*endp) || *endp == '.'))
      break;
    else if (endp < startp + DOTTED_QUAD_LEN)
      {
	if (*endp == '.')
	  dotcount++;
	*ptr = *endp;
      }
    else
      break;
  *line_ptr = endp;
  *ptr = 0;
  return dotcount;
}

static netdef_t *
netdef_parse (char *str)
{
  unsigned int ipaddr, netmask;
  netdef_t *netdef;
  char ipbuf[DOTTED_QUAD_LEN+1];

  if (strcmp (str, "any") == 0)
    {
      ipaddr = 0;
      netmask = 0;
    }
  else
    {
      read_address (&str, ipbuf);
      ipaddr = inet_addr (ipbuf);
      if (ipaddr == INADDR_NONE)
	return NULL;
      if (*str == 0)
	netmask = 0xfffffffful;
      else if (*str != '/')
	return NULL;
      else
	{
	  str++;
	  if (read_address (&str, ipbuf) == 0)
	    {
	      /* netmask length */
	      unsigned int len = strtoul (ipbuf, NULL, 0);
	      if (len > 32)
		return NULL;
	      netmask = 0xfffffffful >> (32-len);
	      netmask <<= (32-len);
	    }
	  else
	    netmask = inet_network (ipbuf);
	  netmask = htonl (netmask);
	}
    }

  netdef = malloc (sizeof *netdef);
  if (!netdef)
    {
      mu_error (_("Out of memory"));
      exit (1);
    }

  netdef->next = NULL;
  netdef->ipaddr = ipaddr;
  netdef->netmask = netmask;

  return netdef;
}

void
read_config (const char *config_file)
{
  FILE *fp;
  int line;
  char buf[128];
  char *ptr;

  if (!config_file)
    return;

  fp = fopen (config_file, "r");
  if (!fp)
    {
      mu_error (_("Cannot open config file %s: %s"), config_file,
		mu_strerror (errno));
      return;
    }

  line = 0;
  while ((ptr = fgets (buf, sizeof buf, fp)))
    {
      int len, i;
      int argc;
      char **argv;
      int  action;
      netdef_t *head, *tail;
      acl_t *acl;

      line++;
      len = strlen (ptr);
      if (len > 0 && ptr[len-1] == '\n')
	ptr[--len] = 0;

      while (*ptr && isspace (*ptr))
	ptr++;
      if (!*ptr || *ptr == '#')
	continue;

      mu_argcv_get (ptr, "", NULL, &argc, &argv);
      if (argc < 2)
	{
	  mu_error (_("%s:%d: too few fields"), config_file, line);
	  mu_argcv_free (argc, argv);
	  continue;
	}

      if (strcmp (argv[0], "allow-biffrc") == 0)
	{
	  if (strcmp (argv[1], "yes") == 0)
	    allow_biffrc = 1;
	  else if (strcmp (argv[1], "no") == 0)
	    allow_biffrc = 0;
	  else
	    mu_error (_("%s:%d: yes or no expected"), config_file, line);
	}
      else if (strcmp (argv[0], "max-requests") == 0)
	maxrequests = strtoul (argv[1], NULL, 0);
      else if (strcmp (argv[0], "request-control-interval") == 0)
	request_control_interval = strtoul (argv[1], NULL, 0);
      else if (strcmp (argv[0], "overflow-control-interval") == 0)
	overflow_control_interval = strtoul (argv[1], NULL, 0);
      else if (strcmp (argv[0], "overflow-delay-time") == 0)
	overflow_delay_time = strtoul (argv[1], NULL, 0);
      else if (strcmp (argv[0], "max-lines") == 0)
	maxlines = strtoul (argv[1], NULL, 0);
      else if (strcmp (argv[0], "acl") == 0)
	{
	  if (strcmp (argv[1], "allow") == 0)
	    action = ACT_ALLOW;
	  else if (strcmp (argv[1], "deny") == 0)
	    action = ACT_DENY;
	  else
	    {
	      mu_error (_("%s:%d: unknown keyword"), config_file, line);
	      mu_argcv_free (argc, argv);
	      continue;
	    }

	  head = tail = NULL;
	  for (i = 2; i < argc; i++)
	    {
	      netdef_t *cur = netdef_parse (argv[i]);
	      if (!cur)
		{
		  mu_error (_("%s:%d: cannot parse netdef: %s"),
			    config_file, line, argv[i]);
		  continue;
		}
	      if (!tail)
		head = cur;
	      else
		tail->next = cur;
	      tail = cur;
	    }

	  mu_argcv_free (argc, argv);

	  acl = malloc (sizeof *acl);
	  if (!acl)
	    {
	      mu_error (_("Out of memory"));
	      exit (1);
	    }
	  acl->next = NULL;
	  acl->action = action;
	  acl->netlist = head;

	  if (!acl_tail)
	    acl_head = acl;
	  else
	    acl_tail->next = acl;
	  acl_tail = acl;
	}
    }
  fclose (fp);
}

/*NOTE: currently unused. */
#if 0
static void
netdef_free (netdef_t *netdef)
{
  netdef_t *next;

  while (netdef)
    {
      next = netdef->next;
      free (netdef);
      netdef = next;
    }
}

static void
acl_free (acl_t *acl)
{
  acl_t *next;

  while (acl)
    {
      next = acl->next;
      netdef_free (acl->netlist);
      free (acl);
      acl = next;
    }
}

static void
discard_acl (acl_t *mark)
{
  if (mark)
    {
      acl_free (mark->next);
      acl_tail = mark;
      acl_tail->next = NULL;
    }
  else
    acl_head = acl_tail = NULL;
}
#endif

int
acl_match (struct sockaddr_in *sa_in)
{
  acl_t *acl;
  unsigned int ip;

  ip = sa_in->sin_addr.s_addr;
  for (acl = acl_head; acl; acl = acl->next)
    {
      netdef_t *net;

      for (net = acl->netlist; net; net = net->next)
	{
	  if (net->ipaddr == (ip & net->netmask))
	    return acl->action;
	}
    }
  return ACT_ALLOW;
}
