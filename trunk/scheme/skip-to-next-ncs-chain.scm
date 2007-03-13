
(define find-first-model-molecule
  (lambda ()
    
    (for-each 
     (lambda (molecule)
       (if (valid-model-molecule? molecules)
	   (break molecule)))
     (molecule-number-list))))

;; Skip the residue in the next chain (typically of a molecule with
;; NCS) with the same residue number.  If on the last chain, then wrap
;; to beginning.  If it can't find anything then don't move (and put a
;; message in the status bar)
;; 
(define (skip-to-next-ncs-chain)

  ;; Given a chain-id and a list of chain-ids, return the chain-id of
  ;; the next chain to be jumped to (use wrapping).  If the list of
  ;; chain-ids is less then length 2, return #f.
  ;; 
  (define (skip-to-chain this-chain-id chain-id-list)
    (format #t "this-chain-id: ~s~%" this-chain-id)
    (if (< (length chain-id-list) 2)
	#f 
	(let loop ((local-chain-id-list chain-id-list))
	  (format #t "considering local-chain-id-list: ~s~%"
		  local-chain-id-list)
	  (cond
	   ((null? local-chain-id-list) (car chain-id-list))
	   ((string=? this-chain-id)
	    (if (null? (cdr local-chain-id-list))
		(car chain-id-list)
		(car (cdr local-chain-id-list))))
	   (else 
	    (loop (cdr local-chain-id-list)))))))


  ;; First, what is imol?  imol is the go to atom molecule
  (let* ((imol (go-to-atom-molecule-number))
	 (chains (chain-ids imol))
	 (this-chain-id (go-to-atom-chain-id))
	 (next-chain (skip-to-chain this-chain-id chains)))

    (format #t "next-chain: ~s~%" next-chain)
	 
    (if (not next-chain)
	(add-status-bar-text "No \"NCS Next Chain\" found")
	(set-go-to-atom-chain-residue-atom-name
	 next-chain 
	 (go-to-atom-residue-number)
	 (go-to-atom-atom-name))
	;; now, did that set-go-to-atom function work (was there a
	;; real atom)?  If not, then that could have been the ligand
	;; chain or the water chain that we tried to go to.  We want
	;; to try again, and we shbould keep trying again until we get
	;; back to this-chain-id - in which case we have a "No NCS
	;; Next Chain atom" status-bar message.
	)))


(define graphics-general-key-press-hook
  (lambda (key)

    (format #t "key: ~s~%" key)
    (cond
     ((= key 107) (skip-to-next-ncs-chain)))))


	 
     

