/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "address0.h"

int GetDigits(char** p, char* e, int min, int max, int* digits);
int GetSpecial(char** p, char* e, char c);
int GetComment(char** p, char* e, char** comment);
int GetAtom(char** p, char* e, char** atom);
int GetQuotedPair(char** p, char* e, char** qpair);
int GetQuotedString(char** p, char* e, char** qstr);
int GetWord(char** p, char* e, char** word);
int GetPhrase(char** p, char* e, char** phrase);

int GetAddressList(address_t* a, char* s);
int GetMailBox(char** p, char* e, address_t* a);
int GetGroup(char** p, char* e, address_t* a);
int GetAddress(char** p, char* e, address_t* a);
int GetRouteAddr(char** p, char* e, address_t* a);
int GetRoute(char** p, char* e, char** route);
int GetAddrSpec(char** p, char* e, address_t* a);
int GetLocalPart(char** p, char* e, char** local_part);
int GetDomain(char** p, char* e, char** domain);
int GetSubDomain(char** p, char* e, char** sub_domain);
int GetDomainRef(char** p, char* e, char** domain_ref);
int GetDomainLiteral(char** p, char* e, char** domain_literal);

// Some convenience functions for dealing with dynamically re-sized
// strings.

int StrAppendN(char** to, char* from, size_t n)
{
	size_t l = 0;

	/* if not to, then silently discard data */
	if(!to) {
		return 1;
	}

	if(*to) {
		char* new;

		l = strlen(*to);

		new = (char*) realloc(*to, l + n + 1);

		if(!new) {
			return 0;
		}
		
		*to = new;
	} else {
		*to = (char*) malloc(n + 1);
	}

	strncpy(&to[0][l], from, n);

	/* strncpy is lame, nul terminate our buffer */

	to[0][l + n] = 0;

	return 1;
}

int StrAppend(char** to, char* from)
{
	return StrAppendN(to, from, strlen(from));
}

int StrAppendChar(char** to, char c)
{
	return StrAppendN(to, &c, 1);
}

int StrAppendRange(char** to, char* b, char* e)
{
	return StrAppendN(to, b, e - b);
}

void StrFree(char** s)
{
	if(s && *s) {
		free(*s);
		*s = 0;
	}
}

//
// MRfc822Tokenizer
//

//
// Character Classification - could be rewritten in a C library
// independent way, my system's C library matches the RFC
// definitions, but I don't know if that's guaranteed.
//
int IsCHAR(char c)
{
	return isascii(c);
}
int IsDIGIT(char** p, char* e)
{
	// DIGIT = <any ASCII decimal digit>

	if(*p == e) {
		return 0;
	}

	return isdigit(**p);
}
int IsCTL(char c)
{
	return iscntrl(c) || c == 127 /* DEL */;
}
int IsSPACE(char c)
{
	return c == ' ';
}
int IsHTAB(char c)
{
	return c == '\t';
}
int IsLWSPChar(char c)
{
	return IsSPACE(c) || IsHTAB(c);
}
int IsSpecial(char c)
{
	return strchr("()<>@,;:\\\".[]", c) ? 1 : 0;
}
int IsAtomChar(char c)
{
	return IsCHAR(c) && !IsSpecial(c) && !IsSPACE(c) && !IsCTL(c);
}
int IsQText(char c)
{
	return IsCHAR(c)
		&& c != '"'
		&& c != '\\'
		&& c != '\r';
}
int IsDText(char c)
{
	return IsCHAR(c)
		&& c != '['
		&& c != ']'
		&& c != '\\'
		&& c != '\r';
}
int IsSmtpQ(char c)
{
	return IsQText(c)
		&& c != '\n';
}

//
// Lexical Analysis - these tokens are all from RFC822,
// section 3.3, Lexical Tokens, though not all tokens are
// implemented.
//

int SkipWs(char** p, char* e)
{
	int ws = 0;

	while((*p != e) && IsLWSPChar(**p)) {
		++ws;
		*p += 1;
	}

	return ws;
}

int SkipComments(char** p, char* e)
{
	int comments;

	while(GetComment(p, e, 0))
		comments++;

	return comments++;
}

int GetDigits(char** p, char* e,
		int min, int max, int* digits)
{
	char* save = *p;

	int i = 0;

	assert(digits);

	*digits = 0;

	while(IsDIGIT(p, e)) {
		*digits = *digits * 10 + **p - '0';
		*p += 1;
		++i;
		if(max != 0 && i == max) {
			break;
		}
	}
	if(i < min) {
		*p = save;
		return 0;
	}

	return 1;
}

int GetSpecial(char** p, char* e, char c)
{
	SkipWs(p, e); // not comments, they start with a special...

	if((*p != e) && **p == c) {
		*p += 1;
		return 1;
	}
	return 0;
}

