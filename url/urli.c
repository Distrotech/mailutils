#include <url.h>
int
(url_get_scheme) (const url_t url, char * scheme, unsigned int n)
{ return url->_get_scheme(url, scheme, n); }

int
(url_get_user) (const url_t url, char *user, unsigned int n)
{ return url->_get_user(url, user, n); }

int
(url_get_passwd) (const url_t url, char *passwd, unsigned int n)
{ return url->_get_passwd(url, passwd, n); }

int
(url_get_host) (const url_t url, char * host, unsigned int n)
{ return url->_get_host(url, host, n); }

int
(url_get_port) (const url_t url, long * port)
{ return url->_get_port(url, port); }

int
(url_get_path) (const url_t url, char *path, unsigned int n)
{ return url->_get_path(url, path, n); }

int
(url_get_query) (const url_t url, char *query, unsigned int n)
{ return url->_get_query(url, query, n); }

int
(url_is_pop) (const url_t url)
{ return (url->type & URL_POP); }

int
(url_is_imap) (const url_t url)
{ return (url->type & URL_IMAP); }

int
(url_is_unixmbox) (const url_t url)
{ return (url->type & URL_MBOX); }
