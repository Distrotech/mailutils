/** utility wrappers around mailutils functionality **/

#include <errno.h>
#include <string.h>

#include "sv.h"

int
sv_mu_errno_to_rc (int eno)
{
  switch (eno)
    {
    case ENOMEM:
      return SIEVE_NOMEM;
    case ENOENT:
      return SIEVE_FAIL;
    case 0:
      return SIEVE_OK;
    }
  return SIEVE_INTERNAL_ERROR;
}

/* we hook mailutils debug output into our diagnostics using this */


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
    mu_debug_set_level (d, level);

  return 0;
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
      mailbox_set_debug (to, debug);
    }

  if (!rc && ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if (!rc)
	rc = mailbox_get_folder (to, &folder);

      if (!rc)
	rc = folder_get_authority (folder, &auth);

      if (!rc)
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
sv_mu_mark_deleted (message_t msg)
{
  attribute_t attr = 0;
  int rc;

  rc = message_get_attribute (msg, &attr);

  if (!rc)
    attribute_set_deleted (attr);

  return rc;
}
