
#include <algorithm>
#include <stdlib.h> // for abs()

#include "residue-and-atom-specs.hh"
#include "bonded-pairs.hh"

bool
coot::bonded_pair_container_t::linked_already_p(mmdb::Residue *r1, mmdb::Residue *r2) const {

   bool r = 0;
   for (unsigned int i=0; i<bonded_residues.size(); i++) {
      if (((bonded_residues[i].res_1 == r1) &&
	   (bonded_residues[i].res_2 == r2)) ||
	  ((bonded_residues[i].res_1 == r2) &&
	   (bonded_residues[i].res_2 == r1))) {
	 r = 1;
	 break;
      }
   }
   return r;
}

bool
coot::bonded_pair_container_t::try_add(const coot::bonded_pair_t &bp) {

   bool found = 0;
   for (unsigned int i=0; i<bonded_residues.size(); i++) {
      if ( (bonded_residues[i].res_1 == bp.res_1 &&
	    bonded_residues[i].res_2 == bp.res_2) ||
	   (bonded_residues[i].res_1 == bp.res_2 &&
	    bonded_residues[i].res_2 == bp.res_1) ) {
	 found = 1;
	 break;
      }
   }
   
   if (! found) {
      bonded_residues.push_back(bp);
   }
   return found;
}

std::ostream&
coot::operator<<(std::ostream &s, coot::bonded_pair_container_t bpc) {

   s << "Bonded Pair Container contains " << bpc.bonded_residues.size() << " bonded residues"
     << "\n";

   for (unsigned int i=0; i<bpc.bonded_residues.size(); i++)
      s << "   " << i << "  [:"
	<< bpc[i].link_type << ": "
	<< bpc[i].res_1->GetChainID() << " " << bpc[i].res_1->GetSeqNum() << " "
	<< bpc[i].res_1->GetInsCode() << " to " << bpc[i].res_2->GetChainID() << " "
	<< bpc[i].res_2->GetSeqNum() << " " << bpc[i].res_2->GetInsCode() << "]"
	<< "\n";

   return s; 
}

std::ostream&
coot::operator<<(std::ostream &s, coot::bonded_pair_t bp) {
   s << "[:";
   if (bp.res_1)
      s << bp.res_1->GetChainID() <<  " " << bp.res_1->GetSeqNum() << " " << bp.res_1->GetInsCode();
   s << " ";
   if (bp.res_2)
      s << bp.res_2->GetChainID() <<  " " << bp.res_2->GetSeqNum() << " " << bp.res_2->GetInsCode();
   s << "]";
   return s;
}


void
coot::bonded_pair_t::apply_chem_mods(const coot::protein_geometry &geom) {

   if (res_2 && res_2) { 
      try { 
	 // apply the mods given the link type

	 // get the chem mods for each residue (can throw a runtime
	 // error if there is one - (not an error).
	 // 
	 std::pair<coot::protein_geometry::chem_mod, coot::protein_geometry::chem_mod>
	    mods = geom.get_chem_mods_for_link(link_type);
	 std::string res_1_name = res_1->GetResName();
	 std::string res_2_name = res_2->GetResName();
	 for (unsigned int i=0; i<mods.first.atom_mods.size(); i++) {
	    if (mods.first.atom_mods[i].function == CHEM_MOD_FUNCTION_DELETE) {
	       std::string atom_name = mods.first.atom_mods[i].atom_id;
	       std::string at_name = geom.atom_id_expand(atom_name, res_1_name);
	       delete_atom(res_1, at_name);
	    }
	 }
	 for (unsigned int i=0; i<mods.second.atom_mods.size(); i++) {
	    if (mods.second.atom_mods[i].function == CHEM_MOD_FUNCTION_DELETE) {
	       std::string atom_name = mods.second.atom_mods[i].atom_id;
	       std::string at_name = geom.atom_id_expand(atom_name, res_2_name);
	       delete_atom(res_2, at_name);
	    }
	 }
      }
      catch (const std::runtime_error &rte) {
	 // it's OK if we don't find a chem mod for this link
      }
   }
}

void
coot::bonded_pair_container_t::apply_chem_mods(const protein_geometry &geom) {

   std::vector<coot::bonded_pair_t>::iterator it;
   for (it=bonded_residues.begin(); it != bonded_residues.end(); it++) {
      it->apply_chem_mods(geom);
   }
} 


void
coot::bonded_pair_t::delete_atom(mmdb::Residue *res, const std::string &atom_name) {
   
   mmdb::PPAtom residue_atoms = 0;
   int n_residue_atoms;
   bool deleted = false;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   for (int iat=0; iat<n_residue_atoms; iat++) {
      mmdb::Atom *at = residue_atoms[iat];
      if (at) {
	 std::string at_name(at->name);
	 if (at_name == atom_name) {
	    delete at;
	    at = NULL;
	    deleted = true;
	 }
      }
   }

   if (deleted)
      res->TrimAtomTable();
}

