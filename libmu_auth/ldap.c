/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include <mailutils/mu_auth.h>
#include <mailutils/cstr.h>
#include <mailutils/io.h>

#ifdef WITH_LDAP
#include "mailutils/argcv.h"
#include "mailutils/wordsplit.h"
#include "mailutils/assoc.h"
#include "mailutils/list.h"
#include "mailutils/iterator.h"
#include "mailutils/mailbox.h"
#include "mailutils/sql.h"
#include "mailutils/mu_auth.h"
#include "mailutils/error.h"
#include "mailutils/errno.h"
#include "mailutils/nls.h"
#include "mailutils/util.h"
#include "mailutils/stream.h"
#include "mailutils/filter.h"
#include "mailutils/md5.h"
#include "mailutils/sha1.h"
#include "mailutils/ldap.h"

#include <ldap.h>
#include <lber.h>

const char *default_field_map =
"name=uid:"
"passwd=userPassword:"
"uid=uidNumber:"
"gid=gidNumber:"
"gecos=gecos:"
"dir=homeDirectory:"
"shell=loginShell";

static struct mu_ldap_module_config ldap_param;

int
mu_ldap_module_init (enum mu_gocs_op op, void *data)
{
  struct mu_ldap_module_config *cfg = data;

  if (op != mu_gocs_op_set)
    return 0;
  
  if (cfg)
    ldap_param = *cfg;

  if (ldap_param.enable)
    {
      if (!ldap_param.getpwnam_filter)
	ldap_param.getpwnam_filter = "(&(objectClass=posixAccount) (uid=%u))";
      if (!ldap_param.getpwuid_filter)
	ldap_param.getpwuid_filter =
	  "&(objectClass=posixAccount) (uidNumber=%u))";
      if (!ldap_param.field_map)
	{
	  int d;
	  mutil_parse_field_map (default_field_map, &ldap_param.field_map, &d);
	}
    }
  return 0;
}

