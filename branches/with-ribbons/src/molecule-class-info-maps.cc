/* src/molecule-class-info.h
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2007 by Paul Emsley
 * Copyright 2007, 2008 by The University of Oxford
 * 
 * Copyright 2008 The University of Oxford
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


// Having to set up the include files like this so that
// molecule-class-info.h can be parsed, is silly.

// For stat, mkdir:
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "Cartesian.h"
#include "mmdb-crystal.h"
#include "molecule-class-info.h"
#include "CIsoSurface.h"

#include "clipper/ccp4/ccp4_mtz_io.h"
#include "clipper/ccp4/ccp4_map_io.h"
#include "clipper/core/xmap.h"
#include "clipper/cns/cns_map_io.h"
#include "clipper/core/hkl_compute.h"
#include "clipper/core/map_utils.h" // Map_stats
#include "clipper/core/resol_basisfn.h"
#include "clipper/core/resol_targetfn.h"
#include "clipper/mmdb/clipper_mmdb.h"
#include "clipper/clipper-phs.h"
#include "clipper/contrib/sfcalc_obs.h"
#include "clipper/contrib/sfscale.h"
#include "clipper/contrib/sfweight.h"

#include "clipper/clipper-cif.h"
#include "clipper/contrib/sfcalc.h"

#include "xmap-utils.h"
#include "xmap-stats.hh"

#include "graphics-info.h"
#include <GL/glut.h> // needed (only?) for wirecube
#include "globjects.h" // for set_bond_colour()
#include "graphical_skel.h"

#include "mmdb.h"


void
molecule_class_info_t::sharpen(float b_factor) {

   int n_data = 0;
   int n_tweaked = 0;

//    clipper::HKL_data< clipper::datatypes::F_phi<float> > fphis = original_fphis;
   clipper::HKL_data< clipper::datatypes::F_phi<float> > fphis;
   clipper::HKL_info::HKL_reference_index hri;   
   for (hri = fphis.first(); !hri.last(); hri.next()) {
      n_data++;

      std::cout << " " << hri.invresolsq() << std::endl;

      fphis[hri].f() = 0.0;
      n_tweaked++;
   }
}



void
molecule_class_info_t::update_map() {

   update_map_internal();
}


void
molecule_class_info_t::update_map_internal() {

   if (has_map()) {
      coot::Cartesian rc(graphics_info_t::RotationCentre_x(),
			 graphics_info_t::RotationCentre_y(),
			 graphics_info_t::RotationCentre_z());

      update_map_triangles(graphics_info_t::box_radius, rc); 
      if (graphics_info_t::use_graphics_interface_flag) {
	 if (graphics_info_t::display_lists_for_maps_flag) {
	    graphics_info_t::make_gl_context_current(graphics_info_t::GL_CONTEXT_MAIN);
	    compile_density_map_display_list(SIDE_BY_SIDE_MAIN);
	    if (graphics_info_t::display_mode_use_secondary_p()) {
	       graphics_info_t::make_gl_context_current(graphics_info_t::GL_CONTEXT_SECONDARY);
	       compile_density_map_display_list(SIDE_BY_SIDE_SECONDARY);
	       graphics_info_t::make_gl_context_current(graphics_info_t::GL_CONTEXT_MAIN);
	    }
	 }
      }
   }
}

// Create a new combo box for this newly created map.
void
molecule_class_info_t::update_map_in_display_control_widget() const { 

   graphics_info_t g; 

   std::string dmn = name_for_display_manager();
   if (g.display_control_window())
      display_control_map_combo_box(g.display_control_window(), 
				    dmn.c_str(),
				    imol_no);

} 

void
molecule_class_info_t::compile_density_map_display_list(short int first_or_second) {

   // std::cout << "Deleting theMapContours " << theMapContours << std::endl;
   if (first_or_second == SIDE_BY_SIDE_MAIN) { 
      glDeleteLists(theMapContours.first, 1);
      theMapContours.first = glGenLists(1);
      if (theMapContours.first > 0) { 
	 glNewList(theMapContours.first, GL_COMPILE);
	 
// 	 std::cout << "in compile_density_map_display_list calling draw_density_map_internal(0,1)"
// 		   << " theMapContours.first " << theMapContours.first << std::endl;
	 draw_density_map_internal(0, 1, first_or_second); // don't use theMapContours (make them!)
	 
	 glEndList();
      } else {
	 std::cout << "Error:: Oops! bad display list index for SIDE_BY_SIDE_MAIN! " << std::endl;
      } 
   } 
   if (first_or_second == SIDE_BY_SIDE_SECONDARY) { 
      glDeleteLists(theMapContours.second, 1);
      theMapContours.second = glGenLists(1);
      if (theMapContours.second > 0) { 
	 glNewList(theMapContours.second, GL_COMPILE);
	 
// 	 std::cout << "in compile_density_map_display_list calling draw_density_map_internal(0,1)"
// 		   << " theMapContours.second " << theMapContours.second << std::endl;
	 draw_density_map_internal(0, 1, first_or_second); // don't use theMapContours (make them!)
	 glEndList();
      } else {
	 std::cout << "Error:: Oops! bad display list index for SIDE_BY_SIDE_SECONDARY! " << std::endl;
      } 
   } 
}


// old and apparently faster (both at rotating (surprisingly) and
// generation of the lines) immediate mode
// 
// This is for electron density of course, not a surface as molecular
// modellers would think of it.
// 
void 
molecule_class_info_t::draw_density_map(short int display_lists_for_maps_flag,
					short int main_or_secondary) {

   if (drawit_for_map)
      draw_density_map_internal(display_lists_for_maps_flag, drawit_for_map,
				main_or_secondary);
}


void
molecule_class_info_t::draw_density_map_internal(short int display_lists_for_maps_flag_local,
						 bool draw_map_local_flag,
						 short int main_or_secondary) {

   // std::cout << "   draw_density_map_internal() called for " << imol_no << std::endl;

   // 20080619:
   // When the screen centre is is moved and we have
   // display_lists_for_maps_flag and we are not displaying the
   // map currently, then the map triangles are getting updated
   // (but not displayed of course) but the display list is not
   // because drawit_for_map is always 0.
   //
   // So we try to solve that by giving
   // compile_density_map_display_list() a different route in.
   // draw_density_map_internal() is created and that is called with
   // display_lists_for_maps_flag as 0 (i.e. generate the vectors in a
   // glNewList() wrapper).


//    std::cout << "draw_density_map_internal for map number " << imol_no
// 	     << " display_lists_for_maps_flag_local " << display_lists_for_maps_flag_local
// 	     << " draw_map_local_flag " << draw_map_local_flag 
// 	     << std::endl;

   int nvecs = n_draw_vectors; // cartesianpair pointer counter (old code)
   if (draw_map_local_flag) {  // i.e. drawit_for_map (except when compiling a new map display list)

      // same test as has_map():
      if (xmap_is_filled[0]) {

// 	 std::cout << "DEBUG:: drawing map for mol " << imol_no
// 		   << " display_lists_for_maps_flag:  "
// 		   << display_lists_for_maps_flag_local << " " << theMapContours << std::endl;
	    
	 if (display_lists_for_maps_flag_local) {

	    GLuint display_list_index = 0; // bad
	    if (main_or_secondary == SIDE_BY_SIDE_MAIN)
	       display_list_index = theMapContours.first;
	    if (main_or_secondary == SIDE_BY_SIDE_SECONDARY)
	       display_list_index = theMapContours.second;
	    if (display_list_index > 0) {
// 	       std::cout << "OK:: using display list " << display_list_index
// 			 << " when main_or_secondary is " << main_or_secondary << std::endl;
	       glCallList(display_list_index);
	    } else { 
	       std::cout << "ERROR:: using display list " << display_list_index
			 << " when main_or_secondary is " << main_or_secondary << std::endl;
	    }
	    
	 } else { 

	    // std::cout << "DEBUG:: some vectors " << nvecs << std::endl;
	    // std::cout << "   debug draw immediate mode " << std::endl;
	    if ( nvecs > 0 ) {

	       coot::Cartesian start, finish;
	       int linesdrawn = 0;
	       //

	       glColor3dv (map_colour[0]);
	       glLineWidth(graphics_info_t::map_line_width);
      
	       glBegin(GL_LINES);
	       for (int i=0; i< nvecs; i++) { 
		  glVertex3f(draw_vectors[i].getStart().x(),
			     draw_vectors[i].getStart().y(),
			     draw_vectors[i].getStart().z());
		  glVertex3f(draw_vectors[i].getFinish().x(),
			     draw_vectors[i].getFinish().y(),
			     draw_vectors[i].getFinish().z());
		  if((++linesdrawn & 1023) == 0){
		     linesdrawn = 0;
		     glEnd();
		     glBegin(GL_LINES);
		  }
	       }
	       glEnd();
	    }

	    if (xmap_is_diff_map[0] == 1) {

	       if (n_diff_map_draw_vectors > 0) { 
	       
		  glColor3dv (map_colour[1]);
		  // we only need to do this if it wasn't done above.
		  if (n_draw_vectors == 0)
		     glLineWidth(graphics_info_t::map_line_width);
	       
		  int linesdrawn_dm = 0;
		  glBegin(GL_LINES);
		  for (int i=0; i< n_diff_map_draw_vectors; i++) { 
		  
		     glVertex3f(diff_map_draw_vectors[i].getStart().get_x(),
				diff_map_draw_vectors[i].getStart().get_y(),
				diff_map_draw_vectors[i].getStart().get_z());
		     glVertex3f(diff_map_draw_vectors[i].getFinish().get_x(),
				diff_map_draw_vectors[i].getFinish().get_y(),
				diff_map_draw_vectors[i].getFinish().get_z());
		     if((++linesdrawn_dm & 1023) == 0) {
			linesdrawn_dm = 0;
			glEnd();
			glBegin(GL_LINES);
		     }
		  }
		  glEnd();
	       }
	    }
	 }
      }
   }
}

// 
void
molecule_class_info_t::update_map_triangles(float radius, coot::Cartesian centre) {
   
   CIsoSurface<float> my_isosurface;
   coot::CartesianPairInfo v;
   int isample_step = 1;
   graphics_info_t g;

   // std::cout   << "DEBUG:: g.zoom: " << g.zoom << std::endl;

   if (g.dynamic_map_resampling == 1)
      // isample_step = 1 + int (0.009*g.zoom);
      isample_step = 1 + int (0.009*(g.zoom + g.dynamic_map_zoom_offset));

   if (isample_step > 15) 
      isample_step = 15;

   // for critical points of size display and resampling being different:
   // 
   float dy_radius = radius;
   if (g.dynamic_map_size_display == 1) { 
      if (isample_step <= 15 )
	 dy_radius *= float(isample_step);
      else
	 dy_radius *= 15.0;
   } 

   // 
   if (isample_step <= 0) { 
      std::cout << "WARNING:: Bad zoom   ("<< g.zoom 
		<< "):  setting isample_step to 1" << std::endl;
      isample_step = 1;
   }
   if (dy_radius <= 0.0) { 
      std::cout << "WARNING:: Bad radius (" << dy_radius 
		<< ") setting to 10" << std::endl;
      dy_radius = 10.0;
   }

   // dynamically transformed maps get their vectors from molecule B
   // (we are looking at molecule at atoms in molecule A) which then
   // have a the inverse of that transformation applied to them.
   //
   // But note that to get from the centre in the A molecule to the
   // corresponding centre in the B molecule we need to apply the
   // *inverse* of the transformation in the map_ghost_info.

   if (is_dynamically_transformed_map_flag) {
      clipper::Coord_orth c(centre.x(), centre.y(), centre.z());
      clipper::Coord_orth ct = c.transform(map_ghost_info.rtop.inverse());
      centre = coot::Cartesian(ct.x(), ct.y(), ct.z());
   }

   for (int i=0; i< max_xmaps; i++) {
      if (xmap_is_filled[i] == 1) {
	 v = my_isosurface.GenerateSurface_from_Xmap(xmap_list[0],
						     contour_level[i],
						     dy_radius, centre,
						     isample_step);
	 if (is_dynamically_transformed_map_flag)
	    dynamically_transform(v);
	 set_draw_vecs(v.data, v.size);
      }
      if (xmap_is_diff_map[i] == 1) {
	 v = my_isosurface.GenerateSurface_from_Xmap(xmap_list[0],
						     -contour_level[i],
						     dy_radius, centre,
						     isample_step);
	 if (is_dynamically_transformed_map_flag)
	    dynamically_transform(v);
	 set_diff_map_draw_vecs(v.data, v.size);
      }
   }
}

// modify v
void
molecule_class_info_t::dynamically_transform(coot::CartesianPairInfo v) {

   int s = v.size;
   for (int i=0; i<s; i++) {
      clipper::Coord_orth c1(v.data[i].getStart().x(),
			     v.data[i].getStart().y(),
			     v.data[i].getStart().z());
      clipper::Coord_orth c2(v.data[i].getFinish().x(),
			     v.data[i].getFinish().y(),
			     v.data[i].getFinish().z());
      clipper::Coord_orth ct1 = c1.transform(map_ghost_info.rtop);
      clipper::Coord_orth ct2 = c2.transform(map_ghost_info.rtop);
      v.data[i] = coot::CartesianPair(coot::Cartesian(ct1.x(), ct1.y(), ct1.z()),
				coot::Cartesian(ct2.x(), ct2.y(), ct2.z()));
   }
   
}


// 
void
molecule_class_info_t::map_fill_from_mtz(std::string mtz_file_name,
					 std::string f_col,
					 std::string phi_col,
					 std::string weight_col,
					 int use_weights,
					 int is_diff_map,
					 float sampling_rate) {

   short int use_reso_flag = 0;
   short int is_anomalous_flag = 0;
   map_fill_from_mtz_with_reso_limits(mtz_file_name,
				      f_col,
				      phi_col,
				      weight_col,
				      use_weights,
				      is_anomalous_flag,
				      is_diff_map,
				      use_reso_flag, 0.0, 0.0, sampling_rate); // don't use these reso limits.

}


// 
void
molecule_class_info_t::map_fill_from_mtz_with_reso_limits(std::string mtz_file_name,
							  std::string f_col,
							  std::string phi_col,
							  std::string weight_col,
							  int use_weights,
							  short int is_anomalous_flag,
							  int is_diff_map,
							  short int use_reso_limits,
							  float low_reso_limit,
							  float high_reso_limit,
							  float map_sampling_rate) {

   graphics_info_t g;

   // save for potential phase recombination in refmac later
   if (use_weights) { 
      fourier_f_label = f_col; 
      fourier_phi_label = phi_col;
      fourier_weight_label = weight_col; // magic label, we can go
					 // combining if this is not
					 // "";
//       std::cout << "DEBUG:: saving fourier_weight_label: " <<
// 	 fourier_weight_label << std::endl;
   }


   // std::cout << "DEBUG:: reso tinkering " << use_reso_limits << std::endl;
   clipper::Resolution user_resolution(high_reso_limit);
   clipper::Resolution fft_reso; // filled later

   clipper::HKL_info myhkl; 
   clipper::MTZdataset myset; 
   clipper::MTZcrystal myxtl; 

   long T0 = 0; // timer
   T0 = glutGet(GLUT_ELAPSED_TIME);

   clipper::CCP4MTZfile mtzin; 
   mtzin.open_read( mtz_file_name );       // open new file 
   mtzin.import_hkl_info( myhkl );         // read sg, cell, reso, hkls
   clipper::HKL_data< clipper::datatypes::F_sigF<float> >   f_sigf_data(myhkl, myxtl);
   clipper::HKL_data< clipper::datatypes::Phi_fom<float> > phi_fom_data(myhkl, myxtl);
   clipper::HKL_data< clipper::datatypes::F_phi<float> >       fphidata(myhkl, myxtl); 
   
   std::string mol_name = mtz_file_name + " "; 
   mol_name += f_col; 
   mol_name += " ";
   mol_name += phi_col; 
   
   if (use_weights) { 
      mol_name += " ";
      mol_name += weight_col; 
   }

   if (use_reso_limits) {
      mol_name += " ";
      mol_name += g.float_to_string(low_reso_limit);
      mol_name += " ";
      mol_name += g.float_to_string(high_reso_limit);
   }
   
   initialize_map_things_on_read_molecule(mol_name,
					  is_diff_map,
					  g.swap_difference_map_colours);
   xmap_list = new clipper::Xmap<float>[5];  // 18Kb each (empty) Xmap
   max_xmaps++;

   // If use weights, use both strings, else just use the first
   std::pair<std::string, std::string> p = make_import_datanames(f_col, phi_col, weight_col, use_weights);

   if (p.first.length() == 0) { // mechanism to signal an error
      std::cout << "ERROR:: fill_map.. - There was a column label error.\n";
   } else { 
      
      if (use_weights) {
	 // 	 std::cout << "DEBUG:: Importing f_sigf_data: " << p.first << std::endl;
	 mtzin.import_hkl_data( f_sigf_data, myset, myxtl, p.first );
	 // std::cout << "DEBUG:: Importing phi_fom_data: " << p.second << std::endl;
	 mtzin.import_hkl_data(phi_fom_data, myset, myxtl, p.second);
	 mtzin.close_read();
	 fphidata.compute(f_sigf_data, phi_fom_data,
			  clipper::datatypes::Compute_fphi_from_fsigf_phifom<float>());
      } else {
	 // std::cout << "DEBUG:: Importing f_phi_data: " << p.first << std::endl;
	 mtzin.import_hkl_data(fphidata, myset, myxtl, p.first);
	 mtzin.close_read();
      }
   
      long T1 = glutGet(GLUT_ELAPSED_TIME);

      int n_reflections = myhkl.num_reflections();
      std::cout << "Number of reflections: " << n_reflections << "\n";
      if (n_reflections <= 0) {
	 std::cout << "WARNING:: No reflections in mtz file!?" << std::endl;
      } else { 
	 if (use_reso_limits) {
	    fft_reso = user_resolution;
	    filter_by_resolution(&fphidata, low_reso_limit, high_reso_limit);
	 } else {
	    // fft_reso = myhkl.resolution();
	    // Kevin says do this instead:
	    fft_reso = clipper::Resolution(1.0/sqrt(fphidata.invresolsq_range().max()));
	 }
      
	 if (is_anomalous_flag) {
	    fix_anomalous_phases(&fphidata);
	 } 
   
   
	 cout << "INFO:: finding ASU unique map points with sampling rate "
   	      << map_sampling_rate	<< endl;
         clipper::Grid_sampling gs(myhkl.spacegroup(),
				   myhkl.cell(),
				   fft_reso,
				   map_sampling_rate);
	 cout << "INFO grid sampling..." << gs.format() << endl; 
	 xmap_list[0].init( myhkl.spacegroup(), myhkl.cell(), gs); // 1.5 default
	 // 	 cout << "Grid..." << xmap_list[0].grid_sampling().format() << "\n";
   
	 long T2 = glutGet(GLUT_ELAPSED_TIME);
// 	 std::cout << "MTZ:: debug:: " << myhkl.spacegroup().symbol_hm() << " " 
// 		   << myhkl.cell().descr().a() << " " 
// 		   << myhkl.cell().descr().b() << " " 
// 		   << myhkl.cell().descr().c() << " " 
// 		   << clipper::Util::rad2d(myhkl.cell().descr().alpha()) << " " 
// 		   << clipper::Util::rad2d(myhkl.cell().descr().beta ()) << " " 
// 		   << clipper::Util::rad2d(myhkl.cell().descr().gamma()) << std::endl;
// 	 std::cout << "MTZ:: debug:: n_reflections: " << myhkl.num_reflections()
// 		   << std::endl;
// 	 int ncount = 0;
// 	 clipper::HKL_info::HKL_reference_index hri;
// 	 for (hri=fphidata.first(); !hri.last(); hri.next()) {
// 	    if (ncount < 500) 
// 	       std::cout << " MTZ fphi: " << hri.hkl().h() << " "
// 			 << hri.hkl().k() << " " << hri.hkl().l() << " "
// 			 << fphidata[hri].f() << " "
// 			 << clipper::Util::rad2d(fphidata[hri].phi()) << std::endl;
// 	    ncount++;
// 	 } 

// 	 original_fphis = fphidata;

	 
	 // cout << "doing fft..." << endl;
	 xmap_list[0].fft_from( fphidata );                  // generate map
	 // cout << "done fft..." << endl;
   
	 long T3 = glutGet(GLUT_ELAPSED_TIME);
	 std::cout << "INFO:: " << float(T1-T0)/1000.0 << " seconds to read MTZ file\n";
	 std::cout << "INFO:: " << float(T2-T1)/1000.0 << " seconds to initialize map\n";
	 std::cout << "INFO:: " << float(T3-T2)/1000.0 << " seconds for FFT\n";
	 xmap_is_filled[0] = 1;  // set the map-is-filled? flag
   
  
	 // Fill the class variables:
	 //   clipper::Map_stats stats(xmap_list[0]);
	 //   map_mean_ = stats.mean();
	 //   map_sigma_ = stats.std_dev();

	 mean_and_variance<float> mv = map_density_distribution(xmap_list[0], 0);

	 save_mtz_file_name = mtz_file_name;
	 save_f_col = f_col;
	 save_phi_col = phi_col;
	 save_weight_col = weight_col;
	 save_use_weights = use_weights;
	 save_is_anomalous_map_flag = is_anomalous_flag;
	 save_is_diff_map_flag = is_diff_map;
	 save_high_reso_limit = high_reso_limit;
	 save_low_reso_limit = low_reso_limit;
	 save_use_reso_limits = use_reso_limits;

	 // 
	 map_mean_  = mv.mean; 
	 map_sigma_ = sqrt(mv.variance);
	 map_max_   = mv.max_density;
	 map_min_   = mv.min_density;

	 long T4 = glutGet(GLUT_ELAPSED_TIME);
	 std::cout << "INFO:: " << float(T4-T3)/1000.0 << " seconds for statistics\n";

	 std::cout << "      Map mean: ........ " << map_mean_ << std::endl;
	 std::cout << "      Map sigma: ....... " << map_sigma_ << std::endl;
	 std::cout << "      Map maximum: ..... " << map_max_ << std::endl;
	 std::cout << "      Map minimum: ..... " << map_min_ << std::endl;

	 set_initial_contour_level();

	 // update_map_colour_menu_manual(g.n_molecules, name_.c_str()); 
	 // update_map_scroll_wheel_menu_manual(g.n_molecules, name_.c_str()); 

	 update_map();
	 long T5 = glutGet(GLUT_ELAPSED_TIME);
	 std::cout << "INFO:: " << float(T5-T4)/1000.0 << " seconds for contour map\n";
	 std::cout << "INFO:: " << float(T5-T0)/1000.0 << " seconds in total\n";

	 // save state strings

	 //
  
	 if (have_sensible_refmac_params) { 
	    save_state_command_strings_.push_back("make-and-draw-map-with-refmac-params");
	    save_state_command_strings_.push_back(single_quote(coot::util::intelligent_debackslash(mtz_file_name)));
	    save_state_command_strings_.push_back(single_quote(f_col));
	    save_state_command_strings_.push_back(single_quote(phi_col));
	    save_state_command_strings_.push_back(single_quote(weight_col));
	    save_state_command_strings_.push_back(g.int_to_string(use_weights));
	    save_state_command_strings_.push_back(g.int_to_string(is_diff_map));
	    save_state_command_strings_.push_back(g.int_to_string(1)); // have refmac params
	    save_state_command_strings_.push_back(single_quote(refmac_fobs_col));
	    save_state_command_strings_.push_back(single_quote(refmac_sigfobs_col));
	    save_state_command_strings_.push_back(single_quote(refmac_r_free_col));
	    save_state_command_strings_.push_back(g.int_to_string(refmac_r_free_flag_sensible));
	 } else { 
	    save_state_command_strings_.push_back("make-and-draw-map");
	    save_state_command_strings_.push_back(single_quote(coot::util::intelligent_debackslash(mtz_file_name)));
	    save_state_command_strings_.push_back(single_quote(f_col));
	    save_state_command_strings_.push_back(single_quote(phi_col));
	    save_state_command_strings_.push_back(single_quote(weight_col));
	    save_state_command_strings_.push_back(g.int_to_string(use_weights));
	    save_state_command_strings_.push_back(g.int_to_string(is_diff_map));
	 }
      }
   }
   // std::cout << "DEBUG:: finishing map_fill_from_mtz_with_reso_limits, imol_no is "
   // << imol_no << std::endl;
}

void
molecule_class_info_t::set_refmac_save_state_commands(std::string mtz_file_name,
						      std::string f_col,
						      std::string phi_col,
						      std::string weight_col,
						      bool use_weights,
						      bool is_diff_map,
						      std::string refmac_fobs_col,
						      std::string refmac_sigfobs_col,
						      std::string refmac_r_free_col,
						      bool refmac_r_free_flag_sensible) {

   have_sensible_refmac_params = 1;
   save_state_command_strings_.clear();
   save_state_command_strings_.push_back("make-and-draw-map-with-refmac-params");
   save_state_command_strings_.push_back(single_quote(coot::util::intelligent_debackslash(mtz_file_name)));
   save_state_command_strings_.push_back(single_quote(f_col));
   save_state_command_strings_.push_back(single_quote(phi_col));
   save_state_command_strings_.push_back(single_quote(weight_col));
   save_state_command_strings_.push_back(coot::util::int_to_string(use_weights));
   save_state_command_strings_.push_back(coot::util::int_to_string(is_diff_map));
   save_state_command_strings_.push_back(coot::util::int_to_string(1)); // have refmac params
   save_state_command_strings_.push_back(single_quote(refmac_fobs_col));
   save_state_command_strings_.push_back(single_quote(refmac_sigfobs_col));
   save_state_command_strings_.push_back(single_quote(refmac_r_free_col));
   save_state_command_strings_.push_back(coot::util::int_to_string(refmac_r_free_flag_sensible));
}


std::vector<coot::atom_attribute_setting_help_t>
molecule_class_info_t::get_refmac_params() const {

   std::vector<coot::atom_attribute_setting_help_t> r;

   if (Have_sensible_refmac_params()) {
      r.push_back(coot::util::intelligent_debackslash(save_mtz_file_name));
      r.push_back(save_f_col);
      r.push_back(save_phi_col);
      r.push_back(save_weight_col);
      r.push_back(save_use_weights);
      r.push_back(save_is_diff_map_flag);
      r.push_back(1); // have refmac_params
      // r.push_back(refmac_mtz_filename); not sure if this should be given twice...
      r.push_back(refmac_fobs_col);
      r.push_back(refmac_sigfobs_col);
      r.push_back(refmac_r_free_col);
      r.push_back(refmac_r_free_flag_sensible);
   }
   return r;
} 


void
molecule_class_info_t::fix_anomalous_phases(clipper::HKL_data< clipper::datatypes::F_phi<float> > *fphidata) const {

   for (clipper::HKL_info::HKL_reference_index hri = fphidata->first(); !hri.last(); hri.next()) {
      (*fphidata)[hri].shift_phase(-M_PI_2);
   }
} 



void
molecule_class_info_t::save_previous_map_colour() {

   if (has_map()) { 
      previous_map_colour.resize(3);
      for (int i=0; i<3; i++) 
	 previous_map_colour[i] = map_colour[0][i];
   }
}


void
molecule_class_info_t::restore_previous_map_colour() {

   if (has_map()) { 
      if (previous_map_colour.size() == 3) { 
	 for (int i=0; i<3; i++) 
	    map_colour[0][i] = previous_map_colour[i];
      }
   }
   update_map();
}



// Where is this used?
int
molecule_class_info_t::next_free_map() {

   return 0; // placeholder FIXME.
}

void
molecule_class_info_t::set_initial_contour_level() {

   float level = 1.0;
   if (xmap_is_diff_map[0]) {
      if (map_sigma_ > 0.05) {
	 level = nearest_step(map_mean_ +
			      graphics_info_t::default_sigma_level_for_fofc_map*map_sigma_, 0.01);
      } else {
	 level = 3.0*map_sigma_;
      }
   } else { 
      if (map_sigma_ > 0.05) {
	 level = nearest_step(map_mean_ + graphics_info_t::default_sigma_level_for_map*map_sigma_, 0.01);
      } else {
	 level = graphics_info_t::default_sigma_level_for_map * map_sigma_;
      }
   }
   contour_level[0] = level;

}


// 
void
molecule_class_info_t::draw_skeleton() {


   if (has_map()) { 

      coot::CartesianPair pair;

      set_bond_colour(grey);
      glLineWidth(2.0);

      if (greer_skeleton_draw_on == 1) {
      
	 //cout << "greer_skeleton_draw_on: "
	 //	   << greer_skel_box.bonds_[0].num_lines<< endl;

	 glBegin(GL_LINES);
	 for (int j=0; j<greer_skel_box.bonds_[0].num_lines; j++) {

            glVertex3f(greer_skel_box.bonds_[0].pair_list[j].getStart().get_x(),
		       greer_skel_box.bonds_[0].pair_list[j].getStart().get_y(),
		       greer_skel_box.bonds_[0].pair_list[j].getStart().get_z());
            glVertex3f(greer_skel_box.bonds_[0].pair_list[j].getFinish().get_x(),
		       greer_skel_box.bonds_[0].pair_list[j].getFinish().get_y(),
		       greer_skel_box.bonds_[0].pair_list[j].getFinish().get_z());
	 }
	 glEnd();
      }

      if (fc_skeleton_draw_on == 1) { 

	 for (int l=0; l<fc_skel_box.num_colours; l++) {
 	    if (colour_skeleton_by_random) {
	       //  	       set_skeleton_bond_colour_random(l, colour_table);
	       set_skeleton_bond_colour(0.96);
 	    } else {
// 	       std::cout << "skel: " << l
// 			 << " of  " <<  fc_skel_box.num_colours <<  " " 
// 			 << (float(l)/float(fc_skel_box.num_colours)+0.01)/1.011
// 			 << std::endl;
	       set_skeleton_bond_colour( (float(l)/float(fc_skel_box.num_colours)+0.01)/1.011 );
	    }


	    glBegin(GL_LINES);
	    for (int j=0; j<fc_skel_box.bonds_[l].num_lines; j++) {

	       glVertex3f(fc_skel_box.bonds_[l].pair_list[j].getStart().get_x(),
			  fc_skel_box.bonds_[l].pair_list[j].getStart().get_y(),
			  fc_skel_box.bonds_[l].pair_list[j].getStart().get_z());
	       glVertex3f(fc_skel_box.bonds_[l].pair_list[j].getFinish().get_x(),
			  fc_skel_box.bonds_[l].pair_list[j].getFinish().get_y(),
			  fc_skel_box.bonds_[l].pair_list[j].getFinish().get_z());
	    }
	    glEnd();
	 }

	 // debugging:
	 //       cout << "Molecule name: " << name_ << endl; 
	 //       for (int l=0; l<fc_skel_box.num_colours; l++) {
	 // 	 cout << "skeleton levels in draw_skeleton: level: " 
	 // 	      << l << " " << skel_levels[l] << endl; 
	 //       }
      }
   }
}

// Added rotate colour_map for EJD 5/5/2004.
void
molecule_class_info_t::set_skeleton_bond_colour(float f) {

   float rotation_size = float(imol_no) * 2.0*graphics_info_t::rotate_colour_map_on_read_pdb/360.0;
   while (rotation_size > 1.0) {
      rotation_size -= 1.0;
   }

   if (0) { 
      std::vector<float> c(3);
      c[0] = 0.1+0.6*f*graphics_info_t::skeleton_colour[0];
      c[1] = 0.1+0.9*f*graphics_info_t::skeleton_colour[1];
      c[2] = 0.1+0.2*f*graphics_info_t::skeleton_colour[2];
      std::vector<float> rgb_new = rotate_rgb(c, rotation_size);
   }

   // none of this colour rotation nonsense.  Just set it to the
   // graphics_info_t variable:
   std::vector<float> rgb_new(3);
   for (int i=0; i<3; i++) 
      rgb_new[i] = graphics_info_t::skeleton_colour[i];

   glColor3f(rgb_new[0], rgb_new[1], rgb_new[2]);
}



void
molecule_class_info_t::set_colour_skeleton_by_segment() { // use random colouring
   
   colour_skeleton_by_random = 1;
} 

void
molecule_class_info_t::set_colour_skeleton_by_level() { // use random colouring
   
   colour_skeleton_by_random = 0;
} 


//
void
molecule_class_info_t::draw_fc_skeleton() {
   
}

//
void
molecule_class_info_t::update_clipper_skeleton() {

   if (has_map()) { 

      // Create map extents (the extents of the skeletonization)
      // from the current centre.

      if (xskel_is_filled == 1) { 

	 // graphics_info_t g;

	 if (xmap_is_filled[0] == 1 && xmap_is_diff_map[0] != 1) { 
	    //
	    float skeleton_box_radius = graphics_info_t::skeleton_box_radius; 

	    GraphicalSkel cowtan; 

	    // fc_skel_box: class object type graphical_bonds_container
	    //
	    coot::Cartesian rc(graphics_info_t::RotationCentre_x(),
			       graphics_info_t::RotationCentre_y(),
			       graphics_info_t::RotationCentre_z());
	    fc_skel_box = cowtan.make_graphical_bonds(xmap_list[0],xskel_cowtan,
						      rc, 
						      skeleton_box_radius,
						      graphics_info_t::skeleton_level);

	    // 	 cout << "DEBUG: " << "fc_skel_box.num_lines = "
	    // 	      << fc_skel_box.num_colours << endl;
	    // 	 cout << "DEBUG: " << "fc_skel_box.bonds_ = "
	    // 	      << fc_skel_box.bonds_ << endl;
	    // 	 cout << "DEBUG: " << "fc_skel_box.bonds_[0].num_lines = "
	    // 	      << fc_skel_box.bonds_[0].num_lines << endl;
	 }
      }
   }
}

void
molecule_class_info_t::unskeletonize_map() { 

   //    std::cout << "DEBUG:: unskeletonize_map" << std::endl;
   fc_skeleton_draw_on = 0;
   xskel_is_filled = 0;
   clipper::Xmap<int> empty; 
   xskel_cowtan = empty;
   // std::cout << "DEBUG:: done unskeletonize_map" << std::endl;
} 

// //
// void
// molecule_class_info_t::update_fc_skeleton_old() {

//    // Create map extents (the extents of the skeletonization)
//    // from the current centre.

//    if (xmap_is_filled[0] == 1 && xmap_is_diff_map[0] != 1) { 
//       //
//       float skeleton_box_radius = 20.0;
//       graphics_info_t g;

//       fc_skeleton foadi_cowtan; 

//       // fc_skel_box: class object type graphical_bonds_container
//       //
//       fc_skel_box = foadi_cowtan.itskel(xmap_list[0], 0.50);

//       cout << "DEBUG: " << "fc_skel_box.num_lines = "
// 	   << fc_skel_box.num_colours << endl;
//       cout << "DEBUG: " << "fc_skel_box.bonds_ = "
// 	   << fc_skel_box.bonds_ << endl;
//       cout << "DEBUG: " << "fc_skel_box.bonds_[0].num_lines = "
// 	   << fc_skel_box.bonds_[0].num_lines << endl;
//    }
// }

// Return -1 on error
int
molecule_class_info_t::read_ccp4_map(std::string filename, int is_diff_map_flag,
				     const std::vector<std::string> &acceptable_extensions) {

   // For now, where we try to read in a map and we crash in importing
   // a file that is not a ccp4 map, lets do some checking: first that
   // the file exists and is not a directory, then that the file has
   // an extension of ".map" or ".ext".  If not, then complain and
   // return having done nothing.

   // stat filename 
   struct stat s;
   int status = stat(filename.c_str(), &s);
   if (status != 0) {
      std::cout << "WARNING:: Error reading " << filename << std::endl;
      return -1;
   } else {
      if (!S_ISREG (s.st_mode)) {
	 if (S_ISDIR(s.st_mode)) {
	    std::cout << "WARNING:: " << filename << " is a directory." << endl;
	 } else {
	    std::cout << "WARNING:: " << filename << " not a regular file." << endl;
	 }
	 return -1;
      }
   }      

   // was a regular file, let's check the extension:
   // 
#ifdef WINDOWS_MINGW
   std::string::size_type islash = coot::util::intelligent_debackslash(filename).find_last_of("/");
#else
   std::string::size_type islash = filename.find_last_of("/");
#endif // MINGW
   std::string tstring;
   if (islash == std::string::npos) { 
      // no slash found
      tstring = filename;
   } else { 
      tstring = filename.substr(islash + 1);
   }
   
   bool good_extension_flag = 0;
   for (unsigned int iextension=0; iextension<acceptable_extensions.size(); iextension++) {
      std::string::size_type imap = tstring.rfind(acceptable_extensions[iextension]);
      if (imap != std::string::npos) {
	 good_extension_flag = 1;
	 break;
      }
   }
      
   // not really extension checking, just that it has it in the
   // filename:
   if (good_extension_flag == 0) { 
	 
      std::cout << "Filename for a CCP4 map must end in .map or .ext "
		<< "or some other approved extension - sorry\n";
      return -1;
      std::string ws = "The filename for a CCP4 map must\n";
      ws += "currently end in .map or .ext - sorry.\n\n";
      ws += "The map must be a CCP4 map or Badness Will Happen! :-)\n";
      GtkWidget *w = graphics_info_t::wrapped_nothing_bad_dialog(ws);
      gtk_widget_show(w);
   }

   // KDC: check map type
   enum MAP_FILE_TYPE { CCP4, CNS };
   MAP_FILE_TYPE map_file_type;
   {
     FILE* file = fopen( filename.c_str(), "r" );
     int c1, c2;
     c1 = c2 = 0;
     for ( int i = 0; i < 16; i++ ) {
       int c = getc(file);
       if ( c == EOF ) break;
       if ( c == 0 )                                 c1++;
       if ( isalpha(c) || isdigit(c) || isspace(c) ) c2++;
     }
     if ( c1 > c2 ) map_file_type = CCP4;
     else           map_file_type = CNS;
   }

   if (map_file_type == CCP4)
      std::cout << "INFO:: map file type was determined to be CCP4 type\n";
   if (map_file_type == CNS)
      std::cout << "INFO:: map file type was determined to be CNS type\n";

   if (max_xmaps == 0) {
      std::cout << "allocating space in read_ccp4_map" << std::endl;
      xmap_list = new clipper::Xmap<float>[1];
      xmap_is_filled   = new int[1];
      xmap_is_diff_map = new int[1];
      contour_level    = new float[1];
   }

   short int bad_read = 0;
   max_xmaps++; 

   if ( map_file_type == CCP4 ) {
     std::cout << "attempting to read CCP4 map: " << filename << std::endl;
     clipper::CCP4MAPfile file;
     try { 
	file.open_read(filename);
	try {
	   file.import_xmap( xmap_list[0] );
	}
	catch (clipper::Message_base exc) {
	   std::cout << "WARNING:: failed to read " << filename
		     << " Bad ASU (inconsistant gridding?)." << std::endl;
	   bad_read = 1;
	}
     } catch (clipper::Message_base exc) {
	std::cout << "WARNING:: failed to open " << filename << std::endl;
	bad_read = 1;
     }
     std::cout << "closing CCP4 map: " << filename << std::endl;
     file.close_read();
   } else {
     std::cout << "attempting to read CNS map: " << filename << std::endl;
     clipper::CNSMAPfile file;
     file.open_read(filename);
     try {
       file.import_xmap( xmap_list[0] );
     }
     catch (clipper::Message_base exc) {
       std::cout << "WARNING:: failed to read " << filename << std::endl;
       bad_read = 1;
     }
     file.close_read();
   }

   if (bad_read == 0) { 
      initialize_map_things_on_read_molecule(filename, is_diff_map_flag, 
					     graphics_info_t::swap_difference_map_colours);

      mean_and_variance<float> mv = map_density_distribution(xmap_list[0], 1); 

      float mean = mv.mean; 
      float var = mv.variance;
      contour_level[0]    = nearest_step(mean + 1.5*sqrt(var), 0.05);
      xmap_is_filled[0] = 1;  // set the map-is-filled? flag
      xmap_is_diff_map[0] = is_diff_map_flag; // but it may be...
      // fill class variables
      map_mean_ = mv.mean;
      map_sigma_ = sqrt(mv.variance);
      map_max_   = mv.max_density;
      map_min_   = mv.min_density;

      set_initial_contour_level();

      std::cout << "      Map mean: ........ " << map_mean_ << std::endl;
      std::cout << "      Map sigma: ....... " << map_sigma_ << std::endl;
      std::cout << "      Map maximum: ..... " << map_max_ << std::endl;
      std::cout << "      Map minimum: ..... " << map_min_ << std::endl;

      // save state strings
      save_state_command_strings_.push_back("handle-read-ccp4-map");
      save_state_command_strings_.push_back(single_quote(coot::util::intelligent_debackslash(filename)));
      save_state_command_strings_.push_back(graphics_info_t::int_to_string(is_diff_map_flag));
   
      update_map();
   }

   int stat = imol_no;
   if (bad_read)
      stat = -1;
   return stat;
}

void
molecule_class_info_t::new_map(const clipper::Xmap<float> &map_in, std::string name_in) {

   if (max_xmaps == 0) {
      xmap_list        = new clipper::Xmap<float>[1];
      xmap_is_filled   = new int[1];
      xmap_is_diff_map = new int[1];
      contour_level    = new float[1];
   }
   xmap_list[0] = map_in; 
   max_xmaps++;
   xmap_is_filled[0] = 1;
   // the map name is filled by using set_name(std::string)
   // sets name_ to name_in:
   initialize_map_things_on_read_molecule(name_in, 0, 0); // not a diff_map

   mean_and_variance<float> mv = map_density_distribution(xmap_list[0], 1); 

   float mean = mv.mean; 
   float var = mv.variance; 
   contour_level[0]  = nearest_step(mean + 1.5*sqrt(var), 0.05);
   xmap_is_filled[0] = 1;  // set the map-is-filled? flag

   // fill class variables
   map_mean_ = mv.mean;
   map_sigma_ = sqrt(mv.variance);

   update_map();
}

void
molecule_class_info_t::set_name(std::string name) {
   name_ = name;
   update_mol_in_display_control_widget();
      
}


int
molecule_class_info_t::make_map_from_phs(std::string pdb_filename,
                                         std::string phs_filename) {

   int iret = -1; // default error return status
   //
   std::cout << "Make a map from " << phs_filename << " using "
	     << pdb_filename << " for the cell and symmetry " << std::endl; 

   atom_selection_container_t SelAtom = get_atom_selection(pdb_filename);

   if (SelAtom.read_success == 1) { // success
      mat44 my_matt;

      int err = SelAtom.mol->GetTMatrix(my_matt, 0,0,0,0);
      if (err != 0) {
         cout << "!! Cryst::GetTMatrix() fails in make_map_from_phs"
              << endl;
      } else { // success
         cout << "Cryst::GetTMatrix() is good in make_map_from_phs"
              << endl;

         // we get to the cell and symmetry thusly:
         
         cout << "PHS Cell:  "
              << SelAtom.mol->get_cell().a << " "
              << SelAtom.mol->get_cell().b << " "
              << SelAtom.mol->get_cell().c << " "
              << SelAtom.mol->get_cell().alpha << " "
              << SelAtom.mol->get_cell().beta << " "
              << SelAtom.mol->get_cell().gamma << " "
              << endl;

         cout << "PHS Spacegroup: " << SelAtom.mol->GetSpaceGroup() << endl; 

	 clipper::Spacegroup spacegroup(clipper::Spgr_descr(SelAtom.mol->GetSpaceGroup()));
	 clipper::Cell cell(clipper::Cell_descr(SelAtom.mol->get_cell().a,
						SelAtom.mol->get_cell().b,
						SelAtom.mol->get_cell().c,
						clipper::Util::d2rad(SelAtom.mol->get_cell().alpha),
						clipper::Util::d2rad(SelAtom.mol->get_cell().beta),
						clipper::Util::d2rad(SelAtom.mol->get_cell().gamma))); 

	 iret = make_map_from_phs(spacegroup, cell, phs_filename);
      }
   }
   return iret;
}


int
molecule_class_info_t::make_map_from_phs_using_reso(std::string phs_filename,
						    const clipper::Spacegroup &sg,
						    const clipper::Cell &cell,
						    float reso_limit_low,
						    float reso_limit_high,
						    float map_sampling_rate) {

   clipper::PHSfile phs;

   phs.open_read(phs_filename);

   // std::cout << "creating resolution" << std::endl;
   clipper::Resolution resolution(reso_limit_high);

   clipper::HKL_info mydata(sg, cell, resolution);
   clipper::HKL_data<clipper::datatypes::F_sigF<float>  >  myfsig(mydata);
   clipper::HKL_data<clipper::datatypes::Phi_fom<float> >  myphwt(mydata);
   clipper::HKL_data<clipper::datatypes::F_phi<float>   >  fphidata(mydata); 

   std::cout << "importing info" << std::endl;
   phs.import_hkl_info(mydata);
   std::cout << "importing data" << std::endl;
   phs.import_hkl_data(myfsig); 
   phs.import_hkl_data(myphwt);

   phs.close_read(); 

   std::cout << "PHS file: Number of reflections: " << mydata.num_reflections() << "\n";

   fphidata.update();

   fphidata.compute(myfsig, myphwt, 
 		    clipper::datatypes::Compute_fphi_from_fsigf_phifom<float>());

//    for (int i=0; i<10; i++) {
//       std::cout << "checking phi weight: " << i << " " << myphwt[i].phi() << "  " << myphwt[i].fom()
// 		<< std::endl;
//        std::cout << "checking f    sigf: " << i << " " << myfsig[i].f() << "   "
// 		 << myfsig[i].sigf() << std::endl;
//        std::cout << "checking missing: " << i << " " << myfsig[i].missing() << " "
// 		 << myphwt[i].missing() << " " << fphidata[i].missing() << std::endl;
//        // << " " << fphidata[i].phi() <<
//    }
   
  if (max_xmaps == 0) {
     
     xmap_list = new clipper::Xmap<float>[10];
     xmap_is_filled   = new int[10];
     xmap_is_diff_map = new int[10];
     contour_level    = new float[10];
   }

   max_xmaps++; 

  std::string mol_name = phs_filename; 

  initialize_map_things_on_read_molecule(mol_name, 0, 0); // not diff map

  cout << "initializing map..."; 
  xmap_list[0].init(mydata.spacegroup(), 
		    mydata.cell(), 
		    clipper::Grid_sampling(mydata.spacegroup(),
					   mydata.cell(), 
					   mydata.resolution(),
					   map_sampling_rate));
  cout << "done."<< endl; 

//   cout << "Map Grid (from phs file)..." 
//        << xmap_list[0].grid_sampling().format()
//        << endl;  

  cout << "doing fft..." ; 
  xmap_list[0].fft_from( fphidata );                  // generate map
  cout << "done." << endl;

  mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

  cout << "Mean and sigma of map from PHS file: " << mv.mean 
       << " and " << sqrt(mv.variance) << endl;

  // fill class variables
  map_mean_ = mv.mean;
  map_sigma_ = sqrt(mv.variance);

  xmap_is_diff_map[0] = 0; 
  xmap_is_filled[0] = 1; 
  contour_level[0] = nearest_step(mv.mean + 1.5*sqrt(mv.variance), 0.05);

  std::cout << "updating map..." << std::endl;
  update_map();
  std::cout << "done updating map..." << std::endl;

  // how do we restore this map?
  save_state_command_strings_.push_back("read-phs-and-make-map-using-cell-symm");
  save_state_command_strings_.push_back(single_quote(phs_filename));
  save_state_command_strings_.push_back(single_quote(sg.symbol_hm()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().a()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().b()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().c()));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().alpha())));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().beta())));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().gamma())));

  return imol_no;
}



// and the molecule number imol_coords where the coordinates are.
int
molecule_class_info_t::make_map_from_cif(int imol_no_in,
					 std::string cif_file_name,
					 int imol_coords) {

   return make_map_from_cif(imol_no_in, cif_file_name,
			    graphics_info_t::molecules[imol_coords].atom_sel); 
}

// and the molecule number imol_coords where the coordinates are.
int
molecule_class_info_t::make_map_from_cif_2fofc(int imol_no_in,
					       std::string cif_file_name,
					       int imol_coords) {

   return make_map_from_cif_2fofc(imol_no_in,
				  cif_file_name,
				  graphics_info_t::molecules[imol_coords].atom_sel); 
}

// and the molecule number imol_coords where the coordinates are.
int
molecule_class_info_t::make_map_from_cif_fofc(int imol_no_in,
					      std::string cif_file_name,
					      int imol_coords) {

   return make_map_from_cif_generic(imol_no_in,
				    cif_file_name,
				    graphics_info_t::molecules[imol_coords].atom_sel,
				    2);  // 2 -> is Fo-Fc map
}


int
molecule_class_info_t::make_map_from_cif(int imol_no_in,
					 std::string cif_file_name,
					 atom_selection_container_t SelAtom) {

   // 0 is not is_2fofc_type map (is sigmaa)
   return make_map_from_cif_generic(imol_no_in, cif_file_name, SelAtom, 0);

}

int
molecule_class_info_t::make_map_from_cif_2fofc(int imol_no_in,
					       std::string cif_file_name,
					       atom_selection_container_t SelAtom) {

   // 1 is is_2fofc_type map (not sigmaa)
   return make_map_from_cif_generic(imol_no_in, cif_file_name, SelAtom, 1); 

}


int
molecule_class_info_t::make_map_from_cif_generic(int imol_in,
						 std::string cif_file_name,
						 atom_selection_container_t SelAtom,
						 short int is_2fofc_type) {

   clipper::HKL_info mydata;
   clipper::CIFfile cif; 
      
   cif.open_read ( cif_file_name ); 
   cif.import_hkl_info(mydata); // set spacegroup, cell and get hkl list. 
   clipper::HKL_data< clipper::datatypes::F_sigF<float> > myfsigf(mydata);
   
   cif.import_hkl_data(myfsigf);
   cif.close_read();

   std::cout << "make_map_from_cif: Read " << mydata.num_reflections()
	     << " from CIF file." << std::endl;

   if (mydata.num_reflections() == 0) {
      std::cout << "Error reading cif file, can't make a map" << std::endl;
      return -1; // Error
   }
   return calculate_sfs_and_make_map(imol_in, cif_file_name, mydata, myfsigf,
				     SelAtom, is_2fofc_type);
}
   
int
molecule_class_info_t::calculate_sfs_and_make_map(int imol_no_in,
						  const std::string &mol_name,
						  const clipper::HKL_info &mydata,
						  const clipper::HKL_data< clipper::datatypes::F_sigF<float> > &myfsigf,
						  atom_selection_container_t SelAtom,
						  short int is_2fofc_type) {

   if (max_xmaps == 0) {
      xmap_list = new clipper::Xmap<float>[10];
      xmap_is_filled   = new int[10];
      xmap_is_diff_map = new int[10];
      contour_level    = new float[10];
   }
   max_xmaps++; 
   initialize_map_things_on_read_molecule(mol_name, 0, 0); // not diff map
   
   std::cout << "calculating structure factors..." << std::endl;

   // Fix up fphidata to contain the calculated structure factors

   // Calculated structure factors go here:
   clipper::HKL_data< clipper::datatypes::F_phi<float> > fphidata(mydata);
   // map coefficients ((combined Fo and scaled Fc) and calc phi) go here:

   clipper::HKL_data< clipper::datatypes::F_phi<float> > map_fphidata(mydata);
  
   // get a list of all the atoms
   clipper::MMDBAtom_list atoms(SelAtom.atom_selection, SelAtom.n_selected_atoms);
  
   std::cout << "isotropic fft of " << SelAtom.n_selected_atoms
	     << " atoms..." << std::endl;
   clipper::SFcalc_iso_fft<float>(fphidata, atoms);
   std::cout << "done iso fft..." << std::endl;

   // debug:: examine fphidata and myfsigf:
   std::cout << "DEBUG:: myfsigf  has " <<  myfsigf.data_size() << " data"
	     << std::endl;
   std::cout << "DEBUG:: fphidata has " << fphidata.data_size() << " data"
	     << std::endl;

   if (0) { // debug
      float sum_fo = 0;
      float sum_fc = 0;
      int n_fo = 0;
      int n_fc = 0;
      for (clipper::HKL_info::HKL_reference_index ih=mydata.first();
	   !ih.last(); ih.next()) {
	 if (!myfsigf[ih].missing()) {
	    n_fo++;
	    n_fc++;
	    sum_fo += myfsigf[ih].f();
	    sum_fc = fphidata[ih].f();
	 }
      }
      
      std::cout << "DEBUG:: fo: sum average: " << sum_fo << " " << sum_fo/float(n_fo)
		<< std::endl; 
      std::cout << "DEBUG:: fc: sum average: " << sum_fc << " " << sum_fc/float(n_fc)
		<< std::endl;
      for (clipper::HKL_info::HKL_reference_index ih=mydata.first();
	   !ih.last(); ih.next())
	      std::cout << "DEBUG::  myfsigf " <<  " " <<  myfsigf[ih].f() << " "
		   << myfsigf[ih].sigf() << " " << myfsigf[ih].missing() << std::endl;
      for (int i=0; i<10; i++)
	 std::cout << "DEBUG:: fphidata " << i << " " << fphidata[i].f()
		   << " " << fphidata[i].phi() << std::endl;
   }
   
   int nprm = 10;
   std::vector<clipper::ftype> params_init( nprm, 1.0 );
   // clipper::BasisFn_spline basis_f1f2( mydata, nprm, 2.0 );
   //  target_f1f2( fc, fo );
   //clipper::TargetFn_scaleF1F2<clipper::datatypes::F_phi<float>,
   //                          clipper::datatypes::F_sigF<float> >
   // target_f1f2( fphidata, myfsigf );
   //clipper::ResolutionFn fscale( mydata, basis_f1f2,
   //                              target_f1f2, params_init );

   float r_top = 0.0, r_bot = 0.0;
   float sum_fo = 0.0, sum_fc = 0.0, sum_scale = 0.0;
   int n_data = 0;

   if (is_2fofc_type == molecule_map_type::TYPE_2FO_FC ||
       is_2fofc_type == molecule_map_type::TYPE_FO_FC) {

      if (is_2fofc_type == molecule_map_type::TYPE_2FO_FC)
	 std::cout << "INFO:: calculating 2fofc map..." << std::endl; 
      if (is_2fofc_type == molecule_map_type::TYPE_FO_FC)
	 std::cout << "INFO:: calculating fofc map..." << std::endl;
      
      clipper::BasisFn_spline basis_f1f2( mydata, nprm, 2.0 );
      //  target_f1f2( fc, fo );
      clipper::TargetFn_scaleF1F2<clipper::datatypes::F_phi<float>,
	 clipper::datatypes::F_sigF<float> >
	 target_f1f2( fphidata, myfsigf );
      clipper::ResolutionFn fscale( mydata, basis_f1f2, target_f1f2, params_init );

      float multiplier = 2.0;
      if (is_2fofc_type == molecule_map_type::TYPE_FO_FC)
	 multiplier = 1.0;
     
      for ( clipper::HKL_info::HKL_reference_index ih=mydata.first();
	    !ih.last(); ih.next() ) { 
	 map_fphidata[ih].phi() = fphidata[ih].phi(); 
	 if (!myfsigf[ih].missing()) {
	    map_fphidata[ih].f() = multiplier*myfsigf[ih].f() -
	       fphidata[ih].f()*sqrt(fscale.f(ih));
	    float top_tmp = fabs(myfsigf[ih].f() - fphidata[ih].f()*sqrt(fscale.f(ih)));
	    if (0) { 
	       std::cout << "debug:: fobs: " << myfsigf[ih].f() << " fscale: "
			 << fphidata[ih].f() << " scale: " << fscale.f(ih)
			 << std::endl;
	    }
	    
	    r_top += top_tmp;
	    r_bot += fabs(myfsigf[ih].f());
// 	    std::cout << "debug:: adding to top: " << top_tmp << " bot: "
// 		      << fabs(myfsigf[ih].f()) << std::endl;
	    sum_fo += myfsigf[ih].f();
	    sum_fc += fphidata[ih].f();
	    sum_scale += sqrt(fscale.f(ih));
	    n_data++; 
	 } else {
	    map_fphidata[ih].f() = 0.0; 
	 }
      }

   } else { // not 2fofc-style, i.e. is sigmaa style
      
      if (is_2fofc_type == molecule_map_type::TYPE_SIGMAA) {
     
	 std::cout << "sigmaa and scaling..." << std::endl;

	 // need an mmdb
	 CMMDBManager *mmdb = SelAtom.mol;
	 // clipper::MTZcrystal cxtl; // and a cxtl, whatever that is...
	 clipper::Cell cxtl = myfsigf.hkl_info().cell();
	 // hkls
	 // fo
	 // 
	    
	 // get a list of all the atoms
	 clipper::mmdb::PPCAtom psel;
	 int hndl, nsel;
	 hndl = mmdb->NewSelection();
	 mmdb->SelectAtoms( hndl, 0, 0, SKEY_NEW );
	 mmdb->GetSelIndex( hndl, psel, nsel );
	 clipper::MMDBAtom_list atoms( psel, nsel );
	 mmdb->DeleteSelection( hndl );

	 // calculate structure factors
	 const clipper::HKL_info& hkls = mydata;
	 const clipper::HKL_data<clipper::datatypes::F_sigF<float> >& fo = myfsigf;
	 clipper::HKL_data<clipper::datatypes::F_phi<float> > fc( hkls, cxtl );
	 clipper::SFcalc_obs_bulk<float> sfcb;
	 sfcb( fc, fo, atoms );

	 // do anisotropic scaling
	 clipper::SFscale_aniso<float> sfscl;
	 // sfscl( fo, fc );  // scale Fobs
	 sfscl( fc, fo );  // scale Fcal

	 // now do sigmaa calc
	 clipper::HKL_data<clipper::datatypes::F_phi<float> >   fb( hkls, cxtl ), fd( hkls, cxtl );
	 clipper::HKL_data<clipper::datatypes::Phi_fom<float> > phiw( hkls, cxtl );
	 clipper::HKL_data<clipper::datatypes::Flag>    flag( hkls, cxtl );
	 typedef clipper::HKL_data_base::HKL_reference_index HRI;
	 // If no free flag is available, then use all reflections..
	 for (HRI ih = flag.first(); !ih.last(); ih.next() )
	    flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;
	 /* This code uses free reflections only for sigmaa and scaling...
	 for (HRI ih = flag.first(); !ih.last(); ih.next() )
	    if ( !fo[ih].missing() && (free[ih].missing()||free[ih].flag()==0) )
	       flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;
	    else
	       flag[ih].flag() = clipper::SFweight_spline<float>::NONE;
	 */

	 // do sigmaa calc

	 // Bug! 20071218 Kevin fixes it.
