/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This module implements execution of MH format strings. */

#include <mh.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

typedef struct       /* A string object type */
{
  int size;          /* Allocated size or 0 for static storage */
  char *ptr;         /* Actual data */
} strobj_t;

struct mh_machine
{
  strobj_t reg_str;         /* String register */
  int reg_num;              /* Numeric register */

  strobj_t arg_str;         /* String argument */
  int arg_num;              /* Numeric argument */
  
  size_t pc;                /* Program counter */
  size_t progsize;          /* Size of allocated program*/
  mh_opcode_t *prog;        /* Program itself */
  int stop;                 /* Stop execution immediately */

  char *outbuf;             /* Output buffer */
  size_t width;             /* Output buffer size */
  size_t ind;               /* Output buffer index */

  int fmtflags;             /* Current formatting flags */
  
  message_t message;        /* Current message */
  size_t msgno;             /* Its number */
};

/* Functions for handling string objects. */

void
strobj_free (strobj_t *obj)
{
  if (obj->size)
    free (obj->ptr);
  obj->size = 0;
  obj->ptr = NULL;
}

#define strobj_ptr(p) ((p)->ptr ? (p)->ptr : "")
#define strobj_len(p) strlen((p)->ptr)
#define strobj_is_static(p) ((p)->size == 0)

static void
strobj_create (strobj_t *lvalue, char *str)
{
  if (!str)
    {
      lvalue->size = 0;
      lvalue->ptr = NULL;
    }
  else
    {
      lvalue->size = strlen (str) + 1;
      lvalue->ptr = xmalloc (lvalue->size);
      memcpy (lvalue->ptr, str, lvalue->size);
    }
}

static void
strobj_set (strobj_t *lvalue, char *str)
{
  lvalue->size = 0;
  lvalue->ptr = str;
}

static void
strobj_assign (strobj_t *lvalue, strobj_t *rvalue)
{
  strobj_free (lvalue);
  *lvalue = *rvalue;
  rvalue->size = 0;
  rvalue->ptr = NULL;
}

static void
strobj_copy (strobj_t *lvalue, strobj_t *rvalue)
{
  if (lvalue->size >= strobj_len (rvalue) + 1)
    memcpy (lvalue->ptr, strobj_ptr (rvalue), strobj_len (rvalue) + 1);
  else
    {
      if (lvalue->size)
	strobj_free (lvalue);
      strobj_create (lvalue, strobj_ptr (rvalue));
    }
}

/* Compress whitespace in a string */
static void
compress_ws (char *str, size_t *size)
{
  unsigned char *p, *q;
  size_t len = *size;
  int space = 0;

  for (p = q = (unsigned char*) str; len; len--, q++)
    {
      if (isspace (*q))
	{
	  if (!space)
	    *p++ = ' ';
	  space++;
	  continue;
	}
      else if (space)
	space = 0;

      if (isprint (*q))
	*p++ = *q;
    }
  *p = 0;
  *size = p - (unsigned char*) str;
}

/* Print len bytes from str into mach->outbuf */
static void
print_string (struct mh_machine *mach, char *str, size_t len)
{
  size_t rest = strlen (str);

  if (len > rest)
    len = rest;
  rest = mach->width - mach->ind;
  if (len > rest)
    len = rest;
  memcpy (mach->outbuf + mach->ind, str, len);
  mach->ind += len;
}

static void
reset_fmt_defaults (struct mh_machine *mach)
{
  mach->fmtflags = 0;
}

/* Execute pre-compiled format on message msg with number msgno.
   buffer and bufsize specify output storage */
