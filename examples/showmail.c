#include "mailbox.h"
#include <paths.h>

int
main (int argc, char *argv[])
{
  mailbox *mbox;
  char *user;
  char *body;
  char mailpath[256];
  int i;

  user = getenv ("USER");
  snprintf (mailpath, 256, "%s/%s", _PATH_MAILDIR, user);
  mbox = mbox_open (mailpath);
  for (i = 0; i < mbox->messages; ++i)
    {
      body = mbox_get_body (mbox, i);
      printf ("%s\n", body);
    }
  mbox_close (mbox);
}

