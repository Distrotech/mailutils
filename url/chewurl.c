#include <url.h>
#include <stdio.h>
#include <string.h>

int
main (int argc, char ** argv)
{
    int status;
    long port;
    url_t u;
    char buffer[1024];
    char str[1024];
    
    while (fgets(str, sizeof (str), stdin) != NULL)
    {
	str[strlen(str) - 1] = '\0'; /* chop newline */
	status = url_create (&u, str);
	if (status == -1) {
	    printf("%s --> FAILED\n", str);
	    continue;
	}
	printf("%s --> SUCCESS\n", str);

	url_get_scheme (u, buffer, sizeof (buffer));
	printf("\tscheme %s\n", buffer);

	url_get_user (u, buffer, sizeof (buffer));
	printf("\tuser %s\n", buffer);

	url_get_passwd (u, buffer, sizeof (buffer));
	printf("\tpasswd %s\n", buffer);

	url_pop_get_auth (u, buffer, sizeof (buffer));
	printf("\tauth %s\n", buffer);

	url_get_host (u, buffer, sizeof (buffer));
	printf("\thost %s\n", buffer);

	url_get_port (u, &port);
	printf("\tport %ld\n", port);
	
	url_destroy (&u);
    }
    return 0;
}

