
(if (defined? 'coot-main-menubar)

    (let ((menu (coot-menubar-menu "Extensions")))

      (add-simple-coot-menu-menuitem
       menu "[Post MR] Fill Partial Residues..."
       (lambda ()
	 (molecule-chooser-gui "Find and Fill residues with missing atoms"
			       (lambda (imol)
				 (fill-partial-residues imol)))))

      (add-simple-coot-menu-menuitem
       menu "[Post MR] Fit Protein..."
       (lambda ()
	 (molecule-chooser-gui "Fit Protein using Rotamer Search"
			       (lambda (imol)
				 (if (not (= (imol-refinement-map) -1))
				     (fit-protein imol)
				     (let ((s "oops.  Must set a map to fit"))
				       (add-status-bar-text s)))))))

      (add-simple-coot-menu-menuitem 
       menu "[Post MR] Stepped Refine..." 
       (lambda ()
	 (molecule-chooser-gui "Stepped Refine: " 
			       (lambda (imol) 
				 (if (not (= (imol-refinement-map) -1))
				     (stepped-refine-protein imol)
				     (let ((s  "oops.  Must set a map to fit"))
				       (add-status-bar-text s)))))))

      (add-simple-coot-menu-menuitem menu "Residue Type Selection..."
				     (lambda ()
				       (generic-chooser-and-entry 
					"Choose a molecule from which to select residues:"
					"Residue Type:" ""
					(lambda (imol text)
					  (new-molecule-by-residue-type-selection imol text)
					  (update-go-to-atom-window-on-new-mol)))))


      (add-simple-coot-menu-menuitem
       menu "Mask Map by Atom Selection..."
       (lambda ()
	 (molecule-chooser-gui "Define the molecule that has atoms to mask the map"
			       (lambda (imol)
				 (generic-double-entry 
				  " Map molecule number: "
				  " Atom selection: " 
				  (let f ((molecule-list (molecule-number-list)))
				    (cond 
				     ((null? molecule-list) "")
				     ((= 1 (is-valid-map-molecule (car molecule-list)))
				      (number->string (car molecule-list)))
				     (else 
				      (f (cdr molecule-list)))))
				  "//A/1"
				  " Invert Masking? "
				  (lambda (active-state) 
				    (format #t "changed active state to ~s~%" active-state))
				  "  Mask Map  "
				  (lambda (text-1 text-2 invert-mask?)
				    (let ((n (string->number text-1))
					  (invert (if invert-mask? 1 0)))
				      (if (number? n)
					  (mask-map-by-atom-selection n imol text-2 invert)))))))))

      (add-simple-coot-menu-menuitem
       menu "Copy Coordinates Molecule...."
       (lambda ()
	 (molecule-chooser-gui "Molecule to Copy..."
			       (lambda (imol)
				 (copy-molecule imol)))))

      (add-simple-coot-menu-menuitem
       menu "Copy Map Molecule...."
       (lambda ()
	 (map-molecule-chooser-gui "Molecule to Copy..."
				   (lambda (imol)
				     (copy-molecule imol)))))

      (add-simple-coot-menu-menuitem
       menu "Copy Fragment..."
       (lambda ()
	 (generic-chooser-and-entry "Create a new Molecule\nFrom which molecule shall we seed?"
				    "Atom selection for fragment"
				    "//A/1-10" 
				    (lambda (imol text)
				      (new-molecule-by-atom-selection imol text)))))

      (add-simple-coot-menu-menuitem
       menu "Replace Fragment"
       (lambda ()
	 (molecule-chooser-gui "Define the molecule that needs updating"
			       (lambda (imol-base)
				 (generic-chooser-and-entry
				  "Molecule that contains the new fragment:"
				  "Atom Selection" "//"
				  (lambda (imol-fragment atom-selection-str)
				    (replace-fragment 
				     imol-base imol-fragment atom-selection-str)))))))


      (let ((submenu (gtk-menu-new))
	    (menuitem2 (gtk-menu-item-new-with-label "Peptide Restraints...")))
	
	(gtk-menu-item-set-submenu menuitem2 submenu) 
	(gtk-menu-append menu menuitem2)
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
       menu "Set Undo Molecule..."
       (lambda () 
	 (molecule-chooser-gui "Set the Molecule for \"Undo\" Operations"
			       (lambda (imol)
				 (set-undo-molecule imol)))))

      (add-simple-coot-menu-menuitem
       menu "Set map is a difference map..."
       (lambda ()
	 (map-molecule-chooser-gui "Which map should be considered a difference map?"
				   (lambda (imol)
				     (format #t "setting map number ~s to be a difference map~%"
					     imol)
				     (set-map-is-difference-map imol)))))

      (add-simple-coot-menu-menuitem
       menu "Use SEGIDs..."
       (lambda () 
	 (molecule-chooser-gui 
	  "Exchange the Chain IDs, replace with SEG IDs"
	  (lambda (imol)
	    (exchange-chain-ids-for-seg-ids imol)))))
      
      (add-simple-coot-menu-menuitem
       menu "Dotted Surface..."
       (lambda ()
	   (generic-chooser-and-entry "Surface for molecule"
				      "Atom Selection:"
				      "//A/1-2"
				      (lambda (imol text)
					(let ((dots-handle (dots imol text 1 1)))
					  (format #t "dots handle: ~s~%" dots-handle))))))

      (add-simple-coot-menu-menuitem
       menu "Clear Surface Dots..."
       (lambda ()
	 (generic-chooser-and-entry "Molecule with Dotted Surface"
				    "Dots Handle Number:"
				    "0"
				    (lambda (imol text)
				      (let ((n (string->number text)))
					(if (number? n)
					    (clear-dots imol n)))))))
      
      (add-simple-coot-menu-menuitem menu "B factor bonds scale factor..."
				     (lambda ()
				       (generic-chooser-and-entry 
					"Choose a molecule to which the b-factor colour scale is applied:"
					"B factor scale:" "1.0"
					(lambda (imol text)
					  (let ((n (string->number text)))
					    (if (number? n)
						(set-b-factor-bonds-scale-factor imol n)))))))

      (add-simple-coot-menu-menuitem
       menu "Renumber Waters..."
       (lambda () 
	 (molecule-chooser-gui 
	  "Renumber waters of which molecule?"
	  (lambda (imol)
	    (renumber-waters imol)))))

      (add-simple-coot-menu-menuitem
       menu "Residues with Alt Confs..."
       (lambda ()
	 (molecule-chooser-gui
	  "Which molecule to check for Alt Confs?"
	  (lambda (imol)
	    (alt-confs-gui imol)))))

      (add-simple-coot-menu-menuitem
       menu "Set Matrix (Refinement Weight)..."
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
       menu "Set Density Fit Graph Weight..."
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

      ;; An example with a submenu:
      ;; 
      (let ((submenu (gtk-menu-new))
	    (menuitem2 (gtk-menu-item-new-with-label "Refinement Speed...")))

	(gtk-menu-item-set-submenu menuitem2 submenu) 
	(gtk-menu-append menu menuitem2)
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

      ;; 
      (let ((submenu (gtk-menu-new))
	    (menuitem2 (gtk-menu-item-new-with-label "Rotate Translate Zone Mode...")))
	
	(gtk-menu-item-set-submenu menuitem2 submenu) 
	(gtk-menu-append menu menuitem2)
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
       menu "All Molecules use \"Near Chains\" Symmetry"
       (lambda ()
	 (for-each (lambda (imol)
		     (if (valid-model-molecule? imol)
			 (set-symmetry-whole-chain imol 1)))
		   (molecule-number-list))))
      
      (add-simple-coot-menu-menuitem
       menu "Question Accept Refinement"
       (lambda ()
	 (set-refinement-immediate-replacement 0)))

      (add-simple-coot-menu-menuitem
       menu "SHELXL Refine..."
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

      (add-simple-coot-menu-menuitem 
       menu "Phosphorylate this residue"
       (lambda ()
	 (phosphorylate-active-residue)))

      (add-simple-coot-menu-menuitem 
       menu "Set Spin Speed..."
       (lambda ()
	 (generic-single-entry "Set Spin Speed (smaller is slower)"
			       (number->string (idle-function-rotate-angle))
			       "Set it" (lambda (text)
					  (let ((t (string->number text)))
					    (if (number? t)
						(set-idle-function-rotate-angle t)))))))

      (add-simple-coot-menu-menuitem
       menu "Brighten Maps"
       (lambda ()
	 (brighten-maps)))

      (add-simple-coot-menu-menuitem
       menu "Add Strand Here..."
       (lambda ()
	 (place-strand-here-gui)))


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
					(add-spin-view "Spin" n1 n2)))))))

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


      
      )); finish let and if