static int
_mu_conn_setup (LDAP **pld)
{
  int rc;
  LDAPURLDesc *ludlist, **ludp;
  char **urls = NULL;
  int nurls = 0;
  char *ldapuri = NULL;
  LDAP *ld = NULL;
  int protocol = LDAP_VERSION3; /* FIXME: must be configurable */
  
  if (ldap_param.debug)
    {
      if (ber_set_option (NULL, LBER_OPT_DEBUG_LEVEL, &ldap_param.debug)
	  != LBER_OPT_SUCCESS )
	mu_error (_("cannot set LBER_OPT_DEBUG_LEVEL %d"), ldap_param.debug);

      if (ldap_set_option (NULL, LDAP_OPT_DEBUG_LEVEL, &ldap_param.debug)
	  != LDAP_OPT_SUCCESS )
	mu_error (_("could not set LDAP_OPT_DEBUG_LEVEL %d"),
		  ldap_param.debug);
    }

  if (ldap_param.url)
    {
      rc = ldap_url_parse (ldap_param.url, &ludlist);
      if (rc != LDAP_URL_SUCCESS)
	{
	  mu_error (_("cannot parse LDAP URL(s)=%s (%d)"),
		    ldap_param.url, rc);
	  return 1;
	}
      
      for (ludp = &ludlist; *ludp; )
	{
	  LDAPURLDesc *lud = *ludp;
	  char **tmp;
	  
	  if (lud->lud_dn && lud->lud_dn[0]
	      && (lud->lud_host == NULL || lud->lud_host[0] == '\0'))
	    {
	      /* if no host but a DN is provided, try DNS SRV to gather the
		 host list */
	      char *domain = NULL, *hostlist = NULL;
	      size_t i;
	      struct mu_wordsplit ws;
	      
	      if (ldap_dn2domain (lud->lud_dn, &domain) || !domain)
		{
		  mu_error (_("DNS SRV: cannot convert DN=\"%s\" into a domain"),
			    lud->lud_dn );
		  goto dnssrv_free;
		}
	      
	      rc = ldap_domain2hostlist (domain, &hostlist);
	      if (rc)
		{
		  mu_error (_("DNS SRV: cannot convert domain=%s into a hostlist"),
			    domain);
		  goto dnssrv_free;
		}

	      if (mu_wordsplit (hostlist, &ws, MU_WRDSF_DEFFLAGS))
		{
		  mu_error (_("DNS SRV: could not parse hostlist=\"%s\": %s"),
			    hostlist, mu_wordsplit_strerror (&ws));
		  goto dnssrv_free;
		}
	      
	      tmp = realloc (urls, sizeof(char *) * (nurls + ws.ws_wordc + 1));
	      if (!tmp)
		{
		  mu_error ("DNS SRV %s", mu_strerror (errno));
		  goto dnssrv_free;
		}
	      
	      urls = tmp;
	      urls[nurls] = NULL;
	      
	      for (i = 0; i < ws.ws_wordc; i++)
		{
		  urls[nurls + i + 1] = NULL;
		  rc = mu_asprintf (&urls[nurls + i],
				    "%s://%s",
				    lud->lud_scheme, ws.ws_wordv[i]);
		  if (rc)
		    {
		      mu_error ("DNS SRV %s", mu_strerror (rc));
		      goto dnssrv_free;
		    }
		}
	      
	      nurls += i;
	      
	    dnssrv_free:
	      mu_wordsplit_free (&ws);
	      ber_memfree (hostlist);
	      ber_memfree (domain);
	    }
	  else
	    {
	      tmp = realloc (urls, sizeof(char *) * (nurls + 2));
	      if (!tmp)
		{
		  mu_error ("DNS SRV %s", mu_strerror (errno));
		  break;
		}
	      urls = tmp;
	      urls[nurls + 1] = NULL;
	      
	      urls[nurls] = ldap_url_desc2str (lud);
	      if (!urls[nurls])
		{
		  mu_error ("DNS SRV %s", mu_strerror (errno));
		  break;
		}
	      nurls++;
	    }
	  
	  *ludp = lud->lud_next;
	  
	  lud->lud_next = NULL;
	  ldap_free_urldesc (lud);
	}

      if (ludlist)
	{
	  ldap_free_urldesc (ludlist);
	  return 1;
	}
      else if (!urls)
	return 1;
      
      rc = mu_argcv_string (nurls, urls, &ldapuri);
      if (rc)
	{
	  mu_error ("%s", mu_strerror (rc));
	  return 1;
	}
      
      ber_memvfree ((void **)urls);
    }

  mu_diag_output (MU_DIAG_INFO,
		  "constructed LDAP URI: %s", ldapuri ? ldapuri : "<DEFAULT>");

  rc = ldap_initialize (&ld, ldapuri);
  if (rc != LDAP_SUCCESS)
    {
      mu_error (_("cannot create LDAP session handle for URI=%s (%d): %s"),
		ldapuri, rc, ldap_err2string (rc));

      free (ldapuri);
      return 1;
    }
  free (ldapuri);
  
  if (ldap_param.tls)
    {
      rc = ldap_start_tls_s (ld, NULL, NULL);
      if (rc != LDAP_SUCCESS)
	{
	  mu_error (_("ldap_start_tls failed: %s"), ldap_err2string (rc));
	  return 1;
	}
    }

  ldap_set_option (ld, LDAP_OPT_PROTOCOL_VERSION, &protocol);

  /* FIXME: Timeouts, SASL, etc. */
  *pld = ld;
  return 0;
}
  
     

