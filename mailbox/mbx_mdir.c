#include <url_mdir.h>
#include <mbx_mdir.h>

struct mailbox_type _mailbox_maildir_type =
{
  "MAILDIR",
  (int)&_url_maildir_type, &_url_maildir_type,
  mailbox_maildir_init, mailbox_maildir_destroy
};

int
mailbox_maildir_init (mailbox_t *mbox, const char *name)
{
  return -1;
}

void
mailbox_maildir_destroy (mailbox_t *mbox)
{
  return;
}
