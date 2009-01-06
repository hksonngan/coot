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

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "mmdb-extras.h"
#include "mmdb.h"

#define BUILT_IN_TESTING

std::string stringify(double x);
std::string stringify(int i);
std::string stringify(unsigned int i);
std::string greg_test(const std::string &file_name);

// a shorthand so that the push back line doesn't get too long:
typedef std::pair<int(*)(), std::string> named_func;

void add_test(int(*)(), const std::string &test_name, std::vector<named_func> *functions);

int test_internal();

int test_alt_conf_rotamers();
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


