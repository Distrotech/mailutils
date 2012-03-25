/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/mailutils.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>

struct mu_sockaddr *target_sa;
mu_acl_t acl;

void
read_rules (FILE *fp)
{
  char buf[512];
  int line = 0;
  int wsflags = MU_WRDSF_DEFFLAGS | MU_WRDSF_COMMENT;
  struct mu_wordsplit ws;
  int rc;
  
  rc = mu_acl_create (&acl);
  if (rc)
    {
      mu_error ("cannot create acl: %s", mu_strerror (rc));
      exit (1);
    }

  ws.ws_comment = "#";
  while (fgets (buf, sizeof buf, fp))
    {
      struct mu_cidr cidr;
      mu_acl_action_t action;
      void *data = NULL;
      
      int len = strlen (buf);
      if (len == 0)
	continue;
      if (buf[len-1] != '\n')
	{
	  mu_error ("%d: line too long", line);
	  continue;
	}
      buf[len-1] = 0;
      line++;
      if (buf[0] == '#')
	continue;

      if (mu_wordsplit (buf, &ws, wsflags))
	{
	  mu_error ("cannot split line `%s': %s", buf,
		    mu_wordsplit_strerror (&ws));
	  continue;
	}
      wsflags |= MU_WRDSF_REUSE;
      if (ws.ws_wordc < 2)
	{
 	  mu_error ("%d: invalid input", line);
	  continue;
	}

      if (strcmp (ws.ws_wordv[1], "any") == 0)
	memset (&cidr, 0, sizeof (cidr));
      else
	{
	  rc = mu_cidr_from_string (&cidr, ws.ws_wordv[1]);
	  if (rc)
	    {
	      mu_error ("%d: invalid source CIDR: %s",
			line, mu_strerror (rc));
	      continue;
	    }
	}

      /* accept addr
	 deny addr
	 log addr [rest ...]
	 exec addr [rest ...]
	 execif addr rest ....]
      */
      if (mu_acl_string_to_action (ws.ws_wordv[0], &action))
	{
	  mu_error ("%d: invalid command", line);
	  continue;
	}

      rc = 0;
      switch (action)
	{
	case mu_acl_accept:
	case mu_acl_deny:
	  break;

	case mu_acl_log:
	case mu_acl_exec:
	case mu_acl_ifexec:
	  data = strdup (ws.ws_wordv[2]);
	  if (!data)
	    {
              rc = ENOMEM;
              mu_error ("%d: %s", line, mu_strerror (rc));
	    }
	}

      if (rc == 0)
        {
          rc = mu_acl_append (acl, action, data, &cidr);
          if (rc)
            mu_error ("%d: cannot append acl entry: %s", line,
		      mu_strerror (rc));
        }
    }
  mu_wordsplit_free (&ws);
}

int
main (int argc, char **argv)
{
  int rc;
  FILE *file = NULL;
  mu_acl_result_t result;
  
  mu_set_program_name (argv[0]);
  while ((rc = getopt (argc, argv, "Dd:a:f:")) != EOF)
    {
      switch (rc)
	{
	case 'D':
	  mu_debug_line_info = 1;
	  break;
	  
	case 'd':
	  mu_debug_parse_spec (optarg);
	  break;

	case 'a':
	  rc = mu_sockaddr_from_node (&target_sa, optarg, NULL, NULL);
	  if (rc)
	    {
	      mu_error ("mu_sockaddr_from_node: %s", mu_strerror (rc));
	      exit (1);
	    }
	  break;

	case 'f':
	  file = fopen (optarg, "r");
	  if (file == 0)
	    {
	      mu_error ("cannot open file %s: %s", optarg, 
			mu_strerror (errno));
	      exit (1);
	    }
	  break;
	  
	default:
	  exit (1);
	}
    }

  argv += optind;
  argc -= optind;

  read_rules (file ? file : stdin);
  rc = mu_acl_check_sockaddr (acl, target_sa->addr, target_sa->addrlen,
			      &result);
  if (rc)
    {
      mu_error ("mu_acl_check_sockaddr failed: %s", mu_strerror (rc));
      exit (1);
    }
  switch (result)
    {
    case mu_acl_result_undefined:
      puts ("undefined");
      break;
      
    case mu_acl_result_accept:
      puts ("accept");
      break;

    case mu_acl_result_deny:
      puts ("deny");
      break;
    }
  exit (0);
}
