// -*-c++-*-
/* coords/Bond_lines.h
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
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


// OK, the thinking here is that molecule_class_info_t functions
// create Bond_lines_containers via various constructors and perhaps
// other manipulations.  These then get converted to a
// graphical_bonds_container, and the drawing routine knows how to
// iterate through that to make the right coloured lines - that
// routine uses display_bonds(), which operates on a
// graphical_bonds_container bonds_box (the drawing function also
// takes a bond_width)
// 
// 

#ifndef BOND_LINES_H
#define BOND_LINES_H

#include <vector>
#include <string>

#include "geometry/protein-geometry.hh"

namespace coot { 

   static std::string b_factor_bonds_scale_handle_name;
   
   enum bond_colour_t { COLOUR_BY_CHAIN=0,
			COLOUR_BY_CHAIN_C_ONLY=20,
			COLOUR_BY_ATOM_TYPE=1, 
		       COLOUR_BY_SEC_STRUCT=2, DISULFIDE_COLOUR=3,
                       COLOUR_BY_MOLECULE=4, COLOUR_BY_RAINBOW=5, 
		       COLOUR_BY_OCCUPANCY=6, COLOUR_BY_B_FACTOR=7};

  class my_atom_colour_map_t { 

    public:
    std::vector<std::string> atom_colour_map;
     unsigned int index_for_chain(const std::string &chain) { 
       unsigned int isize = atom_colour_map.size();
       for (unsigned int i=0; i<isize; i++) { 
	  if (atom_colour_map[i] == chain) {
	     return i;
	  }
       }
       atom_colour_map.push_back(chain);
       return isize;
    }
    // These colours ranges need to be echoed in the GL bond drawing
    // routine.
    int index_for_rainbow(float wheel_colour) { 
       return int(30.0*wheel_colour);
    }
    int index_for_occupancy(float wheel_colour) { 
       return int(5.0*wheel_colour);
    }
    int index_for_b_factor(float wheel_colour) { 
       return int(30.0*wheel_colour);
    }
  };

   class model_bond_atom_info_t {
      std::vector<PCAtom> hydrogen_atoms_;
      std::vector<PCAtom> non_hydrogen_atoms_;
   public:
      PPCAtom     Hydrogen_atoms() const;
      PPCAtom non_Hydrogen_atoms() const;
      int n_H() const { return hydrogen_atoms_.size(); }
      int n_non_H() const { return non_hydrogen_atoms_.size(); }
      void add_atom(CAtom *atom) {
	 std::string element = atom->element;
	 if (element == " H" || element == " D") {
	    hydrogen_atoms_.push_back(atom);
	 } else {
	    non_hydrogen_atoms_.push_back(atom);
	 }
      }
   };

   // For OpenGL solid model, it is much better to draw a torus than a
   // set of short sticks (particularly for the ring representing
   // aromaticity).  So now (20100831 Bond_lines_container contains a
   // number of torus descriptions).
   // 
   class torus_description_t {
   public:
      double inner_radius;
      double outer_radius;
      int n_sides;
      int n_rings;
      clipper::Coord_orth centre;
      clipper::Coord_orth normal;
      torus_description_t(const clipper::Coord_orth &pt,
			  const clipper::Coord_orth &normal_in,
			  double ir1, double ir2, int n1, int n2) {
	 inner_radius = ir1;
	 outer_radius = ir2;
	 n_sides = n1;
	 n_rings = n2;
	 centre = pt;
	 normal = normal_in;
      }
   };
}

class graphics_line_t {
public:
   coot::CartesianPair positions;
   bool has_begin_cap;
   bool has_end_cap;
   graphics_line_t(const coot::CartesianPair &p, bool b, bool e) {
      positions = p;
      has_begin_cap = b;
      has_end_cap = e;
   }
   graphics_line_t() { }
};
 
// A poor man's vector.  For use when we can't use vectors
// 
class Lines_list { 

 public:   
   // contain a number of elements
   int num_lines;
   // coot::CartesianPair *pair_list;
   graphics_line_t *pair_list;
   bool thin_lines_flag;
   
   Lines_list() { 
      pair_list = NULL;
      thin_lines_flag = 0;
   }
};

// Uses Lines_list
// 
// a graphical_bonds_container is used by draw_molecule() and are
// created from a Bond_lines_container (which uses a vector).
// 
class graphical_bonds_container { 

 public:
   
   int num_colours;
   Lines_list *bonds_; 

   int symmetry_has_been_created;
   Lines_list *symmetry_bonds_;

   coot::Cartesian *zero_occ_spots_ptr;
   coot::Cartesian *deuterium_spots_ptr;
   int n_zero_occ_spots;
   int n_deuterium_spots;

   std::pair<bool,coot::Cartesian> *atom_centres_;
   int n_atom_centres_;
   int *atom_centres_colour_;
   std::vector<coot::torus_description_t> rings;

   graphical_bonds_container() { 
      num_colours = 0; 
      bonds_ = NULL;
      symmetry_has_been_created = 0; 
      symmetry_bonds_ = NULL; 
      zero_occ_spots_ptr = NULL;
      n_zero_occ_spots = 0;
      deuterium_spots_ptr = NULL;
      n_deuterium_spots = 0;
      atom_centres_colour_ = NULL;
      atom_centres_ = NULL; 
      n_atom_centres_ = 0;
   }

   void clear_up() { 

      if (bonds_)
	 for (int icol=0; icol<num_colours; icol++)
	    delete [] bonds_[icol].pair_list;
      if (symmetry_bonds_)
	 for (int icol=0; icol<num_colours; icol++)
	    delete [] symmetry_bonds_[icol].pair_list;

//       if (bonds_)
// 	 std::cout << " clearing bonds " << bonds_ << std::endl;
//       if (symmetry_bonds_)
// 	 std::cout << " clearing symmetry bonds " << symmetry_bonds_ << std::endl;
      
      delete [] bonds_;  // null testing part of delete
      delete [] symmetry_bonds_; 
      delete [] atom_centres_;
      delete [] atom_centres_colour_;
      bonds_ = NULL;
      symmetry_bonds_ = NULL;
      atom_centres_ = NULL;
      atom_centres_colour_ = NULL;
      if (n_zero_occ_spots) 
	 delete [] zero_occ_spots_ptr;
      if (n_deuterium_spots)
	 delete [] deuterium_spots_ptr;
      n_zero_occ_spots = 0;
      n_deuterium_spots = 0;
      n_atom_centres_ = 0;
   }

   graphical_bonds_container(const std::vector<graphics_line_t> &a) { 

      std::cout << "constructing a graphical_bonds_container from a vector " 
		<< "of size " << a.size() << std::endl;

      num_colours = 1;
      
      bonds_ = new Lines_list[1]; // only 1 Lines_list needed
      bonds_[0].pair_list = new graphics_line_t[(a.size())];
      bonds_[0].num_lines = a.size();

      // copy over
      for(int i=0; i<bonds_[0].num_lines; i++)
	 bonds_[0].pair_list[i] = a[i];

      symmetry_bonds_ = NULL; 
      symmetry_has_been_created = 0; 
      zero_occ_spots_ptr = NULL;
      n_zero_occ_spots = 0;
      deuterium_spots_ptr = NULL;
      n_deuterium_spots = 0;
      atom_centres_colour_ = NULL;
      atom_centres_ = NULL; 
      n_atom_centres_ = 0;
   }
      
   void add_colour(const  std::vector<graphics_line_t> &a ) {
      
 /*       cout << "filling a graphical_bonds_container from a vector "  */
