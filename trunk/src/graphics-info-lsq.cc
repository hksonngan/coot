/* src/graphics-info-lsq.cc
 * 
 * Copyright 2004 by The University of York
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


#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif

#include <gtk/gtk.h>  // must come after mmdb_manager on MacOS X Darwin
#include <GL/glut.h>  // for some reason...  // Eh?

#include "clipper/core/rotation.h"

#include <iostream>

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "interface.h"

#include "molecule-class-info.h"

#include "graphics-info.h"

#include "coot-utils.hh"



// --------- LSQing ---------------
std::pair<int, clipper::RTop_orth>
graphics_info_t::apply_lsq(int imol_ref, int imol_moving,
			   const std::vector<coot::lsq_range_match_info_t> &matches) {

   int status = 0;
   clipper::RTop_orth rtop_r;
   if (imol_ref < n_molecules) {
      if (molecules[imol_ref].has_model()) {
	 if (imol_moving < n_molecules) {
	    if (molecules[imol_moving].has_model()) {
	       std::pair<short int, clipper::RTop_orth> rtop_info =
		  coot::util::get_lsq_matrix(molecules[imol_ref].atom_sel.mol,
					     molecules[imol_moving].atom_sel.mol,
					     matches);
	       if (rtop_info.first) {

		  // A blob of code from Kevin for the axis orienation:
		  clipper::Mat33<> mat = rtop_info.second.rot();
		  clipper::Vec3<> v0( mat(0,0) - 1.0, mat(1,0), mat(2,0) );
		  clipper::Vec3<> v1( mat(0,1), mat(1,1) - 1.0, mat(2,1) );
		  clipper::Vec3<> v2( mat(0,2), mat(1,2), mat(2,2) - 1.0 );
		  clipper::Vec3<> v3 = clipper::Vec3<>::cross( v1, v2 );
		  clipper::Vec3<> v4 = clipper::Vec3<>::cross( v0, v2 );
		  clipper::Vec3<> v5 = clipper::Vec3<>::cross( v0, v1 );
		  if ( v3*v3 > v4*v4 && v3*v3 > v5*v5 )
		     v0 = v3.unit();
		  else if ( v4*v4 > v5*v5 )
		     v0 = v4.unit();
		  else
		     v0 = v5.unit();

		  std::cout << "INFO:: Axis orientation: " << v0.format() << std::endl;
		  std::cout << "INFO:: Rotation in CCP4 Polar Angles: "
			    << clipper::Rotation(rtop_info.second.rot()).polar_ccp4().format()
			    << std::endl;
		  

		  molecules[imol_moving].transform_by(rtop_info.second);
		  rtop_r = rtop_info.second;
		  graphics_draw();
		  status = 1; // done good (used to dismiss/destroy the widget)
	       }
	    }
	 }
      }
   }

   return std::pair<int, clipper::RTop_orth> (status, rtop_r);
}
