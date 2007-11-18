/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005,
   2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_CMDLINE_H
#define _MAILUTILS_CMDLINE_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "mailutils/types.h"
#include "mailutils/gocs.h"
#include "mailutils/nls.h"
#include "mailutils/error.h"
#include "mailutils/errno.h"
#include "argp.h"
#include <errno.h> /* May declare program_invocation_name */
#include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  OPT_LOG_FACILITY = 256,          
  OPT_LOCK_FLAGS,            
  OPT_LOCK_RETRY_COUNT,      
  OPT_LOCK_RETRY_TIMEOUT,    
  OPT_LOCK_EXPIRE_TIMEOUT,   
  OPT_LOCK_EXTERNAL_PROGRAM, 
  OPT_SHOW_OPTIONS,          
  OPT_LICENSE,               
  OPT_MAILBOX_TYPE,          
  OPT_CRAM_PASSWD, 
  OPT_PAM_SERVICE, 
  OPT_AUTH_REQUEST,
  OPT_GETPWNAM_REQUEST,
  OPT_GETPWUID_REQUEST, 
  OPT_RADIUS_DIR,       
  OPT_SQL_INTERFACE,        
  OPT_SQL_GETPWNAM,         
  OPT_SQL_GETPWUID,         
  OPT_SQL_GETPASS,          
  OPT_SQL_HOST,             
  OPT_SQL_USER,             
  OPT_SQL_PASSWD,           
  OPT_SQL_DB,               
  OPT_SQL_PORT,             
  OPT_SQL_MU_PASSWORD_TYPE, 
  OPT_SQL_FIELD_MAP,        
  OPT_TLS,         
  OPT_SSL_CERT,   
  OPT_SSL_KEY,    
  OPT_SSL_CAFILE, 
  OPT_PWDDIR,
  OPT_AUTHORIZATION,
  OPT_AUTHENTICATION,
  OPT_CLEAR_AUTHORIZATION,
  OPT_CLEAR_AUTHENTICATION,
  OPT_NO_USER_RCFILE,
  OPT_NO_SITE_RCFILE,
  OPT_RCFILE,
  OPT_CLEAR_INCLUDE_PATH,
  OPT_CLEAR_LIBRARY_PATH
};

struct mu_cmdline_capa
{
  char *name;
  struct argp_child *child;
};

extern struct mu_cmdline_capa mu_common_cmdline;
extern struct mu_cmdline_capa mu_logging_cmdline;
extern struct mu_cmdline_capa mu_license_cmdline;
extern struct mu_cmdline_capa mu_mailbox_cmdline;
extern struct mu_cmdline_capa mu_locking_cmdline;
extern struct mu_cmdline_capa mu_address_cmdline;
extern struct mu_cmdline_capa mu_mailer_cmdline;
extern struct mu_cmdline_capa mu_daemon_cmdline;
extern struct mu_cmdline_capa mu_sieve_cmdline;
  
extern struct mu_cmdline_capa mu_pam_cmdline;
extern struct mu_cmdline_capa mu_gsasl_cmdline;
extern struct mu_cmdline_capa mu_tls_cmdline;
extern struct mu_cmdline_capa mu_radius_cmdline;
extern struct mu_cmdline_capa mu_sql_cmdline;
extern struct mu_cmdline_capa mu_virtdomain_cmdline;
extern struct mu_cmdline_capa mu_auth_cmdline;

extern char *mu_license_text;

extern void mu_libargp_init (void);
  
extern struct argp *mu_argp_build (const struct argp *argp);
extern void mu_argp_done (struct argp *argp);
  
extern int mu_register_argp_capa (const char *name, struct argp_child *child);

extern void mu_print_options (void);
extern const char *mu_check_option (char *name);
  
#ifdef __cplusplus
}
#endif

#endif

