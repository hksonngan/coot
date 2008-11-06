;;;; Copyright 2007 by The University of York
;;;; Copyright 2007 by Paul Emsley
;;;; 
;;;; This program is free software; you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation; either version 3 of the License, or (at
;;;; your option) any later version.
;;;; 
;;;; This program is distributed in the hope that it will be useful, but
;;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;;; General Public License for more details.
 
;;;; You should have received a copy of the GNU General Public License
;;;; along with this program; if not, write to the Free Software
;;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
;;;; 02110-1301, USA

(define (add-coot-menu-seperator menu)
  (let ((sep (gtk-menu-item-new)))
    (gtk-container-add menu sep)
    (gtk-widget-set-sensitive sep #f)
    (gtk-widget-show sep)))

(if (defined? 'coot-main-menubar)

    ;; ---------------------------------------------
    ;;           coot news dialog
    ;; ---------------------------------------------
    (let ((menu (coot-menubar-menu "About")))
      (if menu
	  (add-simple-coot-menu-menuitem
	   menu "Coot News"
	   (lambda ()
	     (whats-new-dialog))))))


(if (defined? 'coot-main-menubar)
    ;; ---------------------------------------------
    ;;           extensions
    ;; ---------------------------------------------
    ;; 
    (let ((menu (coot-menubar-menu "Extensions")))

      ;; make the submenus:
      (let ((submenu-all-molecule (gtk-menu-new))
	    (menuitem-2 (gtk-menu-item-new-with-label "All Molecule..."))
	    (submenu-maps (gtk-menu-new))
	    (menuitem-3 (gtk-menu-item-new-with-label "Maps..."))
	    (submenu-models (gtk-menu-new))
	    (menuitem-4 (gtk-menu-item-new-with-label "Modelling..."))
	    (submenu-refine (gtk-menu-new))
	    (menuitem-5 (gtk-menu-item-new-with-label "Refine..."))
	    (submenu-representation (gtk-menu-new))
	    (menuitem-6 (gtk-menu-item-new-with-label "Representation..."))
	    (submenu-settings (gtk-menu-new))
	    (menuitem-7 (gtk-menu-item-new-with-label "Settings..."))
	    (submenu-ncs (gtk-menu-new))
	    (menuitem-ncs (gtk-menu-item-new-with-label "NCS...")))

	(gtk-menu-item-set-submenu menuitem-2 submenu-all-molecule)
	(gtk-menu-append menu menuitem-2)
	(gtk-widget-show menuitem-2)

	(gtk-menu-item-set-submenu menuitem-3 submenu-maps)
	(gtk-menu-append menu menuitem-3)
	(gtk-widget-show menuitem-3)

	(gtk-menu-item-set-submenu menuitem-4 submenu-models)
	(gtk-menu-append menu menuitem-4)
	(gtk-widget-show menuitem-4)

	(gtk-menu-item-set-submenu menuitem-ncs submenu-ncs) 
	(gtk-menu-append menu menuitem-ncs)
	(gtk-widget-show menuitem-ncs)
	
	(gtk-menu-item-set-submenu menuitem-5 submenu-refine)
	(gtk-menu-append menu menuitem-5)
	(gtk-widget-show menuitem-5)

	(gtk-menu-item-set-submenu menuitem-6 submenu-representation)
	(gtk-menu-append menu menuitem-6)
	(gtk-widget-show menuitem-6)

	(gtk-menu-item-set-submenu menuitem-7 submenu-settings)
	(gtk-menu-append menu menuitem-7)
	(gtk-widget-show menuitem-7)




	
	;; ---------------------------------------------------------------------
	;;     Post MR
	;;
	;; ---------------------------------------------------------------------
	
	(add-simple-coot-menu-menuitem
	 submenu-all-molecule "[Post MR] Fill Partial Residues..."
	 (lambda ()
	   (molecule-chooser-gui "Find and Fill residues with missing atoms"
				 (lambda (imol)
				   (fill-partial-residues imol)))))

	(add-simple-coot-menu-menuitem
	 submenu-all-molecule "[Post MR] Fit Protein..."
	 (lambda ()
	   (molecule-chooser-gui "Fit Protein using Rotamer Search"
				 (lambda (imol)
				   (if (not (= (imol-refinement-map) -1))
				       (fit-protein imol)
				       (let ((s "oops.  Must set a map to fit"))
					 (add-status-bar-text s)))))))

	(add-simple-coot-menu-menuitem 
	 submenu-all-molecule "[Post MR] Stepped Refine..." 
	 (lambda ()
	   (molecule-chooser-gui "Stepped Refine: " 
				 (lambda (imol) 
				   (if (not (= (imol-refinement-map) -1))
				       (stepped-refine-protein imol)
				       (let ((s  "oops.  Must set a map to fit"))
					 (add-status-bar-text s)))))))

	(add-simple-coot-menu-menuitem 
	 submenu-all-molecule "Refine/Improve Ramachandran Plot..." 
	 (lambda ()
	   (molecule-chooser-gui "Refine Protein with Ramachanran Plot Optimization: "
				 (lambda (imol) 
				   (if (not (= (imol-refinement-map) -1))
				       (stepped-refine-protein-for-rama imol)
				       (let ((s  "oops.  Must set a map to fit"))
					 (add-status-bar-text s)))))))


	;; ---------------------------------------------------------------------
	;;     Map functions
	;;
	;; ---------------------------------------------------------------------
	(add-simple-coot-menu-menuitem
	 submenu-maps "Mask Map by Atom Selection..."
	 (lambda ()
	   (molecule-chooser-gui "Define the molecule that has atoms to mask the map"
				 (lambda (imol)
				   (generic-multiple-entries-with-check-button
				    (list
				     (list
				      " Map molecule number: "
				      (let f ((molecule-list (molecule-number-list)))
					(cond 
					 ((null? molecule-list) "")
					 ((= 1 (is-valid-map-molecule (car molecule-list)))
					  (number->string (car molecule-list)))
					 (else 
					  (f (cdr molecule-list))))))
				     (list 
				      " Atom selection: " 
				      "//A/1")
				     (list 
				      "Radius around atoms: "
				      (let ((rad  (map-mask-atom-radius)))
					(if (> rad 0)
					    (number->string rad)
					    "default"))))
				    (list
				     " Invert Masking? "
				     (lambda (active-state) 
				       (format #t "changed active state to ~s~%" active-state)))
				    "  Mask Map  "
				    (lambda (texts-list invert-mask?)
				      (let* ((text-1 (car texts-list))
					     (text-2 (car (cdr texts-list)))
					     (text-3 (car (cdr (cdr texts-list))))
					     (n (string->number text-1))
					     (invert (if invert-mask? 1 0)))
					(format #t "debug:: invert-mask? is ~s~%" invert-mask?)
					(let ((new-radius (if (string=? text-3 "default")
							      #f
							      (string->number text-3))))
					  (if (number? new-radius)
					      (set-map-mask-atom-radius new-radius)))
					(if (number? n)
					    (mask-map-by-atom-selection n imol text-2 invert)))))))))


	(add-simple-coot-menu-menuitem
	 submenu-maps "Copy Map Molecule...."
	 (lambda ()
	   (map-molecule-chooser-gui "Molecule to Copy..."
				     (lambda (imol)
				       (copy-molecule imol)))))

	(add-simple-coot-menu-menuitem
	 submenu-maps "Make a Difference Map..."
	 (lambda () (make-difference-map-gui)))

	(add-simple-coot-menu-menuitem
	 submenu-maps "Transform map by LSQ model fit..."
	 (lambda () (transform-map-using-lsq-matrix-gui)))

	(add-simple-coot-menu-menuitem
	 submenu-maps "Export map..."
	 (lambda ()
	   (generic-chooser-and-file-selector
	    "Export Map: " 
	    valid-map-molecule?
	    "File-name: " 
	    "" 
	    (lambda (imol text)
	      (let ((export-status (export-map imol text)))
		(if (= export-status 1)
		    (let ((s (string-append 
			      "Map " (number->string imol)
			      " exported to " text)))
		      (add-status-bar-text s))))))))


	(add-simple-coot-menu-menuitem
	 submenu-maps "Brighten Maps"
	 (lambda ()
	   (brighten-maps)))

	(add-simple-coot-menu-menuitem
	 submenu-maps "Set map is a difference map..."
	 (lambda ()
	   (map-molecule-chooser-gui "Which map should be considered a difference map?"
				     (lambda (imol)
				       (format #t "setting map number ~s to be a difference map~%"
					       imol)
				       (set-map-is-difference-map imol)))))


	(add-simple-coot-menu-menuitem
	 submenu-maps "Another level..."
	 (lambda ()
	   (another-level)))

	(add-simple-coot-menu-menuitem
	 submenu-maps "Multi-chicken..."
	 (lambda ()
	   (map-molecule-chooser-gui "Choose a molecule for multiple contouring"
				     (lambda (imol)
				       (set-map-displayed imol 0)
				       (multi-chicken imol)))))



	;; 
	;; ---------------------------------------------------------------------
	;;     Molecule functions
	;;
	;; ---------------------------------------------------------------------
	(add-simple-coot-menu-menuitem
	 submenu-models "Copy Fragment..."
	 (lambda ()
	   (generic-chooser-and-entry "Create a new Molecule\nFrom which molecule shall we seed?"
				      "Atom selection for fragment"
				      "//A/1-10" 
				      (lambda (imol text)
					(new-molecule-by-atom-selection imol text)))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Copy Coordinates Molecule...."
	 (lambda ()
	   (molecule-chooser-gui "Molecule to Copy..."
				 (lambda (imol)
				   (copy-molecule imol)))))


	(add-simple-coot-menu-menuitem submenu-models "Residue Type Selection..."
				       (lambda ()
					 (generic-chooser-and-entry 
					  "Choose a molecule from which to select residues:"
					  "Residue Type:" ""
					  (lambda (imol text)
					    (new-molecule-by-residue-type-selection imol text)
					    (update-go-to-atom-window-on-new-mol)))))

	(add-simple-coot-menu-menuitem submenu-models "New Molecule by Sphere..."
				       (lambda ()
					 (generic-chooser-and-entry
					  "Choose a molecule from which to select a sphere of atoms:"
					  "Radius:" "10.0"
					  (lambda (imol text)
					    (let ((radius (string->number text)))
					      (if (number? radius)
						  (apply new-molecule-by-sphere-selection
							 imol
							 (append 
							  (rotation-centre) 
							  (list radius)))))))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Replace Fragment..."
	 (lambda ()
	   (molecule-chooser-gui "Define the molecule that needs updating"
				 (lambda (imol-base)
				   (generic-chooser-and-entry
				    "Molecule that contains the new fragment:"
				    "Atom Selection" "//"
				    (lambda (imol-fragment atom-selection-str)
				      (replace-fragment 
				       imol-base imol-fragment atom-selection-str)))))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Reorder Chains..."
	 (lambda () 
	   (molecule-chooser-gui "Sort Chain IDs in molecule:"
				 (lambda (imol)
				   (reorder-chains imol)))))
				    
	(add-simple-coot-menu-menuitem
	 submenu-models "Add Strand Here..."
	 (lambda ()
	   (place-strand-here-gui)))

	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Dock Sequence...")))
	  
	  (gtk-menu-item-set-submenu menuitem2 submenu)
	  (gtk-menu-append menu menuitem2)
	  (gtk-widget-show menuitem2)
	  
	  ;; 
	  (if (coot-has-pygtk?)
	      (add-simple-coot-menu-menuitem
	       submenu "Dock Sequence (py)..."
	       (lambda ()
		 (run-python-command "cootaneer_gui_bl()"))))
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Associate Sequence...."
	   (lambda ()
	     (generic-chooser-entry-and-file-selector 
	      "Associate Sequence to Model: "
	      valid-model-molecule?
	      "Chain ID"
	      ""
	      "Select PIR file"
	      (lambda (imol chain-id pir-file-name)
		(format #t "assoc seq: ~s ~s ~s~%" imol chain-id pir-file-name)
		(if (file-exists? pir-file-name)
		    (let ((seq-text 
			   (call-with-input-file pir-file-name
			     (lambda (port)
			       (let loop  ((lines '())
					   (line (read-line port)))
				 (cond
				  ((eof-object? line) 
				   (string-append-with-string (reverse lines) "\n"))
				  (else
				   (loop (cons line lines) (read-line port)))))))))
		      
		      (assign-pir-sequence imol chain-id seq-text)))))))

	  ;; only add this to the GUI if the python version is not available.
	  (if (not (coot-has-pygtk?))
	      (add-simple-coot-menu-menuitem
	       submenu "Dock sequence on this fragment..."
	       (lambda ()
		 (molecule-chooser-gui "Choose a molecule to apply sequence assignment"
				       (lambda (imol)
					 (cootaneer-gui imol)))))))



	(add-simple-coot-menu-menuitem
	 submenu-models "Renumber Waters..."
	 (lambda () 
	   (molecule-chooser-gui 
	    "Renumber waters of which molecule?"
	    (lambda (imol)
	      (renumber-waters imol)))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Alt Confs..."
	 (lambda ()
	   (molecule-chooser-gui
	    "Which molecule to check for Alt Confs?"
	    (lambda (imol)
	      (alt-confs-gui imol)))))
	
	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Missing Atoms..."
	 (lambda ()
	   (molecule-chooser-gui
	    "Which molecule to check for Missing Atoms?"
	    (lambda (imol)
	      (missing-atoms-gui imol)))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Cis Peptides Bonds..."
	 (lambda ()
	   (molecule-chooser-gui "Choose a molecule for checking for Cis Peptides" 
				 (lambda (imol)
				   (cis-peptides-gui imol)))))

	(add-simple-coot-menu-menuitem 
	 submenu-models "Phosphorylate this residue"
	 (lambda ()
	   (phosphorylate-active-residue)))
	
	(add-simple-coot-menu-menuitem
	 submenu-models "Use SEGIDs..."
	 (lambda () 
	   (molecule-chooser-gui 
	    "Exchange the Chain IDs, replace with SEG IDs"
	    (lambda (imol)
	      (exchange-chain-ids-for-seg-ids imol)))))

	(add-simple-coot-menu-menuitem 
	 submenu-ncs "Copy NCS Reside Range..."
	 (lambda ()
	   (generic-chooser-and-entry "Apply NCS Range from Master"
				      "Master Chain ID"
				      ""
				      (lambda (imol chain-id)
					(generic-double-entry 
					 "Start of Residue Number Range"
					 "End of Residue Number Range"
					 "" "" #f #f "Apply NCS Residue Range" 
					 (lambda (text1 text2 dum) 
					   (let ((r1 (string->number text1))
						 (r2 (string->number text2)))
					     (if (and (number? r1)
						      (number? r2))
						 (copy-residue-range-from-ncs-master-to-others
						  imol chain-id r1 r2)))))))))

	(add-simple-coot-menu-menuitem
	 submenu-ncs "Copy NCS Chain..."
	 (lambda ()
	   (generic-chooser-and-entry 
	    "Apply NCS edits from NCS Master Chain to Other Chains"
	    "Master Chain ID"
	    ""
	    (lambda (imol chain-id)
	      (let ((ncs-chains (ncs-chain-ids imol)))
		(if (null? ncs-chains)
		    (let ((s (string-append 
			      "You need to define NCS operators for molecule "
			      (number->string imol))))
		      (info-dialog s))
		    (copy-from-ncs-master-to-others imol chain-id)))))))
	
	(add-simple-coot-menu-menuitem
	 submenu-ncs "NCS ligands..."
	 (lambda ()
	   (ncs-ligand-gui)))

	;; ---------------------------------------------------------------------
	;;     Building
	;; ---------------------------------------------------------------------
	;; 

	;; Not this.  Instead fire up a new top level, where we have a molecule chooser, 
	;; an entry for the chain spec and a text window where can paste in a sequence.
	;; 
	;; No, that doesn't work either.  We need a set of pairs of entries
	;; and text boxes.
	;; 
	;; Hmm..

;      (add-simple-coot-menu-menuitem
;       menu "Cootaneer this fragment [try sequence assignment]"
;       (lambda ()
;	 (let ((imol-map (imol-refinement-map)))
;	   (if (= imol-map -1)
;	       (info-dialog "Need to assign a map to fit against")
;	       (let ((active-atom (active-residue)))
;		 (if (list? active-atom)
;		     (let ((imol     (list-ref active-atom 0))
;			   (chain-id (list-ref active-atom 1))
;			   (resno    (list-ref active-atom 2))
;			   (inscode  (list-ref active-atom 3))
;			   (at-name  (list-ref active-atom 4))
;			   (alt-conf (list-ref active-atom 5)))
;		       (cootaneer imol-map imol (list chain-id resno inscode 
;						      at-name alt-conf)))))))))


	;; ---------------------------------------------------------------------
	;;     Settings
	;; ---------------------------------------------------------------------
	;; 

	(if (coot-has-pygtk?)

	    (add-simple-coot-menu-menuitem
	     submenu-refine "Set Refinement Options (py)..."
	     (lambda ()
	       (run-python-command "refinement_options_gui()"))))

	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Peptide Restraints...")))
	  
	  (gtk-menu-item-set-submenu menuitem2 submenu) 
	  (gtk-menu-append submenu-refine menuitem2)
	  (gtk-widget-show menuitem2)
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Add Planar Peptide Restraints"
	   (lambda ()
	     (format #t "Planar Peptide Restraints added~%")
	     (add-planar-peptide-restraints)))
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Remove Planar Peptide Restraints"
	   (lambda ()
	     (format #t "Planar Peptide Restraints removed~%")
	     (remove-planar-peptide-restraints))))

	(add-simple-coot-menu-menuitem
	 submenu-refine "SHELXL Refine..."
	 (lambda ()

	   (let ((window (gtk-window-new 'toplevel))
		 (hbox (gtk-vbox-new #f 0))
		 (vbox (gtk-hbox-new #f 0))
		 (go-button (gtk-button-new-with-label "  Refine  "))
		 (cancel-button (gtk-button-new-with-label "  Cancel  "))
		 (entry-hint-text "HKL data filename \n(leave blank for default)")
		 (chooser-hint-text " Choose molecule for SHELX refinement  ")
		 (h-sep (gtk-hseparator-new)))

	     (gtk-container-add window hbox)
	     (let ((option-menu-mol-list-pair (generic-molecule-chooser 
					       hbox chooser-hint-text))
		   (entry (file-selector-entry hbox entry-hint-text)))
	       (gtk-signal-connect go-button "clicked"
				   (lambda () 
				     (let ((txt (gtk-entry-get-text entry))
					   (imol (apply get-option-menu-active-molecule 
							option-menu-mol-list-pair)))
				       (if (number? imol)
					   (if (= (string-length txt) 0)
					       (shelxl-refine imol)
					       (shelxl-refine imol txt)))
				       (gtk-widget-destroy window))))
	       (gtk-signal-connect cancel-button "clicked"
				   (lambda ()
				     (gtk-widget-destroy window)))

	       (gtk-box-pack-start hbox h-sep #f #f 2)
	       (gtk-box-pack-start hbox vbox #f #f 2)
	       (gtk-box-pack-start vbox go-button #t #f 0)
	       (gtk-box-pack-start vbox cancel-button #t #f 0)
	       (gtk-widget-show-all window)))))


	(if (coot-has-pygtk?)
	    (add-simple-coot-menu-menuitem
	     submenu-refine "Read Refmac logfile (py)..."
	     (lambda ()
	       (generic-chooser-and-file-selector "Read Refmac log file"
						  valid-model-molecule?
						  "Logfile name: " ""
						  (lambda (imol text)
						    (let ((cmd (string-append "read_refmac_log("
									      (number->string imol)
									      ", \"" text "\")")))
						      (run-python-command cmd)))))))

	
	;; An example with a submenu:
	;; 
	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Refinement Speed...")))

	  (gtk-menu-item-set-submenu menuitem2 submenu) 
	  (gtk-menu-append submenu-refine menuitem2)
	  (gtk-widget-show menuitem2)

	  (add-simple-coot-menu-menuitem
	   submenu "Molasses Refinement mode"
	   (lambda ()
	     (format #t "Molasses...~%")
	     (set-dragged-refinement-steps-per-frame 4)))

	  (add-simple-coot-menu-menuitem
	   submenu "Crocodile Refinement mode"
	   (lambda ()
	     (format #t "Crock...~%")
	     (set-dragged-refinement-steps-per-frame 120)))

	  (add-simple-coot-menu-menuitem
	   submenu "Default Refinement mode"
	   (lambda ()
	     (format #t "Default Speed...~%")
	     (set-dragged-refinement-steps-per-frame 50))))


	(add-simple-coot-menu-menuitem
	 submenu-refine "Set Undo Molecule..."
	 (lambda () 
	   (molecule-chooser-gui "Set the Molecule for \"Undo\" Operations"
				 (lambda (imol)
				   (set-undo-molecule imol)))))

	(add-simple-coot-menu-menuitem submenu-refine "B factor bonds scale factor..."
				       (lambda ()
					 (generic-chooser-and-entry 
					  "Choose a molecule to which the b-factor colour scale is applied:"
					  "B factor scale:" "1.0"
					  (lambda (imol text)
					    (let ((n (string->number text)))
					      (if (number? n)
						  (set-b-factor-bonds-scale-factor imol n)))))))

	(add-simple-coot-menu-menuitem
	 submenu-refine "Set Matrix (Refinement Weight)..."
	 (lambda ()
	   (generic-single-entry "set matrix: (smaller means better geometry)" 
				 (number->string (matrix-state))
				 "  Set it  " (lambda (text) 
						(let ((t (string->number text)))
						  (if (number? t)
						      (begin
							(let ((s (string-append "Matrix set to " text)))
							  (set-matrix t)
							  (add-status-bar-text s)))
						      (begin
							(add-status-bar-text 
							 "Failed to read a number"))))))))
	
	(add-simple-coot-menu-menuitem
	 submenu-refine "Set Density Fit Graph Weight..."
	 (lambda ()
	   (generic-single-entry "set scale factor (smaller number means smaller bars)" 
				 "1.0"
				 "Set it" (lambda (text) 
					    (let ((t (string->number text)))
					      (if (number? t)
						  (begin
						    (let ((s (string-append 
							      "Density Fit scale factor set to " 
							      text)))
						      (set-residue-density-fit-scale-factor t)
						      (add-status-bar-text s)))
						  (begin
						    (add-status-bar-text 
						     "Failed to read a number"))))))))


	

	;; ---------------------------------------------------------------------
	;;     Views/Representations
	;; ---------------------------------------------------------------------
	;; 

	(add-simple-coot-menu-menuitem 
	 submenu-representation "Set Spin Speed..."
	 (lambda ()
	   (generic-single-entry "Set Spin Speed (smaller is slower)"
				 (number->string (idle-function-rotate-angle))
				 "Set it" (lambda (text)
					    (let ((t (string->number text)))
					      (if (number? t)
						  (set-idle-function-rotate-angle t)))))))

	(add-simple-coot-menu-menuitem
	 submenu-representation "Ball & Stick..."
	 (lambda ()
	   (generic-chooser-and-entry "Ball & Stick"
				      "Atom Selection:"
				      "//A/1-2"
				      (lambda (imol text)
					(let ((handle (make-ball-and-stick imol text 0.14 0.3 1)))
					  (format #t "handle: ~s~%" handle))))))

	(add-simple-coot-menu-menuitem
	 submenu-representation "Clear Ball & Stick..."
	 (lambda () 
	   (molecule-chooser-gui "Choose a molecule from which to clear Ball&Stick objects"
				 (lambda (imol)
				   (clear-ball-and-stick imol)))))

	
	(add-simple-coot-menu-menuitem
	 submenu-representation "Hilight Interesting Site (here)..."
	 (lambda ()
	   
	   (let ((active-atom (active-residue)))
	     (if active-atom
		 (let ((imol (car active-atom))
		       (centre-residue-spec 
			(list
			 (list-ref active-atom 1)
			 (list-ref active-atom 2)
			 (list-ref active-atom 3))))
		   (hilight-binding-site imol centre-residue-spec 230 4))))))



	(add-simple-coot-menu-menuitem
	 submenu-representation "Dotted Surface..."
	 (lambda ()
	   (generic-chooser-and-entry "Surface for molecule"
				      "Atom Selection:"
				      "//A/1-2"
				      (lambda (imol text)
					(let ((dots-handle (dots imol text 1 1)))
					  (format #t "dots handle: ~s~%" dots-handle))))))

	(add-simple-coot-menu-menuitem
	 submenu-representation "Clear Surface Dots..."
	 (lambda ()
	   (generic-chooser-and-entry "Molecule with Dotted Surface"
				      "Dots Handle Number:"
				      "0"
				      (lambda (imol text)
					(let ((n (string->number text)))
					  (if (number? n)
					      (clear-dots imol n)))))))
	(add-simple-coot-menu-menuitem
	 submenu-representation "Label All CAs..."
	 (lambda ()
	   (molecule-chooser-gui "Choose a molecule to label"
				 (lambda (imol)
				   (label-all-CAs imol)))))


	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Views")))
	  
	  (gtk-menu-item-set-submenu menuitem2 submenu) 
	  (gtk-menu-append menu menuitem2)
	  (gtk-widget-show menuitem2)
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Add View..."
	   (lambda ()
	     (view-saver-gui)))

	  
	  (add-simple-coot-menu-menuitem
	   submenu "Add a Spin View"
	   (lambda ()
	     (generic-double-entry "Number of Step" "Number of Degrees (total)"
				   "3600" "360" 
				   #f #f ; check button text and callback 
				   "  Add Spin  " 
				   (lambda (text1 text2 dummy)
				     (let ((n1 (string->number text1))
					   (n2 (string->number text2)))
				       
				       (if (and (number? n1)
						(number? n2))
					   (let* ((view-name "Spin")
						  (new-view-number (add-spin-view view-name n1 n2)))
					     (add-view-to-views-panel view-name new-view-number))))))))

	  (add-simple-coot-menu-menuitem 
	   submenu "Views Panel..."
	   (lambda ()
	     (views-panel-gui)))

	  (add-simple-coot-menu-menuitem
	   submenu "Play Views"
	   (lambda () 
	     (go-to-first-view 1)
	     (sleep 1)
	     (play-views)))

	  (add-simple-coot-menu-menuitem
	   submenu "Set Views Play Speed..."
	   (lambda ()
	     (generic-single-entry "Set Views Play Speed" 
				   (number->string (views-play-speed))
				   "  Set it " 
				   (lambda (text)
				     (let ((n (string->number text)))
				       (if (number? n)
					   (set-views-play-speed n)))))))

	  (add-simple-coot-menu-menuitem
	   submenu "Save Views..."
	   (lambda ()
	     (generic-single-entry "Save Views" "coot-views.scm" " Save "
				   (lambda (txt)
				     (save-views txt))))))


	;; ---------------------------------------------------------------------
	;;     Other Representation Programs
	;; ---------------------------------------------------------------------

	(add-simple-coot-menu-menuitem
	 submenu-representation "CCP4MG..."
	 (lambda ()
	   (let ((pd-file-name "1.mgpic.py"))
	     (write-ccp4mg-picture-description pd-file-name)
	     (if (command-in-path? "ccp4mg")
		 (run-concurrently "ccp4mg" "-pict" pd-file-name)))))


	;; ---------------------------------------------------------------------
	;;     Settings
	;; ---------------------------------------------------------------------
	;; 
	;; 
	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Rotate Translate Zone Mode...")))
	  
	  (gtk-menu-item-set-submenu menuitem2 submenu) 
	  (gtk-menu-append submenu-settings menuitem2)
	  (gtk-widget-show menuitem2)

	  (add-simple-coot-menu-menuitem
	   submenu "Rotate About Fragment Centre"
	   (lambda ()
	     (set-rotate-translate-zone-rotates-about-zone-centre 1)))

	  (add-simple-coot-menu-menuitem
	   submenu "Rotate About Second Clicked Atom"
	   (lambda ()
	     (set-rotate-translate-zone-rotates-about-zone-centre 0))))

	(add-simple-coot-menu-menuitem
	 submenu-settings "Nudge Centre..."
	 (lambda ()
	   (nudge-screen-centre-gui)))

	(add-simple-coot-menu-menuitem
	 submenu-refine "All Molecules use \"Near Chains\" Symmetry"
	 (lambda ()
	   (for-each (lambda (imol)
		       (if (valid-model-molecule? imol)
			   (set-symmetry-whole-chain imol 1)))
		     (molecule-number-list))))

	(add-simple-coot-menu-menuitem
	 submenu-refine "Question Accept Refinement"
	 (lambda ()
	   (set-refinement-immediate-replacement 0)))


	(add-simple-coot-menu-menuitem
	 submenu-settings "Save Dialog Positions..."
	 (lambda ()
	   (post-model-fit-refine-dialog)
	   (post-go-to-atom-window)
	   
	   (let* ((window (gtk-window-new 'toplevel))
		  (label-text (string-append
			       "   When happy, press \"Save\" to save   \n"
			       "   dialog positions"))
		  (label (gtk-label-new label-text))
		  (h-sep (gtk-hseparator-new))
		  (cancel-button (gtk-button-new-with-label "  Cancel  "))
		  (go-button (gtk-button-new-with-label "  Save  "))
		  (vbox (gtk-vbox-new #f 4))
		  (hbox (gtk-hbox-new #f 4)))
	     
	     (gtk-box-pack-start hbox     go-button #f #f 6)
	     (gtk-box-pack-start hbox cancel-button #f #f 6)
	     (gtk-box-pack-start vbox label     #f #f 6)
	     (gtk-box-pack-start vbox h-sep     #f #f 6)
	     (gtk-box-pack-start vbox hbox      #f #f 6)
	     (gtk-container-add window vbox)
	     
	     (gtk-signal-connect go-button "clicked" 
				 (lambda () 
				   (save-dialog-positions-to-init-file)
				   (gtk-widget-destroy window)))
	     (gtk-signal-connect cancel-button "clicked" 
				 (lambda ()
				   (gtk-widget-destroy window)))

	     (gtk-widget-show-all window))))


	(add-simple-coot-menu-menuitem
	 submenu-settings "Key Bindings..."
	 (lambda ()
	   (key-bindings-gui)))
	

	
	))) ;  finish let and if


;; This is not working right.
;;                           
;(let ((menu (coot-menubar-menu "Validate")))

;  (add-simple-coot-menu-menuitem 
;   menu "Pukka Puckers...?"
;   (lambda()
;     (molecule-chooser-gui "Choose a molecule for ribose pucker analysis"
;			   (lambda (imol)
;			     (pukka-puckers? imol))))))

  