int
mh_format (mh_format_t *fmt, message_t msg, size_t msgno,
	   char *buffer, size_t bufsize)
{
  struct mh_machine mach;
  char buf[64];
  
  memset (&mach, 0, sizeof (mach));
  mach.progsize = fmt->progsize;
  mach.prog = fmt->prog;

  mach.message = msg;
  mach.msgno = msgno;
  
  mach.width = bufsize - 1;  /* Allow for terminating NULL */
  mach.outbuf = buffer;
  mach.pc = 1;
  reset_fmt_defaults (&mach);
  
  while (!mach.stop)
    {
      mh_opcode_t opcode;
      switch (opcode = mach.prog[mach.pc++])
	{
	case mhop_stop:
	  mach.stop = 1;
	  break;

	case mhop_branch:
	  mach.pc += mach.prog[mach.pc];
	  break;

	case mhop_num_asgn:
	  mach.reg_num = mach.arg_num;
	  break;
	  
	case mhop_str_asgn:
	  strobj_assign (&mach.reg_str, &mach.arg_str);
	  break;
	  
	case mhop_num_arg:
	  mach.arg_num = mach.prog[mach.pc++];
	  break;
	  
	case mhop_str_arg:
	  {
	    size_t skip = mach.prog[mach.pc++];
	    strobj_set (&mach.arg_str, (char*)&mach.prog[mach.pc]);
	    mach.pc += skip;
	  }
	  break;

	case mhop_num_branch:
	  if (!mach.arg_num)
	    mach.pc += mach.prog[mach.pc];
	  else
	    mach.pc++;
	  break;

	case mhop_str_branch:
	  if (!*strobj_ptr (&mach.arg_str))
	    mach.pc += mach.prog[mach.pc];
	  else
	    mach.pc++;
	  break;

	case mhop_call:
	  (*(mh_builtin_fp) mach.prog[mach.pc++]) (&mach);
	  break;

	case mhop_header:
	  {
	    header_t hdr = NULL;
	    char *value = NULL;
	    message_get_header (mach.message, &hdr);
	    header_aget_value (hdr, strobj_ptr (&mach.arg_str), &value);
	    strobj_free (&mach.arg_str);
	    if (value)
	      {
		int len = strlen (value);
		mach.arg_str.size = len + 1;
      		compress_ws (value, &len);
		mach.arg_str.ptr = value;
	      }
	  }
	  break;

	case mhop_body:
	  {
	    body_t body = NULL;
	    stream_t stream = NULL;
	    size_t size = 0, off, str_off, nread;
	    size_t rest = mach.width - mach.ind;

	    strobj_free (&mach.arg_str);
	    message_get_body (mach.message, &body);
	    body_size (body, &size);
	    body_get_stream (body, &stream);
	    if (size == 0 || !stream)
	      break;
	    if (size > rest)
	      size = rest;

	    mach.arg_str.ptr = xmalloc (size+1);
	    mach.arg_str.size = size;
	    
	    off = 0;
	    str_off = 0;
	    while (!stream_read (stream, mach.arg_str.ptr + str_off,
				 mach.arg_str.size - str_off, off, &nread)
		   && nread != 0
		   && str_off < size)
	      {
		off += nread;
       		compress_ws (mach.arg_str.ptr + str_off, &nread);
		if (nread)
		  str_off += nread;
	      }
	    mach.arg_str.ptr[str_off] = 0;
	  }
	  break;
	  
	  /* assign reg_num to arg_num */
	case mhop_num_to_arg:
	  mach.arg_num = mach.reg_num;
	  break;
	  
	  /* assign reg_str to arg_str */
	case mhop_str_to_arg:
	  strobj_copy (&mach.arg_str, &mach.reg_str);
	  break;

	  /* Convert arg_str to arg_num */
	case mhop_str_to_num:
	  mach.arg_num = strtoul (strobj_ptr (&mach.arg_str), NULL, 0);
	  break;
  
	  /* Convert arg_num to arg_str */
	case mhop_num_to_str:
	  snprintf (buf, sizeof buf, "%d", mach.arg_num);
	  strobj_free (&mach.arg_str);
	  strobj_create (&mach.arg_str, buf);
	  break;

	case mhop_num_print:
	  {
	    int n;
	    char buf[64];
	    char *ptr;
	    int fmtwidth = mach.fmtflags & MH_WIDTH_MASK;
	    int padchar = mach.fmtflags & MH_FMT_ZEROPAD ? '0' : ' ';
	    
	    n = snprintf (buf, sizeof buf, "%d", mach.reg_num);

	    if (fmtwidth)
	      {
		if (n > fmtwidth)
		  {
		    ptr = buf + n - fmtwidth;
		    *ptr = '?';
		  }
		else
		  {
		    int i;
		    ptr = buf;
		    for (i = n; i < fmtwidth && mach.ind < mach.width;
			 i++, mach.ind++)
		      mach.outbuf[mach.ind] = padchar;
		  }
	      }
	    else
	      ptr = buf;

	    print_string (&mach, ptr, strlen (ptr));
	    reset_fmt_defaults (&mach);
	  }
	  break;
	  
	case mhop_str_print:
	  if (mach.fmtflags)
	    {
	      int len = strobj_len (&mach.reg_str);
	      int fmtwidth = mach.fmtflags & MH_WIDTH_MASK;
	      int padchar = ' ';
				
	      if (mach.fmtflags & MH_FMT_RALIGN)
		{
		  int i, n;

		  n = fmtwidth - len;
		  for (i = 0; i < n && mach.ind < mach.width;
		       i++, mach.ind++, fmtwidth--)
		    mach.outbuf[mach.ind] = padchar;
		}
		
	      print_string (&mach, strobj_ptr (&mach.reg_str), fmtwidth);
	      reset_fmt_defaults (&mach);
	    }
	  else
	    {
	      print_string (&mach,
			   strobj_ptr (&mach.reg_str),
			   strobj_len (&mach.reg_str));
	    }
	  break;

	case mhop_fmtspec:
	  mach.fmtflags = mach.prog[mach.pc++];
	  break;
	  
	default:
	  mh_error ("Unknown opcode: %x", opcode);
	  abort ();
	}
    }
  strobj_free (&mach.reg_str);
  strobj_free (&mach.arg_str);
  mach.outbuf[mach.ind] = 0;
  return mach.ind;
}

