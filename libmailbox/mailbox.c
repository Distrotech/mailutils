/* copyright and license info go here */

#include "mailbox.h"
#include "unixmbox.h"

#ifdef _HAVE_ERRNO_H
#include <errno.h>
#endif

int _mbox_dummy1 (mailbox * mbox);
int _mbox_dummy2 (mailbox * mbox, int num);
int _mbox_dummy3 (mailbox * mbox, char *c);
char *_mbox_dummy4 (mailbox * mbox, int num);

mailbox *
mbox_open (char *name)
{
  mailbox *mbox = malloc (sizeof (mailbox));
  if (mbox == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  /* Set up with default data and dummy functions */
  mbox->name = strdup (name);
  if (mbox->name == NULL)
    {
      errno = ENOMEM;
      free (mbox);
      return NULL;
    }
  mbox->messages = 0;
  mbox->num_deleted = 0;
  mbox->sizes = NULL;
  mbox->_data = NULL;
  mbox->_close = _mbox_dummy1;
  mbox->_delete = _mbox_dummy2;
  mbox->_undelete = _mbox_dummy2;
  mbox->_expunge = _mbox_dummy1;
  mbox->_add_message = _mbox_dummy3;
  mbox->_is_deleted = _mbox_dummy2;
  mbox->_get_body = _mbox_dummy4;
  mbox->_get_header = _mbox_dummy4;

  if (unixmbox_open (mbox) == 0)
    return mbox;
  else
    {
      /*
       * Check errno to find out why it failed, if it's simply not a
       * unix mbox format message, then try other mailbox types,
       * otherwise, leave errno set and return NULL
       */
    }

  /* for example...
     if (maildir_open (mbox, name) == 1)
       return mbox;
     else if (errno != 0)
       return NULL;
     else
       errno = 0;
   */

  errno = ENOSYS;
  return NULL;
}

/*
 * Bogus functions for unimplemented functions that return int
 */
int
_mbox_dummy1 (mailbox * mbox)
{
  errno = ENOSYS;
  return -1;
}

int 
_mbox_dummy2 (mailbox * mbox, int num)
{
  return _mbox_dummy1 (mbox);
}
int 
_mbox_dummy3 (mailbox * mbox, char *c)
{
  return _mbox_dummy1 (mbox);
}

/*
 * Bogus function for unimplemented functions that return char *
 */
char *
_mbox_dummy4 (mailbox * mbox, int num)
{
  errno = ENOSYS;
  return NULL;
}