// 	 int n_refln = mydata.num_reflections();
// 	 int n_param = 10;
	 int n_refln = 1000;
	 int n_param = 20;
	 clipper::SFweight_spline<float> sfw( n_refln, n_param );
	 sfw( fb, fd, phiw, fo, fc, flag );

	 // OK, so now fb and fd contain F_phis, one for "best"
	 // sigmaa, one for difference map.  Let's just use the "best"
	 // map for now.
	 map_fphidata = fb;
      }
   } // is 2fofc else sigmaa style check

   std::cout << "DEBUG:: rdiffsum/rsum: " << r_top << "/" << r_bot << std::endl;
   if (r_bot>0.0) { 
      std::cout << "Isotropic R-factor: " << 100.0*r_top/r_bot << "%"
		<< " for " << n_data  << " reflections" <<  std::endl;
      std::cout << "DEBUG:: sums: fo: " << sum_fo/float(n_data) << " fc: "
		<< sum_fc/n_data << " scale: " << sum_scale/n_data << " with "
		<< n_data << " data" << std::endl;
   } else {
      std::cout << "Problem with structure factors, no structure factor sum!?"
		<< std::endl;
   } 
   cout << "initializing map..."; 
   xmap_list[0].init(mydata.spacegroup(), 
		     mydata.cell(), 
		     clipper::Grid_sampling(mydata.spacegroup(),
					    mydata.cell(), 
					    mydata.resolution()));
   cout << "done."<< endl; 


   cout << "doing fft..." ; 
   xmap_list[0].fft_from( map_fphidata ); // generate map
   cout << "done." << endl;

   mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

   cout << "Mean and sigma of map " << mol_name << " " << mv.mean 
	<< " and " << sqrt(mv.variance) << endl; 

   // fill class variables
   map_mean_ = mv.mean;
   map_sigma_ = sqrt(mv.variance);
   map_max_   = mv.max_density;
   map_min_   = mv.min_density;
  
   xmap_is_diff_map[0] = 0; 
   xmap_is_filled[0] = 1; 

   std::cout << "      Map mean: ........ " << map_mean_ << std::endl;
   std::cout << "      Map sigma: ....... " << map_sigma_ << std::endl;
   std::cout << "      Map maximum: ..... " << map_max_ << std::endl;
   std::cout << "      Map minimum: ..... " << map_min_ << std::endl;
   
  set_initial_contour_level();

   int imol = imol_no_in;
   update_map(); 
   return imol;

}

