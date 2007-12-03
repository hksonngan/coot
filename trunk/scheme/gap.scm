
;;;; Copyright 2004, 2005, 2006 by Paul Emsley, The University of York
 
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
;;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
;;;; 02110-1301, USA


;; Fit missing loop in protein.
;;
;; direction is either 'forwards or 'backwards
;; 
;; start-resno is higher than stop-resno if we are building backwards
;; 
;; (fit-gap 0 "A"  23 26)   ; we'll build forwards
;; (fit-gap 0 "A"  26 23)   ; we'll build backwards
;;
(define (fit-gap imol chain-id start-resno stop-resno . sequence)

    (define stop-now?
      (lambda (resno)
	
	;; If 96 (stop-resno) is the last residue we want added, then
	;; we do want to stop when resno is 96 - 1

	(>= resno (- stop-resno 1))))

    (if (not (valid-model-molecule? imol))
	
	(format #t "Molecule number ~s is not a valid model molecule" imol)

	(begin 

	  (let ((backup-mode (backup-state imol)))
	    (make-backup imol)
	    (turn-off-backup imol)

    	     ;; -----------------------------------------------
	     ;; Make poly ala
	     ;; -----------------------------------------------
	   
	    ;(set-terminal-residue-do-rigid-body-refine 0)
            ;(set-add-terminal-residue-n-phi-psi-trials 2000)
	    (set-residue-selection-flash-frames-number 0)

	    (let* ((immediate-refinement-mode 
		    (refinement-immediate-replacement-state))
		   (direction (if (< stop-resno start-resno)
				  'backwards
				  'forwards))
		   (next-residue (if (eq? direction 'forwards) + -)))
	      
	      (format #t "direction is ~s~%" direction)
	      
	      (set-refinement-immediate-replacement 1)
	      
	      ;; recur over residues:
	      (let f ((resno (if (eq? direction 'forwards)
				 (- start-resno 1)
				 (+ start-resno 1))))
		
		(format #t "~%~%add-terminal-residue: residue number: ~s~%~%" resno)
		(let ((status (add-terminal-residue imol chain-id resno "ALA" 1)))
		  (cond 
		   ((= 1 status)
					; first do a refinement of what we have 
		    (refine-auto-range imol chain-id resno "")
		    (accept-regularizement)
		    (if (not (stop-now? resno))
			(f (next-residue resno 1))))
		   (else 
		    (format #t "Failure in fit-gap at residue ~s~%" resno)))))

	      ;; -----------------------------------------------
	      ;; From poly ala to sequence (if given):
	      ;; -----------------------------------------------

	      (if (not (null? sequence))
		  (begin
		    (format #t "mutate-and-autofit-residue-range ~s ~S ~s ~s ~s~%"
			    imol chain-id start-resno stop-resno (car sequence))
		    (mutate-and-autofit-residue-range imol chain-id 
						      start-resno stop-resno
						      (car sequence))))

	      ;; -----------------------------------------------
	      ;; Refine new zone
	      ;; -----------------------------------------------

	      (if (residue-exists? imol chain-id (- start-resno 1) "")
		  (format #t "Test finds~%")
		  (format #t "Test: not there~%"))

	      (let* ((low-end (if (residue-exists? imol chain-id (- start-resno 1) "")
				  (- start-resno 1)
				  start-resno))
		     (high-end (if (residue-exists? imol chain-id (+ stop-resno 1) "")
				   (+ stop-resno 1)
				   stop-resno))
		     (final-zone (if (eq? direction 'forwards)
				     (cons low-end high-end)
				     (cons high-end low-end))))

		;; we also need to check that start-resno-1 exists and
		;; stop-resno+1 exists.

		(refine-zone imol chain-id (car final-zone) (cdr final-zone) "")
					; set the refinement dialog flag back to what it was:
		(if (= immediate-refinement-mode 0)
		    (set-refinement-immediate-replacement 0))
		(accept-regularizement)))
	    (if (= backup-mode 1)
		(turn-on-backup imol))))))


       

;;;; For Kay Diederichs, autofit without a map (find rotamer with best
;;;; clash score). This ignores alt conformations and residues with
;;;; insertion codes.
;;;;
(define de-clash
  (lambda (imol chain-id resno-start resno-end)

    (map (lambda (resno)
	   (auto-fit-best-rotamer resno "" "" chain-id imol -1 1 0.1))
	 (number-list resno-start resno-end))))