/* 	   << "of size " << a.size() << endl; */

     Lines_list *new_bonds_ = new Lines_list[num_colours+1];
     if ( bonds_ != NULL ) {
       for (int i = 0; i < num_colours; i++ ) new_bonds_[i] = bonds_[i];
       delete[] bonds_;
     }
     bonds_ = new_bonds_;
     // bonds_[num_colours].pair_list = new coot::CartesianPair[(a.size())];
     bonds_[num_colours].pair_list = new graphics_line_t[(a.size())];
     bonds_[num_colours].num_lines = a.size();

     // copy over
     for(unsigned int i=0; i<a.size(); i++) { 
	bonds_[num_colours].pair_list[i] = a[i];
     }
     num_colours++;

     symmetry_bonds_ = NULL;
     symmetry_has_been_created = 0; 
   }

   void add_zero_occ_spots(const std::vector<coot::Cartesian> &spots);
   void add_deuterium_spots(const std::vector<coot::Cartesian> &spots);
   void add_atom_centres(const std::vector<std::pair<bool,coot::Cartesian> > &centres,
			 const std::vector<int> &colours);
   bool have_rings() const { return rings.size();
   }
};



// Bond_lines is a container class, containing a colour index
// and a vector of pairs of coot::Cartesians that should be drawn
// _in_ that colour.
//
class Bond_lines { 
   int colour;
   std::vector<graphics_line_t> points;

