#include <url.h>
#include <url0.h>
#include <stdlib.h>
#include <string.h>

#define POP_PORT 110

static int get_auth (const url_pop_t up, char * s, unsigned int n);

static int
get_auth (const url_pop_t up, char * s, unsigned int n)
{
    if (up == NULL) return -1;
    return _cpystr (up->auth, s, n);
}

int
(url_pop_get_auth) (const url_t url, char * auth, unsigned int n)
{
    return ((url_pop_t) (url->data))->_get_auth(url->data, auth, n);
}

void
url_pop_destroy (url_t * url)
{
    if (url && *url)
      {
	url_t u = *url;
	if (u->scheme)
	  {
	    free (u->scheme);
	  }
	if (u->user)
	  {
	    free (u->user);
	  }
	if (u->passwd)
	  {
	    free (u->passwd);
	  }
	if (u->host)
	  {
	    free (u->host);
	  }
	if (u->data)
	  {
	    url_pop_t up = u->data;
	    if (up->auth)
	      {
		free (up->auth);
	      }
	    free (u->data);
	  }
	free (u);
	u = NULL;
      }
}

/*
   POP URL
 pop://<user>;AUTH=<auth>@<host>:<port>
*/
int
url_pop_create (url_t * url, const char * name)
{
    const char * host_port, * index;
    url_t u;
    url_pop_t up;

    /* reject the obvious */
    if (name == NULL ||
	    strncmp ("pop://", name, 6) != 0 ||
	    (host_port = strchr (name, '@')) == NULL ||
	    strlen(name) < 9 /* 6(scheme)+1(user)+1(@)+1(host)*/) {
	return -1;
    }
    
    /* do I need to decode url encoding '% hex hex' ? */
    
    u = xcalloc(1, sizeof (*u));
    u->data = up = xcalloc(1, sizeof(*up));
    up->_get_auth = get_auth;

    /* type */
    u->type = URL_POP;
    u->scheme = xstrdup ("pop://");

    name += 6; /* pass the scheme */

    /* looking for user;auth=auth-enc */
#if 1
    for (index = name; index != host_port; index++)
      {
	/* Auth ? */
	if (*index == ';')
	  {
	    if (strncasecmp(index +1, "auth=", 5) == 0 )
	    break;
	  }
      }
#endif
    
    /* USER */
    if (index == name)
      {
	//free();
	return -1;
      }
    u->user = malloc(index - name + 1);
    ((char *)memcpy(u->user, name, index - name))[index - name] = '\0';

    /* AUTH */
    if ((host_port - index) <= 6 /*strlen(";AUTH=")*/)
      {
	/* default AUth is '*'*/
	up->auth = malloc (1 + 1);
	up->auth[0] = '*';
	up->auth[1] = '\0';
      }
    else
      {
	/* move pass AUTH= */
	index += 6;
	up->auth = malloc (host_port - index + 1);
	((char *) memcpy (up->auth, index, host_port - index))
	    [host_port - index] = '\0';
      }
    
    /* HOST:PORT */
    index = strchr (++host_port, ':');
    if (index == NULL)
      {
	int len = strlen (host_port);
	u->host = malloc (len + 1);
	((char *)memcpy (u->host, host_port, len))[len] = '\0';
	u->port = POP_PORT;
      }
    else
      {
	long p = strtol(index + 1, NULL, 10);
	u->host = malloc (index - host_port + 1);
	((char *)memcpy (u->host, host_port, index - host_port))
	    [index - host_port]='\0';
	u->port = (p == 0) ? POP_PORT : p;
      }
    
    *url = u;
    return 0;
}
