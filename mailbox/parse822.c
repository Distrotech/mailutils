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

/*
Things to consider:

  - A group should create an address node with a group, accessable
    with address_get_group().

  - Make domain optional in addr-spec, for parsing address lists
    provided to local mail utilities.

  - Should we do best effort parsing, so parsing "sam@locahost, foo@"
    gets one address, or just say it is or it isn't in RFC format?
    Right now we're strict, we'll see how it goes.

  - quote local-part when generating email field of address_t.

  - parse field names and bodies?
  - parse dates?
  - parse Received: field?

  - test for memory leaks on malloc failure
  - fix the realloc, try a struct _string { char* b, size_t sz };

*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "address0.h"

#include <mailutils/parse822.h>

#ifdef EOK
# undef EOK
#endif

#define EOK	0
#define EPARSE	ENOENT

/*
 * Some convenience functions for dealing with dynamically re-sized
 * strings.
 */

static int str_append_n(char** to, const char* from, size_t n)
{
    size_t l = 0;

    /* if not to, then silently discard data */
    if(!to) {
	return EOK;
    }

    if(*to) {
	char* bigger;

	l = strlen(*to);

	bigger = realloc(*to, l + n + 1);

	if(!bigger) {
	    return ENOMEM;
	}

	*to = bigger;
    } else {
	*to = malloc(n + 1);
    }

    strncpy(&to[0][l], from, n);

    /* strncpy is lame, nul terminate our buffer */

    to[0][l + n] = 0;

    return EOK;
}
static int str_append(char** to, const char* from)
{
    return str_append_n(to, from, strlen(from));
}
static int str_append_char(char** to, char c)
{
    return str_append_n(to, &c, 1);
}
static int str_append_range(char** to, const char* b, const char* e)
{
    return str_append_n(to, b, e - b);
}
static void str_free(char** s)
{
    if(s && *s) {
	free(*s);
	*s = 0;
    }
}

/*
 * Character Classification - could be rewritten in a C library
 * independent way, my system's C library matches the RFC
 * definitions. I don't know if that's guaranteed.
 *
 * Note that all return values are:
 *   1 -> TRUE
 *   0 -> FALSE
 * This may be appear different than the 0 == success return
 * values of the other functions, but I was getting lost in
 * boolean arithmetic.
 */
int parse822_is_char(char c)
{
    return isascii(c);
}
int parse822_is_digit(char c)
{
    /* digit = <any ASCII decimal digit> */

    return isdigit(c);
}
int parse822_is_ctl(char c)
{
    return iscntrl(c) || c == 127 /* DEL */;
}
int parse822_is_space(char c)
{
    return c == ' ';
}
int parse822_is_htab(char c)
{
    return c == '\t';
}
int parse822_is_lwsp_char(char c)
{
    return parse822_is_space(c) || parse822_is_htab(c);
}
int parse822_is_special(char c)
{
    return strchr("()<>@,;:\\\".[]", c) ? 1 : 0;
}
int parse822_is_atom_char(char c)
{
    return
	parse822_is_char(c) &&
	!parse822_is_special(c) &&
	!parse822_is_space(c) &&
	!parse822_is_ctl(c)
	;
}
int parse822_is_q_text(char c)
{
    return
	parse822_is_char(c)
	&& c != '"'
	&& c != '\\'
	&& c != '\r'
	;
}
int parse822_is_d_text(char c)
{
    return
	parse822_is_char(c)
	&& c != '['
	&& c != ']'
	&& c != '\\'
	&& c != '\r'
	;
}
/*
 * SMTP's version of qtext, called <q> in the RFC 821 syntax,
 * also excludes <LF>.
 */
int parse822_is_smtp_q(char c)
{
    return parse822_is_q_text(c)
	&& c != '\n';
}

/*
 * Lexical Analysis - these tokens are all from RFC822,
 * section 3.3, Lexical Tokens, though not all tokens are
 * implemented. The names match those used int the extended
 * BNF of the RFC where possible.
 */

