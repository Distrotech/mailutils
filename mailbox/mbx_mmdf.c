#include <url_mmdf.h>
#include <mbx_mmdf.h>

struct mailbox_type _mailbox_maildir_type =
{
  "MMDF",
  (int)&_url_mmdf_type, &_url_mmdf_type,
  mailbox_mmdf_init, mailbox_mmdf_destroy
};

int
mailbox_mmdf_init (mailbox_t *mbox, const char *name)
{
  return -1;
}

void
mailbox_mmdf_destroy (mailbox_t *mbox)
{
  return;
}