static int
_mu_ldap_bind (LDAP *ld)
{
  int msgid, err, rc;
  LDAPMessage *result;
  LDAPControl **ctrls;
  char msgbuf[256];
  char *matched = NULL;
  char *info = NULL;
  char **refs = NULL;
  static struct berval passwd;

  passwd.bv_val = ldap_param.passwd;
  passwd.bv_len = passwd.bv_val ? strlen (passwd.bv_val) : 0;


  msgbuf[0] = 0;

  rc = ldap_sasl_bind (ld, ldap_param.binddn, LDAP_SASL_SIMPLE, &passwd,
		       NULL, NULL, &msgid);
  if (msgid == -1)
    {
      mu_error ("ldap_sasl_bind(SIMPLE) failed: %s", ldap_err2string (rc));
      return 1;
    }

  if (ldap_result (ld, msgid, LDAP_MSG_ALL, NULL, &result ) == -1)
    {
      mu_error ("ldap_result failed");
      return 1;
    }

  rc = ldap_parse_result (ld, result, &err, &matched, &info, &refs,
			  &ctrls, 1);
  if (rc != LDAP_SUCCESS)
    {
      mu_error ("ldap_parse_result failed: %s", ldap_err2string (rc));
      return 1;
    }

  if (ctrls)
    ldap_controls_free (ctrls);

  if (err != LDAP_SUCCESS
      || msgbuf[0]
      || (matched && matched[0])
      || (info && info[0])
      || refs)
    {
      /* FIXME: Use debug output for that */
      mu_error ("ldap_bind: %s (%d)%s", ldap_err2string (err), err, msgbuf);

      if (matched && *matched) 
	mu_error ("matched DN: %s", matched);

      if (info && *info)
	mu_error ("additional info: %s", info);

      if (refs && *refs)
	{
	  int i;
	  mu_error ("referrals:");
	  for (i = 0; refs[i]; i++) 
	    mu_error ("%s", refs[i]);
	}

    }

  if (matched)
    ber_memfree (matched);
  if (info)
    ber_memfree (info);
  if (refs)
    ber_memvfree ((void **)refs);

  return err == LDAP_SUCCESS ? 0 : MU_ERR_FAILURE;
}

static void
_mu_ldap_unbind (LDAP *ld)
{
  if (ld)
    {
      ldap_set_option (ld, LDAP_OPT_SERVER_CONTROLS, NULL);
      ldap_unbind_ext (ld, NULL, NULL);
    }
}

static int 
_construct_attr_array (size_t *pargc, char ***pargv)
{
  size_t count, i;
  char **argv;
  mu_iterator_t itr = NULL;
  
  mu_assoc_count (ldap_param.field_map, &count);
  if (count == 0)
    return MU_ERR_FAILURE;
  argv = calloc (count + 1, sizeof argv[0]);

  mu_assoc_get_iterator (ldap_param.field_map, &itr);
  for (i = 0, mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr), i++)
    {
      char **str;
      mu_iterator_current (itr, (void**) &str); 
      if ((argv[i] = strdup (*str)) == NULL)
        {
          mu_argcv_free (i, argv);
          return ENOMEM;
        }
    }
  mu_iterator_destroy (&itr);
  argv[i] = NULL;
  
  *pargc = count;
  *pargv = argv;
  
  return 0;
}

/* FIXME: Duplicated in other modules */
static void
get_quota (mu_off_t *pquota, const char *str)
{
  char *p;
  mu_off_t quota = strtoul (str, &p, 10);
  switch (*p)
    {
    case 0:
      break;
	      
    case 'k':
    case 'K':
      quota *= 1024;
      break;
      
    case 'm':
    case 'M':
      quota *= 1024*1024;
      break;
	      
    default:
      mu_error (_("invalid value for quota: %s"), str);
    }
}

static void
_free_partial_auth_data (struct mu_auth_data *d)
{
  free (d->name);
  free (d->passwd);
  free (d->gecos);
  free (d->dir);
  free (d->shell);
  free (d->mailbox);
}

static int
_assign_partial_auth_data (struct mu_auth_data *d, const char *key,
			   const char *val)
{
  int rc = 0;
  
  if (strcmp (key, MU_AUTH_NAME) == 0)
    rc = (d->name = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_PASSWD) == 0)
    rc = (d->passwd = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_UID) == 0)
    d->uid = atoi (val);
  else if (strcmp (key, MU_AUTH_GID) == 0)
    d->gid = atoi (val);
  else if (strcmp (key, MU_AUTH_GECOS) == 0)
    rc = (d->gecos = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_DIR) == 0)
    rc = (d->dir = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_SHELL) == 0)   
    rc = (d->shell = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_MAILBOX) == 0)
    rc = (d->mailbox = strdup (val)) ? 0 : errno;
  else if (strcmp (key, MU_AUTH_QUOTA) == 0)   
    get_quota (&d->quota, val);
  return rc;
}

