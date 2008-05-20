/* coords/mmdb.h
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

// needs to have included mmdb_manager.h"
// and "mmdb-extras.h" for atom_selection_container_t
#include "Cartesian.h"


coot::Cartesian
centre_of_molecule(atom_selection_container_t SelAtom); 


atom_selection_container_t get_atom_selection(std::string t);
int fix_nucleic_acid_residue_names(atom_selection_container_t asc);
int fix_nucleic_acid_residue_name(CResidue *r); // return whether it was changed or not.

// return the number of fixed atoms
int fix_away_atoms(atom_selection_container_t asc);

// return the number of changed hydrogen names
int 
fix_hydrogen_names(atom_selection_container_t asc);

int write_atom_selection_file(atom_selection_container_t asc,
			      const std::string &filename, 
			      byte gz);


// needs <iostream.h>
// 
ostream& operator<<(ostream& s, CAtom &atom);

ostream& operator<<(ostream& s, PCAtom atom); 
