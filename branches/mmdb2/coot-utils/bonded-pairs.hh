
#ifndef HAVE_BONDED_PAIRS_HH
#define HAVE_BONDED_PAIRS_HH

#include <string>
#include <vector>
#include <iostream>

#include <mmdb2/mmdb_manager.h>

#include "geometry/protein-geometry.hh"

namespace coot {

   // The residues here are in order.  res_1 is comp_1 and res_2 is comp_2
   class bonded_pair_t {
      void delete_atom(CResidue *res, const std::string &atom_name);
   public:
      CResidue *res_1;
      CResidue *res_2;
      std::string link_type;
      bool is_fixed_first;
      bool is_fixed_second;
      bonded_pair_t(CResidue *r1, CResidue *r2, bool is_fixed_first_in, bool is_fixed_second_in,
		    const std::string &lt) {
	 res_1 = r1;
	 res_2 = r2;
	 link_type = lt;
	 is_fixed_first = is_fixed_first_in;
	 is_fixed_second = is_fixed_second_in;
      }
      bonded_pair_t() {
	 res_1=0;
	 res_2=0;
	 is_fixed_first  = false;
	 is_fixed_second = false;
      } 
      bool matches(CResidue *r1, CResidue *r2) const {
	 if (r1 == res_1 && r2 == res_2) {
	    return true;
	 } else{
	    if (r1 == res_2 && r2 == res_1) {
	       return true;
	    } else {
	       return false;
	    } 
	 }
      }
      bonded_pair_t swap() const {
	 bonded_pair_t bp;
	 bp.link_type = link_type;
	 bp.res_1 = res_2;
	 bp.res_2 = res_1;
	 bp.is_fixed_first  = is_fixed_second;
	 bp.is_fixed_second = is_fixed_first;
	 return bp;
      } 
      // matches?, swap-is-needed-to-match?
      std::pair<bool, bool> matches_info(CResidue *r1, CResidue *r2) const {
	 if (r1 == res_1 && r2 == res_2) {
	    return std::pair<bool, bool> (true, false);
	 } else{
	    if (r1 == res_2 && r2 == res_1) {
	       return std::pair<bool, bool> (true, true);
	    } else {
	       return std::pair<bool, bool> (false, false);
	    }
	 }
      }
      void apply_chem_mods(const protein_geometry &geom);
   };
   std::ostream &operator<<(std::ostream &s, bonded_pair_t bp);

   class bonded_pair_match_info_t {
   public:
      bool state;
      bool swap_needed;
      std::string link_type;
      bonded_pair_match_info_t(bool state_in, bool swap_needed_in, const std::string &link_type_in) {
	 state = state_in;
	 swap_needed = swap_needed_in;
	 link_type = link_type_in;
      }
   };
	 
   class bonded_pair_container_t {
   public:
      std::vector<bonded_pair_t> bonded_residues;
      bool try_add(const bonded_pair_t &bp); // check for null residues too.
      unsigned int size() const { return bonded_residues.size(); }
      bonded_pair_t operator[](unsigned int i) { return bonded_residues[i]; }
      const bonded_pair_t operator[](unsigned int i) const { return bonded_residues[i]; }
      bool linked_already_p(CResidue *r1, CResidue *r2) const;
      friend std::ostream& operator<<(std::ostream &s, bonded_pair_container_t bpc);
      // test order switch too.
      bool matches(CResidue *r1, CResidue *r2) const {
	 bool r = false;
	 for (unsigned int i=0; i<bonded_residues.size(); i++) { 
	    if (bonded_residues[i].matches(r1, r2)) {
	       r = true;
	       break;
	    }
	 }
	 return r;
      }
      bonded_pair_match_info_t match_info(CResidue *r1, CResidue *r2) const {
	 bonded_pair_match_info_t mi(false, false, "");
	 for (unsigned int i=0; i<bonded_residues.size(); i++) {
	    std::pair<bool, bool> bb = bonded_residues[i].matches_info(r1, r2);
	    if (bb.first) {
	       mi.link_type = bonded_residues[i].link_type;
	       mi.state = true;
	       if (bb.second)
		  mi.swap_needed = true;
	       break;
	    }
	 }
	 return mi;
      }
      void apply_chem_mods(const protein_geometry &geom);
   };
   std::ostream& operator<<(std::ostream &s, bonded_pair_container_t bpc);

}

#endif // HAVE_BONDED_PAIRS_HH
