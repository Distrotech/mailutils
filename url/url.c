#include <url.h>
#include <url0.h>
#include <string.h>

/* forward declaration */
static int get_scheme (const url_t, char *, unsigned int);
static int get_user (const url_t, char *, unsigned int);
static int get_passwd (const url_t, char *, unsigned int);
static int get_host (const url_t, char *, unsigned int);
static int get_port (const url_t, long *);
static int get_path (const url_t, char *, unsigned int);
static int get_query (const url_t, char *, unsigned int);
static int is_pop (const url_t);
static int is_imap (const url_t);

static struct _supported_scheme
{
    char * scheme;
    char len;
    int (*_create) __P ((url_t *, const char *name));
    void (*_destroy) __P ((url_t *));
} _supported_scheme[] = {
    { "mailto:", 7, url_mailto_create, url_mailto_destroy },
    { "pop://", 6, url_pop_create, url_pop_destroy },
    { "imap://", 7, url_imap_create, url_imap_destroy },
    { "file://", 7, url_mbox_create, url_mbox_destroy },
    { "/", 1, url_mbox_create, url_mbox_destroy },         /* hack ? */
    { NULL, NULL, NULL, NULL }
};

static int
get_scheme (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr (u->scheme, s, n);
}

static int
get_user (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr (u->user, s, n);
}

/* FIXME: We should not store passwd in clear, but rather
   have a simple encoding, and decoding mechanism */
static int
get_passwd (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr (u->passwd, s, n);
}

static int
get_host (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr (u->host, s, n);
}

static int
get_port (const url_t u, long * p)
{
    if (u == NULL)
	return -1;
    return *p = u->port;
}

static int
get_path (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr(u->path, s, n);
}

static int
get_query (const url_t u, char * s, unsigned int n)
{
    if (u == NULL)
	return -1;
    return _cpystr(u->query, s, n);
}

static int
is_pop (const url_t u)
{
    return u->type & URL_POP;
}

static int
is_imap (const url_t u)
{
    return u->type & URL_IMAP;
}

static int
is_unixmbox (const url_t u)
{
    return u->type & URL_MBOX;
}

int
url_create (url_t * url, const char * name)
{
    int status = -1, i ;
    
    /* sanity checks */
    if (name == NULL || *name == '\0')
      {
	return status;
      }

    /* scheme://scheme-specific-part */
    for (i = 0; _supported_scheme[i].scheme; i++)
      {
	if (strncasecmp (name, _supported_scheme[i].scheme,
		    strlen(_supported_scheme[i].scheme)) == 0)
	  {
	    status = 1;
	    break;
	  }
      }
    
    if (status == 1)
      {
	status =_supported_scheme[i]._create(url, name);
	if (status == 0)
	  {
	    url_t u = *url;
	    if (u->_get_scheme == NULL)
	      {
		u->_get_scheme = get_scheme;
	      }
	    if (u->_get_user == NULL)
	      {
		u->_get_user = get_user;
	      }
	    if (u->_get_passwd == NULL)
	      {
		u->_get_passwd = get_passwd;
	      }
	    if (u->_get_host == NULL)
	      {
		u->_get_host = get_host;
	      }
	    if (u->_get_port == NULL)
	      {
		u->_get_port = get_port;
	      }
	    if (u->_get_path == NULL)
	      {
		u->_get_path = get_path;
	      }
	    if (u->_get_query == NULL)
	      {
		u->_get_query = get_query;
	      }
	    if (u->_is_pop == NULL)
	      {
		u->_is_pop = is_pop;
	      }
	    if (u->_is_imap == NULL)
	      {
		u->_is_imap = is_imap;
	      }
	    if (u->_is_unixmbox == NULL)
	      {
		u->_is_unixmbox = is_unixmbox;
	      }
	    if (u->_create == NULL)
	      {
		u->_create = _supported_scheme[i]._create;
	      }
	    if (u->_destroy == NULL)
	      {
		u->_destroy= _supported_scheme[i]._destroy;
	      }
	  }
      }
    return status;
}

void
url_destroy (url_t * url)
{
    if (url && *url)
      {
	url_t u = *url;
	u->_destroy(url);
	u = NULL;
      }
}
