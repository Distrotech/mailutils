#include <mailutils/mutil.h>
#include "mu_argp.h"

#include <stdio.h>

const char *capa[] = {
  "address",
  NULL
};

int
main (int argc, char *argv[])
{
  int arg = 1;

  if (mu_argp_parse (NULL, &argc, &argv, 0, capa, &arg, NULL))
    abort ();

  if (!argv[arg])
    printf ("current user -> %s\n", mu_get_user_email (0));
  else
    {
      for (; argv[arg]; arg++)
	printf ("%s -> %s\n", argv[arg], mu_get_user_email (argv[arg]));
    }

  return 0;
}

