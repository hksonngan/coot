

#ifndef PICK_H
#define PICK_H


#include <string>

#include "mmdb_manager.h" 
#include "mmdb-extras.h"
#include "mmdb-crystal.h"

#include "clipper/core/coords.h"

coot::Cartesian unproject(float screen_z);
coot::Cartesian unproject_xyz(int x, int y, float screen_z);

class pick_info { 
 public:
  int success; 
  int atom_index;
  int imol;
  float min_dist;
}; 

// this is a class, because it contains a class (symm_trans_t)
//
class symm_atom_info_t {

 public:
   symm_atom_info_t() { success = 0; atom_index = 0; };
   int success;
   int atom_index;
   symm_trans_t symm_trans;
   PPCAtom trans_sel;
   int n_trans_sel;
   int imol;

};

namespace coot { 
  class clip_hybrid_atom { 
  public:
    CAtom *atom;
    // clipper::Coord_orth pos;
    coot::Cartesian pos;
    clip_hybrid_atom() { atom = NULL; }
    clip_hybrid_atom(CAtom *mmdb_atom_p, const coot::Cartesian &p) { 
      atom = mmdb_atom_p;
      pos = p;
    } 
  };

  class Symm_Atom_Pick_Info_t { 
  public: 
    int success;
    int atom_index;
    int imol; 
    clip_hybrid_atom hybrid_atom;
    symm_trans_t symm_trans; 
    Cell_Translation pre_shift_to_origin; 
    Symm_Atom_Pick_Info_t() {
      success = 0;
      symm_trans = symm_trans_t(-1, -999, -999, -999);
    }
    clip_hybrid_atom Hyb_atom() const { 
      return hybrid_atom;
    }
  }; 

}


enum { PICK_ATOM_ALL_ATOM, PICK_ATOM_CA_ONLY };

pick_info atom_pick(GdkEventButton *event); // atom index in the atom selection
// pick_info moving_atoms_atom_pick(); not here, it's in graphics.

pick_info pick_atom(const atom_selection_container_t &SelAtom, int imol,
		    const coot::Cartesian &front, const coot::Cartesian &back, 
		    short int pick_mode, bool verbose_mode);

pick_info pick_intermediate_atom(const atom_selection_container_t &SelAtom);


// symm_atom_info_t symmetry_atom_pick();

coot::CartesianPair
screen_x_to_real_space_vector(GtkWidget *widget);
coot::Cartesian screen_z_to_real_space_vector(GtkWidget *widget);

std::string make_symm_atom_label_string(PCAtom atom, symm_trans_t symm_trans);

coot::Symm_Atom_Pick_Info_t symmetry_atom_pick();
// used by symmetry_atom_pick():
void 
fill_hybrid_atoms(std::vector<coot::clip_hybrid_atom> *hybrid_atoms, 
		  const atom_selection_container_t &basc,
		  const clipper::Spacegroup &spg, 
		  const clipper::Cell &cell,
		  const symm_trans_t &symm_trans); 


// void clear_symm_atom_info(symm_atom_info_t symm_atom_info); 


// now in molecule_class_info, where they should have been.
/* std::string make_atom_label_string(PCAtom atom); */
/* std::string make_atom_label_string_old(PCAtom atom); */



#endif // PIck
