;;;; GNU mailutils - a suite of utilities for electronic mail
;;;; Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
;;;;
;;;; This program is free software; you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation; either version 2, or (at your option)
;;;; any later version.
;;;; 
;;;; This program is distributed in the hope that it will be useful,
;;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;; GNU General Public License for more details.
;;;; 
;;;; You should have received a copy of the GNU General Public License
;;;; along with this program; if not, write to the Free Software
;;;; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
;;;;

;;;; This is a Sieve to Scheme translator.
;;;;
;;;; To convert a sieve script into equivalent Scheme program, executable
;;;; by guimb, run:
;;;;
;;;;  guile -s sieve.scm --file <sieve-script-name> --output <output-file-name>
;;;;
;;;; To compile and execute a sieve script upon a mailbox, run:
;;;;
;;;;  guimb -f sieve.scm -{ --file <sieve-script-name> -} --mailbox ~/mbox

(define sieve-debug 0)
(define sieve-parser #t)
(define sieve-libdir #f)
(define sieve-load-files '())

(define error-count 0)
(define current-token #f)
(define putback-list '())
(define input-port #f)
(define input-file "")
(define input-line-number 0)
(define input-line "")
(define input-index 0)
(define input-length 0)
(define nesting-level 0)
(define recovery-line-number -1)
(define recovery-index -1)

(define (DEBUG level . rest)
  (if (>= sieve-debug level)
      (begin
	(display "DEBUG(")
	(display level)
	(display "):")
	(for-each (lambda (x)
		    (display x))
		  rest)
	(newline))))

;;;; Lexical scanner
(define (delimiter? c)
  (or (member c (list #\[ #\] #\{ #\} #\, #\; #\( #\)))
      (char-whitespace? c)))

(define (lex-error . rest)
  (set! error-count (1+ error-count))
  (display input-file)
  (display ":")
  (display input-line-number)
  (display ": ")
  (for-each (lambda (x)
	      (display x))
	    rest)
  (newline)
  #t)

(define (syntax-error . rest)
  (set! error-count (1+ error-count))
  (display input-file)
  (display ":")
  (display input-line-number)
  (display ": ")
  (for-each (lambda (x)
	      (display x))
	    rest)
  (newline)
  (throw 'syntax-error))


;;; If current input position points to end of line or to a start of
;;; # comment, return #f. Otherwise return cons whose car contains
;;; token type and cdr contains token itself (string). 
(define (next-token)
  (let ((start (do ((i input-index (1+ i)))
		   ((or (>= i input-length)
			(not (char-whitespace? (string-ref input-line i))))
		    i))))
;    (DEBUG 100 "START " start ": " (substring input-line start)) 
    (if (< start input-length)
	(let ((char (string-ref input-line start)))
	  (DEBUG 100 "CHAR " char)
	  (case char
	    ((#\#)
	     (set! input-index input-length)
	     #f)
	    ((#\[ #\] #\{ #\} #\( #\) #\, #\;)
	     (set! input-index (1+ start))
	     (cons 'delimiter char))
	    ((#\")
	     (let ((end (do ((end (1+ start) (1+ end)))
			    ((or (>= end input-length)
				 (char=? (string-ref input-line end) #\"))
			     end))))
	       (if (>= end input-length)
		   (lex-error "Unterminated string constant"))
		  (set! input-index (1+ end))
		  (cons 'string (substring input-line (1+ start) end))))
	    (else
	     (DEBUG 100 "MATCH else")
	     (cond
	      ((and (char=? (string-ref input-line start) #\/)
		    (< (1+ start) input-length)
		    (char=? (string-ref input-line (1+ start)) #\*))
	       (set! input-index (+ start 2))
	       (cons 'bracket-comment "/*"))
	      ((char-numeric? char)
	       (let* ((end (do ((end start (1+ end)))
			       ((or
				 (>= end input-length)
				 (not (char-numeric?
				       (string-ref input-line end))))
				 end)))
		      (num (string->number (substring input-line start end))) 
		      (q (string-ref input-line end))
		      (k 1))
		 (case q
		   ((#\K)
		    (set! end (1+ end))
		    (set! k 1024))
		   ((#\M)
		    (set! end (1+ end))
		    (set! k 1048576))
		   ((#\G)
		    (set! end (1+ end))
		    (set! k 1073741824))
		   (else
		    (if (not (delimiter? q))
			(lex-error "Unknown qualifier (" q ")"))))
		 (set! input-index end)
		 (cons 'number (* num k))))
	      (else
	       (let ((end (do ((end start (1+ end)))
			      ((or (>= end input-length)
				   (delimiter? (string-ref input-line end)))
			       end))))
		 (DEBUG 100 "END " end)
		 (set! input-index end)
		 (cond
		  ((char=? char #\:)
		   (cons 'tag (substring input-line (1+ start) end)))
		  (else
		   (cons 'identifier (substring input-line start end))))))))))
	#f)))

(define (end-of-line?)
  (do ((i input-index (1+ i)))
      ((or (>= i input-length)
	   (not (char-whitespace? (string-ref input-line i))))
       (>= i input-length))))

(define (read-input-line port)
  (set! input-line (read-line port))
  (if (not (eof-object? input-line))
      (begin
	(set! input-line-number (1+ input-line-number))
	(set! input-length (string-length input-line))
	(set! input-index 0)))
  input-line)

(define (next-token-from-port port)
  (let ((tok (or (next-token)
		 (begin
		   (DEBUG 100 "2nd")
		   (set! input-line (read-line port))
		   (if (not (eof-object? input-line))
		       (begin
                         (set! input-line-number (1+ input-line-number))
			 (set! input-length (string-length input-line))
			 (set! input-index 0)
			 (next-token))
		       input-line)))))
    (cond
     ((or (not tok) (eof-object? tok))
      tok)
     ((and (eq? (car tok) 'identifier)
	   (string=? (cdr tok) "text:")
	   (end-of-line?))
      (let ((text "")
	    (string-start input-line-number))
	(do ((line (read-line port) (read-line port)))
	    ((or (and
		  (eof-object? line)
		  (lex-error
		   "Unexpected end of file in multiline string started on line "
		   string-start)
		  (throw 'end-of-file))
		 (let ((len (string-length line)))
		   (and (> len 0)
			(char=? (string-ref line 0) #\.)
			(do ((i 1 (1+ i)))
			    ((or (>= i len)
				 (not
				  (char-whitespace?
				   (string-ref line i))))
			     (>= i len))))))
	     #f)
          (set! input-line-number (1+ input-line-number))
	  (if (and (not (string-null? line))
                   (char=? (string-ref line 0) #\.)
		   (char=? (string-ref line 1) #\.))
	      (set! line (make-shared-substring line 1)))
	  (set! text (string-append text "\n" line)))
	(set! input-length 0)
	(set! input-index 0)
	(cons 'string text)))
     ((eq? (car tok) 'bracket-comment)
      (let ((comment-start input-line-number))
	(set! input-length (- input-length input-index))
	   (if (> input-length 0)
	       (begin
		 (set! input-line
		       (substring input-line input-index input-length))
		 (set! input-index 0))
	       (read-input-line port))
	   (do ()
	       ((> input-index 0) #f)
	     (cond
	      ((eof-object? input-line)
	       (lex-error
		"Unexpected end of file in comment started on line "
		comment-start)
	       (throw 'end-of-file))
	      (else
	       (let ((t (string-index input-line #\*)))
		 (if (and t
			  (< (1+ t) input-length)
			  (char=? (string-ref input-line (1+ t)) #\/))
		     (set! input-index (+ t 2))
		     (read-input-line port))))))))
     (else
	tok))))

(define (delimiter token c)
  (and (pair? token)
       (eq? (car token) 'delimiter)
       (char=? (cdr token) c)))

(define (identifier token c)
  (and (eq? (car token) 'identifier)
       (string=? (cdr token) c)))

(define (putback-token)
  (set! putback-list (append (list current-token)
			     putback-list)))

(define (read-token)
  (cond
   ((not (null? putback-list))
    (set! current-token (car putback-list))
    (set! putback-list (cdr putback-list)))
   (else
    (set! current-token (do ((token (next-token-from-port input-port)
				    (next-token-from-port input-port)))
			    (token token)))))
  current-token)

(define (require-semicolon . read)
  (if (null? read)
      (read-token))
  (if (or (eof-object? current-token)
	  (not (delimiter current-token #\;)))
      (syntax-error "Missing ;")
      current-token))

(define (require-tag . read)
  (if (null? read)
      (read-token))
  (cond
   ((eof-object? current-token)
    (syntax-error "Expected tag but found " current-token))
   ((not (eq? (car current-token) 'tag))
    (syntax-error "Expected tag but found " (car current-token))))
  current-token)

(define (require-string . read)
  (if (null? read)
      (read-token))
  (cond
   ((eof-object? current-token)
    (syntax-error "Expected string but found " current-token))
   ((not (eq? (car current-token) 'string))
    (syntax-error "Expected string but found " (car current-token))))
  current-token)

(define (require-number . read)
  (if (null? read)
      (read-token))
  (cond
   ((eof-object? current-token)
    (syntax-error "Expected number but found " current-token))
   ((not (eq? (car current-token) 'number))
    (syntax-error "Expected number but found " (car current-token))))
  current-token)

(define (require-string-list . read)
  (if (null? read)
      (read-token))
  (cond
   ((eof-object? current-token)
    (syntax-error "Expected string-list but found " current-token))
   ((eq? (car current-token) 'string)
    (list (cdr current-token)))
   ((not (eq? (car current-token) 'delimiter))
    (syntax-error "Expected string-list but found " (car current-token)))
   ((char=? (cdr current-token) #\[)
    (do ((slist '())
	 (token (read-token) (read-token)))
	((if (not (eq? (car token) 'string))
	     (begin
	       (syntax-error "Expected string but found " (car token))
	       #t)
	     (begin
	       (set! slist (append slist (list (cdr token))))
	       (read-token)
	       (cond
		((eof-object? current-token)
		 (syntax-error "Unexpected end of file in string list")
		 #t) ;; break;
		((eq? (car current-token) 'delimiter)
		 (cond
		  ((char=? (cdr current-token) #\,) #f) ;; continue
		  ((char=? (cdr current-token) #\]) #t) ;; break
		  (else
		   (lex-error "Expected ',' or ']' but found "
			      (cdr current-token))
		   #t)))
		(else
		 (lex-error "Expected delimiter but found "
			    (car current-token))
		 #t))))
	 (cons 'string-list slist))))
   (else
    (syntax-error "Expected '[' but found " (car current-token)))))
  
(define (require-identifier . read)
  (if (null? read)
      (read-token))
  (cond
   ((eof-object? current-token)
    (syntax-error "1. Expected identifier but found " current-token))
   ((not (eq? (car current-token) 'identifier))
    (syntax-error "2. Expected identifier but found " (car current-token))))
  current-token)

;;;;

(define (sieve-syntax-table-lookup table name)
    (call-with-current-continuation
        (lambda (exit)
	  (for-each (lambda (x)
		      (if (string=? (car x) name)
			  (exit (cdr x))))
		    table)
	  #f)))

;;;; Comparators

;;; Each entry is (list COMP-NAME COMP-FUNCTION)
(define sieve-comparator-table '())

(define (sieve-find-comparator name)
  (sieve-syntax-table-lookup sieve-comparator-table name))

(define (sieve-register-comparator name function)
  (if (not (or (eq? function #f)
	       (eq? function #t)
	       (procedure? function)))
      (lex-error "sieve-register-comparator: bad type for function"
	          function))
  (set! sieve-comparator-table
	(append sieve-comparator-table (list
					(cons name function)))))
      

;;;; Command parsers

;;; Each entry is: (list NAME FUNCTION OPT-ARG-LIST REQ-ARG-LIST)
;;; OPT-ARG-LIST is a list of optional arguments (aka keywords, tags).
;;; It consists of conses: (cons TAG-NAME FLAG) where FLAG is #t
;;; if the tag requires an argument (e.g. :comparator <comp-name>),
;;; and is #f otherwise.
;;; REQ-ARG-LIST is a list of required (positional) arguments. It
;;; is a list of argument types.
(define sieve-test-table '())

(define (sieve-find-test name)
  (sieve-syntax-table-lookup sieve-test-table name))

(define (sieve-register-test name function opt-arg-list req-arg-list)
  (cond
   ((not (list? opt-arg-list))
    (lex-error "sieve-register-test: opt-arg-list must be a list"))
   ((not (list? req-arg-list))
    (lex-error "sieve-register-test: req-arg-list must be a list"))
   ((not (or (eq? function #f)
	     (eq? function #t)
	     (procedure? function)))
    (lex-error "sieve-register-test: bad type for function" function))
   (else
    (set! sieve-test-table
	  (append sieve-test-table
		  (list
		   (list name function opt-arg-list req-arg-list)))))))
   

;;; arguments = *argument [test / test-list]
;;; argument = string-list / number / tag

(define (sieve-parse-arguments tag-gram)
  (do ((opt-list '())   ;; List of optional arguments (tags)
       (arg-list '())   ;; List of positional arguments
       (last-tag #f)    ;; Description of the last tag from tag-gram
       (state 'opt)     ;; 'opt when scanning optional arguments,
                        ;; 'arg when scanning positional arguments
       (token (read-token) (read-token))) ;; Obtain next token
      ((cond
	((eof-object? token)
	 (syntax-error "Expected argument but found " token))
	((eq? (car token) 'tag)
	 (if (not (eq? state 'opt))
	     (syntax-error "Misplaced tag: :" (cdr token)))
	 (set! last-tag (assoc (cdr token) tag-gram))
	 (if (not last-tag)
	     (syntax-error
	      "Tag :" (cdr token) " is not allowed in this context"))
	 (set! opt-list (append opt-list (list token)))
	 #f)
	((or (eq? (car token) 'number)
	     (eq? (car token) 'string))
	 (cond
	  ((and (eq? state 'opt) (pair? last-tag))
	   (cond
	    ((cdr last-tag)
	     (if (not (eq? (cdr last-tag) (car token)))
		 (syntax-error
		  "Tag :" (car last-tag) " takes " (cdr last-tag) " argument"))
	     (cond
	      ((string=? (car last-tag) "comparator")
	       (let ((comp (sieve-find-comparator (cdr token))))
		 (if (not comp)
		     (syntax-error "Undefined comparator: " (cdr token)))
		 (set-cdr! token comp))))
	     (set! opt-list (append opt-list (list token)))
	     (set! last-tag #f))
	    (else
	     (set! state 'arg)
	     (set! arg-list (append arg-list (list token))))))
	  (else
	   (set! arg-list (append arg-list (list token)))))
	 #f)
	((delimiter token #\[) ;;FIXME: tags are not allowed to take
	                       ;;string-list arguments. 
	 (set! state 'arg)
	 (putback-token)
	 (set! arg-list (append arg-list
				(list (require-string-list))))
	 #f)
	(else
	 #t)) 
       (cons opt-list arg-list))))

;;; test-list = "(" test *("," test) ")"
(define (sieve-parse-test-list)
  (do ((token (sieve-parse-test) (sieve-parse-test)))
      ((cond
	((delimiter token #\))
	 #t) ;; break;
	((delimiter token #\,)
	 #f) ;; continue
	((eof-object? token)
	 (syntax-error "Unexpected end of file in test-list")
	 #t) ;; break
	(else
	 (syntax-error "Expected ',' or ')' but found " (cdr token))
	 #t)) ;; break
       (read-token))))
	
;;; test = identifier arguments
(define (sieve-parse-test)
  (let ((ident (require-identifier)))
    (sieve-exp-begin)
    (cond
     ((string=? (cdr ident) "not")
      (sieve-exp-append 'not)
      (sieve-parse-test))
     (else
      (read-token)
      (cond
       ((eof-object? current-token)
	(syntax-error "Unexpected end of file in conditional"))
       ((delimiter current-token #\()
	(cond
	 ((string=? (cdr ident) "allof")
	  (sieve-exp-append 'and))
	 ((string=? (cdr ident) "anyof")
	  (sieve-exp-append 'or))
	 (else
	  (syntax-error "Unexpected '('")))
	(sieve-parse-test-list))
       ((delimiter current-token #\)))
       (else
	(let ((test (sieve-find-test (cdr ident))))
	  (if (not test)
	      (syntax-error "Unknown test name: " (cdr ident)))
	  (putback-token)
	  (let ((arg-list (sieve-parse-arguments (car (cdr test)))))
	    (sieve-exp-append (car test))
	    ;; Process positional arguments
	    (do ((expect (car (cdr (cdr test))) (cdr expect))
		 (argl (cdr arg-list) (cdr argl))
		 (n 1 (1+ n)))
		((cond
		  ((null? expect)
		   (if (not (null? argl))
		       (syntax-error
			"Too many positional arguments for " ident
			" (bailed out at " (car argl) ")"))
		   #t)
		  ((null? argl)
		   (if (not (null? expect))
		       (syntax-error
			"Too few positional arguments for " ident))
		   #t)
		  (else #f)) #f)
		(let ((expect-type (car expect))
		      (arg (car argl)))
		  (cond
		   ((and (eq? expect-type 'string-list)
			 (eq? (car arg) 'string))
		    ;; Coerce string to string-list
		    (sieve-exp-append (list 'list (cdr arg))))
		   ((eq? expect-type (car arg))
		    (if (eq? expect-type 'string-list)
			(sieve-exp-append (append (list 'list) (cdr arg)))
			(sieve-exp-append (cdr arg))))
		   (else
		    (syntax-error
		     "Type mismatch in argument " n " to " (cdr ident)
		     "; expected " expect-type ", but got " (car arg))))))
	    ;; Process optional arguments (tags).
	    ;; They have already been tested 
	    (for-each
	     (lambda (tag)
	       (sieve-exp-append (if (eq? (car tag) 'tag)
				     (string->symbol
				      (string-append "#:" (cdr tag)))
				   (cdr tag))))
	     (car arg-list))
	    ))))))
    (sieve-exp-finish))
  current-token)

(define (sieve-parse-block . read)
  (if (not (null? read))
      (read-token))
  (if (delimiter current-token #\{)
      (begin
	(set! nesting-level (1+ nesting-level))
	(do ((token (read-token) (read-token)))
	    ((cond
	      ((eof-object? token)
	       (syntax-error "Unexpected end of file in block")
	       #t)
	      ((delimiter token #\})
	       #t)
	      (else
	       (putback-token)
	       (sieve-parse-command)
	       #f))) #f)
	(set! nesting-level (1- nesting-level)))
      (require-semicolon 'dont-read)))

;;; if <test1: test> <block1: block>
(define (sieve-parse-if-internal)
  (DEBUG 10 "sieve-parse-if-internal" current-token)
  (sieve-exp-begin)
  
  (sieve-parse-test)
  
  (sieve-parse-block)
  (sieve-exp-finish)
  
  (read-token)
  (cond
   ((eof-object? current-token) )
   ((identifier current-token "elsif")
    (sieve-parse-if-internal))
   ((identifier current-token "else")
    (sieve-exp-begin 'else)
    (sieve-parse-block 'read)
    (sieve-exp-finish))
   (else
    (putback-token))))

(define (sieve-parse-if)
  (sieve-exp-begin 'cond)
  (sieve-parse-if-internal)
  (sieve-exp-finish))

(define (sieve-parse-else)
  (syntax-error "else without if"))

(define (sieve-parse-elsif)
  (syntax-error "elsif without if"))

;;; require <capabilities: string-list>
(define (sieve-parse-require)
  (for-each
   (lambda (capability)
     (if (not
	  (cond
	   ((and
	     (>= (string-length capability) 5)
	     (string=? (make-shared-substring capability 0 5) "test-"))
	    (sieve-find-test (make-shared-substring capability 5)))
	   ((and
	     (>= (string-length capability) 11)
	     (string=? (make-shared-substring capability 0 11) "comparator-"))
	    (sieve-find-comparator (make-shared-substring capability 11)))
	   (else
	    (sieve-find-action capability))))
	 (let ((name (string-append sieve-libdir
				    "/" capability ".scm")))
	   (set! sieve-load-files (append sieve-load-files (list name)))
	   (catch #t
		  (lambda ()
		    (load name))
		  (lambda args
		    (lex-error "Can't load required capability "
			       capability)
		    args)))))
   (cdr (require-string-list)))
  (require-semicolon))

;;; stop
(define (sieve-parse-stop)
  (sieve-exp-begin sieve-stop)
  (sieve-exp-finish)
  (require-semicolon))

;;; Actions

;;; Each entry is: (list ACTION-NAME FUNCTION ARG-LIST)
;;; ARG-LIST is a list of argument types
(define sieve-action-table '())

(define (sieve-find-action name)
  (sieve-syntax-table-lookup sieve-action-table name))

(define (sieve-register-action name proc . arg-list)
  (if (not (sieve-find-action name))
      (set! sieve-action-table (append sieve-action-table
				       (list
					(append
					 (list name proc) arg-list))))))

(define (sieve-parse-action)
  (let* ((name (cdr current-token))
	 (descr (sieve-find-action name)))
    (cond
     (descr
      (if (car descr)
	  (sieve-exp-begin (car descr)))
      (do ((arg (cdr descr) (cdr arg)))
	  ((null? arg) #f)
	(read-token)
	(case (car arg)
	  ((string)
	   (require-string 'dont-read))
	  ((string-list)
	   (require-string-list 'dont-read))
	  ((number)
	   (require-number 'dont-read))
	  ((tag)
	   (require-tag 'dont-read))
	  (else
	   (syntax-error "Malformed table entry for " name " :" (car arg))))
	(if (car descr)
	    (sieve-exp-append (cdr current-token))))
      (require-semicolon)
      (if (car descr)
	  (sieve-exp-finish)))
     (else
      (syntax-error "Unknown identifier: " name)))))

;;;; Parser

(define (sieve-parse-command)
  (DEBUG 10 "sieve-parse-command" current-token)
  (catch 'syntax-error
   (lambda ()
     (read-token)
     (cond
      ((or (not current-token)
	   (eof-object? current-token))) ;; Skip comments and #<eof>
      ((eq? (car current-token) 'identifier)
       ;; Process a command
       (let ((elt (assoc (string->symbol (cdr current-token))
			 (list
			  (cons 'if sieve-parse-if)
			  (cons 'elsif sieve-parse-elsif)
			  (cons 'else sieve-parse-else)
		          (cons 'require sieve-parse-require)
		          (cons 'stop sieve-parse-stop)))))
	 (if (not elt)
	     (sieve-parse-action)
	     (apply (cdr elt) '()))))
      (else
       (syntax-error "3. Expected identifier but found "
		     (cdr current-token)))))
   (lambda args
     ;; Error recovery: skip until we find a ';' or '}'.
     (if (and (= input-line-number recovery-line-number)
	      (= input-index recovery-index))
	 (begin
	   (lex-error "ERROR RECOVERY: Skipping to end of file")
	   (throw 'end-of-file)))
     (set! recovery-line-number input-line-number)
     (set! recovery-index input-index)

     (if (or (delimiter current-token #\})
	     (delimiter current-token #\;))
	 (read-token))
     (DEBUG 50 "ERROR RECOVERY at " current-token)
     (do ((token current-token (read-token)))
	 ((cond
	   ((eof-object? token)
	    (throw 'end-of-file))
	   ((delimiter token #\;)
	    #t)
	   ((delimiter token #\})
	    (cond
	     ((> nesting-level 0)
	      (putback-token)
	      #t)
	     (else
	      #f)))
	   ((delimiter token #\{)
	    (sieve-skip-block)
	    (putback-token)
	    #f)
	   (else
	    #f)) #f))
     (DEBUG 50 "ERROR RECOVERY FINISHED AT " current-token)))
  current-token)

(define (sieve-skip-block)
  (do ((token (read-token) (read-token)))
      ((cond
	((eof-object? token)
	 (throw 'end-of-file))
	((delimiter token #\{)
	 (sieve-skip-block)
	 #f)
	((delimiter token #\})
	 #t)
	(else
	 #f)) #f)))

(define (sieve-parse-from-port port)
  (set! input-port port)
  (do ((token (sieve-parse-command) (sieve-parse-command)))
      ((eof-object? token) #f)) )

(define (sieve-parse filename)
  (if (file-exists? filename)
      (catch 'end-of-file 
        (lambda ()
	  (set! error-count 0)
	  (set! current-token #f)
	  (set! input-file filename)
	  (set! input-line-number 0)
	  (set! putback-list '())
	  (call-with-input-file filename sieve-parse-from-port))
	(lambda args args))))

;;;; Code generator

(define sieve-exp '())  ;; Expression currently being built
(define sieve-exp-stack '())
(define sieve-code-list '())  ;; Resulting scheme code

(define (sieve-exp-begin . exp)
  (set! sieve-exp-stack (append (list sieve-exp) sieve-exp-stack))
  (set! sieve-exp exp))

(define (sieve-exp-append exp)
  (set! sieve-exp (append sieve-exp (list exp))))

(define (sieve-exp-finish)
  (set! sieve-exp (append (car sieve-exp-stack) (list sieve-exp)))
  (set! sieve-exp-stack (cdr sieve-exp-stack)))

(define (sieve-code-begin)
  (set! sieve-exp-stack '())
  (set! sieve-exp '()))

(define (sieve-code-finish)
  (if (not (null? sieve-exp))
      (set! sieve-code-list (append sieve-code-list sieve-exp (list #t)))))

;;; Print the program

(define (sieve-code-print-list exp . opt-port)
  (let ((port (if (null? opt-port)
		  (current-output-port)
		(car opt-port))))
    (display "(" port)
    (for-each
     (lambda (x)
       (cond
	((procedure? x)
	 (display (procedure-name x) port))
	((list? x)
	 (sieve-code-print-list x port))
	(else
	 (write x port)))
       (display " " port))
     exp)
    (display ")" port)))

;;; Save the program

(define (sieve-save-program outfile)
  (call-with-output-file
   outfile
   (lambda (port)
     (display "#! /home/gray/mailutils/guimb/guimb --source\n!#\n" port)
     (display (string-append
	       ";;;; A Guile mailbox parser made from " filename) port)
     (newline port) 
     (display ";;;; by sieve.scm, GNU mailutils (0.0.9)" port)
     (newline port)
     
     (display "(define sieve-parser #f)" port)
     (newline port)
     (display (string-append
	       "(define sieve-source \"" filename "\")") port)
     (newline port)
     
     (display (string-append
	       "(load \"" sieve-libdir "/sieve-core.scm\")") port)
     (newline port)
     (for-each
      (lambda (file)
	(display (string-append
		  "(load \"" file "\")") port)
	(newline port))
      sieve-load-files)
     (sieve-code-print-list sieve-code-list port)
     (newline port)
     (display "(sieve-main)" port))))
     
;;;; Main

(define filename #f)
(define output #f)

(define (sieve-usage)
  (display "usage: sieve.scm --file FILENAME [--output FILENAME]")
  (newline)
  (exit 0))

(define (sieve-expand-filename name)
  (let ((index (string-index name #\%)))
    (if (or (not index) (= index (string-length name)))
	name
      (let ((ch (string-ref name (1+ index))))
	(string-append
	 (make-shared-substring name 0 index)
	 (case ch
	   ((#\%)
	    "%")
	   ((#\u)
	    user-name)
	   ((#\h)
	    (passwd:dir (getpwnam user-name)))
	   (else
	    (make-shared-substring name index 2)))
	 (sieve-expand-filename
	  (make-shared-substring name (+ index 2))))))))

;;; Parse command line 

(use-modules (ice-9 getopt-long))
(define grammar
  `((file   (single-char #\f)
            (value #t))
    (output (single-char #\o)
	    (value #t))
    (help   (single-char #\h))))

(define program-name (car (command-line)))

(for-each
 (lambda (x)
   (cond
    ((pair? x)
     (case (car x)
       ((file)
	(set! filename (cdr x)))
       ((output)
	(set! output (cdr x)))
       ((help)
	(sieve-usage))))))
 (getopt-long (command-line) grammar))

(if (not filename)
    (begin
     (display program-name)
     (display ": missing input filename")
     (newline)
     (sieve-usage)))

(define guimb? (string->obarray-symbol #f "mu-mailbox-open" #t))

(if (and guimb? (string? user-name))
    (begin
     (set! filename (sieve-expand-filename filename))
     (if (not (file-exists? filename))
	 (exit 0)))
  (if (not (file-exists? filename))
      (begin
       (display (string-append
		 program-name
		 ": Input file " filename " does not exist."))
       (newline)
       (exit 0))))
	
(if (not sieve-libdir)
    (set! sieve-libdir
	    (let ((myname (car (command-line))))
	      (if (not (char=? (string-ref myname 0) #\/))
		  (set! myname (string-append (getcwd) "/" myname)))
	      (let ((slash (string-rindex myname #\/)))
		(substring myname 0 slash)))))

(load (string-append sieve-libdir "/sieve-core.scm"))

(sieve-exp-append 'define)
(sieve-exp-append (list 'sieve-process-message))
(sieve-parse filename)
(sieve-code-finish)

(cond
 ((> error-count 0)
  (display error-count)
  (display " errors.")
  (newline)
  (exit 1))
 (output
  (sieve-save-program output))
 ((not guimb?)
  (display program-name)
  (display ": Either use guimb to compile and execute the script")
  (newline)
  (display "or use --output option to save the Scheme program.")
  (newline)
  (exit 1))
 (else
  (let ((temp-file (tmpnam))
	(saved-umask (umask #o077)))
    (sieve-save-program temp-file)
    (load temp-file)
    (delete-file temp-file)	
    (umask saved-umask))))





