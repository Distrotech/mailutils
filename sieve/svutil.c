/** utility wrappers around mailutils functionality **/

#include <errno.h>

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
    case EOK:
      return SIEVE_OK;
    }
  return SIEVE_INTERNAL_ERROR;
}

/* we hook mailutils debug output into our diagnostics using this */

int
sv_mu_debug_print (mu_debug_t d, const char *fmt, va_list ap)
{
  sv_printv(mu_debug_get_owner(d), SV_PRN_MU, fmt, ap);

  return 0;
}

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
sv_mu_save_to (const char *toname, message_t mesg,
	    ticket_t ticket, const char **errmsg)
{
  int res = 0;
  mailbox_t to = 0;
  mailbox_t from = 0;

  res = mailbox_create_default (&to, toname);

  if (res == ENOENT)
    *errmsg = "no handler for this type of mailbox";

  if (!res && ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if (!res)
      {
	*errmsg = "mailbox_get_folder";
	res = mailbox_get_folder (to, &folder);
      }

      if (!res)
      {
	*errmsg = "folder_get_authority";
	res = folder_get_authority (folder, &auth);
      }

      if (!res)
      {
	*errmsg = "authority_set_ticket";
	res = authority_set_ticket (auth, ticket);
      }
    }
  if (!res)
    {
      if (message_get_mailbox (mesg, &from) == 0)
	sv_mu_copy_debug_level (from, to);
    }
  if (!res)
    {
      *errmsg = "mailbox_open";
      res = mailbox_open (to, MU_STREAM_WRITE | MU_STREAM_CREAT);
    }
  if (!res)
    {
      *errmsg = "mailbox_append_message";
      res = mailbox_append_message (to, mesg);

      if (!res)
	{
	  *errmsg = "mailbox_close";
	  res = mailbox_close (to);
	}
      else
	{
	  mailbox_close (to);
	}
    }
  mailbox_destroy (&to);

  if(res == 0)
    *errmsg = 0;

  return res;
}

int
sv_mu_mark_deleted (message_t msg)
{
  attribute_t attr = 0;
  int res;

  res = message_get_attribute (msg, &attr);

  if (!res)
    attribute_set_deleted (attr);

  return res;
}

