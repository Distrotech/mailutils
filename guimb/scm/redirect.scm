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

;;;; This module provides sieve's "redirect" action.

(use-modules (ice-9 popen))

(define PATH-SENDMAIL "/usr/lib/sendmail")
(define sieve-my-email "")

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
	     (close-output-port port)))
	 (lambda args
	   (runtime-error LOG_ERR "redirect: Can't send message")
	   (write args))))

;;; rfc3028 says: 
;;; "Implementations SHOULD take measures to implement loop control,"
;;; We do this by appending an "X-Sender" header to each message
;;; being redirected. If one of the "X-Sender" headers of the message
;;; contains our email address, we assume it is a loop and bail out.

(define (sent-from-me? msg)
  (call-with-current-continuation
   (lambda (x)
     (for-each
      (lambda (hdr)
	(if (and (string=? (car hdr) "X-Sender")
		 (string=? (mu-address-get-email (cdr hdr))
			   sieve-my-email))
	    (x #t)))
      (mu-message-get-header-fields sieve-current-message))
     #f)))

;;; redirect action
(define (action-redirect address)
  (if sieve-my-email
      (cond
       ((sent-from-me? sieve-current-message)
	(runtime-error LOG_ERR "redirect: Loop detected"))
       (else
	(let ((out-msg (mu-message-copy sieve-current-message)))
	  (mu-message-set-header out-msg "X-Sender" sieve-my-email)
	  (sieve-message-bounce out-msg (list address)))))
      (sieve-message-bounce out-msg (list address))))

;;; Register action
(if sieve-parser
    (sieve-register-action "redirect" action-redirect 'string))


