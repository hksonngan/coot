/* src/c-interface.cc
 * 
 * Copyright 2010 by The University of Oxford
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
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


#ifdef USE_GUILE
#include <libguile.h>
#endif

#include "cc-interface.hh"
#include "graphics-info.h"

#include "c-interface.h"

/*  ----------------------------------------------------------------------- */
/*                  Functions for FLEV layout callbacks                     */
/*  ----------------------------------------------------------------------- */
// orient the graphics somehow so that the interaction between
// central_residue and neighbour_residue is perpendicular to screen z.
void orient_view(int imol,
		 const coot::residue_spec_t &central_residue_spec, // ligand typically
		 const coot::residue_spec_t &neighbour_residue_spec) {

   graphics_info_t g;
   if (g.use_graphics_interface_flag) {
      if (g.is_valid_model_molecule(imol)) {
	 try { 
	    clipper::Coord_orth vec = g.molecules[imol].get_vector(central_residue_spec,
								   neighbour_residue_spec);
	    coot::Cartesian b = unproject(1);
	    coot::Cartesian f = unproject(0);
	    coot::Cartesian vec_cart(vec);
	    coot::Cartesian b_to_f_cart = f - b;

	    coot::Cartesian centre = unproject_xyz(0, 0, 0.5);
	    coot::Cartesian front  = unproject_xyz(0, 0, 0.0);
	    coot::Cartesian right  = unproject_xyz(1, 0, 0.5);
	    coot::Cartesian top    = unproject_xyz(0, 1, 0.5);

	    coot::Cartesian screen_x = (right - centre);
	    coot::Cartesian screen_y = (top   - centre);
	    coot::Cartesian screen_z = (front - centre);

	    screen_x.unit_vector_yourself();
	    screen_y.unit_vector_yourself();
	    screen_z.unit_vector_yourself();

	    double a_s_x = dot_product(screen_x, vec_cart);
	    double a_s_z = dot_product(screen_z, vec_cart);

	    double theta = atan2(a_s_z, a_s_x) * 0.5;

	    std::cout << "theta: " << clipper::Util::rad2d(theta) << " degrees with asx " 
		      << a_s_x << " and asz " << a_s_z << std::endl;

	    rotate_y_scene(100, 0.01 * clipper::Util::rad2d(theta));

				       
	 }
	 catch (std::runtime_error rte) {
	    std::cout << rte.what() << std::endl;
	 }
      }
   }
}

/*  ----------------------------------------------------------------------- */
/*                         single-model view */
/*  ----------------------------------------------------------------------- */
/*! \name single-model view */
/* \{ */
/*! \brief put molecule number imol to display only model number imodel */
void single_model_view_model_number(int imol, int imodel) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.molecules[imol].single_model_view_model_number(imodel);
      graphics_draw();
      std::string s = "Model number ";
      s += coot::util::int_to_string(imodel);
      add_status_bar_text(s.c_str());
   } 
}

/*! \brief the current model number being displayed */
int single_model_view_this_model_number(int imol) {
   int r = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      r = g.molecules[imol].single_model_view_this_model_number();
      std::string s = "Model number ";
      s += coot::util::int_to_string(r);
      add_status_bar_text(s.c_str());
      graphics_draw();
   }
   return r;
}

/*! \brief the next model number to be displayed */
int single_model_view_next_model_number(int imol) {
   int r = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      r = g.molecules[imol].single_model_view_next_model_number();
      std::string s = "Model number ";
      s += coot::util::int_to_string(r);
      add_status_bar_text(s.c_str());
      graphics_draw();
   }
   return r;
}

/*! \brief the previous model number to be displayed */
int single_model_view_prev_model_number(int imol) {
   int r = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      r = g.molecules[imol].single_model_view_prev_model_number();
      std::string s = "Model number ";
      s += coot::util::int_to_string(r);
      add_status_bar_text(s.c_str());
      graphics_draw();
   }
   return r;
}

