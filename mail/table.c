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

const struct mail_command_entry mail_command_table[] = {
  { "a",	"alias",       0, mail_alias,
			       "a[lias] [alias [address...]]" },
  { "alt",	"alternates",  0, mail_alt,	"alt[ernates] name..." },
  { "C",	"Copy",	       0, mail_copy,	"C[opy] [msglist]" },
  { "cd",	"cd",	       0, mail_cd,	"cd [directory]" },
  { "ch",	"chdir",       0, mail_cd,	"ch[dir] directory" },
  { "c",	"copy",	       0, mail_copy,	"c[opy] [[msglist] file]" },
  { "d",	"delete",      0, mail_delete,	"d[elete] [msglist]" },
  { "di",	"discard",     0, mail_discard,
			       "di[scard] [header-field...]" },
  { "dp",	"dp",	       0, mail_dp,	"dp [msglist]" },
  { "dt",	"dt",	       0, mail_dp,	"dt [msglist]" },
  { "ec",	"echo",	       0, mail_echo,	"ec[ho] string ..." },
  { "e",	"edit",	       0, mail_edit,	"e[dit] [msglist]" },
  { "el",	"else",	       EF_FLOW, mail_else,	"el[se]" },
  { "en",	"endif",       EF_FLOW, mail_endif,	"en[dif]" },
  { "ex",	"exit",	       0, mail_exit,	"ex[it]" },
  { "F",	"Followup",    EF_SEND, mail_followup,"F[ollowup] [msglist]" },
  { "fi",	"file",	       0, mail_file,	"fi[le] [file]" },
  { "fold",	"folder",      0, mail_file,	"fold[er] [file]" },
  { "folders",	"folders",     0, mail_folders,"folders" },
  { "fo",	"followup",    EF_SEND, mail_followup,"fo[llowup] [msglist]" },
  { "f",	"from",	       0, mail_from,	"f[rom] [msglist]" },
  { "g",	"group",       0, mail_alias,
			       "g[roup] [alias [address...]]" },
  { "h",	"headers",     0, mail_headers,"h[eaders] [msglist]" },
  { "hel",	"help",	       0, mail_help,	"hel[p] [command...]" },
  { "ho",	"hold",	       0, mail_hold,	"ho[ld] [msglist]" },
  { "i",	"if",	       EF_FLOW, mail_if,	"i[f] s|r|t" },
  { "ig",	"ignore",      0, mail_discard,"ig[nore] [header-field...]" },
  { "inc",      "incorporate", 0, mail_inc,     "inc[orporate]" },
  { "l",	"list",	       0, mail_list,	"l[ist]" },
  { "m",	"mail",	       EF_SEND, mail_send,    "m[ail] [address...]" },
  { "mb",	"mbox",	       0, mail_mbox,	"mb[ox] [msglist]" },
  { "n",	"next",	       0, mail_next,	"n[ext] [message]" },
  { "P",	"Print",       0, mail_print,	"P[rint] [msglist]" },
  { "pi",	"pipe",	       0, mail_pipe,	"pi[pe] [[msglist] command]" },
  { "pre",	"preserve",    0, mail_hold,	"pre[serve] [msglist]" },
  { "prev",	"previous",    0, mail_previous,"prev[ious] [message]" },
  { "p",	"print",       0, mail_print,	"p[rint] [msglist]" },
  { "q",	"quit",	       0, mail_quit,	"q[uit]" },
  { "R",	"Reply",       EF_SEND, mail_reply,   "R[eply] [msglist]" },
  { "R",	"Respond",     EF_SEND, mail_reply,    "R[espond] [msglist]" },
  { "r",	"reply",       EF_SEND, mail_reply,    "r[eply] [msglist]" },
  { "r",	"respond",     EF_SEND, mail_reply,    "r[espond] [msglist]" },
  { "ret",	"retain",      0, mail_retain,	"ret[ain] [header-field]" },
  { "S",	"Save",	       0, mail_save,	"S[ave] [msglist]" },
  { "s",	"save",	       0, mail_save,	"s[ave] [[msglist] file]" },
  { "se",	"set",	       0, mail_set,
		"se[t] [name[=[string]]...] [name=number...] [noname...]" },
  { "sh",	"shell",       0, mail_shell,	"sh[ell] [command]" },
  { "si",	"size",	       0, mail_size,	"si[ze] [msglist]" },
  { "so",	"source",      0, mail_source,	"so[urce] file" },
  { "su",	"summary",     0, mail_summary,	"su[mmary]" },
  { "T",	"Type",	       0, mail_print,	"T[ype] [msglist]" },
  { "to",	"top",	       0, mail_top,	"to[p] [msglist]" },
  { "tou",	"touch",       0, mail_touch,	"tou[ch] [msglist]" },
  { "t",	"type",	       0, mail_print,	"t[ype] [msglist]" },
  { "una",	"unalias",     0, mail_unalias,"una[lias] [alias]..." },
  { "u",	"undelete",    0, mail_undelete,"u[ndelete] [msglist]" },
  { "uns",	"unset",       0, mail_unset,	"uns[et] name..." },
  { "ve",	"version",     0, mail_version,	"ve[rsion]" },
  { "v",	"visual",      0, mail_visual,	"v[isual] [msglist]" },
  { "wa",       "warranty",    0, mail_warranty,"wa[rranty]" },
  { "W",	"Write",       0, mail_write,	"W[rite] [msglist]" },
  { "w",	"write",       0, mail_write,	"w[rite] [[msglist] file]" },
  { "x",	"xit",	       0, mail_exit,	"x[it]" },
  { "z",	"",	       0, mail_z,	"z[+|-|. [count]]" },
  { "?",	"?",	       0, mail_help,	"? [command...]" },
  { "!",	"",	       0, mail_shell,	"![command]" },
  { "=",	"=",	       0, mail_eq,	"=" },
  { "#",	"#",	       0, NULL,	"# comment" },
  { "*",	"*",	       0, mail_list,	"*" },
  { "+",	"+",	       0, mail_next,	"+ [message]" },
  { "|",	"|",	       0, mail_pipe,	"| [[msglist] command]" },
  { "-",	"-",	       0, mail_previous,"- [message]" },
  { 0, 0, 0, 0, 0}
};

