/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* Preauth support for imap4d */

#include "imap4d.h"
#include "des.h"


/* Stdio preauth */

static char *
do_preauth_stdio ()
{
  struct passwd *pw = getpwuid (getuid ());
  return pw ? mu_strdup (pw->pw_name) : NULL;
}


/* IDENT (AUTH, RFC1413) preauth */
#define USERNAME_C "USERID :"

/* If the reply matches sscanf expression
   
      "%*[^:]: USERID :%*[^:]:%s"

   return a pointer to the %s part.  Otherwise, return NULL. */

static char *
ident_extract_username (char *reply)
{
  char *p;

  p = strchr (reply, ':');
  if (!p)
    return NULL;
  if (p[1] != ' ' || strncmp (p + 2, USERNAME_C, sizeof (USERNAME_C) - 1))
    return NULL;
  p += 2 + sizeof (USERNAME_C) - 1;
  p = strchr (p, ':');
  if (!p)
    return NULL;
  do
    p++;
  while (*p == ' ');
  return p;
}

static int
is_des_p (const char *name)
{
  int len = strlen (name);
  return len > 1 && name[0] == '[' && name[len-1] == ']';
}

#define smask(step) ((1<<step)-1)
#define pstep(x,step) (((x)&smask(step))^(((x)>>step)&smask(step)))
#define parity_char(x) pstep(pstep(pstep((x),4),2),1)

static void
des_fixup_key_parity (unsigned char key[8])
{
  int i;
  for (i = 0; i < 8; i++)
    {
      key[i] &= 0xfe;
      key[i] |= 1 ^ parity_char (key[i]);
    }
}

static void
des_cbc_cksum (gl_des_ctx *ctx, unsigned char *buf, size_t bufsize,
	       unsigned char out[8], unsigned char key[8])
{
  while (bufsize > 0)
    {
      if (bufsize >= 8)
	{
	  unsigned char *p = key;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  *p++ ^= *buf++;
	  bufsize -= 8;
	}
      else
	{
	  unsigned char *p = key + bufsize;
	  buf += bufsize;
	  switch (bufsize) {
	  case 7:
	    *--p ^= *--buf;
	  case 6:
	    *--p ^= *--buf;
	  case 5:
	    *--p ^= *--buf;
	  case 4:
	    *--p ^= *--buf;
	  case 3:
	    *--p ^= *--buf;
	  case 2:
	    *--p ^= *--buf;
	  case 1:
	    *--p ^= *--buf;
	  }
	  bufsize = 0;
	}
      gl_des_ecb_crypt (ctx, (char*) key, (char*) key, 0);
    }
}

static void
des_string_to_key (char *buf, size_t bufsize, unsigned char key[8])
{
  size_t i;
  int j;
  unsigned temp;
  unsigned char *p;
  char *p_char;
  char k_char[64];
  gl_des_ctx context;
  char *pstr;
  int forward = 1;
  p_char = k_char;
  memset (k_char, 0, sizeof (k_char));
  
  /* get next 8 bytes, strip parity, xor */
  pstr = buf;
  for (i = 1; i <= bufsize; i++)
    {
      /* get next input key byte */
      temp = (unsigned int) *pstr++;
      /* loop through bits within byte, ignore parity */
      for (j = 0; j <= 6; j++)
	{
	  if (forward)
	    *p_char++ ^= (int) temp & 01;
	  else
	    *--p_char ^= (int) temp & 01;
	  temp = temp >> 1;
	}
      while (--j > 0);

      /* check and flip direction */
      if ((i%8) == 0)
	forward = !forward;
    }
  
  p_char = k_char;
  p = (unsigned char *) key;

  for (i = 0; i <= 7; i++)
    {
      temp = 0;
      for (j = 0; j <= 6; j++)
	temp |= *p_char++ << (1 + j);
      *p++ = (unsigned char) temp;
    }

  des_fixup_key_parity (key);
  gl_des_setkey (&context, (char*) key);
  des_cbc_cksum (&context, (unsigned char*) buf, bufsize, key, key);
  memset (&context, 0, sizeof context);
  des_fixup_key_parity (key);
}

static int
decode64_buf (const char *name, unsigned char **pbuf, size_t *psize)
{
  mu_stream_t str = NULL, flt = NULL;
  size_t namelen;
  unsigned char buf[512];
  size_t size;
  
  name++;
  namelen = strlen (name) - 1;
  mu_static_memory_stream_create (&str, name, namelen);
  mu_filter_create (&flt, str, "base64", MU_FILTER_DECODE, MU_STREAM_READ);
  mu_stream_unref (str);
  mu_stream_read (flt, buf, sizeof buf, &size);
  mu_stream_destroy (&flt);
  *pbuf = malloc (size);
  if (!*pbuf)
    return 1;
  memcpy (*pbuf, buf, size);
  *psize = size;
  return 0;
}