int parse822_skip_ws(const char** p, const char* e)
{
    while((*p != e) && parse822_is_lwsp_char(**p)) {
	*p += 1;
    }
    return EOK;
}

int parse822_skip_comments(const char** p, const char* e)
{
    int status;

    while((status = parse822_comment(p, e, 0)) == EOK)
	;

    return EOK;
}

int parse822_digits(const char** p, const char* e,
	int min, int max, int* digits)
{
    const char* save = *p;

    int i = 0;

    assert(digits);

    *digits = 0;

    while(*p < e && parse822_is_digit(**p)) {
	*digits = *digits * 10 + **p - '0';
	*p += 1;
	++i;
	if(max != 0 && i == max) {
	    break;
	}
    }
    if(i < min) {
	*p = save;
	return EPARSE;
    }

    return EOK;
}

int parse822_special(const char** p, const char* e, char c)
{
    parse822_skip_ws(p, e); /* not comments, they start with a special... */

    if((*p != e) && **p == c) {
	*p += 1;
	return EOK;
    }
    return EPARSE;
}

int parse822_comment(const char** p, const char* e, char** comment)
{
    /* comment = "(" *(ctext / quoted-pair / comment) ")"
     * ctext = <any char except "(", ")", "\", & CR, including lwsp>
     */
    const char* save = *p;
    int rc;

    if((rc = parse822_special(p, e, '('))) {
	return rc;
    }

    while(*p != e) {
	char c = **p;

	if(c == ')') {
	    *p += 1;
	    return EOK; /* found end-of-comment */
	} else if(c == '(') {
	    rc = parse822_comment(p, e, comment);
	} else if(c == '\\') {
	    rc = parse822_quoted_pair(p, e, comment);
	} else if(c == '\r') {
	    /* invalid character... */
	    *p += 1;
	} else if(parse822_is_char(c)) {
	    rc = str_append_char(comment, c);
	    *p += 1;
	} else {
	    /* invalid character... */
	    *p += 1;
	}
	if(rc != EOK)
	    break;
    }

    if(*p == e) {
	rc = EPARSE; /* end-of-comment not found */
    }

    *p = save;

    assert(rc != EOK);

    return rc;
}

int parse822_atom(const char** p, const char* e, char** atom)
{
    /* atom = 1*<an atom char> */

    const char* save = *p;
    int rc = EPARSE;

    parse822_skip_comments(p, e);

    save = *p;

    while((*p != e) && parse822_is_atom_char(**p))
    {
	rc = str_append_char(atom, **p);
	*p += 1;
	if(rc != EOK) {
	    *p = save;
	    break;
	}
    }
    return rc;
}

int parse822_quoted_pair(const char** p, const char* e, char** qpair)
{
    /* quoted-pair = "\" char */

    int rc;

    /* need TWO characters to be available */
    if((e - *p) < 2)
	return EPARSE;

    if(**p != '\\')
	return EPARSE;

    if((rc = str_append_char(qpair, *(*p + 1))))
	return rc;

    *p += 2;

    return EOK;
}

int parse822_quoted_string(const char** p, const char* e, char** qstr)
{
    /* quoted-string = <"> *(qtext/quoted-pair) <">
     * qtext = char except <">, "\", & CR, including lwsp-char
     */

    const char* save = *p;
    int rc;

    parse822_skip_comments(p, e);

    save = *p;

    if((rc = parse822_special(p, e, '"')))
	return rc;

    while(*p != e)
    {
	char c = **p;

	if(c == '"') {
	    *p += 1;
	    return EOK; /* found end-of-qstr */
	} else if(c == '\\') {
	    rc = parse822_quoted_pair(p, e, qstr);
	} else if(c == '\r') {
	    /* invalid character... */
	    *p += 1;
	} else if(parse822_is_char(c)) {
	    rc = str_append_char(qstr, c);
	    *p += 1;
	} else {
	    /* invalid character... */
	    *p += 1;
	}
	if(rc) {
	    *p = save;
	    return rc;
	}
    }
    *p = save;

    return EPARSE; /* end-of-qstr not found */
}

