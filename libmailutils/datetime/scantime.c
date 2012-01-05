/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2007, 2009-2012 Free Software Foundation,
   Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/diag.h>
#include <mailutils/datetime.h>
#include <mailutils/util.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>

static int
_mu_short_weekday_string (const char *str)
{
  int i;
  
  for (i = 0; i < 7; i++)
    {
      if (mu_c_strncasecmp (str, _mu_datetime_short_wday[i], 3) == 0)
	return i;
    }
  return -1;
}

static int
_mu_full_weekday_string (const char *str, char **endp)
{
  int i;
  
  for (i = 0; i < 7; i++)
    {
      if (mu_c_strcasecmp (str, _mu_datetime_full_wday[i]) == 0)
	{
	  if (endp)
	    *endp = (char*) (str + strlen (_mu_datetime_full_wday[i]));
	  return i;
	}
    }
  return -1;
}

static int
_mu_short_month_string (const char *str)
{  
  int i;
  
  for (i = 0; i < 12; i++)
    {
      if (mu_c_strncasecmp (str, _mu_datetime_short_month[i], 3) == 0)
	return i;
    }
  return -1;
}

static int
_mu_full_month_string (const char *str, char **endp)
{  
  int i;
  
  for (i = 0; i < 12; i++)
    {
      if (mu_c_strcasecmp (str, _mu_datetime_full_month[i]) == 0)
	{
	  if (endp)
	    *endp = (char*) (str + strlen (_mu_datetime_full_month[i]));
	  return i;
	}
    }
  return -1;
}

int
get_num (const char *str, char **endp, int ndig, int minval, int maxval,
	 int *pn)
{
  int x = 0;
  int i;
  
  errno = 0;
  for (i = 0; i < ndig && *str && mu_isdigit (*str); str++, i++)
    x = x * 10 + *str - '0';

  *endp = (char*) str;
  if (i == 0)
    return -1;
  else if (pn)
    *pn = i;
  else if (i != ndig)
    return -1;
  if (x < minval || x > maxval)
    return -1;
  return x;
}

#define DT_YEAR  0x01
#define DT_MONTH 0x02
#define DT_MDAY  0x04
#define DT_WDAY  0x08
#define DT_HOUR  0x10
#define DT_MIN   0x20
#define DT_SEC   0x40

#define ST_NON   -1
#define ST_OPT   0
#define ST_ALT   1

struct save_input
{
  int state;
  const char *input;
};

static int
push_input (mu_list_t *plist, int state, const char *input)
{
  mu_list_t list = *plist;
  struct save_input *inp = malloc (sizeof (*inp));
  if (!inp)
    return ENOMEM;
  if (!list)
    {
      int rc = mu_list_create (&list);
      if (rc)
	{
	  free (inp);
	  return rc;
	}
      mu_list_set_destroy_item (list, mu_list_free_item);
      *plist = list;
    }
  inp->state = state;
  inp->input = input;
  return mu_list_push (list, (void*)inp);
}

static int
peek_state (mu_list_t list, int *state, const char **input)
{
  int rc;
  struct save_input *inp;

  rc = mu_list_tail (list, (void**)&inp);
  if (rc)
    return rc;
  *state = inp->state;
  if (input)
    *input = inp->input;
  return 0;
}      

static int
pop_input (mu_list_t list, int *state, const char **input)
{
  int rc;
  struct save_input *inp;

  rc = mu_list_pop (list, (void**)&inp);
  if (rc)
    return rc;
  *state = inp->state;
  if (input)
    *input = inp->input;
  return 0;
}

static int
bracket_to_state (int c)
{
  switch (c)
    {
    case '[':
    case ']':
      return ST_OPT;
    case '(':
    case ')':
      return ST_ALT;
    }
  return ST_NON;
}

static int
state_to_closing_bracket (int st)
{
  switch (st)
    {
    case ST_OPT:
      return ']';
    case ST_ALT:
      return ')';
    }
  return '?';
}