int GetComment(char** p, char* e, char** comment)
{
	// comment = "(" *(ctext / quoted-pair / comment) ")"
	// ctext = <any CHAR except "(", ")", "\", & CR, including LWSP>

	if(!GetSpecial(p, e, '(')) {
		return 0;
	}
	
	while(*p != e) {
		char c = **p;

		if(c == ')') {
			*p += 1;
			return 1; // found end-of-comment
		} else if(c == '(') {
			GetComment(p, e, comment);
		} else if(c == '\\') {
			GetQuotedPair(p, e, comment);
		} else if(c == '\r') {
			// invalid character...
			*p += 1;
		} else if(IsCHAR(c)) {
			StrAppendChar(comment, c);
			*p += 1;
		} else {
			// invalid character... should I append it?
			*p += 1;
		}
	}
	return 0; // end-of-comment not found
}

int GetAtom(char** p, char* e, char** atom)
{
	// atom = 1*<an atom char>

	int ok = 0;

	SkipComments(p, e);

	while((*p != e) && IsAtomChar(**p))
	{
		++ok;
		StrAppendChar(atom, **p);
		*p += 1;
	}
	return ok;
}
int GetQuotedPair(char** p, char* e, char** qpair)
{
	// quoted-pair = "\" CHAR

	/* need TWO characters to be available */
	if((e - *p) < 2)
		return 0;

	if(**p != '\\')
		return 0;

	*p += 1;
	if(*p == e)
		return 0;

	StrAppendChar(qpair, **p);

	*p += 1;

	return 1;
}
int GetQuotedString(char** p, char* e, char** qstr)
{
	// quoted-string = <"> *(qtext/quoted-pair) <">
	// qtext = CHAR except <">, "\", & CR, including LWSP-char

	SkipComments(p, e);

	if(!GetSpecial(p, e, '"'))
		return 0;

	while(*p != e)
	{
		char c = **p;

		if(c == '"') {
			*p += 1;
			return 1; // found end-of-qstr
		} else if(c == '\\') {
			GetQuotedPair(p, e, qstr);
		} else if(c == '\r') {
			// invalid character...
			*p += 1;
		} else if(IsCHAR(c)) {
			StrAppendChar(qstr, c);
			*p += 1;
		} else {
			// invalid character...
			*p += 1;
		}
	}
	return 0; // end-of-qstr not found
}
int GetWord(char** p, char* e, char** word)
{
	// word = atom / quoted-string

	char* save = *p;

	SkipComments(p, e);

	{
		char* qstr = 0;
		if(GetQuotedString(p, e, &qstr)) {
			StrAppend(word, qstr);

			StrFree(&qstr);

			return 1;
		}
	}

	*p = save;
		// Necessary because the quoted string could have found
		// a partial string (invalid syntax). Thus reset, the atom
		// will fail to if the syntax is invalid.

	{
		char* atom = 0;
		if(GetAtom(p, e, &atom)) {
			StrAppend(word, atom);

			StrFree(&atom);

			return 1;
		}
	}
	*p = save;

	return 0;
}
int GetPhrase(char** p, char* e, char** phrase)
{
	// phrase = 1*word

	if(!GetWord(p, e, phrase)) {
		return 0;
	}
	// ok, got the 1 word, now append all the others we can
	{
		char* word = 0;
		while(GetWord(p, e, &word)) {
			StrAppendChar(phrase, ' ');
			StrAppend(phrase, word);
			*word = 0;
		}
	}
	return 1;
}

// this is all a bit of a hack....
typedef struct _address mailbox_t;

mailbox_t* new_mb(void) {
	return (mailbox_t*) calloc(1, sizeof(mailbox_t));
}

mailbox_t* fill_mb(char* comments, char* personal, char* local, char* domain)
{
	mailbox_t* mb = new_mb();

	if(!mb) {
		return 0;
	}

	mb->comments = comments;
	mb->personal = personal;

	/* this is wrong, local must be quoted */
	StrAppend(&mb->email, local);
	StrAppend(&mb->email, "@");
	StrAppend(&mb->email, domain);

	mb->local_part = local;
	mb->domain = domain;

	return mb;
}

int address_create0 (address_t* a, const char* s)
{
	// a must exist, and can't already have been initialized
	int status = 0;

	if(!a || *a) {
		return EINVAL;
	}
	
	status = GetAddressList(a, (char*) s);

	if(status > 0) {
		(*a)->addr = strdup(s);
		if(!(*a)->addr) {
			address_destroy(a);
			return ENOMEM;
		}
	}

	return 0;
}