/* Free any memory associated with a format structure. The structure
   itself is assumed to be in static storage. */
void
mh_format_free (mh_format_t *fmt)
{
  if (fmt->prog)
    free (fmt->prog);
  fmt->progsize = 0;
  fmt->prog = NULL;
}

/* Built-in functions */

/* Handler for unimplemented functions */
static void
builtin_not_implemented (char *name)
{
  mh_error ("%s is not yet implemented.", name);
}

static void
builtin_msg (struct mh_machine *mach)
{
  size_t msgno = mach->msgno;
  mh_message_number (mach->message, &msgno);
  mach->arg_num = msgno;
}

static void
builtin_cur (struct mh_machine *mach)
{
  size_t msgno = mach->msgno;
  mh_message_number (mach->message, &msgno);
  mach->arg_num = msgno == current_message;
}

static void
builtin_size (struct mh_machine *mach)
{
  message_size (mach->message, &mach->arg_num);
}

static void
builtin_strlen (struct mh_machine *mach)
{
  mach->arg_num = strlen (strobj_ptr (&mach->arg_str));
}

static void
builtin_width (struct mh_machine *mach)
{
  mach->arg_num = mach->width;
}

static void
builtin_charleft (struct mh_machine *mach)
{
  mach->arg_num = mach->width - mach->ind;
}

static void
builtin_timenow (struct mh_machine *mach)
{
  time_t t;
  
  time (&t);
  mach->arg_num = t;
}

static void
builtin_me (struct mh_machine *mach)
{
  /*FIXME*/
  /*  mach->arg_str = "me";*/
}

static void
builtin_eq (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num == mach->arg_num;
}

