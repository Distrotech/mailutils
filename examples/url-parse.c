#include <mailutils/url.h>
#include <stdio.h>
#include <string.h>

int
main ()
{
  char str[1024];
  char buffer[1024];
  long port = 0;
  int len = sizeof (buffer);
  url_t u = NULL;

  while (fgets (str, sizeof (str), stdin) != NULL)
    {
      int rc;

      str[strlen (str) - 1] = '\0';	/* chop newline */
      if(strspn(str, " \t") == strlen(str))
	continue; /* skip empty lines */
      if ((rc = url_create(&u, str)) != 0)
      {
	fprintf(stderr, "url_create %s ERROR: [%d] %s",
	  str, rc, strerror(rc));
	exit (1);
      }
      if ((rc = url_parse (u)) != 0)
	{
	  printf ("%s --> FAILED: [%d] %s\n",
	    str, rc, strerror(rc));
	  continue;
	}
      printf ("%s --> SUCCESS\n", str);

      url_get_scheme (u, buffer, len, NULL);
      printf ("	scheme <%s>\n", buffer);

      url_get_user (u, buffer, len, NULL);
      printf ("	user <%s>\n", buffer);

      url_get_passwd (u, buffer, len, NULL);
      printf ("	passwd <%s>\n", buffer);

      url_get_auth (u, buffer, len, NULL);
      printf ("	auth <%s>\n", buffer);

      url_get_host (u, buffer, len, NULL);
      printf ("	host <%s>\n", buffer);

      url_get_port (u, &port);
      printf ("	port %ld\n", port);

      url_get_path (u, buffer, len, NULL);
      printf ("	path <%s>\n", buffer);

      url_get_query (u, buffer, len, NULL);
      printf ("	query <%s>\n", buffer);

      url_destroy (&u);

    }
  return 0;
}

