/* coords/mmdb-extras.h
 * -*-c++-*-  
 * 
 * Copyright 2005 by The University of York
 * Author: Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

// mmdb-extras

// needs mmdb_manager.h and <string>

#ifndef MMDB_EXTRAS_H
#define MMDB_EXTRAS_H

#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif

#include <iostream>

using namespace std;  // ugh.  This should not be here.  FIXME

#include "math.h"

#ifndef MMDB_MANAGER_H
#define MMDB_MANAGER_H
#include <mmdb2/mmdb_manager.h>
#endif

class MyCMMDBManager : public CMMDBManager { 
 public: 
  const CMMDBCryst& get_cell() { return Cryst; } 
  CMMDBCryst* get_cell_p() { return & Cryst; } 
}; 


#include "geometry/protein-geometry.hh"

// we need this for bonded_pair_container_t.
#include "coot-utils/bonded-pairs.hh"
// and atom quads
#include "mini-mol/atom-quads.hh"

// 
//
class atom_selection_container_t { 
public:
   CMMDBManager *mol;
   int n_selected_atoms; 
   mmdb::PPAtom atom_selection; 
   std::string read_error_message;
   int read_success;
   int SelectionHandle;
   int UDDAtomIndexHandle; // no negative is OK.
   int UDDOldAtomIndexHandle; // ditto. // set initially to -1 in make_asc

   atom_selection_container_t(CMMDBManager *mol_in, int selhnd) {
      // one day this casting should go away...
      mol =  (MyCMMDBManager *) mol_in;
      atom_selection = NULL;
      mol->GetSelIndex(selhnd, atom_selection, n_selected_atoms);
      SelectionHandle = selhnd;
      UDDAtomIndexHandle = -1;
      UDDOldAtomIndexHandle = -1;
      read_success = 1;
   }

   atom_selection_container_t() {
      mol = NULL;
      atom_selection = NULL;
      SelectionHandle = -1;
      UDDAtomIndexHandle = -1;
      UDDOldAtomIndexHandle = -1;
      read_success = 0;
   } 
  
   void clear_up() { 
      if (read_success) 
	 if (SelectionHandle)
	    mol->DeleteSelection(SelectionHandle);
      delete mol;
      atom_selection = 0;
      mol = 0; 
   } 
   void delete_atom_selection() { 
      mol->DeleteSelection(SelectionHandle);
      n_selected_atoms = 0;
      atom_selection = 0;
   }
   void apply_shift(float x_shift, float y_shift, float z_shift) {
      for (int i=0; i<n_selected_atoms; i++) { 
	 atom_selection[i]->x -= x_shift;
	 atom_selection[i]->y -= y_shift;
	 atom_selection[i]->z -= z_shift;
      }
   }
};

// debug this struct
void
debug_atom_selection_container(atom_selection_container_t asc);

// create this struct:
atom_selection_container_t make_asc(CMMDBManager *mol); 

// Bond things
//
// enum bond_colours { green, red, blue, yellow, white, grey }; 
enum bond_colours { YELLOW_BOND, RED_BOND, BLUE_BOND, GREEN_BOND, MAGENTA_BOND, 
		    GREY_BOND, ORANGE_BOND, CYAN_BOND, HYDROGEN_GREY_BOND }; 

float max_bond_length(const std::string &element);

int atom_colour(const std::string &element); 

// CCP4 symmetry library checking:
int check_ccp4_symm(); 

namespace coot { 
  // 
  // Note, we also create a chain and add this residue to that chain.
  // We do this so that we have a holder for the segid.
  // 
  // if atom_index_handle is not negative, then we try to copy the
  // "atoms index" udd atom_indices to "old atom index".  If it is, we
  // don't.
  // 
  // whole_residue_flag: only copy atoms that are either in this altLoc,
  // or has an altLoc of "".
  //
  // Returns NULL if input residue has no atoms
  // 
  // caller should delete the chain of this residue (which will
  // implicitly delete this residue too).
  // 
  mmdb::Residue *
  deep_copy_this_residue(mmdb::Residue *residue, 
			 const std::string &altconf, 
			 short int whole_residue_flag,
			 int atom_index_handle,
			 bool embed_in_chain_flag=true);

  std::pair<mmdb::Residue *, atom_selection_container_t>
    deep_copy_this_residue_and_make_asc(CMMDBManager *orig_mol,
					mmdb::Residue *residue, 
					const std::string &altconf, 
					short int whole_residue_flag,
					int atom_index_handle, 
					int udd_afix_handle);

  // 13 14 15 20 21 22  -> 1
  // 13 14 15 20 22 21  -> 0
  short int progressive_residues_in_chain_check(const mmdb::Chain *chain_p); 

  class contact_info {

    class contacts_pair { 
    public:
      int id1;
      int id2;
      contacts_pair(int id1_in, int id2_in) { 
	id1 = id1_in; 
	id2 = id2_in;
      }
    };

    std::vector<std::pair<std::string, realtype> > atom_radii;
    void setup_atom_radii();
    realtype get_radius(const std::string &element) const;

    void contacts_from_monomer_restraints(const atom_selection_container_t asc, 
			    std::map<mmdb::Residue *, dictionary_residue_restraints_t> &res_restraints); // non-const for map [] usage

    void setup_from_monomer_restraints(const atom_selection_container_t &asc, 
				       coot::protein_geometry *geom_p);


  public:
    std::vector<contacts_pair> contacts;
    contact_info(PSContact con_in, int nc) {
      for (int i=0; i<nc; i++) { 
	contacts.push_back(contacts_pair(con_in[i].id1, con_in[i].id2));
      }
    }
    contact_info(mmdb::PPAtom atom_selection, PSContact con_in, int nc) {
      setup_atom_radii();
      for (int i=0; i<nc; i++) { 
	mmdb::Atom *at_1 = atom_selection[con_in[i].id1];
	mmdb::Atom *at_2 = atom_selection[con_in[i].id2];
	std::string ele_1 = at_1->element;
	std::string ele_2 = at_2->element;
	realtype dx = at_1->x - at_2->x;
	realtype dy = at_1->y - at_2->y;
	realtype dz = at_1->z - at_2->z;
	realtype dist_2 = dx*dx + dy*dy + dz*dz;
	realtype dist = sqrt(dist_2);
	realtype r1 = get_radius(ele_1);
	realtype r2 = get_radius(ele_2);
	if (dist < (r1 + r2 + 0.1))
	  contacts.push_back(contacts_pair(con_in[i].id1, con_in[i].id2));
      }
    }
    // This contact_info constructor does not take the alt conf(s) into
    // account.  That is becuase (in the current scenario) the alt conf
    // selection has already taken place before we get here.  If you want
    // to account for alt confs, then you'll have to write a new
    // constructor.
    //
    contact_info(const atom_selection_container_t &asc, 
		 const std::string &monomer_type,
		 protein_geometry *geom_p);

    // Here we look up the contacts for each monomer in the atom
    // selection. We also allow descriptions of bonds between
    // monomers.   
    // 
    // Can throw a std::runtime_error.
    // 
    contact_info(const atom_selection_container_t &asc,
		 protein_geometry *geom_p, 
		 const bonded_pair_container_t &bonded_pairs);

    contact_info(const atom_selection_container_t &asc,
		 protein_geometry *geom_p, 
		 const std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> > &link_bond_atoms);

    // like above, but we have link atom quads (selhnd is a selection
    // handle - usually all atoms in mol, but not necessarily).
    // 
    template<class T>
    contact_info(CMMDBManager *mol, int selhnd,
		 const std::vector<T> &link_torsions,
		 protein_geometry *geom_p);

    void add_MSE_Se_bonds(const atom_selection_container_t &asc);
    int n_contacts() const { return contacts.size(); } 

    // one way only
    std::vector<std::vector<int> > get_contact_indices() const;

    // with reverses, e.g. 0->1 and 1->0 too 
    std::vector<std::vector<int> > get_contact_indices_with_reverse_contacts() const;

    void print() const; // debug info
  };
  contact_info getcontacts(const atom_selection_container_t &asc); 
  contact_info getcontacts(const atom_selection_container_t &asc, const std::string &monomer_type, 
			   protein_geometry *geom_p); 

  // Typically this is used on an asc (moving atoms) to get the N of a
  // peptide (say).  Return NULL on atom not found.
  mmdb::Atom *get_first_atom_with_atom_name(const std::string &atomname, 
				       const atom_selection_container_t &asc); 

  // tinker with asc
  void add_atom_index_udd_as_old(atom_selection_container_t asc);


}



#endif  // MMDB_EXTRAS_H
