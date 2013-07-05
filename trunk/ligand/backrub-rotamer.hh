/* ligand/backrub-rotamer.hh
 * 
 * Copyright 2009 by The University of Oxford.
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
 * 02110-1301, USA.
 */

#ifndef BACKRUB_ROTAMER_HH
#define BACKRUB_ROTAMER_HH

#include <string>

#include <clipper/core/xmap.h>
#include "mini-mol/mini-mol.hh"

namespace coot {

   // not just for backrubbing.
   float get_clash_score(const coot::minimol::molecule &a_rotamer,
			 atom_selection_container_t asc);

   class backrub {
      CResidue *orig_this_residue;
      CResidue *orig_prev_residue;
      CResidue *orig_next_residue;
      std::string alt_conf;
      clipper::Coord_orth ca_prev;
      clipper::Coord_orth ca_next;
      clipper::Coord_orth ca_this;
      // thow an exception on failure to find CA of prev or next.
      void setup_this_and_prev_next_ca_positions();
      minimol::fragment make_test_fragment(CResidue *r, double rotation_angle) const;
      std::string chain_id;
      float score_fragment(minimol::fragment &frag) const;
      clipper::Xmap<float> xmap;
      CMMDBManager *stored_mol;
      minimol::residue
      make_residue_include_only(CResidue *orig_prev_residue,
				const std::vector<std::string> &prev_res_atoms) const;
      clipper::Coord_orth rotamer_residue_centre() const;
      float residue_radius(const clipper::Coord_orth &rc);

      // do a check of the residue numbers and chaid id so that "same
      // residue" clashes are not counted.
      float get_clash_score(const coot::minimol::molecule &a_rotamer,
			    PPCAtom sphere_atoms, int n_sphere_atoms) const;

      void rotate_individual_peptide(CResidue *r, double rotation_angle,
				     minimol::fragment *f) const;
      // an analysis function
      // 
      double sample_individual_peptide(CResidue *r, double rotation_angle,
				       minimol::fragment *f,
				       CResidue *residue_front,
				       CResidue *residue_back,
				       bool is_leading_peptide_flag) const;
      // A modelling function.
      // Fiddle with the atom positions of fragment f.
      // 
      void rotate_individual_peptides_back_best(CResidue *r, double rotation_angle,
						minimol::fragment *f) const;

      // fiddle with the peptide position in f
      // 
      void apply_back_rotation(minimol::fragment *f,				   
			       bool is_leading_peptide_flag,
			       double best_back_rotation_angle) const;
      
      
   public:

      // Throw an exception on failure to construct the backrub internals.
      // Throw an exception if this, previous or next residues are null
      backrub(const std::string &chain_id_in,
	      CResidue *this_r,
	      CResidue *prev_r,
	      CResidue *next_r,
	      const std::string &alt_conf_in,
	      CMMDBManager *mol_in,
	      const clipper::Xmap<float> &xmap_in) {
	 orig_this_residue = this_r;
	 orig_prev_residue = prev_r;
	 orig_next_residue = next_r;
	 setup_this_and_prev_next_ca_positions();
	 chain_id = chain_id_in;
	 xmap = xmap_in;
	 alt_conf = alt_conf_in;
	 stored_mol = mol_in;
      }

      // throw an exception on failure to get a good search result.
      std::pair<coot::minimol::molecule, float> search(const dictionary_residue_restraints_t &rest);
   };

}


#endif // BACKRUB_ROTAMER_HH
