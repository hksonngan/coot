/* ligand/rigid-body.hh
 * 
 * Copyright 2005 by Paul Emsley, The University of York
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef COOT_RIGID_BODY_HH
#define COOT_RIGID_BODY_HH

#include "mini-mol.hh"
#include "clipper/core/xmap.h"

// move the atoms of mol
namespace coot { 
   void rigid_body_fit(minimol::molecule *mol, const clipper::Xmap<float> &xmap);
   clipper::Vec3<double>
   get_rigid_body_angle_components(const std::vector<minimol::atom *> &atoms_p,
				   const clipper::Coord_orth &mean_pos,
				   const std::vector<clipper::Grad_orth<float> > &grad_vec,
				   double gradient_scale);
   void apply_angles_to_molecule(const clipper::Vec3<double> &angles,
				 const std::vector<minimol::atom *> *atoms_p,
				 const clipper::Coord_orth &mean_pos);
   float score_molecule(const coot::minimol::molecule &m,
			const clipper::Xmap<float> &xmap);

}

#endif // COOT_RIGID_BODY_HH