int parse822_word(const char** p, const char* e, char** word)
{
    /* word = atom / quoted-string */

    const char* save = *p;
    int rc = EOK;

    parse822_skip_comments(p, e);

    save = *p;

    {
	char* qstr = 0;
	if((rc = parse822_quoted_string(p, e, &qstr)) == EOK) {
	    rc = str_append(word, qstr);

	    str_free(&qstr);

	    if(rc != EOK)
		*p = save;

	    return rc;
	}
    }

    if(rc != EPARSE) {
	/* it's fatal */
	return rc;
    }

    /* Necessary because the quoted string could have found
     * a partial string (invalid syntax). Thus reset, the atom
     * will fail to if the syntax is invalid.
     */

    {
	char* atom = 0;
	if(parse822_atom(p, e, &atom) == EOK) {
	    rc = str_append(word, atom);

	    str_free(&atom);

	    if(rc != EOK)
		*p = save;

	    return rc;
	}
    }

    return EPARSE;
}

int parse822_phrase(const char** p, const char* e, char** phrase)
{
    /* phrase = 1*word */

    const char* save = *p;
    int rc;

    if((rc = parse822_word(p, e, phrase)))
	return rc;

    /* ok, got the 1 word, now append all the others we can */
    {
	char* word = 0;

	while((rc = parse822_word(p, e, &word)) == EOK) {
	    rc = str_append_char(phrase, ' ');

	    if(rc == EOK)
		rc = str_append(phrase, word);

	    str_free(&word);

	    if(rc != EOK)
		break;
	}
	if(rc == EPARSE)
	    rc = EOK; /* its not an error to find no more words */
    }
    if(rc)
	*p = save;

    return rc;
}

static address_t new_mb(void) {
    return calloc(1, sizeof(struct _address));
}

static int fill_mb(
	address_t* a,
	char* comments, char* personal, char* local, char* domain)
{
    int rc = EOK;

    *a = new_mb();

    if(!*a) {
	return ENOMEM;
    }

    (*a)->comments = comments;
    (*a)->personal = personal;

    /* this is wrong, local must be quoted */
    do {
	/* loop exists only to break out of */
	if((rc = str_append(&(*a)->email, local)))
	    break;
	if((rc = str_append(&(*a)->email, "@")))
	    break;
	if((rc = str_append(&(*a)->email, domain)))
	    break;
    } while(0);

    (*a)->local_part = local;
    (*a)->domain = domain;

    if(rc != EOK) {
	/* note that the arguments have NOT been freed, we only own
	 * them on success. */
	free(*a);
    }

    return rc;
}

int parse822_address_list(address_t* a, const char* s)
{
    /* address-list = #(address) */

    const char** p = &s;
    const char*  e = &s[strlen(s)];
    int rc = EOK;
    address_t* n = a; /* the next address we'll be creating */

    if((rc = parse822_address(p, e, n)))
	return rc;

    parse822_skip_comments(p, e);

    while(*p < e)
    {
	/* An address can contain a group, so an entire
	 * list of addresses may have been appended, or no
	 * addresses at all. Walk to the end.
	 */
	while(*n) {
	    n = &(*n)->next;
	}

	/* Remember that ',,a@b' is a valid list! So, we must find
	 * the <,>, but the address after it can be empty.
	 */
	if((rc = parse822_special(p, e, ','))) {
	     break;
	}
	parse822_skip_comments(p, e);

	rc = parse822_address(p, e, n);

	if(rc == EOK || rc == EPARSE) {
	    /* that's cool, it may be a <,>, we'll find out if it isn't
	     * at the top of the loop
	     */
	    rc = EOK;
	} else {
	    /* anything else is a fatal error, break out */
	    break;
	}

	parse822_skip_comments(p, e);
    }

    if(rc) {
	address_destroy(a);
    }

    return rc;
}

