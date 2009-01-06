/* ligand/dipole.hh
 * 
 * Copyright 2009 by The University of Oxford
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
 * 02110-1301, USA.
 */

#include <iostream>
#include <fstream>
#include "protein-geometry.hh"

#ifndef HAVE_MMMDB
#include <mmdb_manager.h>
#define HAVE_MMMDB
#endif

namespace coot {

   class dipole {
      bool dipole_is_good_flag;
      clipper::Coord_orth dipole_;
      clipper::Coord_orth residue_centre;
   public:
      // Thow an exception on failure to make a dipole
      // 
      dipole(const dictionary_residue_restraints_t &rest,
	     CResidue *residue_p);
      friend std::ostream& operator<<(std::ostream &s, const dipole &d);
      clipper::Coord_orth position() const { return residue_centre; };
      clipper::Coord_orth get_dipole() const { return dipole_; }
      clipper::Coord_orth get_unit_dipole() const;
      std::vector<std::pair<CAtom *, float> > charged_atoms(CResidue *r, const dictionary_residue_restraints_t &rest) const;
   }; 
   std::ostream& operator<<(std::ostream &s, const dipole &d);
}