// This needs to be rationalized with the version that *does* pass the
// coordinates.
//
// This is the version that gets called when we use a file selector to
// get the file (i.e. it doesn't specify the coordinates (molecule))
// because there are calculated structure factors in the file.
//
// We make a Fc alpha-c map.  Which is not usually what we want.
// 
int
molecule_class_info_t::make_map_from_cif(int imol_no_in,
					 std::string cif_file_name) {
   return make_map_from_cif_sigmaa(imol_no_in,
				   cif_file_name, molecule_map_type::TYPE_SIGMAA);
}

int
molecule_class_info_t::make_map_from_cif_diff_sigmaa(int imol_no_in,
						     std::string cif_file_name) {
   return make_map_from_cif_sigmaa(imol_no_in,
				   cif_file_name,
				   molecule_map_type::TYPE_DIFF_SIGMAA);
}

// SigmaA map type, either molecule_map_type::TYPE_SIGMAA or TYPE_DIFF_SIGMAA.
// 
int
molecule_class_info_t::make_map_from_cif_sigmaa(int imol_no_in,
						std::string cif_file_name,
						int sigmaa_map_type) {

   imol_no = imol_no_in;
   clipper::HKL_info mydata;
   clipper::CIFfile cif; 
      
   cif.open_read ( cif_file_name );
   cif.import_hkl_info(mydata); // set spacegroup, cell and get hkl list. 
   clipper::HKL_data< clipper::datatypes::F_sigF<float> > myfsigf(mydata); // Fobs
   clipper::HKL_data< clipper::datatypes::F_phi<float> > fc(mydata); // FC PHIC

   cif.import_hkl_data(myfsigf);
   cif.import_hkl_data(fc); 

   cif.close_read(); 
      
   std::cout << "Read " << mydata.num_reflections() << " from CIF file." 
	     << std::endl; 

   if (mydata.num_reflections() == 0) {
      return -1;
   } else {

      // Are all the calculated sfs missing/zero?
      // 
      int non_zero = 0;
      for(int i=0; i< mydata.num_reflections(); i++) {
	 if (! fc[i].missing()) {
	    if (fc[i].f() > 0.0) {
	       non_zero = 1;
	       break;
	    }
	 }
      }

      if (non_zero == 0) {
	 std::cout << "WARNING:: Ooops - all the structure factor amplitudes "
		   << " appear to be zero - or missing.  " << std::endl;
	 std::cout << "WARNING:: Are you sure this file (" << cif_file_name
		   << ") contains calculated structure factors?" << std::endl;
	 std::cout << "WARNING:: No map calculated." << std::endl;
	 std::cout << "INFO:: if you want to calculate structure factors from a"
		   << " set of coordinates, " << std::endl
		   << "       consider the function (read-cif-data cif-file imol)"
		   << std::endl;
      } else {

	 if (max_xmaps == 0) {
     
	    xmap_list = new clipper::Xmap<float>[10];
	    // these are allocated in initialize_map_things_on_read_molecule()
// 	    xmap_is_filled   = new int[10];
// 	    xmap_is_diff_map = new int[10];
// 	    contour_level    = new float[10];
	 }
	 max_xmaps++; 
	 std::string mol_name = cif_file_name;
	 if (sigmaa_map_type == molecule_map_type::TYPE_SIGMAA)
	    mol_name += " SigmaA";
	 if (sigmaa_map_type == molecule_map_type::TYPE_DIFF_SIGMAA)
	    mol_name += " Difference SigmaA";

	 // new sigmaA code... needs to be updated to new Kevin
	 // code... but that is slightly tricky because here we have
	 // sfs, whereas KC code calculates them.
	 
	 std::cout << "sigmaa and scaling..." << std::endl; 
	 
	 clipper::HKL_data< clipper::datatypes::F_phi<float> > map_fphidata(mydata);
	 clipper::HKL_data<clipper::datatypes::Phi_fom<float> > phifom(mydata);

	 clipper::Cell cxtl = myfsigf.hkl_info().cell();
	 // "Aliases" to fix Kevin's sigmaA code into mine
	 const clipper::HKL_data<clipper::datatypes::F_sigF<float> >& fo = myfsigf;
	 const clipper::HKL_info& hkls = mydata;

	 // now do sigmaa calc
	 clipper::HKL_data<clipper::datatypes::F_phi<float> >   fb( hkls, cxtl ), fd( hkls, cxtl );
	 clipper::HKL_data<clipper::datatypes::Phi_fom<float> > phiw( hkls, cxtl );
	 clipper::HKL_data<clipper::datatypes::Flag>    flag( hkls, cxtl );
	 typedef clipper::HKL_data_base::HKL_reference_index HRI;
	 // If no free flag is available, then use all reflections..
	 for (HRI ih = flag.first(); !ih.last(); ih.next() )
	    flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;

	 /* This code uses free reflections only for sigmaa and scaling...
	 for (HRI ih = flag.first(); !ih.last(); ih.next() )
	    if ( !fo[ih].missing() && (free[ih].missing()||free[ih].flag()==0) )
	       flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;
	    else
	       flag[ih].flag() = clipper::SFweight_spline<float>::NONE;
	 */

	 // do sigmaa calc
	 int n_refln = 1000;
	 int n_param = 20;
	 clipper::SFweight_spline<float> sfw( n_refln, n_param );
	 sfw( fb, fd, phiw, fo, fc, flag );
	 // fb is F+phi for "Best"
	 // fd is F+phi for difference map
	 short int is_diff = 0;
	 if (sigmaa_map_type == molecule_map_type::TYPE_DIFF_SIGMAA) {
	    map_fphidata = fd;
	    is_diff = 1;
	 } else { 
	    map_fphidata = fb;
	 }
	 
	 // back to old code 
	 //
	 cout << "initializing map..."; 
	 xmap_list[0].init(mydata.spacegroup(), 
			   mydata.cell(), 
			   clipper::Grid_sampling(mydata.spacegroup(),
						  mydata.cell(), 
						  mydata.resolution(),
						  graphics_info_t::map_sampling_rate));
	 cout << "done."<< endl; 

	 cout << "doing fft..." ; 
	 // xmap_list[0].fft_from( fphidata );       // generate Fc alpha-c map
	 xmap_list[0].fft_from( map_fphidata );       // generate sigmaA map 20050804
	 cout << "done." << endl;
	 initialize_map_things_on_read_molecule(mol_name, is_diff, 0);
	 // now need to fill contour_level, xmap_is_diff_map xmap_is_filled
	 if (is_diff)
	    xmap_is_diff_map[0] = 1;
	 else 
	    xmap_is_diff_map[0] = 0;

	 mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

	 cout << "Mean and sigma of map from CIF file (make_map_from_cif): "
	      << mv.mean << " and " << sqrt(mv.variance) << endl; 

	 xmap_is_filled[0] = 1; 

	 map_mean_  = mv.mean; 
	 map_sigma_ = sqrt(mv.variance);
	 map_max_   = mv.max_density;
	 map_min_   = mv.min_density;
	 
	 set_initial_contour_level();

	 int imol = imol_no_in;
	 update_map(); 

	 if (sigmaa_map_type != molecule_map_type::TYPE_DIFF_SIGMAA) {
	    save_state_command_strings_.push_back("read-cif-data-with-phases-sigmaa");
	    save_state_command_strings_.push_back(single_quote(cif_file_name));
	 } else {
	    save_state_command_strings_.push_back("read-cif-data-with-phases-diff-sigmaa");
	    save_state_command_strings_.push_back(single_quote(cif_file_name));
	 } 
	 return imol;
      } 
	 
   }
   return -1; 
}