int parse822_address(const char** p, const char* e, address_t* a)
{
    /* address = mailbox / group */

    int rc;

    if((rc = parse822_mail_box(p, e, a)) == EPARSE)
    	rc = parse822_group(p, e, a);

    return rc;
}

int parse822_group(const char** p, const char* e, address_t* a)
{
    /* group = phrase ":" [#mailbox] ";" */

    const char* save = *p;
    address_t* asave = a; /* so we can destroy these if parsing fails */
    int rc;

    parse822_skip_comments(p, e);

    *p = save;

    if((rc = parse822_phrase(p, e, 0))) {
	return rc;
    }

    parse822_skip_comments(p, e);

    if((rc = parse822_special(p, e, ':'))) {
	*p = save;
	return rc;
    }

    /* Basically, on each loop, we may find a mailbox, but we must find
     * a comma after the mailbox, otherwise we've popped off the end
     * of the list.
     */
    for(;;) {
	parse822_skip_comments(p, e);

	/* it's ok not be a mailbox, but other errors are fatal */
	rc = parse822_mail_box(p, e, a);
	if(rc == EOK) {
	    a = &(*a)->next;

	    parse822_skip_comments(p, e);
	} else if(rc != EPARSE) {
	    break;
	}

	if((rc = parse822_special(p, e, ','))) {
	    /* the commas aren't optional */
	    break;
	}
    }
    if(rc == EPARSE) {
	rc = EOK; /* ok, as long as we find the ";" next */
    }

    if(rc || (rc = parse822_special(p, e, ';'))) {
	*p = save;

	address_destroy(asave);
    }

    return rc;
}

int parse822_mail_box(const char** p, const char* e, address_t* a)
{
    /* mailbox = addr-spec [ "(" comment ")" ] / [phrase] route-addr
     *
     *	Note: we parse the ancient comment on the right since
     *	  it's such "common practice". :-(
     *	Note: phrase is called display-name in drums.
     *	Note: phrase is optional in drums, though not in RFC 822.
     */
    const char* save = *p;
    int rc;

    /* -> addr-spec */
    if((rc = parse822_addr_spec(p, e, a)) == EOK) {
	parse822_skip_ws(p, e);

	/* yuck. */
	if((rc = parse822_comment(p, e, &(*a)->personal)) == EPARSE) {
	    rc = EOK;
	    /* cool if there's no comment, */
	}
	/* but if something else is wrong, destroy the address */
	if(rc) {
	    address_destroy(a);
	    *p = save;
	}

	return rc;
    }
    if(rc != EPARSE) {
	*p = save;
	return rc;
    }

    /* -> phrase route-addr */
    {
	char* phrase = 0;

	rc = parse822_phrase(p, e, &phrase);

	if(rc != EPARSE && rc != EOK) {
	    return rc;
	}

	if((rc = parse822_route_addr(p, e, a))) {
	    *p = save;
	    str_free(&phrase);
	    return rc;
	}

	/* add the phrase */
	(*a)->personal = phrase;
    }

    return EOK;
}

int parse822_route_addr(const char** p, const char* e, address_t* a)
{
    /* route-addr = "<" [route] addr-spec ">" */

    const char* save = *p;
    char* route = 0;
    int rc;

    parse822_skip_comments(p, e);

    if((rc = parse822_special(p, e, '<'))) {
	*p = save;

	return rc;
    }

    parse822_route(p, e, &route);

    if((rc = parse822_addr_spec(p, e, a))) {
	*p = save;

	str_free(&route);

	return rc;
    }

    (*a)->route = route; /* now we don't have to free our local */

    parse822_skip_comments(p, e);

    if((rc = parse822_special(p, e, '>'))) {
	*p = save;

	address_destroy(a);

	return rc;
    }

    return EOK;
}

