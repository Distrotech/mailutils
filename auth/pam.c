/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_SECURITY_PAM_APPL_H
# include <security/pam_appl.h>
#endif
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>

char *mu_pam_service = NULL;

#ifdef USE_LIBPAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

static int
mu_pam_conv (int num_msg, const struct pam_message **msg,
	     struct pam_response **resp, void *appdata_ptr MU_ARG_UNUSED)
{
  int replies = 0;
  struct pam_response *reply = NULL;

  reply = malloc (sizeof (*reply) * num_msg);
  if (!reply)
    return PAM_CONV_ERR;

  for (replies = 0; replies < num_msg; replies++)
    {
      switch (msg[replies]->msg_style)
	{
	case PAM_PROMPT_ECHO_ON:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = COPY_STRING (_user);
	  /* PAM frees resp */
	  break;

	case PAM_PROMPT_ECHO_OFF:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = COPY_STRING (_pwd);
	  /* PAM frees resp */
	  break;

	case PAM_TEXT_INFO:
	case PAM_ERROR_MSG:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = NULL;
	  break;
 
	default:
	  free (reply);
	  _perr = 1;
	  return PAM_CONV_ERR;
	}
    }
  *resp = reply;
  return PAM_SUCCESS;
}

static struct pam_conv PAM_conversation = { &mu_pam_conv, NULL };

int
mu_authenticate_pam (struct mu_auth_data **return_data MU_ARG_UNUSED,
		     const void *key,
		     void *func_data MU_ARG_UNUSED,
		     void *call_data)
{
  const struct mu_auth_data *auth_data = key;
  char *pass = call_data;
  pam_handle_t *pamh;
  int pamerror;

#define PAM_ERROR if (_perr || (pamerror != PAM_SUCCESS)) \
    goto pam_errlab;

  if (!auth_data)
    return 1;
  
  _user = (char *) auth_data->name;
  _pwd = pass;
  pamerror = pam_start (mu_pam_service, _user, &PAM_conversation, &pamh);
  PAM_ERROR;
  pamerror = pam_authenticate (pamh, 0);
  PAM_ERROR;
  pamerror = pam_acct_mgmt (pamh, 0);
  PAM_ERROR;
  pamerror = pam_setcred (pamh, PAM_ESTABLISH_CRED);
 pam_errlab:
  pam_end (pamh, PAM_SUCCESS);
  return pamerror != PAM_SUCCESS;
}

#else

int
mu_authenticate_pam (struct mu_auth_data **return_data MU_ARG_UNUSED,
		     const void *key MU_ARG_UNUSED,
		     void *func_data MU_ARG_UNUSED,
		     void *call_data MU_ARG_UNUSED)
{
  errno = ENOSYS;
  return 1;
}

#endif

int
mu_pam_module_init (void *data)
{
  if (data)
    {
      struct mu_gocs_pam *p = data;
      mu_pam_service = p->service ? strdup (p->service) : p->service;
    }
  return 0;
}

struct mu_auth_module mu_auth_pam_module = {
  "pam",
  mu_pam_module_init,
  mu_authenticate_pam,
  NULL,
  mu_auth_nosupport,
  NULL,
  mu_auth_nosupport,
  NULL
};