//
// This is the version that gets called when we use a file selector to
// get the file (i.e. it doesn't specify the coordinates (molecule)) because
// this cif file has (or it is hoped that it has) calculated structure factors.
int
molecule_class_info_t::make_map_from_cif_nfofc(int imol_no_in,
					       std::string cif_file_name,
					       int map_type,
					       short int swap_difference_map_colours) {

   int ir = -1;
   imol_no = imol_no_in;
   
   clipper::HKL_info mydata;
   clipper::CIFfile cif; 
      
   cif.open_read ( cif_file_name );
   cif.import_hkl_info(mydata); // set spacegroup, cell and get hkl list. 
   clipper::HKL_data< clipper::datatypes::F_sigF<float> > myfsigf(mydata);
   clipper::HKL_data< clipper::datatypes::F_phi<float> > fphidata(mydata);

   cif.import_hkl_data(myfsigf);
   cif.import_hkl_data(fphidata); 

   cif.close_read(); 
      
   std::cout << "Read " << mydata.num_reflections() << " from CIF file." 
	     << std::endl; 

   if (mydata.num_reflections() == 0) {
      return -1;
   } else {

      int non_zero = 0;
      for(int i=0; i< mydata.num_reflections(); i++) {
	 if (! fphidata[i].missing() ) {
	    if (fphidata[i].f() > 0.0) {
	       non_zero++;
	       break;
	    }
	 }
      }

      if (! non_zero) {
	 std::cout << "WARNING:: Ooops - all the calculated structure factor "
		   << "amplitudes appear"
		   << " to be zero - or missing.  " << std::endl;
	 std::cout << "WARNING:: Are you sure this file (" << cif_file_name
		   << ") contains calculated structure factors?" << std::endl;
	 std::cout << "WARNING:: No map calculated." << std::endl;
      } else {

	 if (max_xmaps == 0) {
     
	    xmap_list = new clipper::Xmap<float>[10];
	    xmap_is_filled   = new int[10];
	    xmap_is_diff_map = new int[10];
	    contour_level    = new float[10];
	 }
	 max_xmaps++; 
	 std::string mol_name = cif_file_name;

	 int is_diff_map_flag = 0;
	 if (map_type == molecule_map_type::TYPE_FO_FC) { 
	    is_diff_map_flag = 1;
	    mol_name += " Fo-Fc";
	 }
	 if (map_type == molecule_map_type::TYPE_2FO_FC) {
	    mol_name += " 2Fo-Fc";
	 }
	 if (map_type == molecule_map_type::TYPE_FO_ALPHA_CALC) {
	    mol_name += " Fo ac";
	 }
	 
	 initialize_map_things_on_read_molecule(mol_name, is_diff_map_flag,
						swap_difference_map_colours);
	
	 cout << "initializing map..."; 
	 xmap_list[0].init(mydata.spacegroup(), 
			   mydata.cell(), 
			   clipper::Grid_sampling(mydata.spacegroup(),
						  mydata.cell(), 
						  mydata.resolution(),
						  graphics_info_t::map_sampling_rate));
	 std::cout << "done."<< std::endl;

	 // Here we need to fix up fphidata to be a combination
	 // of fsigf data and fphidata.
	 //
	 float fo_multiplier = 2.0;
	 float fc_multiplier = 1.0;
	 if (map_type == molecule_map_type::TYPE_FO_FC)
	    fo_multiplier = 1.0;
	 if (map_type == molecule_map_type::TYPE_FO_ALPHA_CALC) {
	    fo_multiplier = 1.0;
	    fc_multiplier = 0.0;
	 }

	 int nprm = 10;
	 std::vector<clipper::ftype> params_init(nprm, 1.0);
	 clipper::BasisFn_spline basis_f1f2( mydata, nprm, 2.0 );
	 clipper::TargetFn_scaleF1F2<clipper::datatypes::F_phi<float>,
	    clipper::datatypes::F_sigF<float> >
	    target_f1f2( fphidata, myfsigf );
	 clipper::ResolutionFn fscale( mydata, basis_f1f2, target_f1f2, params_init );

	 int nrefl = 0;
	 int nmissing = 0;
	 for (clipper::HKL_info::HKL_reference_index ih=mydata.first();
	      !ih.last(); ih.next()) {
	    nrefl++;
	    if (!myfsigf[ih].missing()) {
	       fphidata[ih].f() = fo_multiplier * myfsigf[ih].f() -
		  fc_multiplier * fphidata[ih].f() * sqrt(fscale.f(ih));
	       // std::cout << "scale: " << sqrt(fscale.f(ih)) << std::endl;
	    } else {
	       nmissing++;
	       // std::cout << "missing reflection: " << ih << std::endl;
	       fphidata[ih].f() = 0;
	    }
	 }
	 std::cout << "There were " << nrefl << " reflections of which "
		   << nmissing << " were missing\n";
	 

	 std::cout << "doing fft..." ; 
	 xmap_list[0].fft_from( fphidata );                  // generate map
	 std::cout << "done." << std::endl;

	 mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

	 std::cout << "Mean and sigma of map from CIF file (make_map_from_cif_nfofc): "
		   << mv.mean << " and " << sqrt(mv.variance) << std::endl; 

	 if (is_diff_map_flag == 1) {
	    xmap_is_diff_map[0] = 1; 
	    contour_level[0] = nearest_step(mv.mean + 2.5*sqrt(mv.variance), 0.01);
	 } else { 
	    xmap_is_diff_map[0] = 0; 
	    contour_level[0] = nearest_step(mv.mean + 1.5*sqrt(mv.variance), 0.05);
	 }

	 // fill class variables
	 map_mean_ = mv.mean;
	 map_sigma_ = sqrt(mv.variance);
	 xmap_is_diff_map[0] = is_diff_map_flag; 
	 xmap_is_filled[0] = 1; 

	 int imol = imol_no_in;
   
	 update_map();

	 have_unsaved_changes_flag = 0;
	 std::vector<std::string> strings;
	 if (map_type == molecule_map_type::TYPE_FO_FC)
	    strings.push_back("read-cif-data-with-phases-fo-fc");
	 else 
	    strings.push_back("read-cif-data-with-phases-2fo-fc");
	 strings.push_back(single_quote(cif_file_name));
	 save_state_command_strings_ = strings;

	 return imol;
      }
   }
   return ir;
}

