/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#ifndef _TABLE_H
#define _TABLE_H 1

#include "mail.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct mail_command_entry {
  char *shortname;
  char *longname;
  int (*func) __P((int, char**));
  char *synopsis;
};

static struct mail_command_entry mail_command_table[] = {
  { "a",	"alias",	mail_alias,
				"a[lias] [alias [address...]]" },
  { "g",	"group",	mail_alias,
				"g[roup] [alias [address...]]" },
  { "alt",	"alternates",	mail_alt,	"alt[ernates] name..." },
  { "cd",	"cd",		mail_cd,	"cd [directory]" },
  { "ch",	"chdir",	mail_cd,	"ch[dir] directory" },
  { "c",	"copy",		mail_copy,
				"c[opy] [file]\nc[opy] [msglist] file" },
  { "C",	"Copy",		mail_copy,	"C[opy] [msglist]" },
  { "d",	"delete",	mail_delete,	"d[elete] [msglist]" },
  { "di",	"discard",	mail_discard,
				"di[scard] [header-field...]" },
  { "ig",	"ignore",	mail_discard,	"ig[nore] [header-field...]" },
  { "dp",	"dp",		mail_dp,	"dp [msglist]" },
  { "dt",	"dt",		mail_dp,	"dt [msglist]" },
  { "ec",	"echo",		mail_echo,	"ec[ho] string ..." },
  { "e",	"edit",		mail_edit,	"e[dit] [msglist]" },
  { "ex",	"exit",		mail_exit,	"ex[it]" },
  { "x",	"xit",		mail_exit,	"x[it]" },
  { "fi",	"file",		mail_file,	"fi[le] [file]" },
  { "fold",	"folder",	mail_file,	"fold[er] [file]" },
  { "folders",	"folders",	mail_folders,	"folders" },
  { "fo",	"followup",	mail_followup,	"fo[llowup] [message]" },
  { "F",	"Followup",	mail_followup,	"F[ollowup] [msglist]" },
  { "f",	"from",		mail_from,	"f[rom] [msglist]" },
  { "h",	"headers",	mail_headers,	"h[eaders] [message]" },
  { "hel",	"help",		mail_help,	"hel[p] [command]" },
  { "?",	"?",		mail_help,	"? [command]" },
  { "ho",	"hold",		mail_hold,	"ho[ld] [msglist]" },
  { "pre",	"preserve",	mail_hold,	"pre[server] [msglist]" },

  /* Should if be handled in the main loop rather than a function? */
  { "i",	"if",		mail_if,	"i[f] s|r" },
  { "el",	"else",		mail_if,	"el[se]" },
  { "en",	"endif",	mail_if,	"en[dif]" },

  { "l",	"list",		mail_list,	"l[ist]" },
  { "*",	"*",		mail_list,	"*" },
  { "m",	"mail",		mail_send,	"m[ail] address..." },
  { "mb",	"mbox",		mail_mbox,	"mb[ox] [msglist]" },
  { "n",	"next",		mail_next,	"n[ext] [message]" },
  { "+",	"+",		mail_next,	"+ [message]" },
  { "pi",	"pipe",		mail_pipe,	"pi[pe] [[msglist] command]" },
  { "|",	"|",		mail_pipe,	"| [[msglist] command]" },
  { "P",	"Print",	mail_printall,	"P[rint] [msglist]" },
  { "T",	"Type",		mail_printall,	"T[ype] [msglist]" },
  { "p",	"print",	mail_print,	"p[rint] [msglist]" },
  { "t",	"type",		mail_print,	"t[ype] [msglist]" },
  { "q",	"quit",		mail_quit,	"q[uit]" },

  /* Hmm... will this work? */
  /* { "",		"",		mail_quit,	"<EOF>" }, */

  { "R",	"Reply",	mail_relist,	"R[eply] [msglist]" },
  { "R",	"Respond",	mail_relist,	"R[espond] [msglist]" },
  { "r",	"reply",	mail_reply,	"r[eply] [message]" },
  { "r",	"respond",	mail_reply,	"r[espond] [message]" },
  { "ret",	"retain",	mail_retain,	"ret[ain] [header-field]" },
  { "s",	"save",		mail_save,
				"s[ave] [file]\ns[ave] [msglist] file" },
  { "S",	"Save",		mail_save,	"S[ave] [msglist]" },
  { "se",	"set",		mail_set,
		"se[t] [name[=[string]]...] [name=number...] [noname...]" },
  { "sh",	"shell",	mail_shell,	"sh[ell]" },
  { "si",	"size",		mail_size,	"si[ze] [msglist]" },
  { "so",	"source",	mail_source,	"so[urce] file" },
  { "to",	"top",		mail_top,	"to[p] [msglist]" },
  { "tou",	"touch",	mail_touch,	"tou[ch] [msglist]" },
  { "una",	"unalias",	mail_unalias,	"una[lias] [alias]..." },
  { "u",	"undelete",	mail_undelete,	"u[ndelete] [msglist]" },
  { "uns",	"unset",	mail_unset,	"uns[et] name..." },
  { "v",	"visual",	mail_visual,	"v[isual] [msglist]" },
  { "w",	"write",	mail_write,	"w[rite] [msglist] file" },
  { "z",	"",		mail_z,		"z[+|-]" },
  { "!",	"",		mail_bang,	"!command" },
  { "=",	"=",		mail_eq,	"=" },
  { 0, 0, 0, 0,}
};

#ifdef __cplusplus
}
#endif

#endif /* _TABLE_H */