static void
builtin_ne (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num != mach->arg_num;
}

static void
builtin_gt (struct mh_machine *mach)
{
  mach->arg_num = mach->reg_num > mach->arg_num;
}

static void
builtin_match (struct mh_machine *mach)
{
  mach->arg_num = strstr (strobj_ptr (&mach->reg_str),
			  strobj_ptr (&mach->arg_str)) != NULL;
}

static void
builtin_amatch (struct mh_machine *mach)
{
  int len = strobj_len (&mach->arg_str);
  mach->arg_num = strncmp (strobj_ptr (&mach->reg_str),
			   strobj_ptr (&mach->arg_str), len);
}

static void
builtin_plus (struct mh_machine *mach)
{
  mach->arg_num += mach->reg_num;
}

static void
builtin_minus (struct mh_machine *mach)
{
  mach->arg_num -= mach->reg_num;
}

static void
builtin_divide (struct mh_machine *mach)
{
  if (!mach->arg_num)
    {
      mh_error ("format: divide by zero");
      mach->stop = 1;
    }
  else
    mach->arg_num = mach->reg_num / mach->arg_num;
}

static void
builtin_modulo (struct mh_machine *mach)
{
  if (!mach->arg_num)
    {
      mh_error ("format: divide by zero");
      mach->stop = 1;
    }
  else
    mach->arg_num = mach->reg_num % mach->arg_num;
}

static void
builtin_num (struct mh_machine *mach)
{
  mach->reg_num = mach->arg_num;
}

static void
builtin_lit (struct mh_machine *mach)
{
  strobj_assign (&mach->reg_str, &mach->arg_str);
}

static void
builtin_getenv (struct mh_machine *mach)
{
  char *val = getenv (strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, val);
}

static void
builtin_profile (struct mh_machine *mach)
{
  /*FIXME*/
  /*mach->arg_str = "profile";*/
}

static void
builtin_nonzero (struct mh_machine *mach)
{
    mach->arg_num = mach->reg_num;
}

static void
builtin_zero (struct mh_machine *mach)
{
  mach->arg_num = !mach->reg_num;
}

static void
builtin_null (struct mh_machine *mach)
{
  char *s = strobj_ptr (&mach->reg_str);
  mach->arg_num = !s && !s[0];
}

static void
builtin_nonnull (struct mh_machine *mach)
{
  char *s = strobj_ptr (&mach->reg_str);
  mach->arg_num = s && s[0];
}

/*     comp       comp     string   Set str to component text*/
static void
builtin_comp (struct mh_machine *mach)
{
  strobj_assign (&mach->reg_str, &mach->arg_str);
}

/*     compval    comp     integer  num set to "atoi(comp)"*/
static void
builtin_compval (struct mh_machine *mach)
{
  mach->reg_num = strtoul (strobj_ptr (&mach->arg_str), NULL, 0);
}

/*     trim       expr              trim trailing white-space from str*/
static void
builtin_trim (struct mh_machine *mach)
{
  char *p, *start;
  int len;
  
  if (strobj_is_static (&mach->arg_str))
    strobj_copy (&mach->arg_str, &mach->arg_str);

  start = strobj_ptr (&mach->arg_str);
  len = strlen (start);
  if (len == 0)
    return;
  for (p = start + len - 1; p >= start && isspace (*p); p--)
    ;
  p[1] = 0;
}

/*     putstr     expr              print str*/
static void
builtin_putstr (struct mh_machine *mach)
{
  /*FIXME*/
}

/*     putstrf    expr              print str in a fixed width*/
static void
builtin_putstrf (struct mh_machine *mach)
{
  /*FIXME*/
}

/*     putnum     expr              print num*/
static void
builtin_putnum (struct mh_machine *mach)
{
  /*FIXME*/
}

/*     putnumf    expr              print num in a fixed width*/
static void
builtin_putnumf (struct mh_machine *mach)
{
  /*FIXME*/
}

