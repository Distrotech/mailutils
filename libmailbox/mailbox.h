/* copyright and license info go here */

#ifndef _MAILBOX_H
#define _MAILBOX_H	1

#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*!__P */

/* These need to be documented */
#define mbox_close(m)		m->_close(m)
#define mbox_delete(m,n)	m->_delete(m,n)
#define mbox_undelete(m,n)      m->_undelete(m,n)
#define mbox_expunge(m)		m->_expunge(m)
#define mbox_is_deleted(m,n)	m->_is_deleted(m,n)
#define mbox_add_message(m,s)	m->_add_message(m,s)
#define mbox_get_body(m,n)	m->_get_body(m,n)
#define mbox_get_header(m,n)	m->_get_header(m,n)

typedef struct _mailbox
  {
    /* Data */
    char *name;
    int messages;
    int num_deleted;
    int *sizes;
    void *_data;

    /* Functions */
    int (*_close) __P ((struct _mailbox *));
    int (*_delete) __P ((struct _mailbox *, int));
    int (*_undelete) __P ((struct _mailbox *, int));
    int (*_expunge) __P ((struct _mailbox *));
    int (*_add_message) __P ((struct _mailbox *, char *));
    int (*_is_deleted) __P ((struct _mailbox *, int));
    char *(*_get_body) __P ((struct _mailbox *, int));
    char *(*_get_header) __P ((struct _mailbox *, int));
  }
mailbox;

mailbox *mbox_open __P ((char *name));

#endif
