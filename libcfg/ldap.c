/* This file is part of GNU Mailutils
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include "mailutils/libcfg.h"
#include "mailutils/mutil.h"
#include "mailutils/ldap.h"

static struct mu_ldap_module_config ldap_settings;

static int
cb_field_map (mu_cfg_locus_t *locus, void *data, char *arg)
{
  int err;
  int rc = mutil_parse_field_map (arg, &ldap_settings.field_map, &err);
  if (rc)
    mu_error (_("%s:%d: Error near element %d: %s"),
	      locus->file, locus->line, err, mu_strerror (rc));
  return 0;
}

static struct mu_cfg_param mu_ldap_param[] = {
  { "enable", mu_cfg_bool, &ldap_settings.enable },
  { "url", mu_cfg_string, &ldap_settings.url },
  { "base", mu_cfg_string, &ldap_settings.base },
  { "binddn", mu_cfg_string, &ldap_settings.binddn },
  { "passwd", mu_cfg_string, &ldap_settings.passwd },
  { "tls", mu_cfg_bool, &ldap_settings.tls },
  { "debug", mu_cfg_int, &ldap_settings.debug },
  { "field-map", mu_cfg_callback, NULL, cb_field_map },
  { "getpwnam", mu_cfg_string, &ldap_settings.getpwnam_filter },
  { "getpwuid", mu_cfg_string, &ldap_settings.getpwuid_filter },
  { NULL }
};

int									      
mu_ldap_section_parser
   (enum mu_cfg_section_stage stage, const mu_cfg_node_t *node,	      
    void *section_data, void *call_data)				      
{									      
  switch (stage)							      
    {									      
    case mu_cfg_section_start:
      ldap_settings.enable = 1;
      break;								      
      									      
    case mu_cfg_section_end:						      
      mu_gocs_store ("ldap", &ldap_settings);	      
    }									      
  return 0;								      
}

struct mu_cfg_capa mu_ldap_cfg_capa = {                
  "ldap",  mu_ldap_param, mu_ldap_section_parser
};