int parse822_route(const char** p, const char* e, char** route)
{
    /* route = 1#("@" domain ) ":" */

    const char* save = *p;
    char* accumulator = 0;
    int rc = EOK;

    for(;;) {
	parse822_skip_comments(p, e);

	if((rc = parse822_special(p, e, '@'))) {
	    break;
	}

	if((rc = str_append(&accumulator, "@"))) {
	    break;
	}

	parse822_skip_comments(p, e);

	if((rc = parse822_domain(p, e, &accumulator))) {
	    /* it looked like a route, but there's no domain! */
	    break;
	}

	parse822_skip_comments(p, e);

	if((rc = parse822_special(p, e, ',')) == EPARSE) {
	    /* no more routes, but we got one so its ok */
	    rc = EOK;
	    break;
	}
	if((rc = str_append(&accumulator, ", "))) {
	    break;
	}
    }

    parse822_skip_comments(p, e);

    if(!rc) {
	rc = parse822_special(p, e, ':');
    }

    if(!rc) {
	rc = str_append(route, accumulator);
    }
    if(rc) {
	str_free(&accumulator);
	*p = save;
    }

    return rc;
}

int parse822_addr_spec(const char** p, const char* e, address_t* a)
{
    /* addr-spec = local-part "@" domain */

    const char* save = *p;
    char* local_part = 0;
    char* domain = 0;
    int rc;

    rc = parse822_local_part(p, e, &local_part);

    parse822_skip_comments(p, e);

    if(!rc) {
	rc = parse822_special(p, e, '@');
    }

    if(!rc) {
	rc = parse822_domain(p, e, &domain);
    }

    if(!rc) {
	rc = fill_mb(a, 0, 0, local_part, domain);
    }

    if(rc) {
	*p = save;
	str_free(&local_part);
	str_free(&domain);
    }
    return rc;
}

int parse822_local_part(const char** p, const char* e, char** local_part)
{
    /* local-part = word *("." word)
     *
     * Note: rewrite as ->  word ["." local-part]
     */

    const char* save = *p;
    const char* save2 = *p;
    int rc;

    parse822_skip_comments(p, e);

    if((rc = parse822_word(p, e, local_part))) {
	*p = save;
	return rc;
    }
    /* We've got a local-part, but keep looking for more. */

    parse822_skip_comments(p, e);

    /* If we get a parse error, we roll back to save2, but if
     * something else failed, we have to roll back to save.
     */
    save2 = *p;

    rc = parse822_special(p, e, '.');

    if(!rc) {
	char* more = 0;
	if((rc = parse822_local_part(p, e, &more)) == EOK) {
	    /* append more */
	    if((rc = str_append(local_part, ".")) == EOK) {
		rc = str_append(local_part, more);
	    }
	    str_free(&more);
	}
    }

    if(rc == EPARSE) {
	/* if we didn't get more ("." word) pairs, that's ok */
	*p = save2;
	rc = EOK;
    }
    if(rc) {
	/* if anything else failed, that's real */
	*p = save;
	str_free(local_part);
    }
    return rc;
}

int parse822_domain(const char** p, const char* e, char** domain)
{
    /* domain = sub-domain *("." sub-domain)
     *
     * Note: rewrite as -> sub-domain ("." domain)
     */

    const char* save = *p;
    const char* save2 = 0;
    int rc;

    parse822_skip_comments(p, e);

    if((rc = parse822_sub_domain(p, e, domain))) {
	*p = save;
	return rc;
    }


    /* We save before skipping comments to preserve the comment
     * at the end of a domain, the addr-spec may want to abuse it
     * for a personal name.
     */
    save2 = *p;

    /* we've got the 1, keep looking for more */

    parse822_skip_comments(p, e);

    rc = parse822_special(p, e, '.');

    if(!rc) {
	char* more = 0;
	if((rc = parse822_domain(p, e, &more)) == EOK) {
	    if((rc = str_append(domain, ".")) == EOK) {
		rc = str_append(domain, more);
	    }
	    str_free(&more);
	}
    }
    if(rc == EPARSE) {
	/* we didn't parse more ("." sub-domain) pairs, that's ok */
	*p = save2;
	rc = EOK;
    }

    if(rc) {
	/* something else failed, roll it all back */
	*p = save;
	str_free(domain);
    }
    return rc;
}