 public:
   Bond_lines(const graphics_line_t &pts);
   Bond_lines(); 
   Bond_lines(int col);

   void add_bond(const coot::CartesianPair &p,
		 bool begin_end_cap,
		 bool end_end_cap);
   int size() const; 

   // return the coordinates of the start and finish points of the i'th bond.
   //
   coot::Cartesian GetStart(int i) const;
   coot::Cartesian GetFinish(int i) const;
   graphics_line_t operator[](int i) const;
};

// 
enum symm_keys {NO_SYMMETRY_BONDS}; 

class Bond_lines_container { 

   enum { NO_BOND,
	  BONDED_WITH_STANDARD_ATOM_BOND,
	  BONDED_WITH_BOND_TO_HYDROGEN, 
	  BONDED_WITH_HETATM_BOND /* by dictionary */ };
   bool verbose_reporting;
   bool do_disulfide_bonds_flag;
   bool do_bonds_to_hydrogens;
   int udd_has_ca_handle;
   float b_factor_scale;
   bool for_GL_solid_model_rendering;

   // we rely on SelAtom.atom_selection being properly constucted to
   // contain all atoms
   //
   // if model_number is 0, display all models.
   //
   void construct_from_asc(const atom_selection_container_t &SelAtom, 
			   float min_dist, float max_dist, 
			   int atom_colour_type, 
			   short int is_from_symmetry_flag,
			   int model_number);

   // PDBv3 FIXME
   bool is_hydrogen(const std::string &ele) const {
      if (ele == " H" || ele == " D")
	 return true;
      else
	 return false; 
   } 

   void construct_from_atom_selection(const atom_selection_container_t &asc,
				      const PPCAtom atom_selection_1,
				      int n_selected_atoms_1,
				      const PPCAtom atom_selection_2,
				      int n_selected_atoms_2,
				      float min_dist, float max_dist,
				      int atom_colour_type,
				      bool are_different_atom_selections,
				      bool have_udd_atoms,
				      int udd_handle);
   
   void construct_from_model_links(CModel *model, int atom_colour_type);
   // which wraps...
   void add_link_bond(CModel *model_p, int atom_colour_type, CLink *link);
   void add_link_bond(CModel *model_p, int atom_colour_type, CLinkR *linkr);

   template<class T> void add_link_bond_templ(CModel *model_p, int atom_colour_type, T *link);

   // now wit optional arg.  If atom_colour_type is set, then use/fill
   // it to get colour indices from chainids.
   void handle_MET_or_MSE_case (PCAtom mse_atom, int udd_handle, int atom_colour_type,
				coot::my_atom_colour_map_t *atom_colour_map = 0);
   void handle_long_bonded_atom(PCAtom     atom, int udd_handle, int atom_colour_type,
				coot::my_atom_colour_map_t *atom_colour_map = 0);

   // void check_atom_limits(atom_selection_container_t SelAtoms) const;
   
   void write(std::string) const;

   PPCAtom trans_sel(atom_selection_container_t AtomSel, 
		     const std::pair<symm_trans_t, Cell_Translation> &symm_trans) const;

   void add_zero_occ_spots(const atom_selection_container_t &SelAtom);
   void add_deuterium_spots(const atom_selection_container_t &SelAtom);
   void add_atom_centres(const atom_selection_container_t &SelAtom, int atom_colour_type);
   int add_ligand_bonds(const atom_selection_container_t &SelAtom, 
			PPCAtom ligand_atoms_selection,
			int n_ligand_atoms);

   bool draw_these_residue_contacts(CResidue *this_residue, CResidue *env_residue,
				    coot::protein_geometry *protein_geom);

   // abstract this out of construct_from_atom_selection for cleanliness.
   // 
   void mark_atoms_as_bonded(CAtom *atom_p_1, CAtom *atom_p_2, bool have_udd_atoms, int udd_handle, bool done_bond_udd_handle) const;
   

   void add_half_bonds(const coot::Cartesian &atom_1,
		       const coot::Cartesian &atom_2,
		       CAtom *at_1,
		       CAtom *at_2,
		       int atom_colour_type);