int GetAddressList(mailbox_t** a, char* s)
{
	// address-list = #(address)

	char** p = &s;
	char*  e = &s[strlen(s)];
		/* need to make the parsing api const-correct */
	int ok = 0;
	mailbox_t** an = a; /* the next address we'll be creating */

	if(!GetAddress(p, e, an))
		return 0;

	/* An address can contain a group, so an entire
	 * list of addresses may have been appended, or no
	 * addresses at all. Walk to the end. 
	 */
	while(*an) {
		++ok;
		an = &(*an)->next;
	}

	SkipComments(p, e);

	while(GetSpecial(p, e, ','))
	{
		// Remember that 'a,,b' is a valid list!
		if(GetAddress(p, e, an)) {
			while(*an) {
				++ok;
				an = &(*an)->next;
			}
		}
	}

	// A little problem here in that we return the number of
	// addresses found, but if there was trailing garbage
	// in the text, then we'll just be ignoring that.

	return ok;
}
int GetAddress(char**p, char* e, mailbox_t** a)
{
	// address = mailbox / group

	return
		GetMailBox(p, e, a) ||
		GetGroup(p, e, a);
}
int GetGroup(char**p, char* e, mailbox_t** a)
{
	// group = phrase ":" [#mailbox] ";"

	char* save = *p;

	SkipComments(p, e);

	if(!GetPhrase(p, e, 0)) {
		return 0;
	}

	SkipComments(p, e);

	if(!GetSpecial(p, e, ':')) {
		*p = save;
		return 0;
	}

	SkipComments(p, e);

	if(GetMailBox(p, e, a)) {
		a = &(*a)->next;

		/* see if there are more */
		SkipComments(p, e);
		
		while(GetSpecial(p, e, ',')) {
			SkipComments(p, e);

			/* Rembmeber that a,,b is a valid list! */
			if(GetMailBox(p, e, a)) {
				a = &(*a)->next;
			}
		}
	}

	if(!GetSpecial(p, e, ';')) {
		*p = save;
		return 0;
	}

	return 1;
}
int GetMailBox(char** p, char* e, mailbox_t** a)
{
	// mailbox = addr-spec "(" comment ")" / [phrase] route-addr
	//
	//  Note: we parse the ancient comment on the right since
	//    it's such "common practice". :-(
	//  Note: phrase is called display-name in drums.
	//  Note: phrase is optional in drums, though not in RFC 822.

	// -> addr-spec
	if(GetAddrSpec(p, e, a)) {
		char* comment = 0;

		SkipWs(p, e);

		if(GetComment(p, e, &comment)) {
			// yuck.
			(*a)->personal = comment;
		}

		return 1;
	}

	// -> phrase route-addr
	{
		char* save = *p;
		char* phrase = 0;

		GetPhrase(p, e, &phrase);

		if(!GetRouteAddr(p, e, a)) {
			*p = save;
			return 0;
		}

		/* add the phrase */
		(*a)->personal = phrase;
	}

	return 1;
}
int GetRouteAddr(char** p, char* e, mailbox_t ** a)
{
	// route-addr = "<" [route] addr-spec ">"

	char* save = *p;
	char* route = 0;

	SkipComments(p, e);


	if(!GetSpecial(p, e, '<')) {
		*p = save;

		return 0;
	}

	GetRoute(p, e, &route);

	if(!GetAddrSpec(p, e, a)) {
		*p = save;

		StrFree(&route);

		return 0;
	}

	(*a)->route = route; /* now we don't have to free our local */

	SkipComments(p, e);

	if(!GetSpecial(p, e, '>')) {
		*p = save;

		address_destroy(a);
		
		return 0;
	}

	return 1;
}