static int
_parse_date (struct mh_machine *mach, struct tm *tm, mu_timezone *tz)
{
  char *date = strobj_ptr (&mach->arg_str);
  const char *p = date;
  
  if (parse822_date_time(&p, date+strlen(date), tm, tz))
    {
      mh_error ("can't parse date: [%s]", date);
      return -1;
    }
  return 0;
}

/*     sec        date     integer  seconds of the minute*/
static void
builtin_sec (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_sec;
}

/*     min        date     integer  minutes of the hour*/
static void
builtin_min (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_min;
}

/*     hour       date     integer  hours of the day (0-23)*/
static void
builtin_hour (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_hour;
}

/*     wday       date     integer  day of the week (Sun=0)*/
static void
builtin_wday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_wday;
}

/*     day        date     string   day of the week (abbrev.)*/
static void
builtin_day (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%a", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*     weekday    date     string   day of the week */
static void
builtin_weekday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%A", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*      sday       date     integer  day of the week known?
	(0=implicit,-1=unknown) */
static void
builtin_sday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  /*FIXME: more elaborate check needed */
  if (_parse_date (mach, &tm, &tz))
    mach->arg_num = -1;
  else
    mach->arg_num = 1;
}

/*     mday       date     integer  day of the month*/
static void
builtin_mday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_mday;
}

/*      yday       date     integer  day of the year */
static void
builtin_yday (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_yday;
}

/*     mon        date     integer  month of the year*/
static void
builtin_mon (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_mon+1;
}

/*     month      date     string   month of the year (abbrev.) */
static void
builtin_month (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%b", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*      lmonth     date     string   month of the year*/
static void
builtin_lmonth (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  char buf[80];
  
  if (_parse_date (mach, &tm, &tz))
    return;

  strftime (buf, sizeof buf, "%B", &tm);
  strobj_free (&mach->arg_str);
  strobj_create (&mach->arg_str, buf);
}

/*     year       date     integer  year (may be > 100)*/
static void
builtin_year (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tm.tm_year + 1900;
}

/*     zone       date     integer  timezone in hours*/
static void
builtin_zone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;

  mach->arg_num = tz.utc_offset;
}

/*     tzone      date     string   timezone string */
static void
builtin_tzone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  if (_parse_date (mach, &tm, &tz))
    return;
  
  strobj_free (&mach->arg_str);
  if (tz.tz_name)
    strobj_create (&mach->arg_str, (char*) tz.tz_name);
  else
    {
      char buf[6];
      int s;
      if (tz.utc_offset < 0)
	{
	  s = '-';
	  tz.utc_offset = - tz.utc_offset;
	}
      else
	s = '+';
      snprintf (buf, sizeof buf, "%c%02d%02d", s,
		tz.utc_offset/3600, tz.utc_offset/60);
      strobj_create (&mach->arg_str, buf);
    }
}

/*      szone      date     integer  timezone explicit?
	(0=implicit,-1=unknown) */
static void
builtin_szone (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  /*FIXME: more elaborate check needed */
  if (_parse_date (mach, &tm, &tz))
    mach->arg_num = -1;
  else
    mach->arg_num = 1;
}

/*     date2local date              coerce date to local timezone*/
static void
builtin_date2local (struct mh_machine *mach)
{
  /*FIXME: Noop*/
}

/*     date2gmt   date              coerce date to GMT*/
static void
builtin_date2gmt (struct mh_machine *mach)
{
  /*FIXME: Noop*/
}

/*     dst        date     integer  daylight savings in effect?*/
static void
builtin_dst (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  if (_parse_date (mach, &tm, &tz))
    return;
#ifdef HAVE_STRUCT_TM_TM_ISDST  
  mach->arg_num = tm.tm_isdst;
#else
  mach->arg_num = 0;
#endif
}