const struct mail_command_entry mail_escape_table[] = {
  {"!", "!", 0, var_shell,   "![shell-command]"},
  {":", ":", 0, var_command, ":[mail-command]"},
  {"-", "-", 0, var_command, "-[mail-command]"},
  {"?", "?", 0, var_help,    "?"},
  {"A", "A", 0, var_sign,    "A"},
  {"a", "a", 0, var_sign,    "a"},
  {"b", "b", 0, var_bcc,     "b[bcc-list]"},
  {"c", "c", 0, var_cc,      "c[cc-list]"},
  {"d", "d", 0, var_deadletter, "d"},
  {"e", "e", 0, var_editor,  "e"},
  {"f", "f", 0, var_print,   "f[mesg-list]"},
  {"F", "F", 0, var_print,   "F[mesg-list]"},
  {"h", "h", 0, var_headers, "h"},
  {"i", "i", 0, var_insert,  "i[var-name]"},
  {"m", "m", 0, var_quote,   "m[mesg-list]"},
  {"M", "M", 0, var_quote,   "M[mesg-list]"},
  {"p", "p", 0, var_type_input,"p"},
  {"r", "<", 0, var_read,    "r[filename]"},
  {"s", "s", 0, var_subj,    "s[string]"},
  {"t", "t", 0, var_to,      "t[name-list]"},
  {"v", "v", 0, var_visual,  "v"},
  {"w", "w", 0, var_write,   "w[filename]"},
  {"x", "x", 0, var_exit,    "x"},
  {"|", "|", 0, var_pipe,    "|[shell-command]"},
  {0, 0, 0, 0}
};