   // double and delocalized bonds (default (no optional arg) is double).
   // 
   void add_double_bond(int iat_1, int iat_2, PPCAtom atoms, int n_atoms, int atom_colour_type,
			const std::vector<coot::dict_bond_restraint_t> &bond_restraints,
			bool is_deloc=0);
   // used by above, can throw an exception
   clipper::Coord_orth get_neighb_normal(int iat_1, int iat_2, PPCAtom atoms, int n_atoms, 
	 				 bool also_2nd_order_neighbs=0) const;
   void add_triple_bond(int iat_1, int iat_2, PPCAtom atoms, int n_atoms, int atom_colour_type,
			const std::vector<coot::dict_bond_restraint_t> &bond_restraints);


   bool have_dictionary;
   const coot::protein_geometry *geom;
   enum { NOT_HALF_BOND, HALF_BOND_FIRST_ATOM, HALF_BOND_SECOND_ATOM };

 protected:
   std::vector<Bond_lines> bonds; 
   std::vector<coot::Cartesian>  zero_occ_spots;
   std::vector<coot::Cartesian>  deuterium_spots;
   std::vector<std::pair<bool, coot::Cartesian> >  atom_centres;
   std::vector<int>        atom_centres_colour;
   void addBond(int colour, const coot::Cartesian &first, const coot::Cartesian &second,
		bool add_begin_end_cap = false,
		bool add_end_end_cap = false);
   void addBondtoHydrogen(const coot::Cartesian &first, const coot::Cartesian &second);
   // void add_deloc_bond_lines(int colour, const coot::Cartesian &first, const coot::Cartesian &second,
   // int deloc_half_flag);
   void add_dashed_bond(int col,
			const coot::Cartesian &start,
			const coot::Cartesian &end,
			int half_bond_type_flag);
   void addAtom(int colour, const coot::Cartesian &pos);
   int atom_colour(CAtom *at, int bond_colour_type, coot::my_atom_colour_map_t *atom_colour_map = 0);
   void bonds_size_colour_check(int icol) {
      int bonds_size = bonds.size();
      if (icol >= bonds_size) 
	 bonds.resize(icol+1);
   }

   // return the UDD handle
   int set_rainbow_colours(CMMDBManager *mol);
   void do_colour_by_chain_bonds_change_only(const atom_selection_container_t &asc,
					     int draw_hydrogens_flag);

   void try_set_b_factor_scale(CMMDBManager *mol);
   graphical_bonds_container make_graphical_bonds(bool thinning_flag) const;
   void add_bonds_het_residues(const std::vector<std::pair<bool, CResidue *> > &het_residues, int atom_colour_t, short int have_udd_atoms, int udd_handle);
   void het_residue_aromatic_rings(CResidue *res, const coot::dictionary_residue_restraints_t &restraints, int col);
   // pass a list of atom name that are part of the aromatic ring system.
   void add_aromatic_ring_bond_lines(const std::vector<std::string> &ring_atom_names, CResidue *res, int col);
   bool invert_deloc_bond_displacement_vector(const clipper::Coord_orth &vect,
					      int iat_1, int iat_2, PPCAtom residue_atoms, int n_atoms,
					      const std::vector<coot::dict_bond_restraint_t> &bond_restraints) const;

   // add to het_residues maybe
   bool add_bond_by_dictionary_maybe(CAtom *atom_p_1,
				     CAtom *atom_p_2,
				     std::vector<std::pair<bool, CResidue *> > *het_residues);



public:
   enum bond_representation_type { COLOUR_BY_OCCUPANCY, COLOUR_BY_B_FACTOR}; 

   // getting caught out with Bond_lines_container dependencies?
   // We need:  mmdb-extras.h which needs mmdb-manager.h and <string>
   // Bond_lines_container(const atom_selection_container_t &asc);

   Bond_lines_container(const atom_selection_container_t &asc,
			int include_disulphides=0,
			int include_hydrogens=1);

   // the constructor for bond by dictionary - should use this most of the time.
   // geom_in can be null if you don't have it.
   //
   // if model_number is 0, display all models. If it is not 0 then
   // display only the given model_number (if possible, of course).
   // 
   Bond_lines_container(const atom_selection_container_t &asc,
			coot::protein_geometry *geom_in,
			int include_disulphides,
			int include_hydrogens,
			int model_number);

   Bond_lines_container(atom_selection_container_t, float max_dist);

