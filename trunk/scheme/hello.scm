;;;; 
;;;; Copyright 2003, 2004, 2005, 2006 Paul Emsley, University of York

;; Primarily for Indian Names.
;; 
;; Say we are given str: (list "M." "D." "Albert" "Dorkins").  We want
;; to return ""Albert" not "M.")  We reject names that are one
;; character long and names that are 2 characters long that end in
;; ".".  So, "M." is rejected, but "Ma" is not.
;; 
;; An excercise for the committed is to also reject run-together
;; dotted initials such as "D.K.".  I was sufficiently committed.
;; 
(define first-non-trivial-name
  (lambda (str-list)

    (let f ((ls str-list))
      (cond 
       ((null? ls) "Person with No Name")
       ((= (string-length (car ls)) 4)
	(let ((str-list (string->list (car ls))))
	  (if (and (eq? (list-ref str-list 1) #\.)
		   (eq? (list-ref str-list 3) #\.))
	      (f (cdr ls))
	      (car ls))))
	
       ((> (string-length (car ls)) 2) (car ls))
       ((= (string-length (car ls)) 2) (let ((str-list (string->list (car ls))))
					 (if (eq? (car (cdr str-list)) #\.)
					     (f (cdr ls))
					     (car ls))))
       (else 
	(f (cdr ls)))))))


;; ideally we should say with the right internationalization
;; 
;(let* ((pw-bits (getpwnam (getenv "USER")))
;       (user (vector-ref pw-bits 4)))
(let* ((os-type (vector-ref (uname) 0))
       (user
	(cond
	 ((string=? os-type "Windows XP") (getenv "USERNAME"))
	 (else (vector-ref (getpwnam (getenv "USER")) 4)))))

  (let* ((time-str (let ((hour (tm:hour (localtime (current-time)))))
		     (cond 
		      ((< hour 12) "Good morning")
		      ((< hour 18) "Good afternoon")
		      (else "Good evening"))))
	 (hello-str (format #f "~a ~a. Welcome to Coot." 
			    time-str user)))

    (let* ((name-strings (string-split user #\space))
	   (first-name (car name-strings))
	   (last-name  (car (reverse name-strings)))
	   ;; in Japan (and other places (Korea, for example)?) the
	   ;; personal name comes last.
	   (personal-name 
	    (let ((lang     (getenv "LANG"))
		  (language (getenv "LANGUAGE")))
	      (cond 
	       ((string? lang) (cond 
				((string=? lang "ja") last-name)
				(else first-name)))
	       ((string? language) (cond 
				((string=? language "ja") last-name)
				(else first-name)))
	       (else
		(first-non-trivial-name name-strings)))))
	   (d-string (format #f "~a ~a. Welcome to Coot" time-str personal-name)))
    
      (format #t "~a~%" hello-str)
      (set-display-intro-string d-string))))