int
molecule_class_info_t::make_map_from_mtz_by_calc_phases(int imol_no_in,
							const std::string &mtz_file_name,
							const std::string &f_col,
							const std::string &sigf_col,
							atom_selection_container_t SelAtom,
							short int is_2fofc_type) {

   clipper::HKL_info mydata;
   clipper::CCP4MTZfile mtz;
   clipper::MTZdataset myset; 
   clipper::MTZcrystal myxtal; 

   std::cout << "reading mtz file..." << mtz_file_name << std::endl; 
   mtz.open_read(mtz_file_name);

   // make the data names for import:
   std::pair<std::string, std::string> p = make_import_datanames(f_col, sigf_col, "", 0);
   mtz.import_hkl_info(mydata); // set spacegroup, cell and get hkl list. 
   clipper::HKL_data< clipper::datatypes::F_sigF<float> > myfsigf(mydata, myxtal);
   mtz.import_hkl_data(myfsigf, myset, myxtal, p.first);
   mtz.close_read();
   
   return calculate_sfs_and_make_map(imol_no_in, 
				     mtz_file_name, mydata, myfsigf,
				     SelAtom, is_2fofc_type);
}



// The rest was all interface fluff.  Here is where we do the real work
// (or get clipper to do it :).
// 
int
molecule_class_info_t::make_map_from_phs(const clipper::Spacegroup &sg,
					 const clipper::Cell &cell,
                                         std::string phs_filename) {

   // clipper::Resolution resolution(reso);  // no.

   // clipper::HKL_info mydata(sg, cell, resolution);

   clipper::PHSfile phs;

   std::cout << "PHS:: reading " << phs_filename << std::endl;
   phs.open_read(phs_filename);

   std::cout << "PHS:: creating resolution" << std::endl;
   clipper::Resolution resolution = phs.resolution(cell);
   // mydata.init(sg, cell, resolution);

   std::cout << "PHS:: creating mydata" << std::endl;
   clipper::HKL_info mydata(sg, cell, resolution);
   clipper::HKL_data<clipper::datatypes::F_sigF<float>  >  myfsig(mydata);
   clipper::HKL_data<clipper::datatypes::Phi_fom<float> >  myphwt(mydata);
   clipper::HKL_data<clipper::datatypes::F_phi<float>   >  fphidata(mydata); 

   std::cout << "PHS:: importing info" << std::endl;
   phs.import_hkl_info(mydata);
   std::cout << "PHS:: importing data" << std::endl;
   phs.import_hkl_data(myfsig); 
   phs.import_hkl_data(myphwt);

   phs.close_read();

   std::cout << "PHS:: using cell and symmetry: "
	     << cell.descr().a() << " "
	     << cell.descr().b() << " "
	     << cell.descr().c() << " "
	     << clipper::Util::rad2d(cell.descr().alpha()) << " "
	     << clipper::Util::rad2d(cell.descr().beta())  << " "
	     << clipper::Util::rad2d(cell.descr().gamma()) << " "
	     << single_quote(sg.symbol_hm()) << std::endl;

   std::cout << "PHS:: number of reflections: " << mydata.num_reflections()
	     << "\n";

   fphidata.update();

   int ncount = 0; 
   clipper::HKL_info::HKL_reference_index hri;
//    for (hri=myfsig.first(); !hri.last(); hri.next()) {
//       if (ncount < 300) 
// 	 std::cout << " PHS fsigf: " << hri.hkl().h() << " "
// 		   << hri.hkl().k() << " "
// 		   << hri.hkl().l() << " " << myfsig[hri].f() << " "
// 		   << (myfsig[hri].sigf()) << std::endl;
//       ncount++;
//    }

   ncount = 0; 
//    for (hri=myphwt.first(); !hri.last(); hri.next()) {
//       if (ncount < 300) 
// 	 std::cout << " PHS myphwt: " << hri.hkl().h() << " " << hri.hkl().k() << " "
// 		   << hri.hkl().l() << " " << myphwt[hri].fom() << " "
// 		   << clipper::Util::rad2d(myphwt[hri].phi()) << std::endl;
//       ncount++;
//    }

  fphidata.compute(myfsig, myphwt, 
 		    clipper::datatypes::Compute_fphi_from_fsigf_phifom<float>());

//    for (int i=0; i<10; i++) {
//       std::cout << "checking phi weight: " << i << " " << myphwt[i].phi() << "  "
//              << myphwt[i].fom() << std::endl;
//        std::cout << "checking f    sigf: " << i << " " << myfsig[i].f() << "   "
// 		 << myfsig[i].sigf() << std::endl;
//        std::cout << "checking missing: " << i << " " << myfsig[i].missing() << " "
// 		 << myphwt[i].missing() << " " << fphidata[i].missing() << std::endl;
//        // << " " << fphidata[i].phi() <<
//    }
   
  if (max_xmaps == 0) {
     
     xmap_list = new clipper::Xmap<float>[10];
     xmap_is_filled   = new int[10];
     xmap_is_diff_map = new int[10];
     contour_level    = new float[10];
   }

   max_xmaps++; 

  std::string mol_name = phs_filename; 

  initialize_map_things_on_read_molecule(mol_name, 0, 0); // not diff map

  cout << "initializing map..."; 
  xmap_list[0].init(mydata.spacegroup(), 
		    mydata.cell(), 
		    clipper::Grid_sampling(mydata.spacegroup(),
					   mydata.cell(), 
					   mydata.resolution(),
					   graphics_info_t::map_sampling_rate));
  cout << "done."<< endl;

  std::cout << "PHS:: debug:: " << mydata.spacegroup().symbol_hm() << " " 
	    << mydata.cell().descr().a() << " " 
	    << mydata.cell().descr().b() << " " 
	    << mydata.cell().descr().c() << " " 
	    << clipper::Util::rad2d(mydata.cell().descr().alpha()) << " " 
	    << clipper::Util::rad2d(mydata.cell().descr().beta ()) << " " 
	    << clipper::Util::rad2d(mydata.cell().descr().gamma()) << std::endl;

  std::cout << "PHS:: debug:: n_reflections: " << mydata.num_reflections()
		   << std::endl;

  ncount = 0; 
  // clipper::HKL_info::HKL_reference_index hri;
//   for (hri=fphidata.first(); !hri.last(); hri.next()) {
//      if (ncount < 300) 
// 	std::cout << " PHS fphi: " << hri.hkl().h() << " " << hri.hkl().k() << " "
// 		  << hri.hkl().l() << " " << fphidata[hri].f() << " "
// 		  << clipper::Util::rad2d(fphidata[hri].phi()) << std::endl;
//      ncount++;
//   } 


//   cout << "Map Grid (from phs file)..." 
//        << xmap_list[0].grid_sampling().format()
//        << endl;  

  cout << "doing fft..." ; 
  xmap_list[0].fft_from( fphidata );                  // generate map
  cout << "done." << endl;

  mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

  cout << "Mean and sigma of map from PHS file: " << mv.mean 
       << " and " << sqrt(mv.variance) << endl;

  // fill class variables
  map_mean_ = mv.mean;
  map_sigma_ = sqrt(mv.variance);

  xmap_is_diff_map[0] = 0; 
  xmap_is_filled[0] = 1; 
  contour_level[0] = nearest_step(mv.mean + 1.5*sqrt(mv.variance), 0.05);

  std::cout << "updating map..." << std::endl;
  update_map();
  std::cout << "done updating map..." << std::endl;

  // how do we restore this map?
  save_state_command_strings_.push_back("read-phs-and-make-map-using-cell-symm");
  save_state_command_strings_.push_back(single_quote(phs_filename));
  save_state_command_strings_.push_back(single_quote(sg.symbol_hm()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().a()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().b()));
  save_state_command_strings_.push_back(coot::util::float_to_string(cell.descr().c()));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().alpha())));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().beta())));
  save_state_command_strings_.push_back(coot::util::float_to_string(clipper::Util::rad2d(cell.descr().gamma())));

  return imol_no;
}

