/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "mail.h"

static int is_terminal;

#define COND_STK_SIZE 64
#define COND_STK_INCR 16
static int *_cond_stack;     /* Stack of conditions */
static int _cond_stack_size; /* Number of elements allocated this far */
static int _cond_level;      /* Number of nested `if' blocks */

static void _cond_push(int val);
static int _cond_pop(void);

int
if_cond()
{
  if (_cond_level == 0)
    return 1;
  return _cond_stack[_cond_level-1];
}

void
_cond_push(int val)
{
  if (!_cond_stack)
    {
      _cond_stack = calloc(COND_STK_SIZE, sizeof(_cond_stack[0]));
      _cond_stack_size = COND_STK_SIZE;
      _cond_level = 0;
    }
  else if (_cond_level >= _cond_stack_size)
    {
      _cond_stack_size += COND_STK_INCR;
      _cond_stack = realloc(_cond_stack,
			    sizeof(_cond_stack[0])*_cond_stack_size);
    }
  
  if (!_cond_stack)
    {
      fprintf(ofile, "not enough memeory");
      exit (EXIT_FAILURE);
    }
  _cond_stack[_cond_level++] = val;
}

int
_cond_pop()
{
  if (_cond_level == 0)
    {
      fprintf(ofile, "internal error: condition stack underflow\n");
      abort();
    }
  return _cond_stack[--_cond_level];
}
	       
/*
 * i[f] s|r|t
 * mail-commands
 * el[se]
 * mail-commands
 * en[dif]
 */

/* This may have to go right into the main loop */

int
mail_if (int argc, char **argv)
{
  struct mail_env_entry *mode;
  int cond;

  if (argc != 2)
    {
      fprintf(ofile, "if requires an argument: s | r | t\n");
      return 1;
    }

  if (argv[1][1] != 0)
    {
      fprintf(ofile, "valid if arguments are: s | r | t\n");
      return 1;
    }

  mode = util_find_env("mode");
  if (!mode)
    {
      exit (EXIT_FAILURE);
    }

  if (if_cond() == 0)
    /* Propagate negative condition */
    cond = 0;
  else
    {
      switch (argv[1][0])
	{
	case 's': /* Send mode */
	  cond = strcmp(mode->value, "send") == 0;
	  break;
	case 'r': /* Read mode */
	  cond = strcmp(mode->value, "send") != 0;
	  break;
	case 't': /* Reading from a terminal */
	  cond = is_terminal;
	  break;
	default:
	  fprintf(ofile, "valid if arguments are: s | r | t\n");
	  return 1;
	}
    }
  _cond_push(cond);
  return 0;
}


int
mail_else (int argc, char **argv)
{
  int cond;
  if (_cond_level == 0)
    {
      fprintf(ofile, "else without matching if\n");
      return 1;
    }
  cond = _cond_pop();
  if (_cond_level == 0)
    cond = !cond;
  _cond_push(cond);
  return 0;
}

int
mail_endif (int argc, char **argv)
{
  if (_cond_level == 0)
    {
      fprintf(ofile, "endif without matching if\n");
      return 1;
    }
  _cond_pop();
  return 1;
}

void
mail_set_is_terminal(int val)
{
  is_terminal = val;
}

int
mail_is_terminal(void)
{
  return is_terminal;
}