static int
scan_recovery (const char *fmt, mu_list_t *plist, int skip_alt,
	       const char **endp,
	       const char **input)
{
  int c, rc = 0;
  int nesting_level = 1;
  int st;
  const char *p;
  
  while (*fmt)
    {
      c = *fmt++;
      
      if (c == '%')
	{
	  c = *fmt++;
	  if (!c)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			("%s:%d: error in format: %% at the end of input",
			 __FILE__, __LINE__));
	      rc = MU_ERR_FORMAT;
	      break;
	    }
	      
	  switch (c)
	    {
	    case '[':
	    case '(':
	      nesting_level++;
	      rc = push_input (plist, bracket_to_state (c), NULL);
	      break;
	      
	    case ')':
	    case ']':
	      rc = pop_input (*plist, &st, &p);
	      if (rc || st != bracket_to_state (c))
		{
		  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			    ("%s:%d: error in format: %%%c out of context",
			     __FILE__, __LINE__, c));
		  rc = MU_ERR_FORMAT;
		  break;
		}
	      if (--nesting_level == 0)
		{
		  *endp = fmt;
		  if (skip_alt)
		    return 0;
		  *input = p;
		  if (st == ST_ALT)
		    {
		      if (*fmt == '%' && (fmt[1] == '|' || fmt[1] == ']'))
			return 0;
		      return MU_ERR_PARSE; /* No match found */
		    }
		  return 0;
		}
	      break;

	    case '|':
	      if (skip_alt)
		continue;
	      if (nesting_level == 1)
		{
		  *endp = fmt;
		  return peek_state (*plist, &st, input);
		}
	      break;

	    case '\\':
	      if (*++fmt == 0)
		{
		  peek_state (*plist, &st, NULL);
		  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			    ("%s:%d: error in format: missing closing %%%c",
			     __FILE__, __LINE__,
			     state_to_closing_bracket (st)));
		  return MU_ERR_FORMAT;		  
		}
	    }
	}
    }
  
  peek_state (*plist, &st, NULL);
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
	    ("%s:%d: error in format: missing closing %%%c",
	     __FILE__, __LINE__,
	     state_to_closing_bracket (st)));
  return MU_ERR_FORMAT;		  
}

int
mu_scan_datetime (const char *input, const char *fmt,
		  struct tm *tm, struct mu_timezone *tz, char **endp)
{
  int rc = 0;
  char *p;
  int n;
  int c;
  int st;
  int recovery = 0;
  int eof_ok = 0;
  int datetime_parts = 0;
  mu_list_t save_input_list = NULL;
  
  memset (tm, 0, sizeof *tm);
#ifdef HAVE_STRUCT_TM_TM_ISDST
  tm->tm_isdst = -1;	/* unknown. */
#endif
  /* provide default timezone, in case it is not supplied in input */
  if (tz)
    mu_datetime_tz_local (tz);

  /* Skip leading whitespace */
  input = mu_str_skip_class (input, MU_CTYPE_BLANK);
  for (; *fmt && rc == 0; fmt++)
    {
      if (mu_isspace (*fmt))
	{
	  fmt = mu_str_skip_class (fmt, MU_CTYPE_BLANK);
	  input = mu_str_skip_class (input, MU_CTYPE_BLANK);
	  if (!*fmt)
	    break;
	}
      eof_ok = 0;
      
      if (*fmt == '%')
	{
	  c = *++fmt;
	  if (!c)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			("%s:%d: error in format: %% at the end of input",
			 __FILE__, __LINE__));
	      rc = MU_ERR_FORMAT;
	      break;
	    }
	  
	  switch (c)
	    {
	    case 'a':
	      /* The abbreviated weekday name. */
	      n = _mu_short_weekday_string (input);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_wday = n;
		  datetime_parts |= DT_WDAY;
		  input += 3;
		}
	      break;
		  
