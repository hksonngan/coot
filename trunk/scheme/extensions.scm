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

    ;; --------------------------------------------------
    ;;           coot news dialog and updates dialog
    ;; --------------------------------------------------
    (let ((menu (coot-menubar-menu "About")))
      (if menu
	  (begin
	    (add-simple-coot-menu-menuitem
	     menu "Coot News..."
	     (lambda ()
	       (whats-new-dialog)))
	    (let ((os-type (vector-ref (uname) 0)))
	      (if (not (string=? os-type "Darwin"))
		  (add-simple-coot-menu-menuitem
		   menu "Check for Updates..."
		   (lambda () 
		     (format #t "checking for updates....~%")
		     (check-for-updates-gui)))))))))


(if (defined? 'coot-main-menubar)
    ;; --------------------------------------------------
    ;;           coordinated water validation dialog
    ;; --------------------------------------------------
    (let ((menu (coot-menubar-menu "Validate")))
      (add-simple-coot-menu-menuitem 
       menu "Highly coordinated waters..."
       (lambda ()
	 (water-coordination-gui)))))



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
	    (submenu-pisa (gtk-menu-new))
	    (menuitem-pisa (gtk-menu-item-new-with-label "PISA Assemblies..."))
	    (submenu-plugins (gtk-menu-new))
	    (menuitem-plugins (gtk-menu-item-new-with-label "Plug-ins..."))
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

	(gtk-menu-item-set-submenu menuitem-pisa submenu-pisa)
	(gtk-menu-append menu menuitem-pisa)
	(gtk-widget-show menuitem-pisa)

	(gtk-menu-item-set-submenu menuitem-7 submenu-settings)
	(gtk-menu-append menu menuitem-7)
	(gtk-widget-show menuitem-7)

	(gtk-menu-item-set-submenu menuitem-plugins submenu-plugins)
	(gtk-menu-append menu menuitem-plugins)
	(gtk-widget-show menuitem-plugins)



	
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


; old style - not interruptable.
;
; 	(add-simple-coot-menu-menuitem
; 	 submenu-all-molecule "[Post MR] Fit Protein..."
; 	 (lambda ()
; 	   (molecule-chooser-gui "Fit Protein using Rotamer Search"
; 				 (lambda (imol)
; 				   (if (not (= (imol-refinement-map) -1))
; 				       (fit-protein imol)
; 				       (let ((s "oops.  Must set a map to fit"))
; 					 (add-status-bar-text s)))))))

; 	(add-simple-coot-menu-menuitem 
; 	 submenu-all-molecule "[Post MR] Stepped Refine..." 
; 	 (lambda ()
; 	   (molecule-chooser-gui "Stepped Refine: " 
; 				 (lambda (imol) 
; 				   (if (not (= (imol-refinement-map) -1))
; 				       (stepped-refine-protein imol)
; 				       (let ((s  "oops.  Must set a map to fit"))
; 					 (add-status-bar-text s)))))))

; 	(add-simple-coot-menu-menuitem 
; 	 submenu-all-molecule "Refine/Improve Ramachandran Plot..." 
; 	 (lambda ()
; 	   (molecule-chooser-gui "Refine Protein with Ramachandran Plot Optimization: "
; 				 (lambda (imol) 
; 				   (if (not (= (imol-refinement-map) -1))
; 				       (stepped-refine-protein-for-rama imol)
; 				       (let ((s  "oops.  Must set a map to fit"))
; 					 (add-status-bar-text s)))))))


	(add-simple-coot-menu-menuitem
	 submenu-all-molecule "Fit Protein..." 
	 (lambda ()
	   (molecule-chooser-gui "Fit Protein using Rotamer Search"
				 (lambda (imol)
				   (if (not (= (imol-refinement-map) -1))
				       (begin
					 (set! *continue-multi-refine* #t)
					 (interruptible-fit-protein imol fit-protein-fit-function))
				       (let ((s "oops.  Must set a map to fit"))
					 (add-status-bar-text s)))))))

	(add-simple-coot-menu-menuitem
	 submenu-all-molecule "Stepped Refine..." 
	 (lambda ()
	   (molecule-chooser-gui "Fit Protein using Real-Space Refinement"
				 (lambda (imol)
				   (if (not (= (imol-refinement-map) -1))
				       (begin
					 (set! *continue-multi-refine* #t)
					 (interruptible-fit-protein imol fit-protein-stepped-refine-function))
				       (let ((s "oops.  Must set a map to fit"))
					 (add-status-bar-text s)))))))

	(add-simple-coot-menu-menuitem
	 submenu-all-molecule "Refine/Improve Ramachandran Plot..."
	 (lambda ()
	   (molecule-chooser-gui "Refine Protein with Ramachandran Plot Optimization: "
				 (lambda (imol)
				   (if (not (= (imol-refinement-map) -1))
				       (begin
					 (set! *continue-multi-refine* #t)
					 (interruptible-fit-protein imol fit-protein-rama-fit-function))
				       (let ((s "oops.  Must set a map to fit"))
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
	 submenu-maps "Average Maps..."
	 average-map-gui)
	

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
	 submenu-maps "Export Local Map Fragment..." ;;  (for Pymol, say)
	 (lambda ()
	   (generic-chooser-entry-and-file-selector
	    "Export Map: "
	    valid-map-molecule?
	    "Radius (A): " "10"
	    "File-name: "
	    (lambda (imol radius-string file-name)
	      (let ((radius (string->number radius-string)))
		(if (number? radius)
		    (apply export-map-fragment (append (cons imol (rotation-centre)) 
						       (list radius file-name)))))))))


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
	;;     Molecule functions/Modelling
	;;
	;; ---------------------------------------------------------------------

	(add-simple-coot-menu-menuitem
	 submenu-models "Add Other Solvent Molecules..."
	 (lambda ()
	   (solvent-ligands-gui)))
				    
	(add-simple-coot-menu-menuitem
	 submenu-models "Arrange Waters Around Protein..."
	 (lambda ()
	   (molecule-chooser-gui "Arrange waters in molecule: "
				 (lambda (imol)
				   (move-waters-to-around-protein imol)))))


      (add-simple-coot-menu-menuitem 
       submenu-models "Assign (force) HETATMs for this Residue"
       (lambda ()
	 (using-active-atom
	  (hetify-residue aa-imol aa-chain-id aa-res-no aa-ins-code))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Assign HETATM to molecule..."
	 (lambda() 
	   (molecule-chooser-gui "Assign HETATMs as per PDB definition"
				 (lambda (imol)
					  (assign-hetatms imol)))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Copy Coordinates Molecule...."
	 (lambda ()
	   (molecule-chooser-gui "Molecule to Copy..."
				 (lambda (imol)
				   (copy-molecule imol)))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Copy Fragment..."
	 (lambda ()
	   (generic-chooser-and-entry "Create a new Molecule\nFrom which molecule shall we seed?"
				      "Atom selection for fragment"
				      "//A/1-10" 
				      (lambda (imol text)
					(new-molecule-by-atom-selection imol text)))))

	;; --- D --- 

	(add-simple-coot-menu-menuitem submenu-models "DB Loop..." click-protein-db-loop-gui)


	;; errr... move this...
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
	     (associate-pir-with-molecule-gui #f))) ;; don't do alignement gui on OK press

	  ;; only add this to the GUI if the python version is not available.
	  (if (not (coot-has-pygtk?))
	      (add-simple-coot-menu-menuitem
	       submenu "Dock sequence on this fragment..."
	       (lambda ()
		 (molecule-chooser-gui "Choose a molecule to apply sequence assignment"
				       (lambda (imol)
					 (cootaneer-gui imol)))))))

	;; ---- F ---------

	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "Find Secondary Structure...")))

	  (gtk-menu-item-set-submenu menuitem2 submenu)
	  (gtk-menu-append submenu-models menuitem2)
	  (gtk-widget-show menuitem2)

	  (add-simple-coot-menu-menuitem
	   submenu "Find Helices"
	   (lambda ()
	     (find-helices)))
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Find Strands"
	   (lambda ()
	     (find-strands))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Fix Nomenclature Errors..."
	 (lambda () 
	   (molecule-chooser-gui "Fix Nomenclature Error in molecule:"
				 (lambda (imol)
				   (fix-nomenclature-errors imol)))))

	;; ---- I ---------

	(add-simple-coot-menu-menuitem 
	 submenu-models "Invert This Chiral Centre"
	 (lambda ()
	   (chiral-centre-inverter)))

	
	;; ---- J ---------

	(add-simple-coot-menu-menuitem 
	 submenu-models "JLigand launch"
	 (lambda ()
	   (launch-jligand-function)))

	;; ---- M ---------

	(add-simple-coot-menu-menuitem
	 submenu-models "Make Link (click 2 atoms)..."
	 (lambda () 
	   (user-defined-click 2
			       (lambda (atom-specs)
				 (let ((m-spec-1 (car atom-specs))
				       (m-spec-2 (cadr atom-specs)))
				   (let ((imol-1 (atom-spec->imol m-spec-1))
					 (imol-2 (atom-spec->imol m-spec-2))
					 (spec-1 (cddr m-spec-1))
					 (spec-2 (cddr m-spec-2)))
				     
				     (if (not (= imol-1 imol-2))
					 (format #t "Mismatch molecules~%")
					 (make-link imol-1 spec-1 spec-2 "dummy" 0.1))))))))



	(add-simple-coot-menu-menuitem 
	 submenu-models "Merge Water Chains..."
	 (lambda ()
	   (molecule-chooser-gui "Merge Water Chains in molecule: " 
				 (lambda (imol)
				   (merge-solvent-chains imol)))))


	(add-simple-coot-menu-menuitem
	 submenu-models "Monomer from Dictionary..."
	 (lambda ()
	   (generic-single-entry "Pull coordinates from CIF dictionary for 3-letter-code:" "" 
				 " Get Coords " 
				 (lambda (text) 
				   (let ((idealized? 0))
				     (let ((new-model (get-monomer-from-dictionary text idealized?)))
				       (if (valid-model-molecule? new-model)
					   new-model
					   ;; 
					   (get-monomer text))))))))

	;; ---- N ---------

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
							  (list radius 0)))))))))

	(add-simple-coot-menu-menuitem submenu-models "New Molecule from Symmetry Op..."
				       (lambda ()
					 (generic-chooser-and-entry
					  "Molecule from which to generate a symmetry copy"
					  "SymOp" "X,Y,Z"
					  (lambda (imol text)
					    (let ((pre-shift (origin-pre-shift imol)))
					      (if (not (list? pre-shift))
						  (format #t "bad pre-shift aborting")
						  
						  (new-molecule-by-symop
						   imol text 
						   (inexact->exact (list-ref pre-shift 0))
						   (inexact->exact (list-ref pre-shift 1))
						   (inexact->exact (list-ref pre-shift 2)))))))))


	;; ---- P ---------

	(add-simple-coot-menu-menuitem 
	 submenu-models "Phosphorylate this residue"
	 (lambda ()
	   (phosphorylate-active-residue)))
	
	(add-simple-coot-menu-menuitem
	 submenu-models "Prodrg-ify this residue (generate restraints)"
	 (lambda ()
	   (using-active-atom 
	    (prodrg-ify aa-imol aa-chain-id aa-res-no aa-ins-code))))


	;; ---- R ---------

	;; --- Ren --- 

	(add-simple-coot-menu-menuitem
	 submenu-models "Rename Residue..."
	 (lambda ()
	   (rename-residue-gui)))

	(add-simple-coot-menu-menuitem
	 submenu-models "Renumber Waters..."
	 (lambda () 
	   (molecule-chooser-gui 
	    "Renumber waters of which molecule?"
	    (lambda (imol)
	      (renumber-waters imol)))))

	;; --- Reo --- 

	(add-simple-coot-menu-menuitem
	 submenu-models "Reorder Chains..."
	 (lambda () 
	   (molecule-chooser-gui "Sort Chain IDs in molecule:"
				 (lambda (imol)
				   (sort-chains imol))))) ;; an internal function

	;; --- Rep --- 


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
	 submenu-models "Replace Residue..."
	 (lambda ()
	   (generic-single-entry "Replace this residue with residue of type:" "ALA" "Mutate" 
				 (lambda (text) 
				   (using-active-atom 
				    (mutate-by-overlap aa-imol aa-chain-id aa-res-no text))))))


	;; --- Res --- 

	(add-simple-coot-menu-menuitem submenu-models "Residue Type Selection..."
				       (lambda ()
					 (generic-chooser-and-entry 
					  "Choose a molecule from which to select residues:"
					  "Residue Type:" ""
					  (lambda (imol text)
					    (new-molecule-by-residue-type-selection imol text)
					    (update-go-to-atom-window-on-new-mol)))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Alt Confs..."
	 (lambda ()
	   (molecule-chooser-gui
	    "Which molecule to check for Alt Confs?"
	    (lambda (imol)
	      (alt-confs-gui imol)))))
	
	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Cis Peptides Bonds..."
	 (lambda ()
	   (molecule-chooser-gui "Choose a molecule for checking for Cis Peptides" 
				 (lambda (imol)
				   (cis-peptides-gui imol)))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Residues with Missing Atoms..."
	 (lambda ()
	   (molecule-chooser-gui
	    "Which molecule to check for Missing Atoms?"
	    (lambda (imol)
	      (missing-atoms-gui imol)))))


	;; --- Rid --- 

	(add-simple-coot-menu-menuitem 
	 submenu-models "Rigid Body Fit Residue Ranges..."
	 (lambda ()
	   (let ((func (lambda (imol ls) 
			 (rigid-body-refine-by-residue-ranges imol ls))))
	     (residue-range-gui func "Rigid Body Refine" "  Fit  "))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Rigid Body Fit Molecule..."
	 (lambda () 
	   (molecule-chooser-gui
	    "Rigid Body Fit Molecule"
	    (lambda (imol)
	      (rigid-body-refine-by-atom-selection imol "//")))))
      
	(add-simple-coot-menu-menuitem
	 submenu-models "Superpose ligands..."
	 (lambda ()
	    (superpose-ligand-gui)))

	;; ---- U ---------

	(add-simple-coot-menu-menuitem
	 submenu-models "Use \"Backrub\" Rotamers"
	 (lambda ()
	   (set-rotamer-search-mode (ROTAMERSEARCHLOWRES))))

	(add-simple-coot-menu-menuitem
	 submenu-models "Use SEGIDs..."
	 (lambda () 
	   (molecule-chooser-gui 
	    "Exchange the Chain IDs, replace with SEG IDs"
	    (lambda (imol)
	      (exchange-chain-ids-for-seg-ids imol)))))

	;; ---- W ---------

	(add-simple-coot-menu-menuitem
	 submenu-models "What's this?"
	 (lambda ()
	   (using-active-atom
	    (let* ((central-residue (active-residue))
		   (res-name (apply residue-name (list-head central-residue 4)))
		   (mol-no (car central-residue))
		   (n (comp-id->name res-name))
		   (s (string-append "(mol. no: " (number->string mol-no) ")  "
				     res-name ":  "
				     (if (string? n) n " <no-name-found>"))))
	      (add-status-bar-text s)))))
	    


	;; ---------------------------------------------------------------------
	;;     NCS functions
	;;
	;; ---------------------------------------------------------------------
	
	(add-simple-coot-menu-menuitem 
	 submenu-ncs "Copy NCS Residue Range..."
	 (lambda ()

	   (generic-chooser-and-entry "Apply NCS Range from Master"
				      "Master Chain ID"
				      (get-first-ncs-master-chain) ;; returns "" on fail
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
	    (get-first-ncs-master-chain) ;; can return  "".
	    (lambda (imol chain-id)
	      (let ((ncs-chains (ncs-chain-ids imol)))
		(if (null? ncs-chains)
		    (let ((s (string-append 
			      "You need to define NCS operators for molecule "
			      (number->string imol))))
		      (info-dialog s))
		    (copy-from-ncs-master-to-others imol chain-id)))))))

	(add-simple-coot-menu-menuitem
	 submenu-ncs "NCS Ghosts by Residue Range..."
	 (lambda ()
	   (molecule-chooser-gui 
	    "Make local NCS ghosts for molecule:"
	    (lambda (imol)
	      (let ((label-1 "Start ResNumber")
		    (label-2 "End ResNumber")
		    (entry-1-default-text "10")
		    (entry-2-default-text "20")
		    (go-button-label "Regenerate Ghosts")
		    (handle-go-function
		     (lambda (resno-1-text resno-2-text dummy)
		       (let ((resno-1 (string->number resno-1-text))
			     (resno-2 (string->number resno-2-text)))
			 (if (and (number? resno-1)
				  (number? resno-2))
			     (let ((ghost-ncs-chain-ids (ncs-chain-ids imol)))
			       (if (list? ghost-ncs-chain-ids)
				   ;; because we can have hetero-NCS,
				   ;; but we ignore NCS other that
				   ;; that of the first type.
				   (let ((ghost-chain-list (car ghost-ncs-chain-ids)))
				     (manual-ncs-ghosts imol resno-1 resno-2 ghost-chain-list)))))))))
		
		(generic-double-entry label-1 label-2 entry-1-default-text 
				      entry-2-default-text 
				      #f #f 
				      go-button-label handle-go-function))))))

	
	(add-simple-coot-menu-menuitem
	 submenu-ncs "NCS ligands..."
	 (lambda ()
	   (ncs-ligand-gui)))

        (let ((submenu (gtk-menu-new))
              (menuitem2 (gtk-menu-item-new-with-label "NCS matrix type...")))

          (gtk-menu-item-set-submenu menuitem2 submenu)
          (gtk-menu-append submenu-ncs menuitem2)
          (gtk-widget-show menuitem2)

          (add-simple-coot-menu-menuitem
           submenu "Accurate (SSM)"
           (lambda ()
             (set-ncs-matrix-type 0)))

          (add-simple-coot-menu-menuitem
           submenu "Fast (LSQ)"
           (lambda ()
             (set-ncs-matrix-type 1)))

          (add-simple-coot-menu-menuitem
           submenu "Extra Fast (LSQ, only every 2nd CA)"
           (lambda ()
             (set-ncs-matrix-type 2))))

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
	;;     Refine
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
	     (set-dragged-refinement-steps-per-frame 160)))

	  (add-simple-coot-menu-menuitem
	   submenu "Default Refinement mode"
	   (lambda ()
	     (format #t "Default Speed...~%")
	     (set-dragged-refinement-steps-per-frame 80))))

	(add-simple-coot-menu-menuitem
	 submenu-refine "Auto-weight refinement"
	 auto-weight-for-refinement)


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
				 (format #f "~2,2f" (residue-density-fit-scale-factor))
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
	;;     Recent structures from the PDBe
	;; ---------------------------------------------------------------------
	;;
	;; 20110921 too crashy at the moment (something to do with lots of threads?)
	;; 
	(add-simple-coot-menu-menuitem
	 menu "PDBe recent structures"
	 pdbe-latest-releases-gui)

	(add-simple-coot-menu-menuitem
	 menu "Get from PDBe"
	 (lambda () 
	   (generic-single-entry "Get PDBe accession code"
				 "" " Get it "
				 (lambda (text)
				   (pdbe-get-pdb-and-sfs-cif 'include-sfs text)))))


	;; ---------------------------------------------------------------------
	;;     Tutorial data
	;; ---------------------------------------------------------------------
	;; 
	(add-simple-coot-menu-menuitem 
	 menu "Load tutorial model and data"
	 (lambda ()
	   (let* ((prefix-dir (getenv "COOT_PREFIX")))

	     (let* ((pkg-data-dir
		     (if (string? prefix-dir)
			 (append-dir-dir (append-dir-dir prefix-dir "share") "coot")
			 (pkgdatadir)))
		    (data-dir (append-dir-dir pkg-data-dir "data"))
		    (pdb-file-name (append-dir-file data-dir "tutorial-modern.pdb"))
		    (mtz-file-name (append-dir-file data-dir "rnasa-1.8-all_refmac1.mtz")))

	       (read-pdb pdb-file-name)
	       (make-and-draw-map mtz-file-name "FWT" "PHWT" "" 0 0)
	       (make-and-draw-map mtz-file-name "DELFWT" "PHDELWT" "" 0 1)))))


	
	;; ---------------------------------------------------------------------
	;;     RCrane
	;; ---------------------------------------------------------------------
	;;  hack out until the python wrapper is committed.
	;; 
; 	(if (coot-has-pygtk?)
; 	    (run-python-command "import_rcrane_wrapper()"))
	     
	 
	;; ---------------------------------------------------------------------
	;;     LIDIA
	;; ---------------------------------------------------------------------
	;; 
	(if (coot-can-do-lidia-p)
	    (let ((submenu-lidia (gtk-menu-new))
		  (menuitem-lidia (gtk-menu-item-new-with-label "Lidia...")))

	      (gtk-menu-item-set-submenu menuitem-lidia submenu-lidia)
	      (gtk-menu-append menu menuitem-lidia)
	      (gtk-widget-show menuitem-lidia)


	      (add-simple-coot-menu-menuitem
	       submenu-lidia "Hydrogenate region"
	       (lambda () 
		 (hydrogenate-region 6)))
	      
	      (add-simple-coot-menu-menuitem
	       submenu-lidia "View in LIDIA"
	       (lambda ()
		 (using-active-atom (fle-view aa-imol aa-chain-id aa-res-no aa-ins-code))))

	      (add-simple-coot-menu-menuitem
	       submenu-lidia "Load SBase monomer..."
	       (lambda ()
		 (generic-single-entry "Load SBase Monomer from three-letter-code: " ""
				       " Load "
				       (lambda (tlc)
					 (get-sbase-monomer tlc)))))

	      (add-simple-coot-menu-menuitem
	       submenu-lidia "Activate prodrg flat mode"
	       (lambda ()
		 (using-active-atom 
		  (prodrg-flat aa-imol aa-chain-id aa-res-no))))))



	;; ---------------------------------------------------------------------
	;;     Views/Representations
	;; ---------------------------------------------------------------------
	;; 

	(add-simple-coot-menu-menuitem
	 submenu-representation "Undo Symmetry View" undo-symmetry-view)

	(add-simple-coot-menu-menuitem
	 submenu-representation "Ball & Stick..."
	 (lambda ()
	   (generic-chooser-and-entry "Ball & Stick"
				      "Atom Selection:"
				      *default-ball-and-stick-selection*
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
	 submenu-representation "Electrostatic Surface..."
	 (lambda () 
	   (molecule-chooser-gui (string-append 
				  "Choose a molecule to represent as a surface..."
				  "\n"
				  "Can be SLOW")
				 (lambda (imol)
				   (do-surface imol 1)))))


	(add-simple-coot-menu-menuitem 
	 submenu-representation "Clipped Surface Here (This Residue)"
	 (lambda ()
	   (using-active-atom
	    
	    (let* ((central-residue (active-residue))
		   (residues (residues-near-residue aa-imol (cdr central-residue) 6.0))
		   ;; no waters in surface, thanks.
		   (filtered-residues 
		    (filter (lambda (s) 
			      (not (string=? (apply residue-name (cons aa-imol s)) "HOH")))
			    residues))
		   (imol-copy (copy-molecule aa-imol)))
	      ;; delete the interesting residue from the copy (so that
	      ;; it is not surfaced).
	      (delete-residue imol-copy aa-chain-id aa-res-no aa-ins-code)
	      (do-clipped-surface imol-copy filtered-residues)))))

	(add-simple-coot-menu-menuitem
	 submenu-representation "Full Surface Around Here (This Residue)"
	 (lambda ()
	   (using-active-atom
	    
	    (let* ((central-residue (active-residue))
		   (residues (residues-near-residue aa-imol (cdr central-residue) 6.0))
		   (imol-copy (copy-molecule aa-imol)))
	      (delete-residue imol-copy aa-chain-id aa-res-no aa-ins-code)
	      (do-surface imol-copy 1)))))


	(add-simple-coot-menu-menuitem
	 submenu-representation "Un-Surface..."
	 (lambda () 
	   (molecule-chooser-gui "Choose a molecule to represent conventionally..."
				 (lambda (imol)
				   (do-surface imol 0)))))

	
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
					(let ((dots-handle (dots imol text text 1 1)))
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
	;;     3D annotations
	;; ---------------------------------------------------------------------
	
	(let ((submenu (gtk-menu-new))
	      (menuitem2 (gtk-menu-item-new-with-label "3D Annotations...")))
	  
	  (gtk-menu-item-set-submenu menuitem2 submenu)
	  (gtk-menu-append submenu-representation menuitem2)
	  (gtk-widget-show menuitem2)
	  
	  (add-simple-coot-menu-menuitem
	   submenu "Annotate position..."
	   (lambda ()
	     (generic-single-entry "Annotation: "  "" "Make Annotation" 
				   (lambda (txt)
				     (add-annotation-here txt)))))


	  (add-simple-coot-menu-menuitem
	   submenu "Save Annotations......"
	   (lambda ()
	     (generic-single-entry "Save Annotations" "coot-annotations.scm" " Save "
				   (lambda (file-name)
				     (save-annotations file-name)))))

	  (add-simple-coot-menu-menuitem
	   submenu "Load Annotations..."
	   (lambda ()
	     (generic-single-entry "Load Annotations" "coot-annotations.scm" " Load "
				   (lambda (file-name)
				     (load-annotations file-name))))))





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
	;;     PISA Interface
	;; ---------------------------------------------------------------------


      
	(add-simple-coot-menu-menuitem
	 submenu-pisa "PISA assemblies..." 
	 (lambda ()
	   (molecule-chooser-gui "Choose molecule for PISA assembly analysis"
				 (lambda (imol)
				   (pisa-assemblies imol)))))


	;; ---------------------------------------------------------------------
	;;     Plugins
	;; ---------------------------------------------------------------------


	(add-simple-coot-menu-menuitem
	 submenu-plugins "SHELX..." 
	 (lambda ()
	   (add-plugin-shelx)))



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
	 submenu-settings "Set Spin Speed..."
	 (lambda ()
	   (generic-single-entry "Set Spin Speed (smaller is slower)"
				 (number->string (idle-function-rotate-angle))
				 "Set it" (lambda (text)
					    (let ((t (string->number text)))
					      (if (number? t)
						  (set-idle-function-rotate-angle t)))))))

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
			       "   dialog positions\n"
			       "   (You will have to open and close the "
			       "Edit Chi Angles \n   yourself, if you want that)"
			       ))
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


      (add-simple-coot-menu-menuitem
       submenu-settings "Enable Quick-Save checkpointing..." 
       (lambda ()
	 (generic-single-entry
	  "Checkpoint interval (seconds)"
	  "30"
	  " Start Auto-saving "
	  (lambda (txt)
	    (let ((n (string->number txt)))
	      (if (number? n)
		  (gtk-timeout-add (* 1000 n) (lambda () (quick-save)))))))))

			  
	))) ;  finish let and if


;;                           
(let ((menu (coot-menubar-menu "Validate")))
  
  (add-simple-coot-menu-menuitem 
   menu "Pukka Puckers...?"
   (lambda()
     (molecule-chooser-gui "Choose a molecule for ribose pucker analysis"
			   (lambda (imol)
			     (pukka-puckers? imol)))))

  (add-simple-coot-menu-menuitem
   menu "Alignment vs PIR..."
   (lambda ()
     (molecule-chooser-gui "Alignment vs PIR info for molecule:"
			   (lambda (imol)
			     (wrapper-alignment-mismatches-gui imol))))))


