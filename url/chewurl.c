#include <url.h>
#include <url_pop.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main ()
{
  int status, i;
  long port;
  url_t u;
  char buffer[1024];
  char str[1024];
  struct url_type *utype;
  size_t len = sizeof (buffer);

  url_list_mtype (&utype, &status);
  for  (i = 0; i < status; i++)
    {
      puts (utype[i].scheme);
      puts (utype[i].description);
    }
  free (utype);

  while (fgets(str, sizeof (str), stdin) != NULL)
    {
      str[strlen(str) - 1] = '\0'; /* chop newline */
      status = url_init (&u, str);
      if (status != 0) {
	printf("%s --> FAILED\n", str);
	continue;
      }
      printf("%s --> SUCCESS\n", str);

      url_get_scheme (u, buffer, len, NULL);
      printf("\tscheme <%s>\n", buffer);

      url_get_user (u, buffer, len, NULL);
      printf("\tuser <%s>\n", buffer);

      url_get_passwd (u, buffer, len, NULL);
      printf("\tpasswd <%s>\n", buffer);

      //url_pop_get_auth (u, buffer, len, NULL);
      //printf("\tauth <%s>\n", buffer);

      url_get_host (u, buffer, len, NULL);
      printf("\thost <%s>\n", buffer);

      url_get_port (u, &port);
      printf("\tport %ld\n", port);

      url_get_path (u, buffer, len, NULL);
      printf("\tpath <%s>\n", buffer);

      url_get_query (u, buffer, len, NULL);
      printf("\tquery <%s>\n", buffer);

      url_destroy (&u);
    }
  return 0;
}
