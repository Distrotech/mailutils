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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif 
#include <mailutils/mailutils.h>

void
help (char *progname)
{
  printf ("usage: %s [--file=NAME] [--dump] [COMMAND...]\n", progname);
  printf ("Valid COMMANDs are:\n");
  printf (" X=Y      set property X to value Y\n");
  printf (" X:=Y     same, but fail if X has already been set\n");
  printf (" ?X       query whether X is set\n");
  printf (" +X       query the value of the property X\n");
  printf (" -X       unset property X\n");
  printf (" 0        clear all properties\n");
  printf (" !        invalidate properties\n");
  exit (0);
}

int
main (int argc, char **argv)
{
  mu_property_t prop;
  mu_iterator_t itr;
  int i, j, rc;
  char *filename = NULL;
  int dumpit = 0;
  mu_stream_t str = NULL;

  if (argc == 1)
    help (argv[0]);
  
  for (i = 1; i < argc; i++)
    {
      if (!(argv[i][0] == '-' && argv[i][1] == '-'))
	break;
      if (strcmp (argv[i], "--help") == 0)
	help (argv[0]);
      else if (strncmp (argv[i], "--file=", 7) == 0)
	filename = argv[i] + 7;
      else if (strcmp (argv[i], "--dump") == 0)
	dumpit = 1;
      else
	{
	  mu_error ("unknown switch: %s", argv[i]);
	  return 1;
	}
    }

  if (filename)
    {
      MU_ASSERT (mu_file_stream_create (&str, filename,
					MU_STREAM_RDWR|MU_STREAM_CREAT));
      if (i == argc)
	dumpit = 1;
    }

  MU_ASSERT (mu_property_create_init (&prop, mu_assoc_property_init, str));

  for (j = 0; i < argc; i++, j++)
    {
      char *key = argv[i];
      char *p = strchr (key, '=');

      if (p)
	{
	  int override;
	  if (p > key && p[-1] == ':')
	    {
	      override = 0;
	      p[-1] = 0;
	    }
	  else
	    {
	      override = 1;
	      *p = 0;
	    }
	  p++;
	  rc = mu_property_set_value (prop, key, p, override);
	  printf ("%d: %s=%s: %s\n",
		  j, key, p, mu_strerror (rc));
	}
      else if (key[0] == '?')
	{
	  key++;
	  rc = mu_property_is_set (prop, key);
	  printf ("%d: %s is %s\n", j, key, rc ? "set" : "not set");
	}
      else if (key[0] == '-')
	{
	  key++;
	  rc = mu_property_unset (prop, key);
	  printf ("%d: unset %s: %s\n", j, key, mu_strerror (rc));
	}
      else if (key[0] == '+')
	{
	  const char *val;
	  
	  key++;
	  rc = mu_property_sget_value (prop, key, &val);
	  if (rc)
	    printf ("%d: cannot get %s: %s\n", j, key, mu_strerror (rc));
	  else
	    printf ("%d: %s=%s\n", j, key, val);
	}
      else if (key[0] == '0' && key[1] == 0)
	{
	  rc = mu_property_clear (prop);
	  printf ("%d: clear: %s\n", j, mu_strerror (rc));
	}
      else if (key[0] == '!' && key[1] == 0)
	{
	  rc = mu_property_invalidate (prop);
	  printf ("%d: invalidate: %s\n", j, mu_strerror (rc));
	}      
      else
	{
	  mu_error ("%d: unrecognized command", i);
	}
    }

  if (dumpit)
    {
      if (mu_property_get_iterator (prop, &itr) == 0)
	{
	  printf ("Property dump:\n");
	  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	       mu_iterator_next (itr))
	    {
	      const char *name, *val;
	      
	      mu_iterator_current_kv (itr, (const void **)&name, (void**)&val);
	      printf ("%s=%s\n", name, val);
	    }
	  mu_iterator_destroy (&itr);
	}
    }
  
  mu_property_destroy (&prop);
  return 0;
}
