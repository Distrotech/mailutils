%{
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

%}

%token SIEVE_ADDRESS SIEVE_ALL SIEVE_ALLOF SIEVE_ANYOF SIEVE_COMPARATOR
%token SIEVE_CONTAINS SIEVE_DISCARD SIEVE_DOMAIN SIEVE_ELSE SIEVE_ELSIF
%token SIEVE_ENVELOPE SIEVE_EXISTS SIEVE_FALSE SIEVE_FILEINTO SIEVE_HEADER
%token SIEVE_IF SIEVE_IS SIEVE_KEEP SIEVE_LOCALPART SIEVE_MATCHES
%token SIEVE_NOT SIEVE_NUMBER SIEVE_OVER SIEVE_STRING
%token SIEVE_REDIRECT SIEVE_REJECT SIEVE_REQUIRE SIEVE_SIZE SIEVE_STOP
%token SIEVE_TEST SIEVE_TRUE SIEVE_UNDER

%%

commands : command | command commands ;

command : action_command |  control_command | test_command ;

block : '{' commands '}' | '{' /* Empty block.  */ '}' ;

action_command : action ';' ;

action : discard | fileinto | keep | redirect | require | reject | stop ;

control_command : SIEVE_IF test_command block else_part
          | SIEVE_IF test_command block
                        ;
else_part : SIEVE_ELSIF test_command block else_part
          | SIEVE_ELSE block

test_command : test_address | test_allof | test_anyof | test_envelope
             | test_exists  | test_false | test_header | test_not
             | test_size | test_true ;

fileinto : SIEVE_FILEINTO string_list ;

stop : SIEVE_STOP ;

discard : SIEVE_DISCARD ;

keep : SIEVE_KEEP ;

redirect : SIEVE_REDIRECT string_list ;

reject : SIEVE_REJECT SIEVE_STRING ;

require : SIEVE_REQUIRE string_list ;

test_list : '(' tests ')' ;

tests : test | test ',' tests ;

test : test_address | test_anyof | test_envelope | test_false | test_exists
     | test_header | test_not | test_size | test_true ;

test_address : SIEVE_ADDRESS address_part match_type
             | SIEVE_ADDRESS address_part string_list ;

test_allof : SIEVE_ALLOF test_list ;

test_anyof : SIEVE_ANYOF test_list ;

test_envelope : SIEVE_ENVELOPE ':' comparator | SIEVE_ENVELOPE ':' match_type ;

test_exists : SIEVE_EXISTS string_list ;

test_false : SIEVE_FALSE ;

test_header : SIEVE_HEADER comparator
            | SIEVE_HEADER match_type
            | SIEVE_HEADER string_list ;

test_not : SIEVE_NOT test | SIEVE_NOT '(' test ')' ;

test_size : SIEVE_SIZE ':' SIEVE_OVER SIEVE_NUMBER
          | SIEVE_SIZE ':' SIEVE_UNDER SIEVE_NUMBER ;

test_true : SIEVE_TRUE ;

comparator : ':' SIEVE_COMPARATOR SIEVE_STRING SIEVE_STRING

match_type : ':' SIEVE_IS string_list string_list
           | ':' SIEVE_CONTAINS string_list string_list
           | ':' SIEVE_MATCHES string_list string_list ;

address_part : ':' SIEVE_DOMAIN ;
            | ':' SIEVE_LOCALPART ;
            | ':' SIEVE_ALL ;

strings : SIEVE_STRING | SIEVE_STRING ',' strings

string_list : '[' strings ']' | SIEVE_STRING ;
