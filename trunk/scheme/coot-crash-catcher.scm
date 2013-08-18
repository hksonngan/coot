;;;; Copyright 2000 by Paul Emsley
;;;; Copyright 2008 by The University of York
;;;; Copyright 2008, 2009 by The University of Oxford

;;;; This program is free software; you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation; either version 3 of the License, or (at
;;;; your option) any later version.
 
;;;; This program is distributed in the hope that it will be useful, but
;;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;;; General Public License for more details.
 
;;;; You should have received a copy of the GNU General Public License
;;;; along with this program; if not, write to the Free Software
;;;; Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

(use-modules (gtk gdk)
             (gtk gtk)
             (gui event-loop)
             (gui entry-port)
             (gui text-output-port))

(use-modules (ice-9 popen)
	     (ice-9 string-fun)
	     (ice-9 regex)
	     (ice-9 rdelim)
	     (os process))


;;; What was the executable?  Let's pass that on the command line to
;;; this program.
;;;
;;; What was the core?
;;;
;;; The run gdb and capture the output?

;; Basic scheme function, filter the objects in list ls by function
;; fn.  e.g. (filter even? (list 0 1 2 3) -> '(0 2)
;; 
(define (filter fn ls)

  (cond
   ((null? ls) '())
   ((fn (car ls)) (cons (car ls) (filter fn (cdr ls))))
   (else 
    (filter fn (cdr ls)))))

;; simple scheme functions to concat the strings in ls (ls must contain
;; only strings)
;; 
(define (string-concatenate ls)
  (apply string-append ls))


;;; ls must be a list of strings, atom must be a string.
;;; 
;;; return either #t or #f.
;;;
(define (string-member? atom ls)
  
  (cond 
   ((null? ls) #f)
   ((string=? atom (car ls)) #t)
   (else
    (string-member? atom (cdr ls)))))

;; The following functions from PLEAC (guile version thereof of course).
;; 
;; or define a utility function for this
(define (directory-files dir)

  (if (not (access? dir R_OK))
    '()
    (let ((p (opendir dir)))
      (do ((file (readdir p) (readdir p))
           (ls '()))
          ((eof-object? file) (closedir p) (reverse! ls))
        (set! ls (cons file ls))))))

(define (glob->regexp pat)
  (let ((len (string-length pat))
        (ls '("^"))
        (in-brace? #f))
    (do ((i 0 (1+ i)))
        ((= i len))
      (let ((char (string-ref pat i)))
        (case char
          ((#\*) (set! ls (cons "[^.]*" ls)))
          ((#\?) (set! ls (cons "[^.]" ls)))
          ((#\[) (set! ls (cons "[" ls)))
          ((#\]) (set! ls (cons "]" ls)))
          ((#\\)
           (set! i (1+ i))
           (set! ls (cons (make-string 1 (string-ref pat i)) ls))
           (set! ls (cons "\\" ls)))
          (else
           (set! ls (cons (regexp-quote (make-string 1 char)) ls))))))
    (string-concatenate (reverse (cons "$" ls)))))

;; return a list of file names that match pattern pat in directory dir.
(define (glob pat dir)
  (let ((rx (make-regexp (glob->regexp pat))))
    (filter (lambda (x) (regexp-exec rx x)) (directory-files dir))))


;; Return the strings screen output of cmd (reversed) or #f if command
;; was not found
;; 
(define (run-command/strings cmd args data-list)
    
  (if (not (command-in-path? cmd))

      (begin
	(format #t "command ~s not found~%" cmd)
	#f)

      (begin 
	(let* ((cmd-ports (apply run-with-pipe (append (list "r+" cmd) args)))
	       (pid (car cmd-ports))
	       (output-port (car (cdr cmd-ports)))
	       (input-port  (cdr (cdr cmd-ports))))
	  
	  (let loop ((data-list data-list))
	    (if (null? data-list)
		(begin 
		  (close input-port))
		
		(begin
		  (format input-port "~a~%" (car data-list))
		  (loop (cdr data-list)))))
	  
	  (let f ((obj (read-line output-port))
		  (ls '()))
	    
	    (if (eof-object? obj)
		(begin 
		  (let* ((status-info (waitpid pid))
			 (status (status:exit-val (cdr status-info))))
		    ls)) ; return ls
		
		(begin
		  (f (read-line output-port) (cons obj ls)))))))))

;; Return #t or #f:
(define (command-in-path? cmd)

  ;; test for command (see goosh-command-with-file-input description)
  ;; 
  (if (string? cmd) 
      (let ((have-command? (run "which" cmd)))
	
	(= have-command? 0)) ; return #t or #f
      #f))

;; Return a list if str is a string, else return '()
;; 
(define (string->list-of-strings str)

  (if (not (string? str))
      '()
      (let f ((chars (string->list str))
	      (word-list '())
	      (running-list '()))

	(cond 
	 ((null? chars) (reverse 
			 (if (null? running-list)
			     word-list
			     (cons (list->string (reverse running-list)) word-list))))
	 ((or (char=? (car chars) #\space) 
	      (char=? (car chars) #\tab)
	      (char=? (car chars) #\newline))
	  (f (cdr chars)
					  (if (null? running-list)
					      word-list
					      (cons (list->string 
						     (reverse running-list))
						    word-list))
					  '()))
	 (else 
	  (f (cdr chars)
	     word-list
	     (cons (car chars) running-list)))))))

;; code from thi <ttn at mingle.glug.org>
;; 
;; run command and put the output into a string and return
;; it. (c.f. @code{run-command/strings})
;; 
(define (shell-command-to-string cmd)
  (with-output-to-string
    (lambda ()
      (let ((in-port (open-input-pipe cmd)))
	(let loop ((line (read-line in-port 'concat)))
	  (or (eof-object? line)
	      (begin
		(display line)
		(loop (read-line in-port 'concat)))))))))



;; return string output of gdb:
;;
(define run-gdb 
  (lambda (coot-exe core-file gdb-script)

    (run-command/strings "gdb" 
			 (list "-batch" "-x" gdb-script coot-exe core-file)
			 '())))


;; get the latest core file here
;; return #f on failure to find core file
;;
(define core-file
  (lambda ()

    (let* ((glob-pattern "core.*")
	   (dir ".")
	   (files (glob glob-pattern dir)))

      (let f ((files files)
	      (latest-core #f)
	      (mtime-latest 0))

	(cond 
	 ((null? files) latest-core)
	 (else 
	  (let* ((info (stat (car files)))
		 (mtime (stat:mtime info)))
	    (if (> mtime mtime-latest)
		(f (cdr files) (car files) mtime)
		(f (cdr files) latest-core mtime-latest)))))))))

;; 
(define make-gdb-script
  (lambda ()

    (let ((filename (string-append
		     "/tmp/gdb-coot-"
		     (let ((s (getenv "USER")))
		       (if (string? s)
			   s
			   "no-user-id"))
		     "-"
		     (number->string (getpid))
		     ".txt")))

      (call-with-output-file filename
	  (lambda (port)
	    (display "bt" port) 
	    (newline port)))

      filename)))


;; return #f or list of strings
(define get-gdb-strings
  (lambda (coot-exe)

    (let ((core (core-file)))
      (format #t "core: ~s~%" core)
      (if (eq? core #f)
	  (begin
	    (format #t "No core file found.  No debugging~%")
	    (format #t "   This is not helpful.  ~%")
	    (format #t "   Please turn on core dumps before sending a crash report~%")
	    #f)
	  (let ((gdb-script (make-gdb-script)))
	    (if (string? gdb-script)
		(begin
		  (let ((string-list (run-gdb coot-exe core gdb-script))
			(delete-file gdb-script))
		    (reverse string-list)))
		(begin
		  (format #t "can't write gdb script~%")
		  #f)))))))

(define (send-emsley-text coot-exe gdb-string)

  (let ((info-list (list "\n------------------------------------\n" 
			 (let ((u (getenv "USER")))
			   (format #f "user: ~s ~s~%" s (getpwnam u)))
			 (format #f "hostname: ~s~%" (run-command/strings "hostname" '() '()))
			 (format #f "exe-name: ~s~%" coot-exe)
			 (format #f "version-full: ~s~%" 
				 (run-command/strings coot-exe (list "--version-full") '()))
			 (format #f "working directory: ~s~%" (getcwd)))))

    (run-command/strings "mail"
			 (list "-s" "Coot Crashed" 
			       (string-append "p" "e" "msley" "@" "mrc-lmb" "." "cam" ".ac.uk"))
			 (append info-list (list gdb-string)))))



;; gui
(define make-gui
  (lambda (gdb-strings)

    (let* ((window (gtk-window-new 'toplevel))
	   (label (gtk-label-new "  Coot Crashed! "))
	   (vbox (gtk-vbox-new #f 2))
	   (scrolled-win (gtk-scrolled-window-new))
	   (text (gtk-text-new #f #f))
	   (hbox-buttons (gtk-hbox-new #f 5))
	   (send-button (gtk-button-new-with-label " Send to Paul Emsley "))
	   (send-label (gtk-label-new 
			(string-append "Please send to Paul Emsley (using cut 'n paste)\n"
				       "(a few words to describe what you were \n"
				       "doing might be helpful too)")))
	   (cancel-button (gtk-button-new-with-label " Cancel "))
	   (text-string  gdb-strings))

      (gtk-window-set-default-size window 700 500)
      (gtk-container-add window vbox)
      (gtk-box-pack-start vbox label #f #f 5)
      (gtk-container-add scrolled-win text)
      (gtk-box-pack-start vbox scrolled-win #t #t 5)
      (gtk-box-pack-start vbox hbox-buttons #f #f 5)
      ;; if in MRC show the send-mail button, else show the text.
      (let ((domainname (string->list-of-strings (shell-command-to-string "domainname"))))
	(if (or (string=? "mrc-lmb" domainname)
		(string? (getenv "COOT_DEV_TEST")))
	    (gtk-box-pack-start hbox-buttons send-button #t #f 5)
	    (gtk-box-pack-start hbox-buttons send-label #t #f 5)))
      (gtk-box-pack-start hbox-buttons cancel-button #t #f 5)
      (gtk-text-insert text #f "black" "white" text-string -1)

      (gtk-signal-connect cancel-button "clicked"
			  (lambda args
			    ;; (gtk-widget-destroy window)))
			    (press-the-other-button-window)))

      (gtk-signal-connect send-button "clicked"
			  (lambda args 
			    (let ((coot-exe (car (cdr (command-line)))))
			      (send-emsley-text coot-exe gdb-strings)
			      (gtk-widget-destroy window))))

      (gtk-widget-show-all window)
      (gtk-standalone-main window))))

      
(define add-newlines
  (lambda (ls stub)

    (cond
     ((null? ls) stub)
     (else
      (add-newlines (cdr ls) (string-append stub "\n" (car ls)))))))
;; (add-newlines (list "one" "two" "three") "")


(define (press-the-other-button-window)
    (let* ((window (gtk-window-new 'toplevel))
	   (label (gtk-label-new "  No! Press the Other Button (Go On - it's not hard...) "))
	   (vbox (gtk-vbox-new #f 2))
	   (cancel-button (gtk-button-new-with-label " Cancel ")))
	   
      (gtk-box-pack-start vbox label #f #f 5)
      (gtk-box-pack-start vbox cancel-button #f #f 5)
      (gtk-container-add window vbox)

      (gtk-signal-connect cancel-button "clicked"
			  (lambda args
			    (gtk-widget-destroy window)))
      (gtk-widget-show-all window)))


(define bug-report
  (lambda (coot-exe)

    (format #t "coot-exe: ~s~%" coot-exe)
    (format #t "coot-version: ~%" )
    (run-command/strings coot-exe (list "--version-full") '())
    (format #t "platform: ~%" )
    (run-command/strings "uname" (list "-a") '())
    
    (let ((s (get-gdb-strings coot-exe)))
      (if (list? s)
	  (let ((gui-string (add-newlines s "")))
	    (make-gui gui-string))))))


;; command line invocation:
(let* ((args (command-line)))

  (if (> (length args) 1)
      (let ((coot-exe (car (cdr args))))
	(if (string-member? "--no-graphics" args)
	    
	    (begin
	      (format #t "INFO:: --no-graphics mode prevents bug-report GUI~%")
	      (let ((s (get-gdb-strings coot-exe)))
		(if (not (list? s))
		    (begin
		      (format #t "gdb-strings: ~s~%" s))
		    (begin
		      (map (lambda (line)
			     (display line)
			     (newline)) 
			   s)))))
		     
	    (bug-report coot-exe)))))


			   