static int
_mu_entry_to_auth_data (LDAP *ld, LDAPMessage *msg,
			struct mu_auth_data **return_data)
{
  int rc;
  BerElement *ber = NULL;
  struct berval bv;
  char *ufn = NULL;
  struct mu_auth_data d;
  mu_iterator_t itr = NULL;
  
  memset (&d, 0, sizeof d);
  
  rc = ldap_get_dn_ber (ld, msg, &ber, &bv);
  ufn = ldap_dn2ufn (bv.bv_val);
  /* FIXME: Use debug or diag functions */
  mu_error ("INFO: %s", ufn);
  ldap_memfree (ufn);
  
  mu_assoc_get_iterator (ldap_param.field_map, &itr);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      char *key;
      char **pattr;
      char *attr;
      struct berval **values;
      
      mu_iterator_current_kv (itr, (const void **)&key, (void**) &pattr);
      attr = *pattr;
      values = ldap_get_values_len (ld, msg, attr);
      if (!values || !values[0])
	{
	  mu_error ("LDAP field `%s' (`%s') has NULL value",
		    key, *pattr);
	  _free_partial_auth_data (&d);
	  return MU_ERR_READ;
	}
      
      rc = _assign_partial_auth_data (&d, key, values[0]->bv_val);
      
      ldap_value_free_len (values);
      if (rc)
	{
	  _free_partial_auth_data (&d);
	  return rc;
	}
    }
  
  rc = mu_auth_data_alloc (return_data,
			   d.name,
			   d.passwd,
			   d.uid,
			   d.gid,
			   d.gecos,
			   d.dir,
			   d.shell,
			   d.mailbox,
			   1);
  if (rc == 0)
    mu_auth_data_set_quota (*return_data, d.quota);

  _free_partial_auth_data (&d);
  
  return rc;
}
    

static int
_mu_ldap_search (LDAP *ld, const char *filter_pat, const char *key,
		 struct mu_auth_data **return_data)
{
  int rc;
  char **attrs;
  size_t nattrs;
  LDAPMessage *res, *msg;
  ber_int_t msgid;
  const char *env[3];
  struct mu_wordsplit ws;

  rc = _construct_attr_array (&nattrs, &attrs);
  if (rc)
    return rc;

  env[0] = "user";
  env[1] = key;
  env[3] = NULL;

  ws.ws_env = env;
  if (mu_wordsplit (filter_pat, &ws,
		    MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD |
		    MU_WRDSF_ENV | MU_WRDSF_ENV_KV))
    {
      mu_error (_("cannot expand line `%s': %s"), filter_pat,
		mu_wordsplit_strerror (&ws));
      return MU_ERR_FAILURE;
    }
  else if (ws.ws_wordc == 0)
    {
      mu_error (_("expanding %s yields empty string"), filter_pat);
      mu_wordsplit_free (&ws);
      mu_argcv_free (nattrs, attrs);
      return MU_ERR_FAILURE;
    }
  
  rc = ldap_search_ext (ld, ldap_param.base, LDAP_SCOPE_SUBTREE,
			ws.ws_wordv[0], attrs, 0,
			NULL, NULL, NULL, -1, &msgid);
  mu_wordsplit_free (&ws);
  mu_argcv_free (nattrs, attrs);

  if (rc != LDAP_SUCCESS)
    {
      mu_error ("ldap_search_ext: %s", ldap_err2string (rc));
      if (rc == LDAP_NO_SUCH_OBJECT)
	return MU_ERR_NOENT;
      else
	return MU_ERR_FAILURE;
    }

  rc = ldap_result (ld, msgid, LDAP_MSG_ALL, NULL, &res );
  if (rc < 0)
    {
      mu_error ("ldap_result failed");
      return MU_ERR_FAILURE;
    }

  rc = ldap_count_entries (ld, res);
  if (rc == 0)
    {
      mu_error ("not enough entires");
      return MU_ERR_NOENT;
    }
  if (rc > 1)
    mu_error ("LDAP: too many entries for key %s", key);
      

  msg = ldap_first_entry (ld, res);
  rc = _mu_entry_to_auth_data (ld, msg, return_data);
  ldap_msgfree (res);
  
  return rc;
}



typedef int (*pwcheck_fp) (const char *, const char *);