/*     clock      date     integer  seconds since the UNIX epoch*/
static void
builtin_clock (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;

  if (_parse_date (mach, &tm, &tz))
    return;
  mach->arg_num = mu_tm2time (&tm, &tz);
}

/*     rclock     date     integer  seconds prior to current time*/
void
builtin_rclock (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  time_t now = time (NULL);
  
  if (_parse_date (mach, &tm, &tz))
    return;
  mach->arg_num = now - mu_tm2time (&tm, &tz);
}

/*      tws        date     string   official 822 rendering */
static void
builtin_tws (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     pretty     date     string   user-friendly rendering*/
static void
builtin_pretty (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     nodate     date     integer  str not a date string */
static void
builtin_nodate (struct mh_machine *mach)
{
  struct tm tm;
  mu_timezone tz;
  
  mach->arg_num = _parse_date (mach, &tm, &tz);
}

/*     proper     addr     string   official 822 rendering */
static void
builtin_proper (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     friendly   addr     string   user-friendly rendering*/
static void
builtin_friendly (struct mh_machine *mach)
{
  /*FIXME: noop*/
}

/*     addr       addr     string   mbox@host or host!mbox rendering*/
static void
builtin_addr (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    strobj_create (&mach->arg_str, buf);
  address_destroy (&addr);
}

/*     pers       addr     string   the personal name**/
static void
builtin_pers (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_personal (addr, 1, buf, sizeof buf, &n) == 0)
    strobj_create (&mach->arg_str, buf);
  address_destroy (&addr);
}

/*     note       addr     string   commentary text*/
static void
builtin_note (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_comments (addr, 1, buf, sizeof buf, &n) == 0)
    strobj_create (&mach->arg_str, buf);
  address_destroy (&addr);
}

/*     mbox       addr     string   the local mailbox**/
static void
builtin_mbox (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      char *p = strchr (buf, '@');
      if (p)
	*p = 0;
      strobj_create (&mach->arg_str, p);
    }
  address_destroy (&addr);
}

/*     mymbox     addr     integer  the user's addresses? (0=no,1=yes)*/
static void
builtin_mymbox (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  
  mach->arg_num = 0;
  if (address_create (&addr, strobj_ptr (&mach->arg_str)))
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    mach->arg_num = mh_is_my_name (buf);
  address_destroy (&addr);
}

/*     host       addr     string   the host domain**/
static void
builtin_host (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      char *p = strchr (buf, '@');
      if (p)
	strobj_create (&mach->arg_str, p+1);
    }
  address_destroy (&addr);
}

/*     nohost     addr     integer  no host was present**/
static void
builtin_nohost (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    mach->arg_num = strchr (buf, '@') != NULL;
  else
    mach->arg_num = 0;
  address_destroy (&addr);
}

/*     type       addr     integer  host type* (0=local,1=network,
       -1=uucp,2=unknown)*/
static void
builtin_type (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;

  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;

  if (address_get_email (addr, 1, buf, sizeof buf, &n) == 0)
    {
      if (strchr (buf, '@'))
	mach->arg_num = 1;
      else if (strchr (buf, '@'))
	mach->arg_num = -1;
      else
	mach->arg_num = 0; /* assume local */
    }
  else
    mach->arg_num = 2;
  address_destroy (&addr);
}

/*     path       addr     string   any leading host route**/
static void
builtin_path (struct mh_machine *mach)
{
  address_t addr;
  size_t n;
  char buf[80];
  int rc;
  
  rc = address_create (&addr, strobj_ptr (&mach->arg_str));
  strobj_free (&mach->arg_str);
  if (rc)
    return;
  if (address_get_route (addr, 1, buf, sizeof buf, &n))
    strobj_create (&mach->arg_str, buf);
  address_destroy (&addr);
}

/*     ingrp      addr     integer  address was inside a group**/
static void
builtin_ingrp (struct mh_machine *mach)
{
  /*FIXME:*/
  builtin_not_implemented ("ingrp");
}

