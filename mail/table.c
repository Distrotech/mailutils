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
  { "a",	"alias",	"a[lias] [alias [address...]]",	0, mail_alias, 0 },
  { "alt",	"alternates",	"alt[ernates] name...",		0, mail_alt, 0 },
  { "C",	"Copy",		"C[opy] [msglist]",		0, mail_copy, 0 },
  { "cd",	"cd",		"cd [directory]",		0, mail_cd, 0 },
  { "ch",	"chdir",	"ch[dir] directory",		0, mail_cd, 0 },
  { "c",	"copy",		"c[opy] [[msglist] file]",	0, mail_copy, 0 },
  { "dec",	"decode",	"dec[ode] [msglist]",		0, mail_decode, 0 },
  { "d",	"delete",	"d[elete] [msglist]",		0, mail_delete, 0 },
  { "di",	"discard",	"di[scard] [header-field...]",	0, mail_discard, 0 },
  { "dp",	"dp",		"dp [msglist]",			0, mail_dp, 0 },
  { "dt",	"dt",		"dt [msglist]",			0, mail_dp, 0 },
  { "ec",	"echo",		"ec[ho] string ...",		0, mail_echo, 0 },
  { "e",	"edit",		"e[dit] [msglist]",		0, mail_edit, 0 },
  { "el",	"else",		"el[se]",			EF_FLOW, mail_else, 0 },
  { "en",	"endif",	"en[dif]",			EF_FLOW, mail_endif, 0 },
  { "ex",	"exit",		"ex[it]",			0, mail_exit, 0 },
  { "F",	"Followup",	"F[ollowup] [msglist]",		EF_SEND, mail_followup, 0 },
  { "fi",	"file",		"fi[le] [file]",		0, mail_file, 0 },
  { "fold",	"folder",	"fold[er] [file]",		0, mail_file, 0 },
  { "folders",	"folders",	"folders",			0, mail_folders, 0 },
  { "fo",	"followup",	"fo[llowup] [msglist]",		EF_SEND, mail_followup, 0 },
  { "f",	"from",		"f[rom] [msglist]",		0, mail_from, 0 },
  { "g",	"group",	"g[roup] [alias [address...]]",	0, mail_alias, 0 },
  { "h",	"headers",	"h[eaders] [msglist]",		0, mail_headers, 0 },
  { "hel",	"help",		"hel[p] [command...]",		0, mail_help, 0 },
  { "ho",	"hold",		"ho[ld] [msglist]",		0, mail_hold, 0 },
  { "i",	"if",		"i[f] s|r|t",			EF_FLOW, mail_if, 0 },
  { "ig",	"ignore",	"ig[nore] [header-field...]",	0, mail_discard, 0 },
  { "inc",      "incorporate",	"inc[orporate]",		0, mail_inc, 0 },
  { "l",	"list",		"l[ist]",			0, mail_list, 0 },
  { "m",	"mail",		"m[ail] [address...]",		EF_SEND, mail_send, 0 },
  { "mb",	"mbox",		"mb[ox] [msglist]",		0, mail_mbox, 0 },
  { "n",	"next",		"n[ext] [message]",		0, mail_next, 0 },
  { "P",	"Print",	"P[rint] [msglist]",		0, mail_print, 0 },
  { "pi",	"pipe",		"pi[pe] [[msglist] command]",	0, mail_pipe, 0 },
  { "pre",	"preserve",	"pre[serve] [msglist]",		0, mail_hold, 0 },
  { "prev",	"previous",	"prev[ious] [message]",		0, mail_previous, 0 },
  { "p",	"print",	"p[rint] [msglist]",		0, mail_print, 0 },
  { "q",	"quit",		"q[uit]",			0, mail_quit, 0 },
  { "R",	"Reply",	"R[eply] [msglist]",		EF_SEND, mail_reply, 0 },
  { "R",	"Respond",	"R[espond] [msglist]",		EF_SEND, mail_reply, 0 },
  { "r",	"reply",	"r[eply] [msglist]",		EF_SEND, mail_reply, 0 },
  { "r",	"respond",	"r[espond] [msglist]",		EF_SEND, mail_reply, 0 },
  { "ret",	"retain",	"ret[ain] [header-field]",	0, mail_retain, 0 },
  { "S",	"Save",		"S[ave] [msglist]",		0, mail_save, 0 },
  { "s",	"save",		"s[ave] [[msglist] file]",	0, mail_save, 0 },
  { "se",	"set", "se[t] [name[=[string]]...] [name=number...] [noname...]",
	0, mail_set, 0 },
  { "sh",	"shell",	"sh[ell] [command]",		0, mail_shell, 0 },
  { "si",	"size",		"si[ze] [msglist]",		0, mail_size, 0 },
  { "so",	"source",	"so[urce] file",		0, mail_source, 0 },
  { "su",	"summary",	"su[mmary]",			0, mail_summary, 0 },
  { "T",	"Type",		"T[ype] [msglist]",		0, mail_print, 0 },
  { "ta",       "tag",		"ta[g] [msglist]",		0, mail_tag, 0 },
  { "to",	"top",		"to[p] [msglist]",		0, mail_top, 0 },
  { "tou",	"touch", "tou[ch] [msglist]",			0, mail_touch, 0 },
  { "t",	"type",		"t[ype] [msglist]",		0, mail_print, 0 },
  { "una",	"unalias",	"una[lias] [alias]...",		0, mail_unalias, 0 },
  { "u",	"undelete",	"u[ndelete] [msglist]",		0, mail_undelete, 0 },
  { "uns",	"unset",	"uns[et] name...",		0, mail_unset, 0 },
  { "unt",      "untag",	"unt[ag] [msglist]",		0, mail_tag, 0 },
  { "ve",	"version",	"ve[rsion]",			0, mail_version, 0 },
  { "v",	"visual",	"v[isual] [msglist]",		0, mail_visual, 0 },
  { "wa",       "warranty",	"wa[rranty]",			0, mail_warranty, 0 },
  { "W",	"Write",	"W[rite] [msglist]",		0, mail_write, 0 },
  { "w",	"write",	"w[rite] [[msglist] file]",	0, mail_write, 0 },
  { "x",	"xit",		"x[it]",			0, mail_exit, 0 },
  { "z",	"",		"z[+|-|. [count]]",		0, mail_z, 0 },
  { "?",	"?",		"? [command...]",		0, mail_help, 0 },
  { "!",	"",		"![command]",			0, mail_shell, 0 },
  { "=",	"=",		"=",				0, mail_eq, 0 },
  { "#",	"#",		"# comment",			0, NULL, 0 },
  { "*",	"*",		"*",				0, mail_list, 0 },
  { "+",	"+",		"+ [message]",			0, mail_next, 0 },
  { "|",	"|",		"| [[msglist] command]",	0, mail_pipe, 0 },
  { "-",	"-",		"- [message]",			0, mail_previous, 0 },
  { 0, 0, 0, 0, 0, 0}
};

