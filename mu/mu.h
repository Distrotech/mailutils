/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <mailutils/types.h>

typedef int (*mutool_action_t) (int argc, char **argv);

#define CMD_COALESCE_EXTRA_ARGS 0x01

struct mutool_command
{
  const char *name;	/* User printable name of the function. */
  int argmin;           /* Min. acceptable number of arguments (>= 1) */ 
  int argmax;           /* Max. allowed number of arguments (-1 means not
			   limited */
  int flags;
  mutool_action_t func;	/* Function to call to do the job. */
  const char *argdoc;   /* Documentation for the arguments */
  const char *docstring;/* Documentation for this function. */
};

extern char *mutool_shell_prompt;
extern char **mutool_prompt_env;
extern int mutool_shell_interactive;
int mutool_shell (const char *name, struct mutool_command *cmd);
mu_stream_t mutool_open_pager (void);

int mu_help (void);
mutool_action_t dispatch_find_action (const char *name);
char *dispatch_docstring (const char *text);

int port_from_sa (struct mu_sockaddr *sa);


#define VERBOSE_MASK(n) (1<<((n)+1))
#define SET_VERBOSE_MASK(n) (shell_verbose_flags |= VERBOSE_MASK (n))
#define CLR_VERBOSE_MASK(n) (shell_verbose_flags &= ~VERBOSE_MASK (n))
#define QRY_VERBOSE_MASK(n) (shell_verbose_flags & VERBOSE_MASK (n))
#define HAS_VERBOSE_MASK(n) (shell_verbose_flags & ~1)
#define SET_VERBOSE() (shell_verbose_flags |= 1)
#define CLR_VERBOSE() (shell_verbose_flags &= ~1)
#define QRY_VERBOSE() (shell_verbose_flags & 1)

extern int shell_verbose_flags;
int shell_verbose (int argc, char **argv,
		   void (*set_verbose) (void), void (*set_mask) (void));


int get_bool (const char *str, int *pb);
int get_port (const char *port_str, int *pn);

int mu_getans (const char *variants, const char *fmt, ...);