/*     gname      addr     string   name of group**/
static void
builtin_gname (struct mh_machine *mach)
{
  /*FIXME:*/
  builtin_not_implemented ("gname");
}

/*     formataddr expr              append arg to str as a
       (comma separated) address list */
static void
builtin_formataddr (struct mh_machine *mach)
{
  address_t addr;
  size_t num = 0, n, i;
  char buf[80], *p;
  
  if (address_create (&addr, strobj_ptr (&mach->arg_str)))
    return;
  address_get_count (addr, &num);
  for (i = 1; i <= num; i++)
    {
      address_get_email (addr, i, buf, sizeof buf, &n);
      num += n+1;
    }

  num += strobj_len (&mach->reg_str) + 1;
  mach->reg_str.ptr = xrealloc (mach->reg_str.ptr, num);
  mach->reg_str.size = num + 1;
  p = strobj_ptr (&mach->reg_str) + strobj_len (&mach->reg_str) ;
  for (i = 1; i <= num; i++)
    {
      address_get_email (addr, i, buf, sizeof buf, &n);
      memcpy (p, buf, n);
      p += n;
    }
  *p = 0;
}

/*      putaddr    literal           print str address list with
                                  arg as optional label;
                                  get line width from num */
static void
builtin_putaddr (struct mh_machine *mach)
{
  /*FIXME:*/
  builtin_not_implemented ("putaddr");
}

/* Builtin function table */