struct ident_info
{
  uint32_t checksum;
  uint16_t random;
  uint16_t uid;
  uint32_t date;
  uint32_t ip_local;
  uint32_t ip_remote;
  uint16_t port_local;
  uint16_t port_remote;
};

union ident_data
{
  struct ident_info fields;
  unsigned long longs[6];
  unsigned char chars[24];
};

char *
ident_decrypt (const char *file, const char *name)
{
  unsigned char *buf = NULL;
  size_t size = 0;
  int fd;
  char keybuf[1024];
  union ident_data id;
  
  if (decode64_buf (name, &buf, &size))
    return NULL;

  if (size != 24)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("incorrect length of IDENT DES packet"));
      free (buf);
      return NULL;
    }

  fd = open (file, O_RDONLY);
  if (fd < 0)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("cannot open file %s: %s"),
		      file, mu_strerror (errno));
      return NULL;
    }

  while (read (fd, keybuf, sizeof (keybuf)) == sizeof (keybuf))
    {
      int i;
      unsigned char key[8];
      gl_des_ctx ctx;
      
      des_string_to_key (keybuf, sizeof (keybuf), key);
      gl_des_setkey (&ctx, (char*) key);
      
      memcpy (id.chars, buf, size);
      
      gl_des_ecb_decrypt (&ctx, (char *)&id.longs[4], (char *)&id.longs[4]);
      id.longs[4] ^= id.longs[2];
      id.longs[5] ^= id.longs[3];

      gl_des_ecb_decrypt (&ctx, (char *)&id.longs[2], (char *)&id.longs[2]);
      id.longs[2] ^= id.longs[0];
      id.longs[3] ^= id.longs[1];
      
      gl_des_ecb_decrypt (&ctx, (char *)&id.longs[0], (char *)&id.longs[0]);

      for (i = 1; i < 6; i++)
	id.longs[0] ^= id.longs[i];

      if (id.fields.checksum == 0)
	break;
    }
  close (fd);
  free (buf);
  
  if (id.fields.checksum == 0)
    {
      uid_t uid = ntohs (id.fields.uid);
      auth_data = mu_get_auth_by_uid (uid);
      if (!auth_data)
	{
	  mu_diag_output (MU_DIAG_ERROR, _("no user with UID %u"), uid);
	  return NULL;
	}
      return auth_data->name;
    }
  else
    mu_diag_output (MU_DIAG_ERROR, _("failed to decrypt IDENT reply"));
  return NULL;
}

static char *
do_preauth_ident (struct sockaddr *clt_sa, struct sockaddr *srv_sa)
{
  mu_stream_t stream;
  char hostaddr[16];
  int rc;
  char *buf = NULL;
  size_t size = 0;
  char *name = NULL;
  struct sockaddr_in *srv_addr, *clt_addr;
  char *p;

  if (!srv_sa || !clt_sa)
    {
      mu_diag_output (MU_DIAG_ERROR, _("not enough data for IDENT preauth"));
      return NULL;
    }
  if (srv_sa->sa_family != AF_INET)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("invalid address family (%d) for IDENT preauth"),
		      srv_sa->sa_family);
      return NULL;
    }
  srv_addr = (struct sockaddr_in *) srv_sa;
  clt_addr = (struct sockaddr_in *) clt_sa;
  p = inet_ntoa (clt_addr->sin_addr);
  
  memcpy (hostaddr, p, 15);
  hostaddr[15] = 0;
  rc = mu_tcp_stream_create (&stream, hostaddr, ident_port, MU_STREAM_RDWR);
  if (rc)
    {
      mu_diag_output (MU_DIAG_INFO, _("cannot create TCP stream: %s"),
		      mu_strerror (rc));
      return NULL;
    }

  mu_stream_printf (stream, "%u , %u\n",
		    ntohs (clt_addr->sin_port),
		    ntohs (srv_addr->sin_port));
  mu_stream_shutdown (stream, MU_STREAM_WRITE);

  rc = mu_stream_getline (stream, &buf, &size, NULL);
  mu_stream_close (stream);
  mu_stream_destroy (&stream);
  if (rc)
    {
      mu_diag_output (MU_DIAG_INFO, _("cannot read answer from %s:%d: %s"),
		      hostaddr, ident_port, mu_strerror (rc));
      return NULL;
    }
  mu_diag_output (MU_DIAG_INFO, "Got %s", buf);
  name = ident_extract_username (mu_str_stripws (buf));
  if (!name)
    mu_diag_output (MU_DIAG_INFO,
		    _("malformed IDENT response: `%s', from %s:%d"),
		    buf, hostaddr, ident_port);
  else if (is_des_p (name))
    {
      if (!ident_keyfile)
	{
	  mu_diag_output (MU_DIAG_ERROR,
			  _("keyfile not specified in config; "
			    "use `ident-keyfile FILE'"));
	  name = NULL;
	}
      else
	name = ident_decrypt (ident_keyfile, name);
    }
  else if (ident_encrypt_only)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("refusing unencrypted ident reply from %s:%d"),
		      hostaddr, ident_port);
      name = NULL;
    }
  else
    {
      mu_diag_output (MU_DIAG_INFO, "USERNAME %s", name);
      name = mu_strdup (name);
    }
  
  free (buf);
  return name;
}