const struct mail_command_entry mail_escape_table[] = {
  {"!",	"!",	"![shell-command]",	0, 0, var_shell },
  {":",	":",	":[mail-command]",	0, 0, var_command },
  {"-",	"-",	"-[mail-command]",	0, 0, var_command },
  {"?",	"?",	"?",			0, 0, var_help },
  {"A",	"A",	"A",			0, 0, var_sign },
  {"a",	"a",	"a",			0, 0, var_sign },
  {"b",	"b",	"b[bcc-list]",		0, 0, var_bcc },
  {"c",	"c",	"c[cc-list]",		0, 0, var_cc },
  {"d",	"d",	"d",			0, 0, var_deadletter },
  {"e",	"e",	"e",			0, 0, var_editor },
  {"f",	"f",	"f[mesg-list]",		0, 0, var_print },
  {"F",	"F",	"F[mesg-list]",		0, 0, var_print },
  {"h",	"h",	"h",			0, 0, var_headers },
  {"i",	"i",	"i[var-name]",		0, 0, var_insert },
  {"m",	"m",	"m[mesg-list]",		0, 0, var_quote },
  {"M",	"M",	"M[mesg-list]",		0, 0, var_quote },
  {"p",	"p",	"p",			0, 0, var_type_input },
  {"r",	"<",	"r[filename]",		0, 0, var_read },
  {"s",	"s",	"s[string]",		0, 0, var_subj },
  {"t",	"t",	"t[name-list]",		0, 0, var_to },
  {"v",	"v",	"v",			0, 0, var_visual },
  {"w",	"w",	"w[filename]",		0, 0, var_write },
  {"|",	"|",	"|[shell-command]",	0, 0, var_pipe },
  {0, 0, 0, 0, 0, 0}
};