int parse822_sub_domain(const char** p, const char* e, char** sub_domain)
{
    /* sub-domain = domain-ref / domain-literal
     */

    int rc;

    if((rc = parse822_domain_ref(p, e, sub_domain)) == EPARSE)
    	rc = parse822_domain_literal(p, e, sub_domain);

    return rc;
}

int parse822_domain_ref(const char** p, const char* e, char** domain_ref)
{
    /* domain-ref = atom */

    return parse822_atom(p, e, domain_ref);
}

int parse822_d_text(const char** p, const char* e, char** dtext)
{
    /* d-text = 1*dtext
     *
     * Note: dtext is only defined as a character class in
     *	RFC822, but this definition is more useful for
     *	slurping domain literals.
     */

    const char* start = *p;
    int rc = EOK;

    while(*p < e && parse822_is_d_text(**p)) {
	*p += 1;
    }

    if(start == *p) {
	return EPARSE;
    }

    if((rc = str_append_range(dtext, start, *p))) {
	*p = start;
    }

    return rc;
}

int parse822_domain_literal(const char** p, const char* e, char** domain_literal)
{
    /* domain-literal = "[" *(dtext / quoted-pair) "]" */

    const char* save = *p;
    char* literal = 0;
    int rc;

    if((rc = parse822_special(p, e, '['))) {
	return rc;
    }
    if((rc = str_append_char(&literal, '['))) {
	*p = save;
	return rc;
    }

    while(
	    (rc = parse822_d_text(p, e, &literal)) == EOK ||
	    (rc = parse822_quoted_pair(p, e, &literal)) == EOK
	) {
	/* Eat all of this we can get! */
    }
    if(rc == EPARSE) {
	rc = EOK;
    }
    if(!rc) {
	rc = parse822_special(p, e, ']');
    }
    if(!rc) {
	rc = str_append_char(&literal, ']');
    }
    if(!rc) {
	rc = str_append(domain_literal, literal);
    }

    str_free(&literal);

    if(rc) {
	*p = save;
    }
    return rc;
}

#if 0
int parse822_field_name(const char** p, const char* e, char** fieldname)
{
    /* field-name = 1*<any char, excluding ctlS, space, and ":"> ":" */

    Ptr save = p;

    Rope fn;

    while(*p != e) {
	char c = *p;

	if(!parse822_is_char(c))
	    break;

	if(parse822_is_ctl(c))
	    break;
	if(parse822_is_space(c))
	    break;
	if(c == ':')
	    break;

	fn.append(c);
	*p += 1;
    }
    /* must be at least one char in the field name */
    if(fn.empty()) {
	p = save;
	return 0;
    }
    parse822_skip_comments(p, e);

    if(!parse822_special(p, e, ':')) {
	p = save;
	return 0;
    }

    fieldname = fn;

    return 1;
}

int parse822_field_body(const char** p, const char* e, Rope& fieldbody)
{
    /* field-body = *text [CRLF lwsp-char field-body] */

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
	    break; /* no more, so we're done */

	assert(p[0] == '\r');
	assert(p[1] == '\n');

	p += 2;

	if(*p == e)
	    break; /* no more, so we're done */

	/* check if next line is a continuation line */
	if(*p != ' ' && *p != '\t')
	    break;
    }

    fieldbody = fb;

    return 1;
}
#endif