static int
chk_crypt (const char *db_pass, const char *pass)
{
  return strcmp (db_pass, crypt (pass, db_pass)) == 0 ?
              0 : MU_ERR_AUTH_FAILURE;
}

static int
chk_md5 (const char *db_pass, const char *pass)
{
  unsigned char md5digest[16];
  unsigned char d1[16];
  struct mu_md5_ctx md5context;
  mu_stream_t str = NULL, flt = NULL;
  
  mu_md5_init_ctx (&md5context);
  mu_md5_process_bytes (pass, strlen (pass), &md5context);
  mu_md5_finish_ctx (&md5context, md5digest);

  mu_static_memory_stream_create (&str, db_pass, strlen (db_pass));
  mu_filter_create (&flt, str, "base64", MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);

  mu_stream_read (flt, (char*) d1, sizeof d1, NULL);
  mu_stream_destroy (&flt);
  
  return memcmp (md5digest, d1, sizeof md5digest) == 0 ?
                  0 : MU_ERR_AUTH_FAILURE;
}

static int
chk_smd5 (const char *db_pass, const char *pass)
{
  int rc;
  unsigned char md5digest[16];
  unsigned char *d1;
  struct mu_md5_ctx md5context;
  mu_stream_t str = NULL, flt = NULL;
  size_t size;
  
  size = strlen (db_pass);
  mu_static_memory_stream_create (&str, db_pass, size);
  mu_filter_create (&flt, str, "base64", MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);

  d1 = malloc (size);
  if (!d1)
    {
      mu_stream_destroy (&flt);
      return ENOMEM;
    }
  
  mu_stream_read (flt, (char*) d1, size, &size);
  mu_stream_destroy (&flt);

  if (size <= 16)
    {
      mu_error ("malformed SMD5 password: %s", db_pass);
      return MU_ERR_FAILURE;
    }
  
  mu_md5_init_ctx (&md5context);
  mu_md5_process_bytes (pass, strlen (pass), &md5context);
  mu_md5_process_bytes (d1 + 16, size - 16, &md5context);
  mu_md5_finish_ctx (&md5context, md5digest);

  rc = memcmp (md5digest, d1, sizeof md5digest) == 0 ?
                  0 : MU_ERR_AUTH_FAILURE;
  free (d1);
  return rc;
}

static int
chk_sha (const char *db_pass, const char *pass)
{
  unsigned char sha1digest[20];
  unsigned char d1[20];
  mu_stream_t str = NULL, flt = NULL;
  struct mu_sha1_ctx sha1context;
   
  mu_sha1_init_ctx (&sha1context);
  mu_sha1_process_bytes (pass, strlen (pass), &sha1context);
  mu_sha1_finish_ctx (&sha1context, sha1digest);

  mu_static_memory_stream_create (&str, db_pass, strlen (db_pass));
  mu_filter_create (&flt, str, "base64", MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);

  mu_stream_read (flt, (char*) d1, sizeof d1, NULL);
  mu_stream_destroy (&flt);
  
  return memcmp (sha1digest, d1, sizeof sha1digest) == 0 ?
                  0 : MU_ERR_AUTH_FAILURE;
}

static int
chk_ssha (const char *db_pass, const char *pass)
{
  int rc;
  unsigned char sha1digest[20];
  unsigned char *d1;
  struct mu_sha1_ctx sha1context;
  mu_stream_t str = NULL, flt = NULL;
  size_t size;
  
  size = strlen (db_pass);
  mu_static_memory_stream_create (&str, db_pass, size);
  mu_filter_create (&flt, str, "base64", MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);

  d1 = malloc (size);
  if (!d1)
    {
      mu_stream_destroy (&flt);
      return ENOMEM;
    }
  
  mu_stream_read (flt, (char*) d1, size, &size);
  mu_stream_destroy (&flt);

  if (size <= 16)
    {
      mu_error ("malformed SSHA1 password: %s", db_pass);
      return MU_ERR_FAILURE;
    }
  
  mu_sha1_init_ctx (&sha1context);
  mu_sha1_process_bytes (pass, strlen (pass), &sha1context);
  mu_sha1_process_bytes (d1 + 20, size - 20, &sha1context);
  mu_sha1_finish_ctx (&sha1context, sha1digest);

  rc = memcmp (sha1digest, d1, sizeof sha1digest) == 0 ?
                  0 : MU_ERR_AUTH_FAILURE;
  free (d1);
  return rc;
}

