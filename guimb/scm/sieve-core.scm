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

(define sieve-my-email "")

;;; List of open mailboxes.
;;; Each entry is: (list MAILBOX-NAME OPEN-FLAGS MBOX)
(define sieve-mailbox-list '())

;;; Cached mailbox open: Lookup in the list first, if not found,
;;; call mu-mailbox-open and append to the list.
;;; NOTE: second element of each slot (OPEN-FLAGS) is not currently
;;; used, sinse all the mailboxes are open with "cw".
(define (sieve-mailbox-open name flags)
  (let ((slot (assoc name sieve-mailbox-list)))
    (if slot
	(list-ref slot 2)
      (let ((mbox (mu-mailbox-open name flags)))
	(if mbox
	    (set! sieve-mailbox-list (append
				      sieve-mailbox-list
				      (list
				       (list name flags mbox)))))
	mbox))))

;;; Close all open mailboxes.
(define (sieve-close-mailboxes)
  (for-each
   (lambda (slot)
     (cond
      ((list-ref slot 2)
       => (lambda (mbox)
	    (mu-mailbox-close mbox)))))
   sieve-mailbox-list)
  (set! sieve-mailbox-list '()))

(use-modules (ice-9 popen))

(define PATH-SENDMAIL "/usr/lib/sendmail")

;;; Bounce a message.
;;; Current mailutils API does not provide a way to send a message
;;; specifying its recipients (like "sendmail -t foo@bar.org" does),
;;; hence the need for this function.
(define (sieve-message-bounce message addr-list)
  (catch #t
	 (lambda ()
	   (let ((port (open-output-pipe
			(apply string-append
			       (append
				(list
				 PATH-SENDMAIL
				 " -oi -t ")
				(map
				 (lambda (addr)
				   (string-append addr " "))
				 addr-list))))))
	     ;; Write headers
	     (for-each
	      (lambda (hdr)
		(display (string-append (car hdr) ": " (cdr hdr)) port)
		(newline port))
	      (mu-message-get-header-fields message))
	     (newline port)
	     ;; Write body
	     (let ((body (mu-message-get-body message)))
	       (do ((line (mu-body-read-line body) (mu-body-read-line body)))
		   ((eof-object? line) #f)
		   (display line port)))
	     (close-pipe port)))
	 (lambda args
	   (runtime-error LOG_ERR "redirect: Can't send message")
	   (write args))))

;;; Comparators
(cond
 (sieve-parser
  (sieve-register-comparator "i;octet" string=?)
  (sieve-register-comparator "i;ascii-casemap" string-ci=?)))

;;; Stop statement

(define (sieve-stop)
  (throw 'sieve-stop))

;;; Basic five actions:

;;; reject is defined in reject.scm

;;; fileinto

(define (action-fileinto filename)
  (let ((outbox (sieve-mailbox-open filename "cw")))
    (cond
     (outbox
      (mu-mailbox-append-message outbox sieve-current-message)
      (mu-message-delete sieve-current-message)))))

;;; redirect is defined in redirect.scm

;;; keep -- does nothing worth mentioning :^)

(define (action-keep)
  #f)

;;; discard

(define (action-discard)
  (mu-message-delete sieve-current-message))

;;; Register standard actions
(cond
 (sieve-parser
  (sieve-register-action "keep" action-keep)
  (sieve-register-action "discard" action-discard)
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
  (if (string-null? key)
      ;; rfc3028:
      ;;   If a header listed in the header-names argument exists, it contains
      ;;   the null key ("").  However, if the named header is not present, it
      ;;   does not contain the null key.
      ;; This function gets called only if the header was present. So:
      #t
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
	   #f))))))

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
	 ((member ch (list #\. #\$ #\^ #\[ #\]))
	  (set! cl (append (list ch #\\) cl)))
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
	  (let ((header-fields (mu-message-get-header-fields
				sieve-current-message
				header-list))
		(rx (if (eq? match #:matches)
			(make-regexp (sieve-regexp-to-posix key)
				     (if (eq? comp string-ci=?)
					 regexp/icase
					 '()))
			#f)))
	    (for-each
	     (lambda (h)
	       (let ((hdr (cdr h)))
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
	     header-fields)))
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

(define (test-envelope part-list key-list . opt-args)
  (let ((comp (find-comp opt-args))
	(match (find-match opt-args)))
    (call-with-current-continuation
     (lambda (exit)
       (for-each
	(lambda (part)
	  (cond
	   ((string-ci=? part "From")
	    (let ((sender (mu-message-get-sender sieve-current-message)))
	      (for-each
	       (lambda (key)
		 (if (comp key sender)
		     (exit #t)))
	       key-list)))
	   (else
	    ;; Should we issue a warning?
	    ;;(runtime-error LOG_ERR "Envelope part " part " not supported")
	    #f)))
	part-list)
       #f))))

(define (test-exists header-list)
  (call-with-current-continuation
   (lambda (exit)
     (for-each (lambda (hdr)
		 (let ((val (mu-message-get-header sieve-current-message hdr)))
                   (if (or (not val) (= (string-length val) 0))
		     (exit #f))))
	       header-list)
     #t)))

(define (test-header header-list key-list . opt-args)
  (let ((comp (find-comp opt-args))
	(match (find-match opt-args)))
    (call-with-current-continuation
     (lambda (exit)
       (for-each
	(lambda (key)
	  (let ((header-fields (mu-message-get-header-fields
				sieve-current-message
				header-list))
		(rx (if (eq? match #:matches)
			(make-regexp (sieve-regexp-to-posix key)
				     (if (eq? comp string-ci=?)
					 regexp/icase
					 '()))
			#f)))
	    (for-each
	     (lambda (h)
	       (let ((hdr (cdr h)))
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
	     header-fields)))
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
  (sieve-register-test "envelope"
		       test-envelope
		       (append comparator address-part match-type)
                       (list 'string-list 'string-list))
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
  (if (not sieve-my-email)
      (set! sieve-my-email (mu-username->email)))
  (let ((count (mu-mailbox-messages-count current-mailbox)))
    (do ((n 1 (1+ n)))
	((> n count) #f)
	(display n)(newline)
	(set! sieve-current-message
	      (mu-mailbox-get-message current-mailbox n))
	(catch 'sieve-stop
	       sieve-process-message
	       (lambda args
		 #f)))
    (sieve-close-mailboxes)))
