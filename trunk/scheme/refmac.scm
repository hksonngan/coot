;;;; refmac.scm
;;;;
;;;; Copyright 2004, 2005, 2006, 2007 by The University of York
;;;; Author: Paul Emsley
;;;; 
;;;; This program is free software; you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation; either version 3, or (at your option)
;;;; any later version.
;;;; 
;;;; This program is distributed in the hope that it will be useful,
;;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;; GNU General Public License for more details.
;;;; 
;;;; You should have received a copy of the GNU General Public License
;;;; along with this software; see the file COPYING.  If not, write to
;;;; the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
;;;; Boston, MA 02111-1307 USA
;;;;


;;; Extra parameters can be passed to refmac using either a file
;;; "refmac-extra-params" or by setting the variable
;;; refmac-extra-params.  The LABIN line should not be part those
;;; extra parameters, of course - Coot takes care of that.

;; This is the default refmac version, it is presumed to be in the
;; path.  It can be overridden using a re-definition either at the
;; scripting interface or in one's ~/.coot file. E.g.:
;; (define refmac-exec "/y/programs/xtal/refmac-latest/bin/refmac5-3-dec-2004")
(define refmac-exe "refmac5") 

;; Set this to a list of parameter strings:
;; 
;; If refmac-extra-params is a list of strings, it is used in
;; preference to the "refmac-extra-params" file (should it exist).
;; e.g. (set! refmac-extra-params (list "WEIGHT 0.2" 
;;                                      "NCYC 10" 
;;                                      "REFI BREF ISO"
;;                                      "REFI METH CGMAT"
;;                                      "REFI TYPE REST RESO 20 1.64"))
;; 
(define refmac-extra-params #f)

(define refmac-count 0)

;; If phase-combine-flag is 1 then we should do a phase combination
;; with the coefficients that were used to make the map, specifically
;; PHIB and FOM, that are the result of (say) a DM run.  It is best
;; of course to use HL coefficients, if DM (say) wrote them out.
;; 
;; If phase-combine-flag is 0, then phib-fom-pair is ignored.  If
;; phase-combine-flag is 1 phib-fom-pair is presumed to be an
;; improper list of the PHIB and FOM column labels (as strings).
;; 
;; Is there a case where you want phase recombination, but do not
;; have FOMs?  Don't know, I presume not.  So we required that if
;; phase-combine-flag is 1, then PHIB and FOM are present.  Else we
;; don't run refmac.
;; 
;; If ccp4i project dir is "" then we ignore it and write the refmac
;; log file here.  If it is set to something, it is the prefix of the
;; refmac log file.  In that case it should end in a "/".  This is
;; not tested for here!  The pdb-in-filename etc are dealt with in
;; the c++ execute_refmac function.
;; 
;; if swap_map_colours_post_refmac? is not 1 then imol-mtz-molecule
;; is ignored.
;;
;; It is possible that we get here with long refmac column labels,
;; e.g.  "/RNASE3GMP/COMPLEX/FGMP18" "/RNASE3GMP/COMPLEX/SIGFGMP18"
;; "/RNASE/NATIVE/FreeR_flag".
;; 
;; So we only use text after the last /.
;;
;; make-molecules-flag is tested for being = 0, if not 0, then this is
;; the main thread and we can do graphics things, like read in a pdb
;; and mtz file to make new molecules.
;;
;; 
(define (run-refmac-by-filename pdb-in-filename pdb-out-filename mtz-in-filename mtz-out-filename extra-cif-lib-filename imol-refmac-count swap-map-colours-post-refmac? imol-mtz-molecule show-diff-map-flag phase-combine-flag phib-fom-pair force-n-cycles make-molecules-flag ccp4i-project-dir f-col sig-f-col . r-free-col) 

  (define local-format 
    (lambda args
      (if (not (= make-molecules-flag 0))
	  (apply format args))))

    (local-format #t "got args: pdb-in-filename: ~s, pdb-out-filename: ~s, mtz-in-filename: ~s, mtz-out-filename: ~s, imol-refmac-count: ~s, show-diff-map-flag: ~s, phase-combine-flag: ~s, phib-fom-pair: ~s, force-n-cycles: ~s, f-col: ~s, sig-f-col: ~s, r-free-col: ~s~%"
	    pdb-in-filename pdb-out-filename
	    mtz-in-filename mtz-out-filename
	    imol-refmac-count show-diff-map-flag
	    phase-combine-flag
	    phib-fom-pair
	    force-n-cycles
	    f-col sig-f-col r-free-col)
	
    (let* ((local-r-free-col (if (null? r-free-col) '() (car r-free-col)))
		   ; need to check for f-col being a string or list
	   (labin-string (if (and (string? f-col) (string=? f-col "")) ""
				(apply string-append (append
					   (if (= phase-combine-flag 3)
						   (list "LABIN" " "
								 "F+=" (strip-path (car f-col)) " "
								 "SIGF+=" (strip-path (car sig-f-col)) " "
								 "F-=" (strip-path (cdr f-col)) " "
								 "SIGF-=" (strip-path (cdr sig-f-col)))
						   (if (= (refmac-use-intensities-state) 1)
							   (list "LABIN" " "
									 "IP=" (strip-path f-col) " "
									 "SIGIP=" (strip-path sig-f-col))
							   (list "LABIN" " "
									 "FP=" (strip-path f-col) " "
									 "SIGFP=" (strip-path sig-f-col))))
					   (if (null? local-r-free-col)
						   '()
						   (list " FREE=" (strip-path local-r-free-col)))
					   (if (= phase-combine-flag 1)
						   ; we have Phi-FOM pair
						   (list " - \nPHIB=" (strip-path (car phib-fom-pair)) " "
								 "FOM=" (strip-path (cdr phib-fom-pair))) '())
					   (if (= phase-combine-flag 2)
						   (let ((hl-list (string->list-of-strings (car phib-fom-pair))))
							 (list  " - \nHLA=" (strip-path (list-ref hl-list 0)) " "
									"HLB=" (strip-path (list-ref hl-list 1)) " "
									"HLC=" (strip-path (list-ref hl-list 2)) " "
									"HLD=" (strip-path (list-ref hl-list 3)))) '())))))

	  (command-line-args
	   (append
	    (list 
	     "XYZIN"  pdb-in-filename
	     "XYZOUT" pdb-out-filename
	     "HKLIN"  mtz-in-filename
	     "HKLOUT" mtz-out-filename)
	    (if (string=? extra-cif-lib-filename "")
		(begin
		  (local-format #t "Not Passing LIBIN to refmac LIBIN~%")
		  (list)) ; nothing
		(begin
		  (local-format #t "Passing to refmac LIBIN ~s~%" extra-cif-lib-filename)
		  (list "LIBIN" extra-cif-lib-filename)))))
      
	  (data-lines (let* ((std-lines
			      (list 
			       "MAKE HYDROGENS NO" ; Garib's suggestion 8 Sept 2003
			       (if (= (get-refmac-refinement-method) 1)
					; rigid body
				   "REFInement TYPE RIGID"
				   "")
			       (if (number? force-n-cycles)
					   (if (>= force-n-cycles 0)
						   (string-append
							(if (= (get-refmac-refinement-method) 1)
								"RIGIDbody NCYCle "
								"NCYCLES " )
							(number->string force-n-cycles))
						   "")
					   "")
			       (if (= (get-refmac-refinement-method) 2)
				   "REFI TLSC 5"
				   "")
			       (if (= (refmac-use-twin-state) 1)
				   "TWIN"
				   "")
			       (if (and (= phase-combine-flag 3) (string=? labin-string ""))
				   "REFI SAD"
				   "")
;			       (if (= (refmac-use-sad-state) 1)
;					; need to give some information for SAD atom FIXME
;					; too tricky for now put in a fix one for now
;					;(let((sad-atom-ls (get-sad-atom-info)))))
;				   "ANOM FORM SE -8.0 4.0"
;				   "")
;			       (if (= (refmac-use-ncs-state) 1)
;				   ""  ; needs some chains etc FIXME
;				   "")
			       ))
			     (extra-params (get-refmac-extra-params))
			     (extra-rigid-params (refmac-rigid-params))
;			     (format #t "BL DEBUG:: extra rigid params ~s~%" extra-rigid-params)
			     (extra-ncs-params   (refmac-ncs-params))
			     (extra-sad-params   (refmac-sad-params))
			     )

			(if (extra-params-include-weight? extra-params)
			    (append std-lines
				    extra-params
				    extra-rigid-params
				    extra-ncs-params
				    extra-sad-params
				    (list labin-string))
			    (append std-lines
				    (list "WEIGHT AUTO")
				    extra-params
				    extra-rigid-params
				    extra-ncs-params
				    extra-sad-params
				    (list labin-string)))))

	  (nov (local-format #t "DEBUG:: refmac-extra-params returns ~s~%"
		       (get-refmac-extra-params)))
	  ;; this should be a database filename:
	  ;; 
	  (refmac-log-file-name (string-append 
				 (if (> (string-length ccp4i-project-dir) 0)
				     ccp4i-project-dir ;; is this string terminated with a slash?
				     "")
				 "refmac-from-coot-" 
				 (number->string refmac-count) ".log")))

      (set! refmac-count (+ imol-refmac-count 1))
      (local-format #t "INFO:: Running refmac with these command line args: ~s~%"
	      command-line-args)
      (local-format #t "INFO:: Running refmac with these data lines: ~s~%"
	      data-lines)
      (local-format #t "environment variable:  SYMOP: ~s~%" (getenv "SYMOP"))
      (local-format #t "environment variable: ATOMSF: ~s~%" (getenv "ATOMSF"))
      (local-format #t "environment variable:  CLIBD: ~s~%" (getenv "CLIBD"))
      (local-format #t "environment variable:   CLIB: ~s~%" (getenv "CLIB"))

      ;; first check if refmac exists?
      (local-format #t "INFO:: now checking for refmac exe - [it should give status 1...]~%" )
      (let ((test-refmac-status  (goosh-command refmac-exe '() (list "END")
					   refmac-log-file-name #f)))

	;; potentially a no-clobber problem here, I think.

	(local-format #t "INFO:: test-refmac-status: ~s~%" test-refmac-status)

	(if (not (number? test-refmac-status))
	    -3
	    (if (not (= test-refmac-status 1))
		
		;; problem finding refmac executable
		(begin 
		  (local-format #t "refmac failed (no executable)")
		  (local-format #t " - no new map and molecule available~%"))

		;; OK, we found the executable, this should be OK then...
		(let* ((to-screen-flag (if (= make-molecules-flag 0)
					   #f   ;; In a sub-thread, do it noiselessly.
					   #t)) ;; As normal.
		       (status (goosh-command refmac-exe 
					      command-line-args 
					      data-lines 
					      refmac-log-file-name to-screen-flag))) ; to screen too

		  (if (and (number? status) (= status 0)) ; refmac ran OK

		      (if (= make-molecules-flag 0)

			  (begin
			    (list pdb-out-filename mtz-out-filename)) ;; threaded refmac return value

			  (begin ;; normal/main thread
			    
			    ;; now let's read in those newly-created
			    ;; coordinates and phases for a map:
			    
			    ;; first loggraph maybe.
			    (if (file-exists? refmac-log-file-name)
				(if (command-in-path? "loggraph")
				    (run-concurrently "loggraph" refmac-log-file-name)))
			    
			    ;; 
			    ;; 
			    (let* ((r-free-bit (if (null? r-free-col)
						   (list  "" 0)
						   (list local-r-free-col 1)))
				   (args 
				    (append
					; numbers: use-weights? is-diff-map? have-refmac-params?
				     (list mtz-out-filename "FWT" "PHWT" "" 0 0 1 f-col sig-f-col)
				     r-free-bit))
				   (args-default
				    (append
					; numbers: use-weights? is-diff-map? have-refmac-params?
				     (list mtz-out-filename "FWT" "PHWT" "" 0 0 1 "FP" "SIGFP")
				     r-free-bit))
				   (recentre-status (recentre-on-read-pdb))
				   (novalue (set-recentre-on-read-pdb 0))
				   (novalue2 (local-format #t "DEBUG:: recentre status: ~s~%" recentre-status))
				   (imol
				    (handle-read-draw-molecule pdb-out-filename))) ;; normal/old-style case
			      
			      (if recentre-status (set-recentre-on-read-pdb 1))
			      (set-refmac-counter imol (+ imol-refmac-count 1))
			      
			      (let ((new-map-id
				     (apply make-and-draw-map-with-refmac-params 
					    (if (= phase-combine-flag 3) args-default args))))
				
				(if (= swap-map-colours-post-refmac? 1)
				    (swap-map-colours imol-mtz-molecule new-map-id))

				;; set new map as refinement map
				(if (valid-map-molecule? imol)
				    (set-imol-refinement-map new-map-id))

				(if (= (get-refmac-used-mtz-file-state) 1)
				    (begin
				      (set-stored-refmac-file-mtz-filename new-map-id mtz-in-filename)
				      (if (and (> phase-combine-flag 0) (< phase-combine-flag 3))
					  (begin
					    (let ((phib "")
						  (fom  "")
						  (hla  "")
						  (hlb  "")
						  (hlc  "")
						  (hld  ""))
					      (if (= phase-combine-flag 1)
						  (save-refmac-phase-params-to-map new-map-id
										   (car phib-fom-pair)
										   (cdr phib-fom-pair)
										   hla hlb hlc hld))
					      (if (= phase-combine-flag 2)
						  (let ((hl-list (string->list-of-strings (car phib-fom-pair))))
						    (save-refmac-phase-params-to-map new-map-id
										     phib fom
										     (list-ref hl-list 0)
										     (list-ref hl-list 1)
										     (list-ref hl-list 2)
										     (list-ref hl-list 3))))))))))
			      
			      (if (= 1 show-diff-map-flag) ; flag was set
				  (if (= phase-combine-flag 3)
				      (begin
					(let ((args (append (list mtz-out-filename "DELFWT" "PHDELWT" "" 0 1 1
								  "FP" "SIGFP") r-free-bit)) 
					      (args-ano (append (list mtz-out-filename "FAN" "PHAN" "" 0 1 1
								      "FP" "SIGFP") r-free-bit)))
					
					  (apply make-and-draw-map-with-refmac-params args)
					  (apply make-and-draw-map-with-refmac-params args-ano)))
					
				      (begin
					(let ((args (append
						     (list mtz-out-filename "DELFWT" "PHDELWT" "" 0 1 1
							   f-col sig-f-col)
						     r-free-bit)))
					  
					  (apply make-and-draw-map-with-refmac-params args)))))


			      (if (coot-has-pygtk?)
				  (let ((s (string-append "read_refmac_log("
							  (number->string imol) ", \""
							  refmac-log-file-name "\")")))
				    (run-python-command s)))))))))))))


;; Return #t if the list of strings @var{params-list} contains a
;; string beginning with "WEIGHT".  If not return #f
;; 
(define (extra-params-include-weight? params-list)

  ;; This tests all words in the string.  We should more rigourously
  ;; test only the string either at the beginning or after "".
  ;; 
  (define has-weight-word?
    (lambda (string-list)
      
      (cond
       ((null? string-list) #f)
       ((string-ci=? (car string-list) "WEIGHT") #t)
       (else 
	(has-weight-word? (cdr string-list))))))
  
  ;;main body
  (if (not (list? params-list))
      #f
      (let f ((params-list params-list))

	(cond
	 ((null? params-list) #f)
	 ((has-weight-word? (string->list-of-strings (car params-list))) #t)
	 (else 
	  (f (cdr params-list)))))))


;; If refmac-extra-params is defined (as a list of strings), then
;; return that, else read the file "refmac-extra-params".
;; 
;; Return a list a list of strings.
;; 
(define get-refmac-extra-params
  (lambda ()

    (if (list-of-strings? refmac-extra-params)

	refmac-extra-params

	(let ((extras-file-name
	       "refmac-extra-params")) 
	  
	  (if (not (file-exists? extras-file-name))
	      '()
	      
	      (call-with-input-file extras-file-name
		(lambda (port)
		  
		  (let ((r-list 
			 (let f ((line-list '())
				 (line (read-line port)))
			   
			   ; (format #t "line-list: ~s~%" line-list)
			   
			   (cond
			    ((null? line) line-list)
			    ((eof-object? line) line-list)
			    (else (f (cons line line-list)
				     (read-line port)))))))
		    
		    (reverse r-list)))))))))

(define refmac-rigid-params
  (lambda ()
    (if (= (get-refmac-refinement-method) 1)
	(let ((ret '())
	      (count 0)
	      (imol-coords (refmac-imol-coords)))
	  (for-each (lambda (chain-id)
		      (let* ((n-residues  (chain-n-residues chain-id imol-coords))
			    (start-resno (seqnum-from-serial-number imol-coords chain-id 0))
			    (stop-resno  (seqnum-from-serial-number imol-coords chain-id (- n-residues 1))))
			(set! count (+ count 1))
			(set! ret (append ret (list (string-append "RIGIdbody GROUp " (number->string count)
								   " FROM " (number->string start-resno) " " chain-id
								   " TO "   (number->string stop-resno)  " " chain-id))))))
		      (chain-ids imol-coords))
	  ret)
	'())))
		

(define refmac-ncs-params
  (lambda()
    (if (= (refmac-use-ncs-state) 1)
	(let* ((imol-coords (refmac-imol-coords))
	       (chain-ids-from-ncs (ncs-chain-ids imol-coords)))
	  (if (list? chain-ids-from-ncs)
	      (let ((ret-list '())
		    (ret-string ""))
		(for-each (lambda (ncs-chain-set)
			    (if (> (length ncs-chain-set) 1)
				(let ((ret-string (string-append "NCSRestraints NCHAins " 
								 (number->string (length ncs-chain-set))
								 " CHAIns ")))
				  (for-each (lambda (ncs-chain-id)
					      (set! ret-string (string-append ret-string " " ncs-chain-id)))
					    ncs-chain-set)
				  (set! ret-list (append ret-list (list ret-string))))
				(set! ret-list ret-list)))
			  chain-ids-from-ncs)
		ret-list)
	      '()))
	'())))
	

(define refmac-sad-params
  (lambda ()
    (if (= (refmac-use-sad-state) 1)
	(let ((sad-atom-ls (get-refmac-sad-atom-info))
	      (ret-list '()))
	  (for-each (lambda (sad-atom)
		      (let ((atom-name (list-ref sad-atom 0))
			    (fp        (list-ref sad-atom 1))
			    (fpp       (list-ref sad-atom 2))
			    (wavelen   (list-ref sad-atom 3))
			    (ret-string "ANOM FORM "))
			(set! ret-string (string-append ret-string atom-name " "))
			(if (number? fp)
			    (set! ret-string (string-append ret-string (number->string fp) " ")))
			(if (number? fpp)
			    (set! ret-string (string-append ret-string (number->string fpp) " ")))
			(if (number? wavelen)
			    (set! ret-string (string-append ret-string (number->string wavelen) " ")))
			(set! ret-list (append ret-list (list ret-string)))))
		    sad-atom-ls)
	  ret-list)
	'())))

;; this is not run as a sub-thread, no useful return value.
;; 
(define run-refmac-for-phases
  (lambda (imol mtz-file-name f-col sig-f-col)
    
    (if (file-exists? mtz-file-name)
	(if (valid-model-molecule? imol)
	      (let ((dir-state (make-directory-maybe "coot-refmac")))
		(if (not (= 0))
		    (format #t "Failed to make coot-refmac directory\n")
		    (let* ((stub (string-append "coot-refmac/refmac-for-phases"))
			   (pdb-in  (string-append stub ".pdb"))
			   (pdb-out (string-append stub "-tmp.pdb"))
			   (mtz-out (string-append stub ".mtz"))
			   (cif-lib-filename "")
			   ; preserve the ncs and tls state
			   (ncs-state (refmac-use-ncs-state))
			   (tls-state (refmac-use-tls-state)))
		      (write-pdb-file imol pdb-in)
		      (set-refmac-use-ncs 0)
		      (set-refmac-use-tls 0)
		      (run-refmac-by-filename pdb-in pdb-out
					      mtz-file-name mtz-out
					      cif-lib-filename 0 0 -1
					      1 0 '() 0 
					      1 ;; let run-refmac-by-filename make molecules 
					        ;; (this is not a sub-thread)
					      "" 
					      f-col sig-f-col)
		      ; reset the ncs and tls states
		      (set-refmac-use-ncs ncs-state)
		      (set-refmac-use-tls tls-state))))))))
	

(define refmac-for-phases-and-make-map
  (lambda (mtz-file-name f-col sig-f-col)

    (if (file-exists? mtz-file-name)
	(molecule-chooser-gui 
	 "  Choose a molecule from which to calculate Structure factors:  "  
	 ; a lambda function that accepts the choose imol as its arg:
	 (lambda (imol)
	   (run-refmac-for-phases imol mtz-file-name f-col sig-f-col))))))


		    
(define get-refmac-version
  (let ((cached-result #f))

    (lambda ()
      (if cached-result
	  cached-result
	  (let ((log-file-name "refmac-version-tmp.log"))
	    (goosh-command "refmac5" '("-i") '() log-file-name #f)
	    (call-with-input-file log-file-name
	      (lambda (port)
		(let loop ((line (read-line port)))
		  (cond
		   ((eof-object? line) #f) ; Program line not found
		   ((string-match "Program" line)
		    (let* ((ls (string->list-of-strings line))
			   (version-str (car (reverse ls)))
			   (version-parts (map string->number (string-split version-str #\.))))
		      ;; (format #t "version parts: ~s~%" version-parts)
		      (set! cached-result version-parts)
		      cached-result))
		   (else 
		    (loop (read-line port))))))))))))

	  
