/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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

typedef int (*mutool_action_t) (int argc, char **argv);

struct mutool_command
{
  const char *name;	/* User printable name of the function. */
  int argmin;           /* Min. acceptable number of arguments (-1 means
			   pass all arguments as a single string */ 
  int argmax;           /* Max. allowed number of arguments (-1 means not
			   limited */
  mutool_action_t func;	/* Function to call to do the job. */
  const char *argdoc;   /* Documentation for the arguments */
  const char *docstring;/* Documentation for this function.  */
};


int mutool_pop (int argc, char **argv);
int mutool_filter (int argc, char **argv);
int mutool_flt2047 (int argc, char **argv);
int mutool_info (int argc, char **argv);
int mutool_query (int argc, char **argv);
int mutool_acl (int argc, char **argv);

extern char *mutool_shell_prompt;
extern mu_vartab_t mutool_prompt_vartab;
extern int mutool_shell_interactive;
extern mu_stream_t mustrin, mustrout;
int mutool_shell (const char *name, struct mutool_command *cmd);
mu_stream_t mutool_open_pager (void);
