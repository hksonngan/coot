/* src/testing.hh
 * 
 * Copyright 2008, 2009 by the University of Oxford
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

#ifdef USE_PYTHON
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "mmdb-extras.h"
#include "mmdb.h"

#include "protein-geometry.hh"
#include "coot-coord-extras.hh"


#define BUILT_IN_TESTING

std::string stringify(double x);
std::string stringify(int i);
std::string stringify(unsigned int i);
std::string greg_test(const std::string &file_name); // return directory of testing data

// a shorthand so that the push back line doesn't get too long:
typedef std::pair<int(*)(), std::string> named_func;

void add_test(int(*)(), const std::string &test_name, std::vector<named_func> *functions);

int test_internal();
int greg_internal_tests(); // portal to internal tests from greg
int greg_tests_using_external_data(); // portal to greg test using data from greg data directory
int test_internal_single();

int run_internal_tests(std::vector<named_func> functions);

int test_alt_conf_rotamers();
int test_ligand_conformer_torsion_angles();
int test_wiggly_ligands();
int test_torsion_derivs();
int test_ramachandran_probabilities();
int test_fragmemt_atom_selection();
int kdc_torsion_test();
int test_add_atom();
int restr_res_vector();
int test_peptide_link();
int test_dictionary_partial_charges(); 
int test_dipole();
int test_segid_exchange();
int test_ligand_fit_from_given_point();
int test_peaksearch_non_close_peaks();
int test_symop_card();
int test_coot_atom_tree();
int test_coot_atom_tree_2();
int test_coot_atom_tree_proline();
int test_rotate_round_vector();
int test_ssm_sequence_formatting();
int test_OXT_in_restraints();
int test_relativise_file_name();
int test_previous_water();
int test_sbase();
int test_coordinated_waters();
int test_geometry_distortion_info_type();
int test_translate_close_to_origin();
int test_flev_aromatics();
int test_map_segmentation();
int test_copy_cell_symm_orig_scale_headers();
int test_residue_atom_renaming();

// uses greg data test data
int test_phi_psi_values();


CResidue *test_get_residue(CMMDBManager *mol, const std::string &chain_id, int resno);
bool test_tree_rotation(const coot::dictionary_residue_restraints_t &rest,
			CResidue *res,
			const std::string &rotate_atom_1,
			const std::string &rotate_atom_2,
			bool reverse_flag);
bool
test_rotate_atom_angle(const std::string &atom_name,
		       const clipper::Coord_orth &r_pt_1,
		       const clipper::Coord_orth &r_pt_2,
		       const clipper::Coord_orth &before_pos,
		       const clipper::Coord_orth &after_pos,
		       double test_angle);

class test_atom_tree_t : public coot::atom_tree_t {

public:
   test_atom_tree_t(const std::vector<std::vector<int> > &contact_indices,
		    int base_atom_index, 
		    CResidue *res,
		    const std::string &alconf) :
      coot::atom_tree_t(contact_indices, base_atom_index, res, alconf) {}
   bool test_atom_vec(const std::vector<std::vector<int> > &contact_indices) const;
};


class residue_selection_t {
public:
   CMMDBManager *mol;
   int nSelResidues;
   PCResidue *SelResidues;
   int SelectionHandle;
   void clear_up() {
      mol->DeleteSelection(SelectionHandle);
      delete mol;
      mol = 0;
   } 
};

