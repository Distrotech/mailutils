#include <string.h>
#include <stdlib.h>
#include <paths.h>
#include <errno.h>
#include <stdio.h>
#include <mailutils/mailbox.h>

#ifndef _PATH_MAILDIR
# define _PATH_MAILDIR "/var/spool/mail"
#endif

int
mailbox_create_default (mailbox_t *pmbox, const char *mail)
{
  const char *user = NULL;

  if (pmbox == NULL)
    return EINVAL;

  if (mail == NULL)
    mail = getenv ("MAIL");

  if (mail)
    {
      /* Is it a fullpath ?  */
      if (mail[0] != '/')
	{
	  /* Is it a URL ?  */
	  if (strchr (mail, ':') == NULL)
	    {
	      /* A user name.  */
	      user = mail;
	      mail = NULL;
	    }
	}
    }

  if (mail == NULL)
    {
      int status;
      char *mail0;
      if (user == NULL)
	{
	  user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
	  if (user == NULL)
	    {
	      fprintf (stderr, "Who am I ?\n");
	      return EINVAL;
	    }
	}
      mail0 = malloc (strlen (user) + strlen (_PATH_MAILDIR) + 2);
      if (mail0 == NULL)
	return ENOMEM;
      sprintf (mail0, "%s/%s", _PATH_MAILDIR, user);
      status = mailbox_create (pmbox, mail0, 0);
      free (mail0);
      return status;
    }
  return  mailbox_create (pmbox, mail, 0);
}