void
molecule_class_info_t::fill_skeleton_treenodemap() {

   // if we have a skeleton map but not treenodemap:
   // 
   if (xskel_is_filled && !skeleton_treenodemap_is_filled) {

      // Chomp up the lovely memory! Yum!
      // 
      skeleton_treenodemap.init(xskel_cowtan.spacegroup(), 
				xskel_cowtan.cell(),
				xskel_cowtan.grid_sampling()); 
      clipper::Coord_grid c_g; 
      clipper::Skeleton_basic::Neighbours skel_neighbs(xskel_cowtan);
      
//       std::cout << "Build tree: there are " << skel_neighbs.size() << " skel_neighbs"
// 		<< std::endl;  18, actually.
      
      clipper::Xmap_base::Map_reference_index ix;
      for (ix = xskel_cowtan.first(); !ix.last(); ix.next() ) {
	 if (xskel_cowtan[ix] > 0) { 

	    coot::SkeletonTreeNode stn; 
	    
	    for(int i=0; i< skel_neighbs.size(); i++) {
	       c_g = ix.coord() + skel_neighbs[i]; 
	    
	       if (xskel_cowtan.get_data(c_g) > 0 ) {
	       
		  // OK, so this node has a neighbour:
		  // 
		  stn.neighbs.push_back(c_g);
	       }
	    }
	    stn.near_grid_point = ix.coord();  // Strange but true!
	    // 
	    // We do this because "out of cell" reference
	    // (e.g.  uvw = (  -1, -12, -19)) will get wrapped 
	    // to some (hidden) value.  To get the wrapped
	    // value (i.e the grid), we look it up here. 
	    // Cunning (if it works). 
	    skeleton_treenodemap[ix] = stn; 
	 }
      }
      // set the flag
      skeleton_treenodemap_is_filled = 1;
   }
}