void
coot::bonded_pair_container_t::reorder() {

   for (unsigned int i=0; i<bonded_residues.size(); i++) {
      const bonded_pair_t &bp_i = bonded_residues[i];
      if (bp_i.res_2->GetSeqNum() < bp_i.res_1->GetSeqNum()) {
	 std::string chain_id_1_i = bp_i.res_1->GetChainID();
	 std::string chain_id_2_i = bp_i.res_1->GetChainID();
	 if (chain_id_1_i == chain_id_2_i) {
	    mmdb::Residue *r = bp_i.res_1;
	    bonded_residues[i].res_1 = bp_i.res_2;
	    bonded_residues[i].res_2 = r;
	 }
      }
      // check here for ins code ordering
   }
}


// remove residue 1-3 bonds if residue 1-2 or 2-3 bonds exist.
void
coot::bonded_pair_container_t::filter() {

   reorder();
   std::vector<bonded_pair_t> new_bonded_residues;
   bool debug = false;

   if (debug) { 
      std::cout << ":::: we have these bonded pairs" << std::endl;
      for (unsigned int i=0; i<bonded_residues.size(); i++) {
	 const bonded_pair_t &bp_i = bonded_residues[i];
	 std::cout << i << "   "
		   << residue_spec_t(bp_i.res_1) << " - to - "
		   << residue_spec_t(bp_i.res_2) << std::endl;
      }
   }

   for (unsigned int i=0; i<bonded_residues.size(); i++) {
      bool keep_this = true;
      const bonded_pair_t &bp_i = bonded_residues[i];
      if (bp_i.res_1 && bp_i.res_2) { 
	 int resno_delta_i = bp_i.res_2->GetSeqNum() - bp_i.res_1->GetSeqNum();
	 if (abs(resno_delta_i) > 1) { 
	    std::string chain_id_1_i = bp_i.res_1->GetChainID();
	    std::string chain_id_2_i = bp_i.res_2->GetChainID();
	    if (chain_id_1_i == chain_id_2_i) {
	       for (unsigned int j=0; j<bonded_residues.size(); j++) {
		  if (j!=i) {
		     const bonded_pair_t &bp_j = bonded_residues[j];
		     int resno_delta_j = bp_j.res_2->GetSeqNum() - bp_j.res_1->GetSeqNum();

		     if (((resno_delta_i > 0) && (bp_i.res_1 == bp_j.res_1)) ||
			 ((resno_delta_i < 0) && (bp_i.res_2 == bp_j.res_2))) {
		     
			if (abs(resno_delta_j) < resno_delta_i) {
			   std::string chain_id_1_j = bp_j.res_1->GetChainID();
			   std::string chain_id_2_j = bp_j.res_2->GetChainID();
			   if (chain_id_1_j == chain_id_2_j) {
			      if (chain_id_1_j == chain_id_1_i) {
				 keep_this = false;
				 if (debug)
				    std::cout << ":::::::::::::::::::::: delete bonded pair "
					      << residue_spec_t(bp_i.res_1) << " - to = "
					      << residue_spec_t(bp_i.res_2) << " because "
					      << residue_spec_t(bp_j.res_1) << " - to - "
					      << residue_spec_t(bp_j.res_2) << " is closer " << std::endl;
			      }
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }
      if (keep_this)
	 new_bonded_residues.push_back(bp_i);
   }

   bonded_residues = new_bonded_residues;

   // We can't do this because closer_exists_p() is not static
   //bonded_residues.erase(std::remove_if(bonded_residues.begin(), bonded_residues.end(), closer_exists_p),
   // bonded_residues.end());
   
}

bool
coot::bonded_pair_container_t::closer_exists_p(const coot::bonded_pair_t &bp_in) const {

   bool e = false;

   if (bp_in.res_1 && bp_in.res_2) { 
      int resno_delta_i = bp_in.res_2->GetSeqNum() - bp_in.res_1->GetSeqNum();
      if (abs(resno_delta_i) > 1) { // needs more sophisticated test for ins-code linked residues
	 std::string chain_id_1_i = bp_in.res_1->GetChainID();
	 std::string chain_id_2_i = bp_in.res_1->GetChainID();
	 if (chain_id_1_i == chain_id_2_i) {
	    for (unsigned int j=0; j<bonded_residues.size(); j++) {
	       const bonded_pair_t &bp_j = bonded_residues[j];
	       int resno_delta_j = bp_j.res_2->GetSeqNum() - bp_j.res_1->GetSeqNum();
	       if (abs(resno_delta_j) < resno_delta_i) {
		  std::string chain_id_1_j = bp_j.res_1->GetChainID();
		  std::string chain_id_2_j = bp_j.res_1->GetChainID();
		  if (chain_id_1_j == chain_id_2_j) {
		     if (chain_id_1_i == chain_id_1_j) {
			e = true;
		     }
		  }
	       }
	       if (e)
		  break;
	    }
	 }
      }
   }

   return e;

} 
