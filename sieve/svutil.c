/** utility wrappers around mailutils functionality **/

#include <errno.h>
#include <string.h>

#include "sv.h"

int
sv_mu_copy_debug_level (const mailbox_t from, mailbox_t to)
{
  mu_debug_t d = 0;
  size_t level;
  int rc;

  if (!from || !to)
    return EINVAL;

  rc = mailbox_get_debug (from, &d);

  if (!rc)
    mu_debug_get_level (d, &level);

  if (!rc)
    rc = mailbox_get_debug (to, &d);

  if (!rc)
    rc = mu_debug_set_level (d, level);

  return rc;
}

int
sv_mu_save_to (const char *toname, message_t msg,
	       ticket_t ticket, mu_debug_t debug)
{
  int rc = 0;
  mailbox_t to = 0;

  rc = mailbox_create_default (&to, toname);

  if (!rc && debug)
    {
      folder_t folder = 0;

      mailbox_set_debug (to, debug);

      if (mailbox_get_folder (to, &folder))
	folder_set_debug(folder, debug);

    }

  if (!rc && ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if (!rc)
	rc = mailbox_get_folder (to, &folder);

      if (!rc)
	rc = folder_get_authority (folder, &auth);

      /* Authentication-less folders don't have authorities. */
      if (!rc && auth)
	rc = authority_set_ticket (auth, ticket);
    }

  if (!rc)
    rc = mailbox_open (to, MU_STREAM_WRITE | MU_STREAM_CREAT);

  if (!rc)
    rc = mailbox_append_message (to, msg);

  if (!rc)
    rc = mailbox_close (to);
  else
    {
      mailbox_close (to);
    }

  mailbox_destroy (&to);

  return rc;
}

int
sv_mu_mark_deleted (message_t msg, int deleted)
{
  attribute_t attr = 0;
  int rc;

  rc = message_get_attribute (msg, &attr);

  if (!rc)
    {
      if (deleted)
	rc = attribute_set_deleted (attr);
      else
	rc = attribute_unset_deleted (attr);
    }

  return rc;
}