static struct passwd_algo
{
  char *algo;
  size_t len;
  pwcheck_fp pwcheck;
} pwtab[] = {
#define DP(s, f) { #s, sizeof (#s) - 1, f }
  DP (CRYPT, chk_crypt),
  DP (MD5, chk_md5),
  DP (SMD5, chk_smd5),
  DP (SHA, chk_sha),
  DP (SSHA, chk_ssha),
  { NULL }
#undef DP
};

static pwcheck_fp
find_pwcheck (const char *algo, int len)
{
  struct passwd_algo *p;
  for (p = pwtab; p->algo; p++)
    if (len == p->len && mu_c_strncasecmp (p->algo, algo, len) == 0)
      return p->pwcheck;
  return NULL;
}

static int
mu_ldap_authenticate (struct mu_auth_data **return_data MU_ARG_UNUSED,
		      const void *key,
		      void *func_data MU_ARG_UNUSED, void *call_data)
{
  const struct mu_auth_data *auth_data = key;
  char *db_pass = auth_data->passwd;
  char *pass = call_data;

  if (auth_data->passwd == NULL || !pass)
    return EINVAL;

  if (db_pass[0] == '{')
    {
      int len;
      char *algo;
      pwcheck_fp pwcheck;

      
      algo = db_pass + 1;
      for (len = 0; algo[len] != '}'; len++)
	if (algo[len] == 0)
	  {
	    /* Possibly malformed password, try plaintext anyway */
	    return strcmp (db_pass, pass) == 0 ? 0 : MU_ERR_FAILURE;
	  }
      db_pass = algo + len + 1;
      pwcheck = find_pwcheck (algo, len);
      if (pwcheck)
	return pwcheck (db_pass, pass);
      else
	{
	  mu_error ("Unsupported password algorithm scheme: %.*s",
		    len, algo);
	  return MU_ERR_FAILURE;
	}
    }
  
  return strcmp (db_pass, pass) == 0 ? 0 : MU_ERR_AUTH_FAILURE;
}


static int
mu_auth_ldap_user_by_name (struct mu_auth_data **return_data,
			   const void *key,
			   void *func_data MU_ARG_UNUSED,
			   void *call_data MU_ARG_UNUSED)
{
  int rc;
  LDAP *ld;

  if (!ldap_param.enable)
    return ENOSYS;
  if (_mu_conn_setup (&ld))
    return MU_ERR_FAILURE;
  if (_mu_ldap_bind (ld))
    return MU_ERR_FAILURE;
  rc = _mu_ldap_search (ld, ldap_param.getpwnam_filter, key, return_data);
  _mu_ldap_unbind (ld);
  return rc;
}

static int
mu_auth_ldap_user_by_uid (struct mu_auth_data **return_data,
			  const void *key,
			  void *func_data MU_ARG_UNUSED,
			  void *call_data MU_ARG_UNUSED)
{
  int rc;
  LDAP *ld;
  char uidstr[128];

  if (!ldap_param.enable)
    return ENOSYS;
  if (_mu_conn_setup (&ld))
    return MU_ERR_FAILURE;
  if (_mu_ldap_bind (ld))
    return MU_ERR_FAILURE;

  snprintf (uidstr, sizeof (uidstr), "%u", *(uid_t*)key);
  rc = _mu_ldap_search (ld, ldap_param.getpwuid_filter, uidstr, return_data);
  _mu_ldap_unbind (ld);
  return rc;
}


#else
# define mu_ldap_module_init NULL
# define mu_ldap_authenticate mu_auth_nosupport
# define mu_auth_ldap_user_by_name mu_auth_nosupport 
# define mu_auth_ldap_user_by_uid mu_auth_nosupport
#endif

struct mu_auth_module mu_auth_ldap_module = {
  "ldap",
  mu_ldap_module_init,
  mu_ldap_authenticate,
  NULL,
  mu_auth_ldap_user_by_name,
  NULL,
  mu_auth_ldap_user_by_uid,
  NULL
};