int GetRoute(char** p, char* e, char** route)
{
	// route = 1#("@" domain ) ":"
	// 
	// Note: I don't hav a way of returning the route, so toss it for now.

	char* accumulator = 0;

	for(;;) {
		SkipComments(p, e);

		if(!GetSpecial(p, e, '@')) {
			// it's not a route
			return 0;
		}

		StrAppend(&accumulator, "@");

		SkipComments(p, e);

		if(!GetDomain(p, e, &accumulator)) {
			// it looked like a route, but there's no domain!
			return 0;
		}

		SkipComments(p, e);

		if(!GetSpecial(p, e, ',')) {
			// there's no more routes
			break;
		}
		StrAppend(&accumulator, ", ");
	}

	SkipComments(p, e);

	if(!GetSpecial(p, e, ':')) {
		return 0;
	}

	StrAppend(route, accumulator);
	StrFree(&accumulator);

	return 1;
}
int GetAddrSpec(char** p, char* e, mailbox_t ** a)
{
	// addr-spec = local-part "@" domain

	char* save = *p;
	char* local_part = 0;
	char* domain = 0;

	if(!GetLocalPart(p, e, &local_part))
		return 0;

	SkipComments(p, e);

	if(!GetSpecial(p, e, '@')) {
		*p = save;
		return 0;
	}

	if(!GetDomain(p, e, &domain)) {
		*p = save;
		return 0;
	}

	*a = fill_mb(0, 0, local_part, domain);

	return 1;
}
int GetLocalPart(char** p, char* e, char** local_part)
{
	// local-part = word *("." word)

	// Note: rewrite as ->  word ["." local-part]

	char* save = *p;

	SkipComments(p, e);

	if(!GetWord(p, e, local_part)) {
		*p = save;
		return 0;
	}
	// we've got a local-part, but keep looking for more

	save = *p;

	SkipComments(p, e);

	if(!GetSpecial(p, e, '.')) {
		*p = save;
		return 1;
	}
	{
		char* more = 0;

		if(!GetLocalPart(p, e, &more)) {
			*p = save;
			return 1;
		}
		// append more
		StrAppend(local_part, ".");
		StrAppend(local_part, more);
		StrFree(&more);
	}

	return 1;
}
int GetDomain(char** p, char* e, char** domain)
{
	// domain = sub-domain *("." sub-domain)

	// Note: rewrite as -> sub-domain ("." domain)

	char* save = 0;

	if(!GetSubDomain(p, e, domain))
		return 0;

	// we've got the 1, keep looking for more

	save = *p;

	SkipComments(p, e);

	if(!GetSpecial(p, e, '.')) {
		// we do this to preserve the comment at the end of a
		// domain, the addr-spec may want to abuse it for a
		// personal name.
		*p = save;
		return 1;
	}
	{
		char* more = 0;
		if(!GetDomain(p, e, &more)) {
			*p = save;
			return 1;
		}

		StrAppend(domain, ".");
		StrAppend(domain, more);
		StrFree(&more);
	}

	return 1;
}
int GetSubDomain(char** p, char* e, char** sub_domain)
{
	// sub-domain = domain-ref / domain-literal
	//   Note: domain-literal isn't supported yet.

	return
		GetDomainRef(p, e, sub_domain) ||
		GetDomainLiteral(p, e, sub_domain);
}
int GetDomainRef(char** p, char* e, char** domain_ref)
{
	// domain-ref = atom

	return GetAtom(p, e, domain_ref);
}
int GetDText(char** p, char* e, char** dtext)
{
	// dtext = 1*dtext

	// Note: dtext is only defined as a character class in
	//  RFC822, but this definition is more useful for
	//  slurping domain literals.

	char* start = *p;

	while(*p < e && IsDText(**p)) {
		*p += 1;
	}

	if(start == *p) {
		return 0;
	}

	StrAppendRange(dtext, start, *p);

	return 1;
}

int GetDomainLiteral(char** p, char* e, char** domain_literal)
{
	// domain-literal = "[" *(dtext / quoted-pair) "]"

	char* save = *p;

	char* literal = 0;

	if(!GetSpecial(p, e, '[')) {
		return 0;
	}
	StrAppendChar(&literal, '[');

	while(GetDText(p, e, &literal) || GetQuotedPair(p, e, &literal)) {
		/* Eat all of this we can get! */
	}
	if(!GetSpecial(p, e, ']')) {
		*p = save;

		return 0;
	}
	StrAppendChar(&literal, ']');

	StrAppend(domain_literal, literal);

	StrFree(&literal);

	return 1;
}

#if 0
int GetFieldName(char** p, char* e, char** fieldname)
{
	// field-name = 1*<any CHAR, excluding CTLS, SPACE, and ":"> ":"

	Ptr save = p;

	Rope fn;

	while(*p != e) {
		char c = *p;

		if(!IsCHAR(c))
			break;

		if(IsCTL(c))
			break;
		if(IsSPACE(c))
			break;
		if(c == ':')
			break;

		fn.append(c);
		*p += 1;
	}
	// must be at least one char in the field name
	if(fn.empty()) {
		p = save;
		return 0;
	}
	SkipComments(p, e);

	if(!GetSpecial(p, e, ':')) {
		p = save;
		return 0;
	}

	fieldname = fn;

	return 1;
}
int GetFieldBody(char** p, char* e, Rope& fieldbody)
{
	// field-body = *text [CRLF LWSP-char field-body]

	Ptr save = p;

	Rope fb;

	for(;;)
	{
		Ptr eol = p;
		while(eol != e) {
			char c = *eol;
			if(eol[0] == '\r' && (eol+1) != e && eol[1] == '\n')
				break;
			++eol;
		}
		fb.append(p, eol);
		p = eol;
		if(eol == e)
			break; // no more, so we're done

		assert(p[0] == '\r');
		assert(p[1] == '\n');

		p += 2;

		if(*p == e)
			break; // no more, so we're done

		// check if next line is a continuation line
		if(*p != ' ' && *p != '\t')
			break;
	}

	fieldbody = fb;

	return 1;
}
#endif