mh_builtin_t builtin_tab[] = {
  /* Name       Handling function Return type  Arg type */ 
  { "msg",      builtin_msg,      mhtype_num,  mhtype_none },
  { "cur",      builtin_cur,      mhtype_num,  mhtype_none },
  { "size",     builtin_size,     mhtype_num,  mhtype_none },
  { "strlen",   builtin_strlen,   mhtype_num,  mhtype_none },
  { "width",    builtin_width,    mhtype_num,  mhtype_none },
  { "charleft", builtin_charleft, mhtype_num,  mhtype_none },
  { "timenow",  builtin_timenow,  mhtype_num,  mhtype_none },
  { "me",       builtin_me,       mhtype_str,  mhtype_none },
  { "eq",       builtin_eq,       mhtype_num,  mhtype_num  },
  { "ne",       builtin_ne,       mhtype_num,  mhtype_num  },
  { "gt",       builtin_gt,       mhtype_num,  mhtype_num  },
  { "match",    builtin_match,    mhtype_num,  mhtype_str },
  { "amatch",   builtin_amatch,   mhtype_num,  mhtype_str },
  { "plus",     builtin_plus,     mhtype_num,  mhtype_num },
  { "minus",    builtin_minus,    mhtype_num,  mhtype_num },
  { "divide",   builtin_divide,   mhtype_num,  mhtype_num },
  { "modulo",   builtin_modulo,   mhtype_num,  mhtype_num },
  { "num",      builtin_num,      mhtype_num,  mhtype_num },
  { "lit",      builtin_lit,      mhtype_str,  mhtype_str },
  { "getenv",   builtin_getenv,   mhtype_str,  mhtype_str },
  { "profile",  builtin_profile,  mhtype_str,  mhtype_str },
  { "nonzero",  builtin_nonzero,  mhtype_num,  mhtype_num, 1 },
  { "zero",     builtin_zero,     mhtype_num,  mhtype_num, 1 },
  { "null",     builtin_null,     mhtype_num,  mhtype_str, 1 },
  { "nonnull",  builtin_nonnull,  mhtype_num,  mhtype_str, 1 },
  { "comp",     builtin_comp,     mhtype_num,  mhtype_str },
  { "compval",  builtin_compval,  mhtype_num,  mhtype_str },
  { "trim",     builtin_trim,     mhtype_str,  mhtype_str, 1 },
  { "putstr",   builtin_putstr,   mhtype_str,  mhtype_str, 1 },
  { "putstrf",  builtin_putstrf,  mhtype_str,  mhtype_str, 1 },
  { "putnum",   builtin_putnum,   mhtype_str,  mhtype_num, 1 },
  { "putnumf",  builtin_putnumf,  mhtype_str,  mhtype_num, 1 },
  { "sec",      builtin_sec,      mhtype_num,  mhtype_str },
  { "min",      builtin_min,      mhtype_num,  mhtype_str },
  { "hour",     builtin_hour,     mhtype_num,  mhtype_str },
  { "wday",     builtin_wday,     mhtype_num,  mhtype_str },
  { "day",      builtin_day,      mhtype_str,  mhtype_str },
  { "weekday",  builtin_weekday,  mhtype_str,  mhtype_str },
  { "sday",     builtin_sday,     mhtype_num,  mhtype_str },
  { "mday",     builtin_mday,     mhtype_num,  mhtype_str },
  { "yday",     builtin_yday,     mhtype_num,  mhtype_str },
  { "mon",      builtin_mon,      mhtype_num,  mhtype_str },
  { "month",    builtin_month,    mhtype_str,  mhtype_str },
  { "lmonth",   builtin_lmonth,   mhtype_str,  mhtype_str },
  { "year",     builtin_year,     mhtype_num,  mhtype_str },
  { "zone",     builtin_zone,     mhtype_num,  mhtype_str },
  { "tzone",    builtin_tzone,    mhtype_str,  mhtype_str },
  { "szone",    builtin_szone,    mhtype_num,  mhtype_str },
  { "date2local", builtin_date2local, mhtype_str, mhtype_str },
  { "date2gmt", builtin_date2gmt, mhtype_str,  mhtype_str },
  { "dst",      builtin_dst,      mhtype_num,  mhtype_str },
  { "clock",    builtin_clock,    mhtype_num,  mhtype_str },
  { "rclock",   builtin_rclock,   mhtype_num,  mhtype_str },
  { "tws",      builtin_tws,      mhtype_str,  mhtype_str },
  { "pretty",   builtin_pretty,   mhtype_str,  mhtype_str },
  { "nodate",   builtin_nodate,   mhtype_num,  mhtype_str },
  { "proper",   builtin_proper,   mhtype_str,  mhtype_str },
  { "friendly", builtin_friendly, mhtype_str,  mhtype_str },
  { "addr",     builtin_addr,     mhtype_str,  mhtype_str },
  { "pers",     builtin_pers,     mhtype_str,  mhtype_str },
  { "note",     builtin_note,     mhtype_str,  mhtype_str },
  { "mbox",     builtin_mbox,     mhtype_str,  mhtype_str },
  { "mymbox",   builtin_mymbox,   mhtype_num,  mhtype_str },
  { "host",     builtin_host,     mhtype_str,  mhtype_str },
  { "nohost",   builtin_nohost,   mhtype_num,  mhtype_str },
  { "type",     builtin_type,     mhtype_num,  mhtype_str },
  { "path",     builtin_path,     mhtype_str,  mhtype_str },
  { "ingrp",    builtin_ingrp,    mhtype_num,  mhtype_str },
  { "gname",    builtin_gname,    mhtype_str,  mhtype_str},
  { "formataddr", builtin_formataddr, mhtype_none, mhtype_str, 1 },
  { "putaddr",  builtin_putaddr,  mhtype_none, mhtype_str },
  { 0 }
};

mh_builtin_t *
mh_lookup_builtin (char *name, int *rest)
{
  mh_builtin_t *bp;
  int namelen = strlen (name);
  
  for (bp = builtin_tab; bp->name; bp++)
    {
      int len = strlen (bp->name);
      if (len >= namelen
	  && memcmp (name, bp->name, len) == 0)
	{
	  *rest = namelen - len;
	  return bp;
	}
    }
  return NULL;
}
