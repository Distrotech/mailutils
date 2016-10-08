/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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

#include <mailutils/mailutils.h>

union value
{
  char *v_string;
  signed short v_short;
  unsigned short v_ushort;
  int v_int;
  unsigned v_uint;
  long v_long;
  unsigned long v_ulong;
  size_t v_size;
  time_t v_time;
  struct mu_cidr v_cidr;
};

struct value_handler
{
  void (*format) (union value *, FILE *);
  int (*compare) (union value *, union value *);
};

static void
v_string_format (union value *val, FILE *fp)
{
  char *p;
	
  fputc ('"', fp);
  for (p = val->v_string; *p; p++)
    {
      if (*p == '\\' || *p == '"')
	fputc ('\\', fp);
      fputc (*p, fp);
    }
  fputc ('"', fp);
}
  
static int
v_string_compare (union value *a, union value *b)
{
  return strcmp (a->v_string, b->v_string);
}

static void
v_short_format (union value *val, FILE *fp)
{
  fprintf (fp, "%hd", val->v_short); 
}

static int
v_short_compare (union value *a, union value *b)
{
  return a->v_short != b->v_short;
}

static void
v_ushort_format (union value *val, FILE *fp)
{
  fprintf (fp, "%hu", val->v_short);
}

static int
v_ushort_compare (union value *a, union value *b)
{
  return a->v_ushort != b->v_ushort;
}

static void
v_int_format (union value *val, FILE *fp)
{
  fprintf (fp, "%d", val->v_int);  
}

static int
v_int_compare (union value *a, union value *b)
{
  return a->v_int != b->v_int;
}

static void
v_uint_format (union value *val, FILE *fp)
{
  fprintf (fp, "%u", val->v_uint);
}

static int
v_uint_compare (union value *a, union value *b)
{
  return a->v_uint != b->v_uint;
}

static void
v_long_format (union value *val, FILE *fp)
{
  fprintf (fp, "%ld", val->v_long);  
}

static int
v_long_compare (union value *a, union value *b)
{
  return a->v_long != b->v_long;
}

static void
v_ulong_format (union value *val, FILE *fp)
{
  fprintf (fp, "%lu", val->v_long);  
}

static int
v_ulong_compare (union value *a, union value *b)
{
  return a->v_ulong != b->v_ulong;
}

static void
v_size_format (union value *val, FILE *fp)
{
  size_t v = val->v_size;
  char buf[80];
  size_t buflen = 80;
  char *p = buf + buflen;
  *--p = 0;
  do
    {
      int n = v % 10;
      *--p = n + '0';
      v /= 10;
    }
  while (v && p > buf);

  if (v)
    fputs ("[overflow]", fp);
  else
    fputs (p, fp);
}

static int
v_size_compare (union value *a, union value *b)
{
  return a->v_size != b->v_size;
}

static void
v_cidr_format (union value *val, FILE *fp)
{  
  char *buf;

  mu_cidr_format (&val->v_cidr, 0, &buf);
  fprintf (fp, "%s", buf);
  free (buf);
}

static int
v_cidr_compare (union value *a, union value *b)
{
  if (a->v_cidr.family != b->v_cidr.family)
    return 1;
  if (a->v_cidr.len != b->v_cidr.len)
    return 1;
  if (memcmp (a->v_cidr.address, b->v_cidr.address, a->v_cidr.len))
    return 1;
  if (memcmp (a->v_cidr.netmask, b->v_cidr.netmask, a->v_cidr.len))
    return 1;
  return 0;
}

struct value_handler value_handler[] = {
  [mu_c_string] = { v_string_format, v_string_compare },
  [mu_c_short]  = { v_short_format, v_short_compare },
  [mu_c_ushort] = { v_ushort_format, v_ushort_compare },
  [mu_c_int]    = { v_int_format, v_int_compare },
  [mu_c_uint]   = { v_uint_format, v_uint_compare },
  [mu_c_long]   = { v_long_format, v_long_compare },
  [mu_c_ulong]  = { v_ulong_format, v_ulong_compare },
  [mu_c_size]   = { v_size_format, v_size_compare },
#if 0
    mu_c_time,
#endif
  [mu_c_bool]   = { v_int_format, v_int_compare },
  [mu_c_cidr]   = { v_cidr_format, v_cidr_compare },
  [mu_c_incr]   = { v_int_format, v_int_compare },
};

int
valcmp (mu_c_type_t type, union value *a, union value *b)
{
  if ((size_t)type < sizeof (value_handler) / sizeof (value_handler[0])
      && value_handler[type].compare)
    return value_handler[type].compare (a, b);
  else
    {
      fprintf (stderr, "unsupported value type: %d\n", type);
      abort ();
    }
}

