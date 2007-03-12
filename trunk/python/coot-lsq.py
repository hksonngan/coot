# BL attempt to translate coot-lsq from Paul's script

def lsq_match_type_symbol(match_type_in):
  import operator

  if (operator.isNumberType(match_type_in)):
     match_type_in = match_type_in
  else:
     if (match_type_in in ["CA","ca","Ca"]):
        match_type_in = 0
     elif (match_type_in in ["Main","main","mainchain","Mainchain"]):
        match_type_in = 1
     elif (match_type_in in ["ALL","all","All"]):
        match_type_in = 2
     else:
        match_type_in = -1   # unknowN
  return match_type_in
#BL says, I guess I could make that more elegant...
#we use 0, 1 ,2 for ca, main, all (as it is in c++ code and not Paul's guile script)!!

#; Create matchers, 7 elements: 
#;   (list ref-start-resno ref-end-resno ref-chain-id imol-ref
#; 	   mov-start-resno mov-end-resno mov-chain-id imol-mov
#; 	   match-type)

def set_match_element(m):
# m should be a list!!!!
# something like:
# match1 = [40,50,"A",40,50,"B","all"]

  if (len(m)==7):

        match_type=lsq_match_type_symbol(m[6])
	add_lsq_match(m[0],m[1],m[2],m[3],m[4],m[5],match_type)
  else:
     print "Wrong number of elements in match (was",len(m)," should be 7)"


def lsq_match(imol_ref,imol_moving,match_list):
    
    clear_lsq_matches()
    set_match_element(match_list)
    
    apply_lsq_matches(imol_ref,imol_moving)


def simple_lsq_match (ref_start_resno,ref_end_resno,ref_chain_id,imol_ref,
                      mov_start_resno,mov_end_resno,mov_chain_id,imol_mov,
                      match_type):

      internal_match_type=lsq_match_type_symbol(match_type)
      clear_lsq_matches()
      add_lsq_match(ref_start_resno,ref_end_resno,ref_chain_id,
		    mov_start_resno,mov_end_resno,mov_chain_id,
		    internal_match_type)
      apply_lsq_matches(imol_ref,imol_mov)

		    
# example:
# simple_lsq_match(940,950,"A",0,940,950,"A",1,"main")
#
# or another one:
#match1 = [40,50,"A",40,50,"B","all"]
#match2 = [45,55,"A",45,55,"B","main"]
#clear_lsq_matches()
#set_match_element(match1)
#set_match_element(match2)
#lsq_match(0,1,match1)
#
# BL says: still dont know exactly why we want that (2nd example) and what
# we should do with that! Or I havent understood and translated guile code
# wrong and screwed up!
