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

;;;; This module provides core functionality for the sieve scripts.

;;; Comparators
(cond
 (sieve-parser
  (sieve-register-comparator "i;octet" string=?)
  (sieve-register-comparator "i;ascii-casemap" string-ci=?)))

;;; Stop statement

(define (sieve-stop)
  (exit))

;;; Basic five actions:

;;; reject

(define sieve-option-quote #f)
(define sieve-indent-prefix "\t")

(define (action-reject reason)
  (let* ((out-msg (mu-message-create))
	 (outbody (mu-message-get-body out-msg))
	 (inbody (mu-message-get-body sieve-current-message)))
    (mu-message-set-header out-msg "To"
			   (mu-message-get-header in-msg "From"))
    (mu-message-set-header out-msg "Cc"
			   (mu-message-get-header in-msg "Cc"))
    (mu-message-set-header out-msg "Subject"
			   (string-append
			    "Re: "
			    (mu-message-get-header in-msg "Subject")))
    (mu-body-write outbody reason)

    (cond
     (sieve-option-quote
      (mu-body-write outbody "\n\nOriginal message:\n")
      (do ((hdr (mu-message-get-header-fields sieve-current-message)
		(cdr hdr)))
	  ((null? hdr) #f)
	(let ((s (car hdr)))
	  (mu-body-write outbody (string-append
				  sieve-indent-prefix
				  (car s) ": " (cdr s) "\n"))))
      (mu-body-write outbody (string-append indent-prefix "\n"))
      (do ((line (mu-body-read-line inbody) (mu-body-read-line inbody)))
	  ((eof-object? line) #f)
	(mu-body-write outbody (string-append sieve-indent-prefix line)))))

    (mu-message-send out-msg)))

;;; fileinto

(define (action-fileinto filename)
  (let ((outbox (mu-mailbox-open filename "cw")))
    (cond
     (outbox
      (mu-mailbox-append-message outbox sieve-current-message)
      (mu-mailbox-close outbox)
      (mu-message-delete sieve-current-message)))))

;;; redirect is defined in redirect.scm

;;; keep -- does nothing worth mentioning :^)

;;; discard

(define (action-discard)
  (mu-message-delete sieve-current-message))

;;; Register standard actions
(cond
 (sieve-parser
  (sieve-register-action "keep" #f)
  (sieve-register-action "discard" action-discard)
  (sieve-register-action "reject" action-reject 'string)
  (sieve-register-action "fileinto" action-fileinto 'string)))


;;; Some utilities.

(define (find-comp opt-args)
  (cond
   ((member #:comparator opt-args) =>
    (lambda (x)
      (car (cdr x))))
   (else
    string-ci=?)))

(define (find-match opt-args)
  (cond
   ((member #:is opt-args)
    #:is)
   ((member #:contains opt-args)
    #:contains)
   ((member #:matches opt-args)
    #:matches)
   (else
    #:is)))

(define (sieve-str-str str key comp)
  (let* ((char (string-ref key 0))
	 (str-len (string-length str))
	 (key-len (string-length key))
	 (limit (- str-len key-len)))
    (if (< limit 0)
	#f
	(call-with-current-continuation
	 (lambda (xx)
	   (do ((index 0 (1+ index)))
	       ((cond
		 ((> index limit)
		  #t)
		 ;; FIXME: This is very inefficient, but I have to use this
		 ;; provided (string-index str (string-ref key 0)) may not
		 ;; work...
		 ((comp (make-shared-substring str index (+ index key-len))
			key)
		  (xx #t))
		 (else
		  #f)) #f))
	   #f)))))

;;; Convert sieve-style regexps to POSIX:

(define (sieve-regexp-to-posix regexp)
  (let ((length (string-length regexp)))
    (do ((cl '())
	 (escape #f)
	 (i 0 (1+ i)))
	((= i length) (list->string (reverse cl)))
      (let ((ch (string-ref regexp i)))
	(cond
	 (escape
	  (set! cl (append (list ch) cl))
	  (set! escape #f))
	 ((char=? ch #\\)
	  (set! escape #t))
	 ((char=? ch #\?)
	  (set! cl (append (list #\.) cl)))
	 ((char=? ch #\*)
	  (set! cl (append (list #\* #\.) cl)))
	 (else
	  (set! cl (append (list ch) cl))))))))
		  
;;;; Standard tests:


(define (test-address header-list key-list . opt-args)
  (let ((comp (find-comp opt-args))
	(match (find-match opt-args))
	(part (cond
	       ((member #:localpart opt-args)
		#:localpart)
	       ((member #:domain opt-args)
		#:domain)
	       (else
		#:all))))
    (call-with-current-continuation
     (lambda (exit)
       (for-each
	(lambda (key)
	  (let ((rx (if (eq? match #:matches)
			(make-regexp (sieve-regexp-to-posix key)
				     (if (eq? comp string-ci=?)
					 regexp/icase
					 '()))
			#f)))
	    (for-each
	     (lambda (h)
	       (let ((hdr (mu-message-get-header sieve-current-message h)))
		 (if hdr
		     (let ((naddr (mu-address-get-count hdr)))
		       (do ((n 1 (1+ n)))
			   ((> n naddr) #f)
			 (let ((addr (case part
				       ((#:all)
					(mu-address-get-email hdr n))
				       ((#:localpart)
					(mu-address-get-local hdr n))
				       ((#:domain)
					(mu-address-get-domain hdr n)))))
			   (if addr
			       (case match
				 ((#:is)
				  (if (comp addr key)
				      (exit #t)))
				 ((#:contains)
				  (if (sieve-str-str addr key comp)
				      (exit #t)))
				 ((#:matches)
				  (if (regexp-exec rx addr)
				      (exit #t))))
			       (runtime-error LOG_NOTICE
				"Can't get address parts for message "
				sieve-current-message))))))))
	     header-list)))
	key-list)
       #f))))

(define (test-size key-size . comp)
  (let ((size (mu-message-get-size sieve-current-message)))
    (cond
     ((null? comp)       ;; An extension.
      (= size key-size)) 
     ((eq? (car comp) #:over)
      (> size key-size))
     ((eq? (car comp) #:under)
      (< size key-size))
     (else
      (runtime-error LOG_CRIT "test-size: unknown comparator " comp)))))

(define (test-envelope part key-list . opt-list)
  #f)

(define (test-exists header-list)
  (call-with-current-continuation
   (lambda (exit)
     (for-each (lambda (hdr)
		 (if (not (mu-message-get-header sieve-current-message hdr))
		     (exit #f)))
	       header-list)
     #t)))

(define (test-header header-list key-list . opt-args)
  (let ((comp (find-comp opt-args))
	(match (find-match opt-args)))
    (call-with-current-continuation
     (lambda (exit)
       (for-each
	(lambda (key)
	  (let ((rx (if (eq? match #:matches)
			(make-regexp (sieve-regexp-to-posix key)
				     (if (eq? comp string-ci=?)
					 regexp/icase
					 '()))
			#f)))
	    (for-each
	     (lambda (h)
	       (let ((hdr (mu-message-get-header sieve-current-message h)))
		 (if hdr
		     (case match
		       ((#:is)
			(if (comp hdr key)
			    (exit #t)))
		       ((#:contains)
			(if (sieve-str-str hdr key comp)
			    (exit #t)))
		       ((#:matches)
			(if (regexp-exec rx hdr)
			    (exit #t)))))))
	     header-list)))
	key-list)
       #f))))

;;; Register tests:
(define address-part (list (cons "localpart" #f)
               		   (cons "domain" #f)
			   (cons "all" #f)))
(define match-type   (list (cons "is" #f)
			   (cons "contains" #f)
			   (cons "matches" #f)))
(define size-comp    (list (cons "under" #f)
			   (cons "over" #f)))
(define comparator   (list (cons "comparator" 'string)))

(cond
 (sieve-parser
  (sieve-register-test "address"
		       test-address
		       (append address-part comparator match-type)
                       (list 'string-list 'string-list))
  (sieve-register-test "size"
		       test-size
		       size-comp
                       (list 'number))
;  (sieve-register-test "envelope"
;		       test-envelope
;		       (append comparator address-part match-type)
;                       (list 'string-list 'string-list))
  (sieve-register-test "exists"
		       test-exists
		       '()
                       (list 'string-list))
  (sieve-register-test "header"
		       test-header
		       (append comparator match-type)
		       (list 'string-list 'string-list))
  (sieve-register-test "false" #f '() '())
  (sieve-register-test "true"  #t '() '())))

;;; runtime-error
(define (runtime-error level . text)
  (display (string-append "RUNTIME ERROR in " sieve-source ": "))
  (for-each
   (lambda (s)
     (display s))
   text)
  (newline))

;;; Sieve-main
(define sieve-current-message #f)
(define (sieve-main)
  (let ((count (mu-mailbox-messages-count current-mailbox)))
    (do ((n 1 (1+ n)))
	((> n count) #f)
	(set! sieve-current-message
	      (mu-mailbox-get-message current-mailbox n))
	(sieve-process-message))))
