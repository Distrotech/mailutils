/** utility wrappers around mailutils functionality **/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "sv.h"

#if 0
int
sv_mu_copy_debug_level (const mailbox_t from, mailbox_t to)
{
  mu_debug_t d = 0;
  size_t level;
  int rc;

  if (!from || !to)
    return EINVAL;

  if((rc = mailbox_get_debug (from, &d)))
    return rc;

  if ((mu_debug_get_level (d, &level)))
    return rc;

  if ((rc = mailbox_get_debug (to, &d)))
    return rc;

  rc = mu_debug_set_level (d, level);

  return rc;
}
#endif

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

