
;;;; Copyright 2006 by The University of York
;;;; Author: Paul Emsley
 
;;;; This program is free software; you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation; either version 2 of the License, or (at
;;;; your option) any later version.
 
;;;; This program is distributed in the hope that it will be useful, but
;;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;;; General Public License for more details.
 
;;;; You should have received a copy of the GNU General Public License
;;;; along with this program; if not, write to the Free Software
;;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


;; map to scheme names:
(define generic-object-is-displayed? generic-object-is-displayed-p)

;; map to scheme names:
(define is-closed-generic-object? is-closed-generic-object-p)

;; return a new generic object number for the given object obj-name.
;; If there is no such object with name obj-name, then create a new
;; one.  (Maybe you want the generic object to be cleaned if it exists
;; before returning, this function does not do that).
;; 
(define generic-object-with-name 
  (lambda (obj-name)
      
    (let ((t (generic-object-index obj-name)))
      (if (= t -1)
	  (new-generic-object-number obj-name)
	  t))))



;; display a GUI for generic objects
;; 
(define generic-objects-gui
  (lambda ()
    
    (let ((n-objects (number-of-generic-objects)))

      (if (> n-objects 0)
	  (begin

	    (let* ((window (gtk-window-new 'toplevel))
		   (vbox (gtk-vbox-new #f 10))
		   (label (gtk-label-new " Generic Objects ")))

	      (gtk-container-add vbox label)
	      (gtk-widget-show label)

	      (for-each 
	       (lambda (generic-object-number)

		 (format #t "~s ~s ~s~%" generic-object-number (generic-object-name generic-object-number)
			 (is-closed-generic-object? generic-object-number))

		 (if (= (is-closed-generic-object? generic-object-number) 0)
		     (let ((name (generic-object-name generic-object-number)))
		       
		       (if name 
			   (let* ((label (string-append 
					  (number->string generic-object-number)
					  "  "
					  name))
				  (frame (gtk-frame-new #f))
				  (check-button (gtk-check-button-new-with-label label)))
			     
					; this callback gets called by the
					; gtk-toggle-button-set-active just below,
					; which is why we need the state and active
					; test.
			     (gtk-signal-connect check-button "toggled" 
						 (lambda () 
						   (let ((button-state (gtk-toggle-button-active check-button))
							 (object-state (generic-object-is-displayed? 
									generic-object-number)))
						     (if (and (eq? button-state #t)
							      (= object-state 0))
							 (set-display-generic-object generic-object-number 1))
						     (if (and (eq? button-state #f)
							      (= object-state 1))
							 (set-display-generic-object generic-object-number 0)))))
			     
			     (let ((current-state 
				    (generic-object-is-displayed? 
				     generic-object-number)))
			       (if (= current-state 1)
				   (gtk-toggle-button-set-active check-button #t)))
			     
			     (gtk-container-add vbox frame)
			     (gtk-container-add frame check-button)
			     (gtk-widget-show frame)
			     (gtk-widget-show check-button))))))
		 
	       (number-list 0 (- n-objects 1)))

	      (gtk-widget-show vbox)
	      (gtk-container-add window vbox)
	      (gtk-container-border-width window 10)
	      (gtk-widget-show window)))))))



(define reduce-molecule-updates-current #f)

;; run molprobity (well reduce and probe) to make generic objects (and
;; display the generic objects gui)
;; 
(define probe
  (lambda (imol)

    (if (valid-model-molecule? imol)
	(if (not (command-in-path? *probe-command*))
	    (format #t "command for probe (~s) is not in path~%" *probe-command*)
	    (if (not (command-in-path? *reduce-command*))
		(format #t "command for reduce (~s) is not in path~%" *reduce-command*)
		(begin
		  (make-directory-maybe "coot-molprobity")
		  (let ((mol-pdb-file  "coot-molprobity/for-reduce.pdb")
			(reduce-out-pdb-file "coot-molprobity/reduced.pdb"))
		    (write-pdb-file imol mol-pdb-file)
		    (goosh-command *reduce-command* 
				   (list "-build" "-oldpdb" mol-pdb-file)
				   '() reduce-out-pdb-file #f)
		    (let* ((probe-name-stub (strip-extension (strip-path (molecule-name imol))))
			   (probe-pdb-in  (string-append 
					   "coot-molprobity/" probe-name-stub "-with-H.pdb"))
			   (probe-out "coot-molprobity/probe-dots.out"))

		      (goosh-command "grep" (list "-v" "^USER" reduce-out-pdb-file)
				     '() probe-pdb-in #f)
		      (goosh-command *probe-command* (list "-u" "-stdbonds" "-mc"
							   ; "'(chainA,chainZ) alta'" 
							   "ALL"
							   probe-pdb-in)
				     '() probe-out #f)
		      ; by default, we don't want to click on the
		      ; imol-probe molecule (I think :-)
		      (let* ((recentre-status (recentre-on-read-pdb))
			     (novalue (set-recentre-on-read-pdb 0))
			     (imol-probe 
			      (if reduce-molecule-updates-current
				  (begin
				    (format #t "======= update molecule =======~%")
				    (clear-and-update-model-molecule-from-file imol probe-pdb-in))
				  (begin
				    (format #t "======= read new pdb file =======~%")
				    (read-pdb probe-pdb-in)
				    imol))))
			(if (= 1 recentre-status)
			    (set-recentre-on-read-pdb 1))
			; (toggle-active-mol imol-probe) let's not do
			; that actually.  I no longer think that the
			; new probe molecule should not be clickable
			; when it is initally displayed (that plus
			; there is some active/displayed logic problem
			; for the molecules, which means that after
			; several probes, the wrong molecule is
			; getting refined).
			
			(handle-read-draw-probe-dots-unformatted probe-out imol-probe 1)
			(generic-objects-gui)
			(graphics-draw))))))))))

		    
				   
;; run "probe" interactively, 
;; which in the current implementation, means that this function 
;; can run during a edit-chi angles manipulation, or after
;; a real space refine zone.
;; 
;; Thus function presumes that there are 2 pdb files in the current 
;; directory, one of which is the reference pdb file and the other
;; is a pdb file containing the tmp/moving atom set.
;; 
;; The function takes arguments for the centre of the probe dots
;; and the radius of the probe dots sphere.  The chain id and 
;; residue number are also needed to pass as arguments to probe.
;; 
(define interactive-probe
  (lambda (x-cen y-cen z-cen radius chain-id res-no)

;    (make-directory-maybe "coot-molprobity")
;     (if (not (command-in-path? *probe-command*))
; 	(format #t "command for probe (~s) is not in path~%" *probe-command*)

    (let* ((probe-pdb-in-1 "molprobity-tmp-reference-file.pdb")
	   (probe-pdb-in-2 "molprobity-tmp-moving-file.pdb")
	   (probe-out "coot-molprobity/molprobity-tmp-probe-dots.out")
	   (atom-sel (string-append 
		      "(file1 within "
		      (number->string radius)
		      " of "
		      (number->string x-cen) ", "
		      (number->string y-cen) ", "
		      (number->string z-cen) ", "
		      "not water not ("
		      (if (string=? chain-id "") 
			  ""
			  "chain")
		      (string-downcase chain-id) " "
		      (number->string res-no)
		      ")),file2")))

      (format #t "probe command ~s ~s~%" *probe-command* 
	      (list "-mc" "-u" "-quiet" "-drop" "-stdbonds" "-both" 
		    atom-sel "file2" probe-pdb-in-1 probe-pdb-in-2))
      (goosh-command *probe-command* (list "-mc" "-u" "-quiet" "-drop" "-stdbonds" "-both" 
					   atom-sel "file2"
					   probe-pdb-in-1 probe-pdb-in-2)
		     '() probe-out #f)
      ; don't show the gui, so the imol is not needed/dummy.
      (handle-read-draw-probe-dots-unformatted probe-out 0 0)
      (graphics-draw))))
