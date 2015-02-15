

;; return a molecule number.  Return -1 on fail.
;; 
(define (generate-molecule-from-mmcif comp-id mmcif-file-name)

  ;; return a new molecule number, generator is "pyrogen" or "acedrg"
  ;; 
  (define (dict-gen generator args working-dir)

      (let* ((stub (string-append comp-id "-" generator))
	     (log-file-name (append-dir-file working-dir (string-append stub ".log"))))

	(let ((status (goosh-command generator args '() log-file-name #t)))
	  (if (not (ok-goosh-status? status))
	      -1 ;; bad mol

	      (let ((pdb-name (append-dir-file working-dir (string-append stub ".pdb")))
		    (cif-name (append-dir-file working-dir (string-append stub ".cif"))))
		
		(let ((imol (read-pdb pdb-name)))
		  (read-cif-dictionary cif-name)
		  imol))))))

  (if (not (file-exists? mmcif-file-name))

      -1 ;; fail

      (if (enhanced-ligand-coot?)

	  ;; Use pyrogen if we have mogul
	  ;; 
	  (if *use-mogul*

	      ;; pyrogen
	      ;; 
	      (let* ((working-dir (get-directory "coot-pyrogen"))
		     (args (list "-r" comp-id "-d" working-dir "-c" mmcif-file-name)))
		(dict-gen "pyrogen" args working-dir))
	      
	      ;; acedrg
	      ;; 
	      (let* ((working-dir (get-directory "coot-acedrg"))
		     (stub (append-dir-file working-dir (string-append comp-id "-acedrg")))
		     (args (list "-r" comp-id "-c" mmcif-file-name "-o" stub)))
		(dict-gen "acedrg" args working-dir)))
	  
	  ;; acedrg
	  ;; 
	  (let* ((working-dir (get-directory "coot-acedrg"))
		 (args (list "-M" "-r" comp-id "-c" mmcif-file-name)))
	    (dict-gen "acedrg" args working-dir)))))