void
valprint (FILE *fp, mu_c_type_t type, union value *val)
{
  if ((size_t)type < sizeof (value_handler) / sizeof (value_handler[0])
      && value_handler[type].format)
    value_handler[type].format (val, fp);
  else
    {
      fprintf (stderr, "unsupported value type: %d\n", type);
      abort ();
    }
}

struct testdata
{
  mu_c_type_t type;
  char const *input;
  int err;
  union value val;
};

struct testdata tests[] = {
  { mu_c_string, "now is the time", 0, { .v_string = "now is the time" } },
  { mu_c_short,  "115",             0, { .v_short = 115 } },
  { mu_c_short,  "-400",            0, { .v_short = -400 } },
  { mu_c_short,  "1a",              EINVAL,  },

  { mu_c_ushort, "110",             0, { .v_ushort = 110 } },
  { mu_c_ushort, "-110",            ERANGE  },

  { mu_c_int,    "10568",           0, { .v_int = 10568 } },
  { mu_c_int,    "-10568",          0, { .v_int = -10568 } },

  { mu_c_uint,   "10568",           0, { .v_uint = 10568 } },

  { mu_c_long,   "10568",           0, { .v_long = 10568 } },
  { mu_c_long,   "-10568",          0, { .v_long = -10568 } },

  { mu_c_ulong,  "10568",           0, { .v_ulong = 10568 } },
  
  { mu_c_bool,   "true",            0, { .v_int = 1 } },
  { mu_c_bool,   "false",           0, { .v_int = 0 } },

  { mu_c_cidr,   "127.0.0.0/8",     0, { .v_cidr = {
	.family = 2,
	.len = 4,
	.address = { 127, 0, 0, 0 },
	.netmask = { 255 } } } },
  { mu_c_cidr,   "fe80::4a5b:39ff:fe09:97f0/64", 0,  { .v_cidr = {
	.family = 10,
	.len = 16,
	.address = { 0xfe, 0x80, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,
		     0x4a, 0x5b, 0x39, 0xff, 0xfe, 0x9, 0x97, 0xf0 },
	.netmask = { 255, 255, 255, 255, 255, 255, 255, 255,
		     0,   0,   0,   0,   0,   0,   0,   0 } } } },
  
  { mu_c_incr,   NULL,              0, { .v_int = 1 } },
  { mu_c_void }
};

void
print_test_id (int i, FILE *fp)
{
  fprintf (fp, "%d: %s:%s: ", i, mu_c_type_str[tests[i].type],
	   tests[i].input ? tests[i].input : "NULL");
}	   

void
usage (int code, FILE *fp, char const *argv0)
{
  fprintf (fp, "usage: %s [-v]", argv0);
  exit (code);
}

int
main (int argc, char **argv)
{
  union value val;
  char *errmsg;
  int i;
  unsigned failures = 0;
  int verbose = 0;

  for (i = 1; i < argc; i++) {
    if (strcmp (argv[i], "-v") == 0)
      verbose = 1;
    else if (strcmp (argv[i], "-h") == 0)
      usage (0, stdout, argv[0]);
    else
      usage (1, stderr, argv[0]);
  }

  if (i != argc)
    usage (1, stderr, argv[0]);
  
  for (i = 0; tests[i].type != mu_c_void; i++)
    {
      int rc;

      memset (&val, 0, sizeof (val));
      rc = mu_str_to_c (tests[i].input, tests[i].type, &val, &errmsg);
      if (rc)
	{
	  if (tests[i].err == rc)
	    {
	      if (verbose)
		{
		  print_test_id (i, stdout);
		  fprintf (stdout, "XFAIL\n");
		}
	    }
	  else
	    {
	      print_test_id (i, stderr);
	      fprintf (stderr, "FAIL: error %s", mu_strerror (rc));
	      if (errmsg)
		fprintf (stderr, ": %s", errmsg);
	      fputc ('\n', stderr);
	      free (errmsg);
	      ++failures;
	    }
	}
      else if (valcmp (tests[i].type, &tests[i].val, &val))
	{
	  fprintf (stderr, "%d: FAIL: %s value differ: ",
		   i, mu_c_type_str[tests[i].type]);
	  fprintf (stderr, "expected: ");
	  valprint (stderr, tests[i].type, &tests[i].val);
	  fprintf (stderr, ", but got: ");
	  valprint (stderr, tests[i].type, &val);
	  fputc ('\n', stderr);
	  ++failures;
	}
      else if (verbose)
	{
	  print_test_id (i, stdout);
	  fprintf (stdout, "OK\n");
	}
    }
  exit (!!failures);
}
