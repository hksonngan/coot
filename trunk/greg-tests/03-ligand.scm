

(greg-testcase "Get monomer test" #t 
   (lambda ()

     ;; Test preliminaries, if CCP4 is not available, let's get the
     ;; monomer files from the greg data dir, that should short-cut
     ;; the running of libcheck and refmac.
     ;;
     (if (not have-ccp4?)
	   (map (lambda (file)
		  (let ((f-full (append-dir-file greg-data-dir file)))
		    (if (file-exists? f-full)
			(copy-file f-full file))))
		'("monomer-3GP.pdb" "libcheck_3GP.cif")))

     (let ((imol (monomer-molecule-from-3-let-code "3GP" "")))
       (if (valid-model-molecule? imol)
	   (begin
	     (set! imol-ligand imol) ; for use in next test
	     (delete-residue-hydrogens imol "A" 1 "" "")
	     #t)
	   (begin
	     (format #t "   No ligand molecule - monomer test~%")
	     (throw 'untested)
	     #f)))))


(greg-testcase "Set Bond thickness" #t 
   (lambda ()

     (if (valid-model-molecule? imol-ligand)
	 (begin
	   (set-bond-thickness imol-ligand 5)
	   #t)
	 (begin
	   (format #t "   No ligand molecule - Skipping bond thickness test~%")
	   (throw 'untested)))))
	 

(greg-testcase "Move and Refine Ligand test" #t 
   (lambda ()
     (let ((new-rc (list 55.3 9.1 20.6)))
       (if (not (valid-model-molecule? imol-ligand))
	   (throw 'untested)
	   (begin
	     ;; set the view
	     (let ((view-number (add-view (list    54.5698 8.7148 20.5308)
					  (list 0.046229 -0.157139 -0.805581 0.569395)
					  19.8858
					  "ligand-view")))
	       (go-to-view-number view-number 1))
				
	     ;; updates the map:
	     (apply set-rotation-centre new-rc)
	     (move-molecule-here imol-ligand)
	     (let ((backup-mode (backup-state imol-ligand))
		   (alt-conf "")
		   (replacement-state (refinement-immediate-replacement-state)))
	       
	       (turn-off-backup imol-ligand)
	       (set-refinement-immediate-replacement 1)
	       (refine-zone imol-ligand "A" 1 1 alt-conf)
	       (accept-regularizement)
	       (rotate-y-scene (rotate-n-frames 600) 0.1)
	       (if (= replacement-state 0)
		   (set-refinement-immediate-replacement 0))
	       (if (= backup-mode 1)
		   (turn-on-backup imol-ligand))
	       #t ; success
	       ))))))

(greg-testcase "Many Molecules - Ligand Fitting" #t 
   (lambda ()

     (let* ((npo-pdb  (append-dir-file greg-data-dir "monomer-NPO.pdb"))
	    (43ca-pdb (append-dir-file greg-data-dir "pdb43ca-sans-NPO-refmaced.pdb"))
	    (43ca-mtz (append-dir-file greg-data-dir "pdb43ca-sans-NPO-refmaced.mtz"))
	    (imol-npo (handle-read-draw-molecule-with-recentre npo-pdb 0)))
       
       (if (not (valid-model-molecule? imol-npo))
	   #f
	   (begin 
	     (let loop ((count 0))
	       (cond
		((= count 5) 'done)
		(else 
		 (let ((imol-copy (copy-molecule imol-npo)))
		   (set-mol-displayed imol-copy 0))
		 (loop (+ count 1)))))

	     (let* ((imol-protein (read-pdb 43ca-pdb))
		    (imol-map-2 (auto-read-make-and-draw-maps 43ca-mtz))
		    (imol-map-1 (- imol-map-2 1)))
	     
	       (add-ligand-clear-ligands)
	       (set-ligand-search-protein-molecule imol-protein)
	       (set-ligand-search-map-molecule imol-map-1)
	       (add-ligand-search-ligand-molecule imol-npo)

	       (let ((solutions (execute-ligand-search))) ; crash
		 (format #t "   Fitting NPO gave these results: ~s~%" solutions)
		 (set-map-displayed imol-map-1 0)
		 (set-map-displayed imol-map-2 0)
		 (set-mol-displayed imol-protein 0)
		 (set-mol-displayed imol-npo 0)
		 #t)))))))


(greg-testcase "flip residue (around eigen vectors)" #t 
   (lambda ()

     ;; note to self, make sure this file is available when then test
     ;; is run for real.
     ;;
     (let* ((imol-orig (read-pdb "monomer-3GP.pdb"))
	    (imol-copy (copy-molecule imol-orig)))
       (let ((active-atom (active-residue)))
	 (if (not active-atom)
	     (begin
	       (format #t "No active atom~%")
	       #f)
	     (let ((imol      (list-ref active-atom 0))
		   (chain-id  (list-ref active-atom 1))
		   (res-no    (list-ref active-atom 2))
		   (ins-code  (list-ref active-atom 3))
		   (atom-name (list-ref active-atom 4))
		   (alt-conf  (list-ref active-atom 5)))
	       (if (= imol imol-orig)
		   (begin
		     (format #t "oops - didn't pick the copy for active res~%")
		     #f)
		   (begin
		     (flip-ligand imol chain-id res-no)
		     (let ((atom-orig-1 (get-atom imol-orig "A" 1 " C8 "))
			   (atom-move-1 (get-atom imol      "A" 1 " C8 ")))
		       (let ((d (bond-length (list-ref atom-orig-1 2)
					   (list-ref atom-move-1 2))))
			 (format #t "distance: ~s~%" d)
			 (if (not (> d 2.1))
			     (begin
			       (format #t "fail to move test atom d1~%"))
			     (begin
			       (flip-ligand imol chain-id res-no)
			       (flip-ligand imol chain-id res-no)
			       (flip-ligand imol chain-id res-no)
			       ;; having flipped it round the axes 4
			       ;; times, we should be back where we
			       ;; started.
			       (let ((atom-orig-1 (get-atom imol-orig "A" 1 " C8 "))
				     (atom-move-1 (get-atom imol      "A" 1 " C8 ")))
				 (let ((d2 (bond-length (list-ref atom-orig-1 2)
							(list-ref atom-move-1 2))))
				   (format #t "distance d2: ~s~%" d2)
				   (if (not (< d2 0.001))
				       (begin
					 (format #t "fail to move atom back to start d2~%"))
				       #t)))))))))))))))
				       
