
(use-modules (ice-9 threads)) ; must come after coot-gui or else coot-gui barfs.

;; a place-holder for the coot listener socket
(define %coot-listener-socket #f)

;; Open a coot listener socket, return nothing much, but do set! 
;; %coot-listener-socket.
;; 
;; Hmmm... why make a side-effect like that?  Why not return
;; %coot-listener-socket so that the caller can set it?  There may be
;; a reason...
;; 
;; And the reason is that I can then call
;; coot-listener-idle-function-proc without having to use a c++
;; variable.
;; 
(define (open-coot-listener-socket port-number host-name)

    (format #t "in open-coot-listener-socket port: ~s host: ~s~%"
	     port-number host-name)

    (let ((soc (socket AF_INET SOCK_STREAM 0))
	  (host-address "127.0.0.1"))
      
      (connect soc AF_INET (inet-aton host-address) port-number)
      
      (display "Coot listener socket ready!\n" soc)
      (set! %coot-listener-socket soc)
;        (if soc 
; 	   (set-coot-listener-socket-state-internal 1))))
      

      (call-with-new-thread coot-listener-idle-function-proc
			    coot-listener-error-handler)))


;; yet another go to make a coot port reader work.  This time, we use
;; a gtk-timer to read stuff from the socket.  
;; 
;; The gtk-timer function must return 1 to be called again.  When we
;; want to close the socket reader, simply make the function return 0.
;; 
(define (open-coot-listener-socket-with-timeout port-number host-name)

    (format #t "in open-coot-listener-socket port: ~s host: ~s~%"
	     port-number host-name)

    (let ((soc (socket AF_INET SOCK_STREAM 0))
	  (host-address "127.0.0.1"))
      
      (connect soc AF_INET (inet-aton host-address) port-number)
      
      (display "Coot listener socket ready!\n" soc)
      (set! %coot-listener-socket soc)

      (gtk-timeout-add 500 coot-socket-timeout-func)))

;; based on coot-listener-idle-function-proc
;; 
(define (coot-socket-timeout-func)

    (if %coot-listener-socket
	(begin
	  (let loop ()
	    (let ((continue? (listen-coot-listener-socket-v3 %coot-listener-socket)))
	      (if (not continue?)
		  (begin
		    (format #t "server gone - listener thread ends~%")
		    #f)
		  (begin
		    (format #t "keep listening...~%")
		    #t)))))
	(begin
	  (format #t "coot-listener-idle-function-proc bad sock: ~s~%" 
		  %coot-listener-socket))))


;; Handle error on coot listener socket
;;
(define (coot-listener-error-handler key . args)

    (format #t "coot-listener-error-handler handling error in ~s with args ~s~%"
            key args))



;; Do this thing when idle
;; 
;; currently set to listen to the %coot-listener-socket
;; 
(define coot-listener-idle-function-proc
  (lambda ()
    
    (if %coot-listener-socket
	(begin
	  (let loop ()
	    (let ((continue? (listen-coot-listener-socket-v3 %coot-listener-socket)))
	      (if (not continue?)
		  (format #t "server gone - listener thread ends~%")
		  (begin
		    (usleep 100000)
		    (loop))))))
	(begin
	  (format #t "coot-listener-idle-function-proc bad sock: ~s~%" 
		  %coot-listener-socket)))))

;; the function to run from the main thread to evaluate a string:
(define (eval-socket-string s)

  (with-input-from-string s
    (lambda () 
      
      (let loop ((thing (read)))
	(if (not (eof-object? thing))
	    (begin 
	      ;; (format #t "this code be evaluated: ~s~%" thing)
	      (eval thing (interaction-environment))
	      (loop (read))))))))


;; Must continue to go round this loop until end-communication string
;; is found...
;;
(define (listen-coot-listener-socket-v3 soc)

  (define end-transmition-string "; end transmition")

  (define evaluate-char-list
    (lambda (read-bits)
      
      (let ((s (list->string (reverse read-bits))))
	;; is it python?
	(if (and (>= (string-length s) 8) ; (checked in order)
		 (string-match "# PYTHON" (substring s 0 8)))
	    ;; this is a silly "testing" name
	    (begin
	      (format #t "this python string will be evaluated: ~s~%" s)
	      (safe-python-command-by-char-star s))
	    ;; not python
	    (begin 
	      (format #t "this scheme string will be evaluated: ~s~%" s)
	      (format #t "======== setting socket string waiting...~%")
	      (set-socket-string-waiting s)
	      (format #t "======== done setting socket string waiting...~%")
	      )))))
	      

  ;; remove end-transmition-string from read-bits
  ;; 
  (define strip-tail 
    (lambda (read-bits)

      (if (> (length read-bits) (string-length end-transmition-string))
	  (let loop ((read-bits read-bits)
		     (match-bits (string->list end-transmition-string)))
	    (cond 
	     ((null? match-bits) read-bits)
	     (else 
	      (loop (cdr read-bits) (cdr match-bits)))))
	  '())))

  ;; return #t or #f
  (define (hit-end-transmit? read-bits end-transmition-string)
    (if (< (length read-bits)
	   (string-length end-transmition-string))
	#f
	(let loop ((read-chars read-bits)
		   (match-chars (reverse (string->list end-transmition-string))))
	  (cond 
	   ((null? match-chars) #t)
	   ((not (eq? (car match-chars)
		      (car read-chars))) #f)
	   (else
	    (loop (cdr read-chars) (cdr match-chars)))))))
	
  ;; main body:
  ;; 

  (if (not (char-ready? soc))
      (begin
;	(format #t "nothing on the line...~%")
	#t)
      (begin
;	(format #t "there *is* a ready-char!~%" soc)
;	(format #t "about to read from socket soc: ~s~%" soc)
	(let f ((c (read-char soc))
		(read-bits '()))
	  ;; (format #t "socket read char ~s~%" c)
	  (cond
	   ((eof-object? c) (format #t "server gone\n") #f)
	   ((hit-end-transmit? (cons c read-bits)
			       end-transmition-string)
;	    (format #t "  ~s found in ~s~%" 
;		    end-transmition-string
;		    (list->string (reverse (cons c read-bits))))
	    (evaluate-char-list (strip-tail (cons c read-bits)))
	    #t)

	   (else 
	    (f (read-char soc) (cons c read-bits))))))))


