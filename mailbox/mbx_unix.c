#include <url_unix.h>
#include <mbx_unix.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

struct mailbox_type _mailbox_unix_type =
{
  "UNIX MBOX",
  (int)&_url_unix_type, &_url_unix_type,
  mailbox_unix_init, mailbox_unix_destroy
};

typedef struct _mailbox_unix_msg
{
  off_t header;
  off_t body;
  off_t end;
  int   deleted;
} mailbox_unix_msg_t;

typedef struct _mailbox_unix_data
{
  mailbox_unix_msg_t *messages;
  FILE *file;
  mailbox_lock_t lockmode;
  time_t last_mode_time;
} mailbox_unix_data_t;


/* forward prototypes */

static int mailbox_unix_open (mailbox_t *mbox, const char *name);
static int mailbox_unix_close (mailbox_t *mbox);

static int mailbox_unix_get_name (mailbox_t, int *id, char *name,
			     int offset, int len);
static int mailbox_unix_get_mname (mailbox_t, int *id, char **name, int *len);

/* passwd */
static int mailbox_unix_get_passwd (mailbox_t, char *passwd,
				     int offset, int len);
static int mailbox_unix_get_mpasswd (mailbox_t, char **passwd, int *len);
static int mailbox_unix_set_passwd (mailbox_t, const char *passwd,
				    int offset, int len);

/* deleting */
static int mailbox_unix_delete (mailbox_t, int);
static int mailbox_unix_undelete (mailbox_t, int);
static int mailbox_unix_expunge (mailbox_t);
static int mailbox_unix_is_deleted (mailbox_t, int);

/* appending */
static int mailbox_unix_new_msg (mailbox_t, int * id);
static int mailbox_unix_set_header (mailbox_t, int id, const char *h,
                                            int offset, int n, int replace);
static int mailbox_unix_set_body (mailbox_t, int id, const char *b,
                                            int offset, int n, int replace);
static int mailbox_unix_append (mailbox_t, int id);
static int mailbox_unix_destroy_msg (mailbox_t, int id);

/* reading */
static int mailbox_unix_get_body (mailbox_t, int id, char *b,
                                            int offset, int n);
static int mailbox_unix_get_mbody (mailbox_t, int id, char **b,
                                            int *n);
static int mailbox_unix_get_header (mailbox_t, int id, char *h,
                                            int offset, int n);
static int mailbox_unix_get_mheader (mailbox_t, int id, char **h,
                                             int *n);

/* locking */
static int mailbox_unix_lock (mailbox_t, int flag);
static int mailbox_unix_unlock (mailbox_t);

/* miscellany */
static int mailbox_unix_scan (mailbox_t, int *msgs);
static int mailbox_unix_is_updated (mailbox_t);
static int mailbox_unix_get_timeout (mailbox_t, int *timeout);
static int mailbox_unix_set_timeout (mailbox_t, int timeout);
static int mailbox_unix_get_refresh (mailbox_t, int *refresh);
static int mailbox_unix_set_refresh (mailbox_t, int refresh);
static int mailbox_unix_get_size (mailbox_t, int id, size_t *size);
static int mailbox_unix_set_notification (mailbox_t,
					  int (*func) (mailbox_t, void *arg));


int
mailbox_unix_init (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;

  /*
    Again should we do an open() and check if indeed this is a
    correct mbox ?  I think not, this should be delay to _open()
  */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return -1; /* errno set by calloc() */

  /* set the type */
  mbox->mtype = &_mailbox_unix_type;

  mbox->_open =  mailbox_unix_open;
  mbox->_close = mailbox_unix_close;

  mbox->_get_name = mailbox_unix_get_name;
  mbox->_get_mname = mailbox_unix_get_mname;

  /* passwd */
  mbox->_get_passwd = mailbox_unix_get_passwd;
  mbox->_get_mpasswd = mailbox_unix_get_mpasswd;
  mbox->_set_passwd = mailbox_unix_set_passwd;

  /* deleting */
  mbox->_delete =  mailbox_unix_delete;
  mbox->_undelete = mailbox_unix_undelete;
  mbox->_expunge = mailbox_unix_expunge;
  mbox->_is_deleted = mailbox_unix_is_deleted;

  /* appending */
  mbox->_new_msg = mailbox_unix_new_msg;
  mbox->_set_header = mailbox_unix_set_header;
  mbox->_set_body = mailbox_unix_set_body;
  mbox->_append = mailbox_unix_append;
  mbox->_destry_msg = mailbox_unix_destroy_msg;

  /* reading */
  mbox->_get_body = mailbox_unix_get_body;
  mbox->_get_mbody =  mailbox_unix_get_mbody;
  mbox->_get_header = mailbox_unix_get_header;
  mbox->_get_mheader = mailbox_unix_get_mheader;

  /* locking */
  mbox->_lock = mailbox_unix_lock;
  mbox->_unlock = mailbox_unix_unlock;

  /* miscellany */
  mbox->_scan = mailbox_unix_scan;
  mbox->_is_updated = mailbox_unix_is_updated;
  mbox->_get_timeout = mailbox_unix_get_timeout;
  mbox->_set_timeout = mailbox_unix_set_timeout;
  mbox->_get_refresh = mailbox_unix_get_refresh;
  mbox->_set_refresh = mailbox_unix_set_refresh;
  mbox->_get_size = mailbox_unix_get_size;
  mbox->_set_notification = mailbox_unix_set_notification;

  return 0;
}

void
mailbox_unix_destroy (mailbox_t *mbox)
{
  return;
}


/* start of Mbox Implementation */

static int mailbox_unix_open (mailbox_t mbox, int flags)
{
}