float
molecule_class_info_t::density_at_point(const clipper::Coord_orth &co) const {

   if (!xmap_is_filled[0]) {
      std::cout << " returning bogus value from density_at_point: " << std::endl;
      return -1000.0;
   } else {
      // OLD grid point (quantized) method
//       clipper::Coord_frac cf = co.coord_frac(xmap_list[0].cell());
//       clipper::Coord_grid cg = cf.coord_grid(xmap_list[0].grid_sampling());
//       return xmap_list[0].get_data(cg);

#ifdef HAVE_GSL
      float dv;
      clipper::Coord_frac af = co.coord_frac(xmap_list[0].cell()); 
      clipper::Coord_map  am = af.coord_map(xmap_list[0].grid_sampling()); 
      clipper::Interp_linear::interp(xmap_list[0], am, dv); 
      return dv;
#else
      printf("no GSL so no density at point - remake \n");
      return -1000.0;
#endif // HAVE_GSL
   }
}
// Return status, was the contour level changed?  In that way, we
// don't try to recontour (which is a slow process) when the contour
// level has not been changed.
// 
// We don't change the contour level if the contour level goes too
// low (typically below 0).
//
// We don't change the contour level if the contour level goes too
// high (above the maximum level of the map).
// 
short int
molecule_class_info_t::change_contour(int direction) {

   short int istat = 0;
   if (has_map()) { 
      // std::cout << "DEBUG:: contour_by_sigma_flag " << contour_by_sigma_flag << std::endl;
      // std::cout << "DEBUG:: adding " << contour_sigma_step << " * " << map_sigma_
      // << " to  " << contour_level[0] << std::endl;
      if (has_map()) {

	 float shift = graphics_info_t::diff_map_iso_level_increment;
	 if (contour_by_sigma_flag) { 
	    shift = contour_sigma_step * map_sigma_;
	 } else { 
	    if (xmap_is_diff_map[0]) { 
	       shift = graphics_info_t::diff_map_iso_level_increment;
	    } else { 
	       shift = graphics_info_t::iso_level_increment;
	    }
	 }

	 if (xmap_is_diff_map[0]) {
	    if (direction == -1) {
	       if (graphics_info_t::stop_scroll_diff_map_flag) {
		  if ((contour_level[0] - shift) > 
		      graphics_info_t::stop_scroll_diff_map_level) { 
		     contour_level[0] -= shift;
		     istat = 1;
		  }
	       } else {
		  contour_level[0] -= shift;
		  istat = 1;
	       }
	    } else {
	       // add, but don't go past the top of the map or the bottom of the map
	       // 
	       if (contour_level[0] <= map_max_ || contour_level[0] <= -map_min_) {
		  contour_level[0] += shift;
		  istat = 1;
	       }
	    }
	 } else {
	    // iso map

	    if (direction == -1) {
	       if (graphics_info_t::stop_scroll_iso_map_flag) {
		  if ((contour_level[0] - shift) >
		      graphics_info_t::stop_scroll_iso_map_level) {
		     contour_level[0] -= shift;
		     istat = 1;
		  }
	       } else {
		  contour_level[0] -= shift;
		  istat = 1;
	       }
	    } else {
	       if (contour_level[0] <= map_max_) {
		  contour_level[0] += shift;
		  istat = 1;
	       }
	    } 
	 }
      }
   }
   return istat;
}

// 
void
molecule_class_info_t::set_map_is_difference_map() { 

   if (has_map()) { 
      xmap_is_diff_map[0] = 1;
      update_map();
   }
}

short int
molecule_class_info_t::is_difference_map_p() const {

   short int istat = 0;
   if (has_map())
      if (xmap_is_diff_map[0])
	 istat = 1;
   return istat;
}


void
molecule_class_info_t::set_contour_by_sigma_step(float v, short int state) { 
   contour_by_sigma_flag = state;
   if (state)
      contour_sigma_step = v;
}

