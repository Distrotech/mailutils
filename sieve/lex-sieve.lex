/* Scanner for Sieve
   created : Alain Magloire
   ref: RFC 3028  */

%{
#include <stdio.h>
#include "y.tab.h"

void count ();
%}

DIGIT       [0-9]
WHITE_SPACE [ \r\t]
QUANTIFIER  [KMG]
DELIM       [ \t\v\f\n]
LETTER      [A-Za-z_]
ID          [A-Za-z_][A-Za-z0-9_]*
TAG         {ID}:

%x COMMENT
%x MULTILINE
%%

"/*"           { BEGIN COMMENT; count (); } /* Switch to comment state.  */
<COMMENT>.|\n  /* Throw away text. */;
<COMMENT>"*/"  { BEGIN INITIAL; count (); }

"text:"\n      { BEGIN MULTILINE; count (); }
<MULTILINE>.|\n    { count (); }
<MULTILINE>^\.\n   { BEGIN INITIAL; count (); return SIEVE_STRING; }

#.*            /* Throw away.  */;

{DIGIT}+[QUANTIFIER]?  { count (); return SIEVE_NUMBER; }
\"[^\n"]+\"    { /* " */ count (); return SIEVE_STRING; }

address        { count (); return SIEVE_ADDRESS; }
all            { count (); return SIEVE_ALL; }
allof          { count (); return SIEVE_ALLOF; }
anyof          { count (); return SIEVE_ANYOF; }
comparator     { count (); return SIEVE_COMPARATOR; }
contains       { count (); return SIEVE_CONTAINS; }
discard        { count (); return SIEVE_DISCARD; }
domain         { count (); return SIEVE_DOMAIN; }
else           { count (); return SIEVE_ELSE; }
elsif          { count (); return SIEVE_ELSIF; }
envelope       { count (); return SIEVE_ENVELOPE; }
exists         { count (); return SIEVE_EXISTS; }
false          { count (); return SIEVE_FALSE; }
fileinto       { count (); return SIEVE_FILEINTO; }
header         { count (); return SIEVE_HEADER; }
if             { count (); return SIEVE_IF; }
is             { count (); return SIEVE_IS; }
keep           { count (); return SIEVE_KEEP; }
localpart      { count (); return SIEVE_LOCALPART; }
matches        { count (); return SIEVE_MATCHES; }
not            { count (); return SIEVE_NOT; }
over           { count (); return SIEVE_OVER; }
redirect       { count (); return SIEVE_REDIRECT; }
reject         { count (); return SIEVE_REJECT; }
require        { count (); return SIEVE_REQUIRE; }
size           { count (); return SIEVE_SIZE; }
stop           { count (); return SIEVE_STOP; }
true           { count (); return SIEVE_TRUE; }
\:             { count (); return ':'; }
\;             { count (); return ';'; }
\,             { count (); return ','; }
\(             { count (); return '('; }
\)             { count (); return ')'; }
\[             { count (); return '['; }
\]             { count (); return ']'; }
\{             { count (); return '{'; }
\}             { count (); return '}'; }

%%

int
yywrap ()
{
  return (1);
}

void
count ()
{
  ECHO;
}

int
yyerror (char *s)
{
  fflush(stdout);
  printf ("\n^ Syntax error.\n");
}