   Bond_lines_container(atom_selection_container_t asc, 
			float min_dist, float max_dist);

   // The constructor for ball and stick, this constructor implies that
   // for_GL_solid_model_rendering is set.
   // 
   // geom_in can be null.
   Bond_lines_container (atom_selection_container_t asc, const coot::protein_geometry *geom);

   Bond_lines_container(atom_selection_container_t SelAtom, coot::Cartesian point,
			float symm_distance,
			std::vector<symm_trans_t> symm_trans); // const & FIXME

   // This finds bonds between a residue and the protein (in SelAtom).
   // It is used for the environment bonds box.
   // 
   Bond_lines_container(const atom_selection_container_t &SelAtom,
			PPCAtom residue_atoms,
			int n_residue_atoms,
			coot::protein_geometry *protein_geom, // modifiable, currently
			bool residue_is_water_flag,
			bool draw_env_distances_to_hydrogens_flag,
			float min_dist,
			float max_dist);

   // same as above, except this does symmetry contacts too.
   // 
   Bond_lines_container(const atom_selection_container_t &SelAtom,
			PPCAtom residue_atoms,
			int n_residue_atoms,
			float min_dist,
			float max_dist, 
			bool draw_env_distances_to_hydrogens_flag,
			short int do_symmetry);

   // This is the one for occupancy and B-factor representation
   // 
   Bond_lines_container (const atom_selection_container_t &SelAtom,
			 bond_representation_type by_occ);

   Bond_lines_container(int col);
   Bond_lines_container(symm_keys key);


   // Used by make_colour_by_chain_bonds() - and others in the future?
   //
   Bond_lines_container(coot::protein_geometry *protein_geom) {
      do_bonds_to_hydrogens = 1;  // added 20070629
      b_factor_scale = 1.0;
      have_dictionary = false;
      geom = protein_geom;
      if (protein_geom)
	 have_dictionary = true;
      for_GL_solid_model_rendering = 0;
      if (bonds.size() == 0) { 
	 for (int i=0; i<10; i++) { 
	    Bond_lines a(i);
	    bonds.push_back(a);
	 }
      }
   }

   

   // initial constructor, added to by  addSymmetry_vector_symms from update_symmetry()
   // 
   Bond_lines_container() {
      do_bonds_to_hydrogens = 1;  // added 20070629
      b_factor_scale = 1.0;
      have_dictionary = 0;
      for_GL_solid_model_rendering = 0;
      if (bonds.size() == 0) { 
	 for (int i=0; i<10; i++) { 
	    Bond_lines a(i);
	    bonds.push_back(a);
	 }
      }
   }
   
   // arguments as above.
   // 
   // FYI: there is only one element to symm_trans, the is called from
   // them addSymmetry_vector_symms wrapper
   graphical_bonds_container
      addSymmetry(const atom_selection_container_t &SelAtom,
		  coot::Cartesian point,
		  float symm_distance,
		  const std::vector<std::pair<symm_trans_t, Cell_Translation> > &symm_trans,
		  short int symmetry_as_ca_flag,
		  short int symmetry_whole_chain_flag);
   graphical_bonds_container
      addSymmetry_with_mmdb(const atom_selection_container_t &SelAtom,
			    coot::Cartesian point,
			    float symm_distance,
			    const std::vector<std::pair<symm_trans_t, Cell_Translation> > &symm_trans,
			    short int symmetry_as_ca_flag); 

   // FYI: there is only one element to symm_trans, the is called from
   // them addSymmetry_vector_symms wrapper
   graphical_bonds_container
      addSymmetry_calphas(const atom_selection_container_t &SelAtom,
			  const coot::Cartesian &point,
			  float symm_distance,
			  const std::vector<std::pair<symm_trans_t, Cell_Translation> > &symm_trans);

   // FYI: there is only one element to symm_trans, the is called from
   // them addSymmetry_vector_symms wrapper
   graphical_bonds_container
     addSymmetry_whole_chain(const atom_selection_container_t &SelAtom,
			     const coot::Cartesian &point,
			     float symm_distance, 
			     const std::vector <std::pair<symm_trans_t, Cell_Translation> > &symm_trans);

