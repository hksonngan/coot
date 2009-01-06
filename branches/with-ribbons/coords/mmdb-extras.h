/* coords/mmdb-extras.h
 * 
 * Copyright 2005 by The University of York
 * Author: Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
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

#ifndef MMDB_MANAGER_H
#define MMDB_MANAGER_H
#include "mmdb_manager.h"
#endif

class MyCMMDBManager : public CMMDBManager { 
 public: 
  const CMMDBCryst& get_cell() { return Cryst; } 
  CMMDBCryst* get_cell_p() { return & Cryst; } 
}; 


// 
//
class atom_selection_container_t { 
 public:
  //PCMMDBManager mol; 
  MyCMMDBManager *mol;
  int n_selected_atoms; 
  PPCAtom atom_selection; 
  std::string read_error_message;
  int read_success;
  int SelectionHandle;
  int UDDAtomIndexHandle; // no negative is OK.
  int UDDOldAtomIndexHandle; // ditto. // set initially to -1 in make_asc
  void clear_up() { 
    if (read_success) 
      if (SelectionHandle)
	mol->DeleteSelection(SelectionHandle);
    delete mol;
    atom_selection = 0;
    mol = 0; 
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
enum bond_colours { yellow, red, blue, green, magenta, grey, orange, cyan }; 

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
  CResidue *
  deep_copy_this_residue(CResidue *residue, 
			 const std::string &altconf, 
			 short int whole_residue_flag,
			 int atom_index_handle);

  std::pair<CResidue *, atom_selection_container_t>
    deep_copy_this_residue_and_make_asc(CMMDBManager *orig_mol,
					CResidue *residue, 
					const std::string &altconf, 
					short int whole_residue_flag,
					int atom_index_handle, 
					int udd_afix_handle);

  // 13 14 15 20 21 22  -> 1
  // 13 14 15 20 22 21  -> 0
  short int progressive_residues_in_chain_check(const CChain *chain_p); 

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

  public:
    std::vector<contacts_pair> contacts;
    contact_info(PSContact con_in, int nc) {
      for (int i=0; i<nc; i++) { 
	contacts.push_back(contacts_pair(con_in[i].id1, con_in[i].id2));
      }
    }

    void add_MSE_Se_bonds(const atom_selection_container_t &asc);

    int n_contacts() const { return contacts.size(); } 
  };
  contact_info getcontacts(const atom_selection_container_t &asc); 

  // Typically this is used on an asc (moving atoms) to get the N of a
  // peptide (say).  Return NULL on atom not found.
  CAtom *get_first_atom_with_atom_name(const std::string &atomname, 
				       const atom_selection_container_t &asc); 

  // tinker with asc
  void add_atom_index_udd_as_old(atom_selection_container_t asc);

}



#endif  // MMDB_EXTRAS_H
