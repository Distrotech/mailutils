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

#include "mail.h"

const struct mail_command_entry mail_command_table[] = {
  { "a",	"alias",	mail_alias,
				"a[lias] [alias [address...]]" },
  { "alt",	"alternates",	mail_alt,	"alt[ernates] name..." },
  { "C",	"Copy",		mail_copy,	"C[opy] [msglist]" },
  { "cd",	"cd",		mail_cd,	"cd [directory]" },
  { "ch",	"chdir",	mail_cd,	"ch[dir] directory" },
  { "c",	"copy",		mail_copy,	"c[opy] [[msglist] file]" },
  { "d",	"delete",	mail_delete,	"d[elete] [msglist]" },
  { "di",	"discard",	mail_discard,
				"di[scard] [header-field...]" },
  { "dp",	"dp",		mail_dp,	"dp [msglist]" },
  { "dt",	"dt",		mail_dp,	"dt [msglist]" },
  { "ec",	"echo",		mail_echo,	"ec[ho] string ..." },
  { "e",	"edit",		mail_edit,	"e[dit] [msglist]" },
  { "el",	"else",		mail_if,	"el[se]" }, /* FIXME */
  { "en",	"endif",	mail_if,	"en[dif]" }, /* FIXME */
  { "ex",	"exit",		mail_exit,	"ex[it]" },
  { "F",	"Followup",	mail_followup,	"F[ollowup] [msglist]" },
  { "fi",	"file",		mail_file,	"fi[le] [file]" },
  { "fold",	"folder",	mail_file,	"fold[er] [file]" },
  { "folders",	"folders",	mail_folders,	"folders" },
  { "fo",	"followup",	mail_followup,	"fo[llowup] [msglist]" },
  { "f",	"from",		mail_from,	"f[rom] [msglist]" },
  { "g",	"group",	mail_alias,
				"g[roup] [alias [address...]]" },
  { "h",	"headers",	mail_headers,	"h[eaders] [msglist]" },
  { "hel",	"help",		mail_help,	"hel[p] [command...]" },
  { "ho",	"hold",		mail_hold,	"ho[ld] [msglist]" },
  { "i",	"if",		mail_if,	"i[f] s|r" }, /* FIXME */
  { "ig",	"ignore",	mail_discard,	"ig[nore] [header-field...]" },
  { "l",	"list",		mail_list,	"l[ist]" },
  { "m",	"mail",		mail_send,	"m[ail] [address...]" },
  { "mb",	"mbox",		mail_mbox,	"mb[ox] [msglist]" },
  { "n",	"next",		mail_next,	"n[ext] [message]" },
  { "P",	"Print",	mail_print,	"P[rint] [msglist]" },
  { "pi",	"pipe",		mail_pipe,	"pi[pe] [[msglist] command]" },
  { "pre",	"preserve",	mail_hold,	"pre[serve] [msglist]" },
  { "prev",	"previous",	mail_previous,  "prev[ious] [message]" },
  { "p",	"print",	mail_print,	"p[rint] [msglist]" },
  { "q",	"quit",		mail_quit,	"q[uit]" },
  { "R",	"Reply",	mail_reply,	"R[eply] [msglist]" },
  { "R",	"Respond",	mail_reply,	"R[espond] [msglist]" },
  { "r",	"reply",	mail_reply,	"r[eply] [msglist]" },
  { "r",	"respond",	mail_reply,	"r[espond] [msglist]" },
  { "ret",	"retain",	mail_retain,	"ret[ain] [header-field]" },
  { "S",	"Save",		mail_save,	"S[ave] [msglist]" },
  { "s",	"save",		mail_save,	"s[ave] [[msglist] file]" },
  { "se",	"set",		mail_set,
		"se[t] [name[=[string]]...] [name=number...] [noname...]" },
  { "sh",	"shell",	mail_shell,	"sh[ell] [command]" },
  { "si",	"size",		mail_size,	"si[ze] [msglist]" },
  { "so",	"source",	mail_source,	"so[urce] file" },
  { "T",	"Type",		mail_print,	"T[ype] [msglist]" },
  { "to",	"top",		mail_top,	"to[p] [msglist]" },
  { "tou",	"touch",	mail_touch,	"tou[ch] [msglist]" },
  { "t",	"type",		mail_print,	"t[ype] [msglist]" },
  { "una",	"unalias",	mail_unalias,	"una[lias] [alias]..." },
  { "u",	"undelete",	mail_undelete,	"u[ndelete] [msglist]" },
  { "uns",	"unset",	mail_unset,	"uns[et] name..." },
  { "v",	"visual",	mail_visual,	"v[isual] [msglist]" },
  { "W",	"Write",	mail_write,	"W[rite] [msglist]" },
  { "w",	"write",	mail_write,	"w[rite] [[msglist] file]" },
  { "x",	"xit",		mail_exit,	"x[it]" },
  { "z",	"",		mail_z,		"z[+|-]" },
  { "?",	"?",		mail_help,	"? [command...]" },
  { "!",	"",		mail_shell,	"![command]" },
  { "=",	"=",		mail_eq,	"=" },
  { "#",	"#",		NULL,		"# comment" },
  { "*",	"*",		mail_list,	"*" },
  { "+",	"+",		mail_next,	"+ [message]" },
  { "|",	"|",		mail_pipe,	"| [[msglist] command]" },
  { "-",	"-",		mail_previous,	"- [message]" },
  { 0, 0, 0, 0}
};

const struct mail_command_entry mail_escape_table[] = {
  {0, 0, 0, 0}
};