   // symmetry with colour-by-symmetry-operator: If
   // symmetry_whole_chain_flag is set, then if any part of a symmetry
   // related molecule is found, then select the whole chain.
   // 
   std::vector<std::pair<graphical_bonds_container, std::pair<symm_trans_t, Cell_Translation> > >
      addSymmetry_vector_symms(const atom_selection_container_t &SelAtom,
			       coot::Cartesian point,
			       float symm_distance,
			       const std::vector<std::pair<symm_trans_t, Cell_Translation> > &symm_trans,
			       short int symmetry_as_ca_flag,
			       short int symmetry_whole_chain_flag,
			       short int draw_hydrogens_flag);

   std::vector<std::pair<graphical_bonds_container, symm_trans_t> >
      add_NCS(const atom_selection_container_t &SelAtom,
	      coot::Cartesian point,
	      float symm_distance,
	      std::vector<std::pair<coot::coot_mat44, symm_trans_t> > &strict_ncs_mats,
	      short int symmetry_as_ca_flag,
	      short int symmetry_whole_chain_flag);

   graphical_bonds_container
      add_NCS_molecule(const atom_selection_container_t &SelAtom,
		       const coot::Cartesian &point,
		       float symm_distance,
		       const std::pair<coot::coot_mat44, symm_trans_t> &strict_ncs_mat,
		       short int symmetry_as_ca_flag,
		       short int symmetry_whole_chain_flag);

   graphical_bonds_container
      add_NCS_molecule_calphas(const atom_selection_container_t &SelAtom,
			       const coot::Cartesian &point,
			       float symm_distance,
			       const std::pair<coot::coot_mat44, symm_trans_t> &strict_ncs_mat);

   graphical_bonds_container
      add_NCS_molecule_whole_chain(const atom_selection_container_t &SelAtom,
				   const coot::Cartesian &point,
				   float symm_distance,
				   const std::pair<coot::coot_mat44, symm_trans_t> &strict_ncs_mat);

   graphical_bonds_container make_graphical_bonds() const;
   graphical_bonds_container make_graphical_bonds_no_thinning() const;

     
   // debugging function
   void check() const; 

   void no_symmetry_bonds();


   // void make_graphical_symmetry_bonds() const;
   graphical_bonds_container make_graphical_symmetry_bonds() const; 

   void check_graphical_bonds() const; 
   void check_static() const; 
   void do_disulphide_bonds(atom_selection_container_t, int imodel);
   void do_Ca_bonds(atom_selection_container_t SelAtom, 
		    float min_dist, float max_dist);
   // make bonds/lies dependent on residue order in molecule - no neighbour search needed. Don't show HETATMs
   coot::my_atom_colour_map_t do_Ca_or_P_bonds_internal(atom_selection_container_t SelAtom,
							const char *backbone_atom_id,
							coot::my_atom_colour_map_t acm,
							float min_dist, float max_dist,
							int bond_colour_type); 
   coot::my_atom_colour_map_t do_Ca_or_P_bonds_internal_old(atom_selection_container_t SelAtom,
							const char *backbone_atom_id,
							coot::my_atom_colour_map_t acm,
							float min_dist, float max_dist,
							int bond_colour_type); 
   void do_Ca_plus_ligands_bonds(atom_selection_container_t SelAtom, 
				 float min_dist, float max_dist);
   void do_Ca_plus_ligands_bonds(atom_selection_container_t SelAtom, 
				 float min_dist, float max_dist,
				 int atom_colour_type); 
   void do_colour_by_chain_bonds(const atom_selection_container_t &asc,
				 int draw_hydrogens_flag,
				 short int change_c_only_flag);
   void do_colour_by_molecule_bonds(const atom_selection_container_t &asc,
				    int draw_hydrogens_flag);
   void do_normal_bonds_no_water(const atom_selection_container_t &asc,
				 float min_dist, float max_dist);
   void do_colour_sec_struct_bonds(const atom_selection_container_t &asc,
				   float min_dist, float max_dist); 
   void do_Ca_plus_ligands_colour_sec_struct_bonds(const atom_selection_container_t &asc,
						   float min_dist, float max_dist); 

   atom_selection_container_t
      ContactSel(PPCAtom trans_sel, PSContact contact, int ncontacts) const;
   void do_symmetry_Ca_bonds(atom_selection_container_t SelAtom,
			     symm_trans_t symm_trans);

   void set_verbose_reporting(short int i) { verbose_reporting = i;}

   std::vector<coot::torus_description_t> rings; // for OpenGL rendering of aromaticity bonding.
   bool have_rings() const {
      return rings.size();
   }

};

#endif /* BOND_LINES_H */
