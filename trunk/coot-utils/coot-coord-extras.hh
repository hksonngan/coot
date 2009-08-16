/* coot-utils/coot-coord-extras.cc
 * 
 * Copyright 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2009 by the University of Oxford
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

#ifndef HAVE_COOT_COORD_EXTRAS_HH
#define HAVE_COOT_COORD_EXTRAS_HH

#include <map>
#include "protein-geometry.hh"
#include "atom-quads.hh"


namespace coot {

   namespace util { 
      // geom_p gets updated to include the residue restraints if necessary
      // 
      std::pair<int, std::vector<std::string> >
      check_dictionary_for_residues(PCResidue *SelResidues, int nSelResidues,
				    protein_geometry *geom_p, int read_number);

      // We also now pass regular_residue_flag so that the indexing of the
      // contacts is inverted in the case of not regular residue.  I don't
      // know why this is necessary, but I have stared at it for hours, this
      // is a quick (ugly hack) fix that works.  I suspect that there is
      // some atom order dependency in mgtree that I don't understand.
      // Please fix (remove the necessity of depending on
      // regular_residue_flag) if you know how.
      // 
      std::vector<std::vector<int> >
      get_contact_indices_from_restraints(CResidue *residue,
					  protein_geometry *geom_p,
					  short int regular_residue_flag);

      std::vector<std::vector<int> >
      get_contact_indices_for_PRO_residue(PPCAtom residue_atom,
					  int nResidueAtoms, 
					  coot::protein_geometry *geom_p);

      class missing_atom_info {
      public:
	 std::vector<std::string> residues_with_no_dictionary;
	 std::vector<CResidue *>  residues_with_missing_atoms;
	 std::vector<std::pair<CResidue *, std::vector<CAtom *> > > atoms_in_coords_but_not_in_dict;
	 missing_atom_info() {}
	 missing_atom_info(const std::vector<std::string> &residues_with_no_dictionary_in,
			   const std::vector<CResidue *>  &residues_with_missing_atoms_in,
			   const std::vector<std::pair<CResidue *, std::vector<CAtom *> > > &atoms_in_coords_but_not_in_dict_in) {
	    residues_with_no_dictionary = residues_with_no_dictionary_in;
	    residues_with_missing_atoms = residues_with_missing_atoms_in;
	    atoms_in_coords_but_not_in_dict = atoms_in_coords_but_not_in_dict_in;
	 }
      };

      class dict_atom_info_t {
      public:
	 std::string name;
	 short int is_Hydrogen_flag;
	 dict_atom_info_t(const std::string &name_in, short int is_Hydrogen_flag_in) {
	    name = name_in;
	    is_Hydrogen_flag = is_Hydrogen_flag_in;
	 }
      };

      // a trivial helper class
      class dict_residue_atom_info_t {
      public:
	 std::string residue_name;
	 std::vector<dict_atom_info_t> atom_info;
	 dict_residue_atom_info_t(const std::string &residue_name_in,
				  const std::vector<dict_atom_info_t> &atom_info_in) {
	    residue_name = residue_name_in;
	    atom_info = atom_info_in;
	 }
	 // Here is the clever stuff, get the restraints info for a
	 // particular residue and from that set the atom_info.
	 dict_residue_atom_info_t(const std::string &residue_name,
				  protein_geometry *geom_p);
	 bool is_empty_p() const {
	    return (atom_info.size() == 0);
	 }
      };

      // do we need to pass read number to this function too?
      short int is_nucleotide_by_dict_dynamic_add(CResidue *residue_p, coot::protein_geometry *geom_p);
      short int is_nucleotide_by_dict(CResidue *residue_p, const coot::protein_geometry &geom);
   }

   class atom_vertex {

   public:
      enum connection_type_t { START, END, STANDARD, NONE };
      connection_type_t connection_type;
      std::vector<int> forward;
      std::vector<int> backward;
      std::pair<bool,atom_index_quad> torsion_quad;
      atom_vertex() {
	 connection_type = NONE;
	 torsion_quad.first = 0;
      }
   }; 

   class atom_tree_t {
      
      class atom_tree_index_t {
	 int index_;
      public:
	 enum index_type { UNASSIGNED = -1 };
	 atom_tree_index_t() { index_ = UNASSIGNED; }
	 atom_tree_index_t(int i) { index_ = i; }
	 int index() const { return index_; }
	 bool is_assigned() { return (index_ != UNASSIGNED); }
	 bool operator==(const atom_tree_index_t &ti) const {
	    return (ti.index() == index_);
	 }
      };

   protected: 
      CResidue *residue;
      std::vector<std::pair<int, int> > bonds;
      std::vector<coot::atom_vertex> atom_vertex_vec;
      std::map<std::string, atom_tree_index_t, std::less<std::string> > name_to_index;
   private: 
      bool fill_atom_vertex_vec(const dictionary_residue_restraints_t &rest, CResidue *res,
				const std::string &altconf);
      bool fill_atom_vertex_vec_using_contacts(const std::vector<std::vector<int> > &contact_indices,
					       int base_atom_index);
      bool fill_torsions(const dictionary_residue_restraints_t &rest, CResidue *res,
			 const std::string &altconf);
      void fill_name_map(const std::string &altconf);

      // Throw an exception on not able to fill.
      atom_index_quad get_atom_index_quad(const coot::dict_torsion_restraint_t &tr,
					  CResidue *res, const std::string &altconf) const;
      std::vector<atom_tree_index_t> get_back_atoms(const atom_tree_index_t &index2) const;
      std::vector<atom_tree_index_t> get_forward_atoms(const atom_tree_index_t &index2) const;
      std::vector<coot::atom_tree_t::atom_tree_index_t>
      uniquify_atom_indices(const std::vector<coot::atom_tree_t::atom_tree_index_t> &vin) const;

      // Return the complementary indices c.f. the moving atom set,
      // but do not include index2 or index3 in the returned set (they
      // do not move even with the reverse flag (of course)).
      std::vector<atom_tree_index_t> 
      complementary_indices(const std::vector<atom_tree_index_t> &moving_atom_indices,
			    const atom_tree_index_t &index2,
			    const atom_tree_index_t &index3) const;

      // add forward_atom_index as a forward atom of this_index - but
      // only if forward_atom_index is not already a forward atom of
      // this_index.
      void add_unique_forward_atom(int this_index, int forward_atom_index);

      // so now we have a set of moving and non-moving atoms:
      void rotate_internal(std::vector<coot::atom_tree_t::atom_tree_index_t> moving_atom_indices,
			   const clipper::Coord_orth &dir,
			   const clipper::Coord_orth &base_atom_pos,
			   double angle);
      double quad_to_torsion(const atom_tree_index_t &index2) const;
      
   public:
      // the constructor can throw an exception if there is no tree in the restraints.
      atom_tree_t(const dictionary_residue_restraints_t &rest, CResidue *res,
		  const std::string &altconf);

      // the constructor, given a list of bonds and a base atom index.
      // Used perhaps as the fallback when the above raises an
      // exception.
      atom_tree_t(const std::vector<std::vector<int> > &contact_indices,
		  int base_atom_index, 
		  CResidue *res,
		  const std::string &alconf); 

      // Rotate round the 2 middle atoms of the torsion by angle (in
      // degress).  This is a relative rotation - not setting the torsion angle.
      // The atoms of the CResidue residue are manipulated.
      //
      // The reversed flag (being true) allows the rotation of the
      // base, rather than the branch (dog wags rather than tail).
      // atom1 and atom2 can be passed in either way round, this
      // function will sort out the position in the tree.  The
      // fragment rotation is reversed by setting the reversed_flag
      // (not the atom order).
      // 
      // Return the new torsion angle (use the embedded torsion on index2 if you can)
      // Otherwise return -1.0;
      // 
      double rotate_about(const std::string &atom1, const std::string &atom2,
			  double angle,
			  bool reversed_flag);

      // this can throw an exception
      double set_dihedral(const std::string &atom1, const std::string &atom2,
			  const std::string &atom3, const std::string &atom4,
			  double angle);

   };

}

#endif // HAVE_COOT_COORD_EXTRAS_HH