#define SEQ(s, n, l) \
  (((l) == (sizeof(s) - 1)) && memcmp (s, n, l) == 0)

struct preauth_closure
{
  struct sockaddr_in *s_clt, *s_srv;
};

static const char *
preauth_getvar (const char *name, size_t nlen, void *data)
{
  struct preauth_closure *clos = data;
  
  if (clos->s_clt && clos->s_clt->sin_family == AF_INET)
    {
      if (SEQ ("client_address", name, nlen))
        return inet_ntoa (clos->s_clt->sin_addr);
      if (SEQ ("client_prot", name, nlen))
        return mu_umaxtostr (0, ntohs (clos->s_clt->sin_port));
    }
  if (clos->s_srv && clos->s_srv->sin_family == AF_INET)
    {
      if (SEQ ("server_address", name, nlen))
        return inet_ntoa (clos->s_srv->sin_addr);
      if (SEQ ("server_port", name, nlen))
        return mu_umaxtostr (0, ntohs (clos->s_srv->sin_port));
    }
  return NULL;
}
  
/* External (program) preauth */
static char *
do_preauth_program (struct sockaddr *pcs, struct sockaddr *sa)
{
  int rc;
  mu_stream_t str;
  char *buf = NULL;
  size_t size = 0, n;
  struct mu_wordsplit ws;
  struct preauth_closure clos;

  clos.s_clt = (struct sockaddr_in *) pcs;
  clos.s_srv = (struct sockaddr_in *) sa;

  ws.ws_getvar = preauth_getvar;
  ws.ws_closure = &clos;
  if (mu_wordsplit (preauth_program, &ws,
		    MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD |
		    MU_WRDSF_GETVAR | MU_WRDSF_CLOSURE))
    {
      mu_error (_("cannot expand line `%s': %s"), preauth_program,
		mu_wordsplit_strerror (&ws));
      return NULL;
    }
  else if (ws.ws_wordc == 0)
    {
      mu_wordsplit_free (&ws);
      mu_error (_("`%s' expands to an empty line"), preauth_program);
      return NULL;
    }

  rc = mu_command_stream_create (&str, ws.ws_wordv[0], MU_STREAM_READ);
  mu_wordsplit_free (&ws);
  if (rc)
    {
      mu_error (_("cannot open input pipe from %s"), preauth_program);
      return NULL;
    }
  rc = mu_stream_getline (str, &buf, &size, &n);
  mu_stream_destroy (&str);
  if (rc)
    {
      mu_error (_("read from `%s' failed"), preauth_program);
    }
  else
    {
      mu_rtrim_class (buf, MU_CTYPE_ENDLN);
      return buf;
    }
  return NULL;
}

int
imap4d_preauth_setup (int fd)
{
  struct sockaddr clt_sa, *pclt_sa; 
  socklen_t clt_len = sizeof clt_sa;
  struct sockaddr srv_sa, *psrv_sa;
  socklen_t srv_len = sizeof srv_sa;
  char *username = NULL;

  if (getsockname (fd, &srv_sa, &srv_len) == -1)
    {
      psrv_sa = NULL;
      srv_len = 0;
    }
  else
    psrv_sa = &srv_sa;

  if (test_mode)
    {
      pclt_sa = NULL;
      clt_len = 0;
    }
  else if (getpeername (fd, (struct sockaddr *) &clt_sa, &clt_len) == -1)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("cannot obtain IP address of client: %s"),
		      strerror (errno));
      pclt_sa = NULL;
      clt_len = 0;
    }
  else
    pclt_sa = &clt_sa;

  auth_data = NULL;
  switch (preauth_mode)
    {
    case preauth_none:
      return 0;
      
    case preauth_stdio:
      username = do_preauth_stdio ();
      break;
      
    case preauth_ident:
      username = do_preauth_ident (pclt_sa, psrv_sa);
      break;
      
    case preauth_prog:
      username = do_preauth_program (pclt_sa, psrv_sa);
      break;
    }

  if (username)
    {
      int rc;
      
      if (auth_data)
	rc = imap4d_session_setup0 ();
      else
	{
	  rc = imap4d_session_setup (username);
	  free (username);
	}
      if (rc == 0)
	{
	  state = STATE_AUTH;
	  return 0;
	}
    }
  
  return preauth_only;
}