	    case 'A':
	      /* The full weekday name. */
	      n = _mu_full_weekday_string (input, &p);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_wday = n;
		  datetime_parts |= DT_WDAY;
		  input = p;
		}
	      break;
	      
	    case 'b':
	      /* The abbreviated month name. */
	      n = _mu_short_month_string (input);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_mon = n;
		  datetime_parts |= DT_MONTH;
		  input += 3;
		}
	      break;

	    case 'B':
	      /* The full month name. */
	      n = _mu_full_month_string (input, &p);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_mon = n;
		  datetime_parts |= DT_MONTH;
		  input = p;
		}
	      break;
	      
	    case 'd':
	      /* The day of the month as a decimal number (range 01 to 31). */
	      n = get_num (input, &p, 2, 1, 31, NULL);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_mday = n;
		  datetime_parts |= DT_MDAY;
		  input = p;
		}
	      break;
	      
	    case 'e':
	      /* Like %d, the day of the month as a decimal number, but a
		 leading zero is replaced by a space. */
	      {
		int ndig;
		
		n = get_num (input, &p, 2, 1, 31, &ndig);
		if (n == -1)
		  rc = MU_ERR_PARSE;
		else
		  {
		    tm->tm_mday = n;
		    datetime_parts |= DT_MDAY;
		    input = p;
		  }
	      }
	      break;
	      
	    case 'H':
	      /* The hour as a decimal number using a 24-hour clock (range
		 00 to 23). */
	      n = get_num (input, &p, 2, 0, 23, NULL);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_hour = n;
		  datetime_parts |= DT_HOUR;
		  input = p;
		}
	      break;
	      
	    case 'm':
	      /* The month as a decimal number (range 01 to 12). */
	      n = get_num (input, &p, 2, 1, 12, NULL);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_mon = n - 1;
		  datetime_parts |= DT_MONTH;
		  input = p;
		}
	      break;
	      
	    case 'M':
	      /* The minute as a decimal number (range 00 to 59). */
	      n = get_num (input, &p, 2, 0, 59, NULL);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_min = n;
		  datetime_parts |= DT_MIN;
		  input = p;
		}
	      break;
	      
	    case 'S':
	      /* The second as a decimal number (range 00 to 60) */
	      n = get_num (input, &p, 2, 0, 60, NULL);
	      if (n == -1)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_sec = n;
		  datetime_parts |= DT_SEC;
		  input = p;
		}
	      break;
	      
	    case 'Y':
	      /* The year as a decimal number including the century. */
	      errno = 0;
	      n = strtoul (input, &p, 10);
	      if (errno || p == input)
		rc = MU_ERR_PARSE;
	      else
		{
		  tm->tm_year = n - 1900;
		  datetime_parts |= DT_YEAR;
		  input = p;
		}
	      break;
		  
	    case 'z':
	      /* The time-zone as hour offset from GMT */
	      {
		int sign = 1;
		int hr;
		
		if (*input == '+')
		  input++;
		else if (*input == '-')
		  {
		    input++;
		    sign = -1;
		  }
		n = get_num (input, &p, 2, 0, 11, NULL);
		if (n == -1)
		  rc = MU_ERR_PARSE;
		else
		  {
		    input = p;
		    hr = n;
		    n = get_num (input, &p, 2, 0, 59, NULL);
		    if (n == -1)
		      rc = MU_ERR_PARSE;
		    else
		      {
			input = p;
			if (tz)
			  tz->utc_offset = sign * (hr * 60 + n) * 60;
		      }
		  }
	      }
	      break;
		      
	    case '%':
	      if (*input == '%')
		input++;
	      else
		rc = MU_ERR_PARSE;
	      break;

	      rc = push_input (&save_input_list, ST_ALT, (void*)input);
	      break;
	      
	    case '(':
	    case '[':
	      rc = push_input (&save_input_list, bracket_to_state (c),
			       (void*)input);
	      break;

	    case ')':
	    case ']':
	      if (pop_input (save_input_list, &st, NULL))
		{
		  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			    ("%s:%d: error in format: unbalanced %%%c near %s",
			     __FILE__, __LINE__, c, fmt));
		  rc = MU_ERR_FORMAT;
		}
	      else if (st != bracket_to_state (c))
		{
		  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			    ("%s:%d: error in format: %%%c out of context",
			     __FILE__, __LINE__, c));
		  rc = MU_ERR_FORMAT;
		}
	      break;

	    case '|':
	      rc = scan_recovery (fmt, &save_input_list, 1, &fmt, NULL);
	      if (rc == 0)
		fmt--;
	      break;
	      
	    case '$':
	      eof_ok = 1;
	      break;

	    case '\\':
	      c = *++fmt;
	      if (!c)
		{
		  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			    ("%s:%d: error in format: %% at the end of input",
			     __FILE__, __LINE__));
		  rc = MU_ERR_FORMAT;
		}
	      else if (c == *input)
		input++;
	      else
		rc = MU_ERR_PARSE;
	      break;

	    case '?':
	      input++;
	      break;
	      
	    default:
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			("%s:%d: error in format: unrecognized conversion type"
			 " near %s",
			 __FILE__, __LINE__, fmt));
	      rc = MU_ERR_FORMAT;
	      break;
	    }

	  if (eof_ok && rc == 0 && *input == 0)
	    break;
	}
      else if (!recovery && *input != *fmt)
	rc = MU_ERR_PARSE;
      else
	input++;

      if (rc == MU_ERR_PARSE && !mu_list_is_empty (save_input_list))
	{
	  rc = scan_recovery (fmt, &save_input_list, 0, &fmt, &input);
	  if (rc == 0)
	    --fmt;
	}
    }

  if (!mu_list_is_empty (save_input_list))
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("%s:%d: error in format: closing bracket missing",
		 __FILE__, __LINE__));
      rc = MU_ERR_FORMAT;
    }
  mu_list_destroy (&save_input_list);
  
  if (rc == 0 && recovery)
    rc = MU_ERR_PARSE;
  
  if (!eof_ok && rc == 0 && *input == 0 && *fmt)
    rc = MU_ERR_PARSE;

  if ((datetime_parts & (DT_YEAR|DT_MONTH|DT_MDAY)) ==
      (DT_YEAR|DT_MONTH|DT_MDAY))
    {
      if (!(datetime_parts & DT_WDAY))
	tm->tm_wday = mu_datetime_dayofweek (tm->tm_year + 1900,
					     tm->tm_mon + 1, tm->tm_mday);
      tm->tm_yday = mu_datetime_dayofyear (tm->tm_year + 1900,
					   tm->tm_mon + 1, tm->tm_mday) - 1;
    }
  
  if (endp)
    *endp = (char*) input;
  
  return rc;
}
