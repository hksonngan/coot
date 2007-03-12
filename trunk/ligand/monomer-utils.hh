
#ifndef MONOMER_UTILS_HH
#define MONOMER_UTILS_HH

#ifndef HAVE_STRING
#include <string>
#endif
#ifndef HAVE_VECTOR
#include <vector>
#endif

#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb_manager.h"

#include "clipper/core/coords.h"

namespace coot {

   CResidue *deep_copy_residue(const CResidue *residue);
	 
   // Atom names for a torsion bond
   // 
   class atom_name_pair {
   public:
      std::string atom1;
      std::string atom2;
      atom_name_pair(const std::string &atom_name_1,
		     const std::string &atom_name_2) {
	 atom1 = atom_name_1;
	 atom2 = atom_name_2;
      }
   };

   // For looking up atom names to atom indices in the vector of
   // coords that go to the mgtree.
   //
   class atom_index_pair {
   public:
      int index1;
      int index2;
      atom_index_pair() { // unassigned pair
	 index1 = -1;
	 index2 = -1;
      }
      atom_index_pair(int i1, int i2) {
	 index1 = i1;
	 index2 = i2;
      }
   };

   // Now in mmdb-extras
//    class contact_info {
//    public:
//       contact_info(PSContact con_in, int nc) {
// 	 pscontact = con_in;
// 	 n_contacts = nc;
//       }
//       PSContact pscontact;
//       int n_contacts;
//    };
 

   class monomer_utils {

      std::vector<atom_name_pair> atom_name_pair_list;

   public:
      std::vector<atom_name_pair> AtomPairs() const {
	 return atom_name_pair_list;
      }

       void add_torsion_bond_by_name(const std::string &atom_name_1,
				     const std::string &atom_name_2);

      std::vector<coot::atom_name_pair>
      atom_name_pairs(const std::string &res_type) const; 

      coot::contact_info getcontacts(const atom_selection_container_t &asc) const; 

      std::vector<coot::atom_index_pair> 
      get_atom_index_pairs(const std::vector<coot::atom_name_pair> &atom_name_pairs,
			   const PPCAtom atoms, int nresatoms) const;

      static Cartesian coord_orth_to_cartesian(const clipper::Coord_orth &c);
      static clipper::Coord_orth coord_orth_to_cart(const Cartesian &c);
   };

   

}

#endif
