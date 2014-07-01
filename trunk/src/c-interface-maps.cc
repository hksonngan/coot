/* src/c-interface-maps.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Copyright 2008, 2009, 2011, 2012 by The University of Oxford
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#if defined (USE_PYTHON)
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include "compat/coot-sysdep.h"

// for stat()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "coot-utils/coot-map-utils.hh" // for variance map
#include "coot-utils/xmap-stats.hh"
#include "skeleton/BuildCas.h"
#include "skeleton/graphical_skel.h"
#include "coot-utils/xmap-stats.hh"

#include "c-interface-mmdb.hh"
#include "c-interface-python.hh"


#include "graphics-info.h"
#include "cc-interface.hh"
#include "c-interface.h"

/*  ----------------------------------------------------------------------- */
/*                  Display lists                                           */
/*  ----------------------------------------------------------------------- */
void set_display_lists_for_maps(int istat) {

   graphics_info_t::display_lists_for_maps_flag = istat;

   if (graphics_info_t::use_graphics_interface_flag) {
      for (int i=0; i<graphics_info_t::n_molecules(); i++)
	 if (graphics_info_t::molecules[i].has_xmap() ||
	     graphics_info_t::molecules[i].has_nxmap())
	    graphics_info_t::molecules[i].update_map();
   }
   std::string cmd = "set-display-lists-for-maps";
   std::vector<coot::command_arg_t> args;
   args.push_back(istat);
   add_to_history_typed(cmd, args);
   if (graphics_info_t::use_graphics_interface_flag)
      graphics_draw();
}

int display_lists_for_maps_state() {

   return graphics_info_t::display_lists_for_maps_flag;
}

/* update the maps to the current position - rarely needed */
void update_maps() {
   for(int ii=0; ii<graphics_info_t::n_molecules(); ii++) {
      if (is_valid_map_molecule(ii)) { 
	 // std::cout << "DEBUG:: updating " << ii << std::endl;
	 graphics_info_t::molecules[ii].update_map();
      }
   }
}

void swap_map_colours(int imol1, int imol2) {

   if (is_valid_map_molecule(imol1)) {
      if (is_valid_map_molecule(imol2)) {
	 graphics_info_t g;
	 std::vector<float> map_1_colours = g.molecules[imol1].map_colours();
	 std::vector<float> map_2_colours = g.molecules[imol2].map_colours();
	 double *colours1 = new double[4];
	 colours1[0] = map_1_colours[0];
	 colours1[1] = map_1_colours[1];
	 colours1[2] = map_1_colours[2];
	 double *colours2 = new double[4];
	 colours2[0] = map_2_colours[0];
	 colours2[1] = map_2_colours[1];
	 colours2[2] = map_2_colours[2];
	 short int main_or_secondary = 0; // main
	 g.molecules[imol1].handle_map_colour_change(colours2,
						     g.swap_difference_map_colours,
						     main_or_secondary);
	 g.molecules[imol2].handle_map_colour_change(colours1,
						     g.swap_difference_map_colours,
						     main_or_secondary);
	 if (graphics_info_t::display_mode_use_secondary_p()) {
	    g.make_gl_context_current(graphics_info_t::GL_CONTEXT_SECONDARY);
	    main_or_secondary = 1; // secondary
	    g.molecules[imol1].handle_map_colour_change(colours2,
							g.swap_difference_map_colours,
							main_or_secondary);
	    g.molecules[imol2].handle_map_colour_change(colours1,
							g.swap_difference_map_colours,
							main_or_secondary);
	    g.make_gl_context_current(graphics_info_t::GL_CONTEXT_MAIN);
	 }
	 delete [] colours1;
	 delete [] colours2;
      }
   }
   std::string cmd = "swap-map-colours";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol1);
   args.push_back(imol2);
   add_to_history_typed(cmd, args);
}

void set_keep_map_colour_after_refmac(int istate) {
   std::string cmd = "set-keep-map-colour-after-refmac";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
   graphics_info_t::swap_pre_post_refmac_map_colours_flag = istate;
}

int keep_map_colour_after_refmac_state() {
   add_to_history_simple("keep_map_colour_after_refmac_state");
   return graphics_info_t::swap_pre_post_refmac_map_colours_flag;
} 


void show_select_map_dialog() {
   graphics_info_t g;
   g.show_select_map_dialog();
   add_to_history_simple("show-select-map-dialog");
} 

// return the new molecule number
int make_and_draw_map(const char* mtz_file_name,
		      const char *f_col, const char *phi_col,
		      const char *weight_col, int use_weights,
		      int is_diff_map) {

   graphics_info_t g;
   int imol = -1; // failure initially.
   struct stat buf;

   std::string f_col_str(f_col);
   std::string phi_col_str(phi_col);
   std::string weight_col_str("");
   if (use_weights)
      weight_col_str = std::string(weight_col);

   int status = stat(mtz_file_name, &buf);
   // 
   if (status != 0) {
      std::cout << "WARNING:: Can't find file " << mtz_file_name << std::endl;
      if (S_ISDIR(buf.st_mode)) {
	 std::cout << mtz_file_name << " is a directory! " << std::endl;
      }
   } else {

      if (0)
	 std::cout << "valid_labels(" << mtz_file_name << ","
		   << f_col << "," 
		   << phi_col << "," 
		   << weight_col << "," 
		   << use_weights << ") returns "
		   << valid_labels(mtz_file_name, f_col, phi_col, weight_col, use_weights)
		   << std::endl;

      if (valid_labels(mtz_file_name, f_col, phi_col, weight_col, use_weights)) { 
      
	 std::vector<std::string> command_strings;
	 command_strings.push_back("make-and-draw-map");
	 command_strings.push_back(single_quote(mtz_file_name));
	 command_strings.push_back(single_quote(f_col));
	 command_strings.push_back(single_quote(phi_col));
	 command_strings.push_back(single_quote(weight_col));
	 command_strings.push_back(graphics_info_t::int_to_string(use_weights));
	 command_strings.push_back(graphics_info_t::int_to_string(is_diff_map));
	 add_to_history(command_strings);

	 std::cout << "INFO:: making map from mtz filename " << mtz_file_name << std::endl;
	 imol = g.create_molecule();
	 std::string cwd = coot::util::current_working_dir();
	 g.molecules[imol].map_fill_from_mtz(std::string(mtz_file_name),
					     cwd,
					     f_col_str,
					     phi_col_str,
					     weight_col_str,
					     use_weights, is_diff_map,
					     graphics_info_t::map_sampling_rate);
	 // save the mtz file from where the map comes
	 g.molecules[imol].store_refmac_mtz_filename(std::string(mtz_file_name));
	 if (! is_diff_map)
	    g.scroll_wheel_map = imol;
	 graphics_draw();
	 g.activate_scroll_radio_button_in_display_manager(imol);
	 
      } else {
	 std::cout << "WARNING:: label(s) not found in mtz file " 
		   << mtz_file_name << " " << f_col_str << " " 
		   <<  phi_col_str << " ";
	 if (use_weights)
	    std::cout << weight_col_str << std::endl;
	 else 
	    std::cout << std::endl;
      }      
   }
   return imol; // possibly -1
}

int make_and_draw_patterson(const char *mtz_file_name, 
			    const char *f_col, 
			    const char *sigf_col) { 
   graphics_info_t g;
   int imol = g.create_molecule();
   int status = g.molecules[imol].make_patterson(mtz_file_name,
						 f_col, sigf_col,
						 g.map_sampling_rate);

   if (! status) { 
      g.erase_last_molecule();
      imol = -1;
   } else {
      graphics_draw();
   } 
   return imol;
} 


int  make_and_draw_map_with_refmac_params(const char *mtz_file_name, 
					  const char *a, const char *b,
					  const char *weight,
					  int use_weights, int is_diff_map,
					  short int have_refmac_params,
					  const char *fobs_col,
					  const char *sigfobs_col,
					  const char *r_free_col,
					  short int sensible_f_free_col) { 

   graphics_info_t g;
   int imol = -1;

   // this is order dependent.  the restore-state comand that is
   // constructed in make_and_draw_map checks to see if we
   // have_refmac_params, so we need to set them before making the map.
   // 
   imol = make_and_draw_map(mtz_file_name, a, b, weight, use_weights, is_diff_map);
   if (is_valid_map_molecule(imol)) {
      g.molecules[imol].store_refmac_params(std::string(mtz_file_name),
					    std::string(fobs_col), 
					    std::string(sigfobs_col), 
					    std::string(r_free_col),
					    sensible_f_free_col);
      g.molecules[imol].set_refmac_save_state_commands(mtz_file_name, a, b, weight, use_weights, is_diff_map, fobs_col, sigfobs_col, r_free_col, sensible_f_free_col);
   }
   return imol;
}

// return imol, possibly -1;
int make_and_draw_map_with_reso_with_refmac_params(const char *mtz_file_name, 
						   const char *f_col,
						   const char *phi_col,
						   const char *weight_col,
						   int use_weights, int is_diff_map,
						   short int have_refmac_params,
						   const char *fobs_col,
						   const char *sigfobs_col,
						   const char *r_free_col,
						   short int sensible_r_free_col,
						   short int is_anomalous_flag,
						   short int use_reso_limits,
						   float low_reso_limit,
						   float high_reso_limit) {

   graphics_info_t g;
   int imol = -1;

   struct stat buf;
   int status = stat(mtz_file_name, &buf);

   if (status != 0) {
      std::cout << "Error finding MTZ file " << mtz_file_name << std::endl;
      if (S_ISDIR(buf.st_mode)) {
	 std::cout << mtz_file_name << " is a directory! " << std::endl;
      }
   } else {
      std::string map_type;
      if (is_diff_map)
	 map_type = "difference";
      else
	 map_type = "conventional";
      
      std::cout << "INFO:: making " << map_type << " map from MTZ filename "
		<< mtz_file_name << " using " << f_col << " "
		<< phi_col << std::endl;

      if (valid_labels(mtz_file_name, f_col, phi_col, weight_col, use_weights)) {
	 std::string weight_col_str("");
	 if (use_weights)
	    weight_col_str = std::string(weight_col);
	 imol = g.create_molecule();
	 float msr = graphics_info_t::map_sampling_rate;
	 std::string cwd = coot::util::current_working_dir();
	 g.molecules[imol].map_fill_from_mtz_with_reso_limits(std::string(mtz_file_name),
							      cwd,
							      std::string(f_col),
							      std::string(phi_col),
							      weight_col_str,
							      use_weights,
							      is_anomalous_flag,
							      is_diff_map,
							      use_reso_limits,
							      low_reso_limit,
							      high_reso_limit,
							      msr);
	 if (have_refmac_params) {
	    g.molecules[imol].store_refmac_params(std::string(mtz_file_name),
						  std::string(fobs_col), 
						  std::string(sigfobs_col), 
						  std::string(r_free_col),
						  sensible_r_free_col);
	    g.molecules[imol].set_refmac_save_state_commands(mtz_file_name, f_col, phi_col,
							     weight_col, use_weights, is_diff_map,
							     fobs_col, sigfobs_col,
							     r_free_col, sensible_r_free_col);
	 } else {
	   // save at least the mtz file from where the map comes
	   g.molecules[imol].store_refmac_mtz_filename(std::string(mtz_file_name));
	 }
         if (! is_diff_map) {
	   g.scroll_wheel_map = imol;
	   g.activate_scroll_radio_button_in_display_manager(imol);
	 }
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: label(s) not found in MTZ file " 
		   << mtz_file_name << " " << f_col << " " 
		   <<  phi_col << " ";
	 if (use_weights)
	    std::cout << weight_col << std::endl;
	 else 
	    std::cout << std::endl;
      }
   }
   if (imol != -1) {

      // We reset some strings if we weren't given refmac params -
      // otherwise we quote garbage or unallocated memory.
      std::string weight_col_str;
      std::string fobs_col_str;
      std::string sigfobs_col_str;
      std::string r_free_col_str;
      if (weight_col)
	 weight_col_str = single_quote(weight_col);
      else
	 weight_col_str = single_quote("Weight:None-specified");
      
      if (! have_refmac_params) {
	 fobs_col_str    = single_quote("Fobs:None-specified");
	 sigfobs_col_str = single_quote("SigF:None-specified");
	 r_free_col_str  = single_quote("RFree:None-specified");
	 sensible_r_free_col = 0;
      } else {
	 fobs_col_str    = single_quote(fobs_col);
	 sigfobs_col_str = single_quote(sigfobs_col_str);
	 r_free_col_str  = single_quote(r_free_col);
      }
      
      std::vector<std::string> command_strings;
      command_strings.push_back("make-and-draw-map-with-reso-with-refmac-params");
      command_strings.push_back(single_quote(coot::util::intelligent_debackslash(mtz_file_name)));
      command_strings.push_back(single_quote(f_col));
      command_strings.push_back(single_quote(phi_col));
      command_strings.push_back(weight_col_str);
      command_strings.push_back(graphics_info_t::int_to_string(use_weights));
      command_strings.push_back(graphics_info_t::int_to_string(is_diff_map));
      command_strings.push_back(graphics_info_t::int_to_string(have_refmac_params));
      command_strings.push_back(fobs_col_str);
      command_strings.push_back(sigfobs_col_str);
      command_strings.push_back(r_free_col_str);
      command_strings.push_back(graphics_info_t::int_to_string(sensible_r_free_col));
      command_strings.push_back(graphics_info_t::int_to_string(is_anomalous_flag));
      command_strings.push_back(graphics_info_t::int_to_string(use_reso_limits));
      command_strings.push_back(graphics_info_t::float_to_string(low_reso_limit));
      command_strings.push_back(graphics_info_t::float_to_string(high_reso_limit));
      add_to_history(command_strings);
   }
   return imol;
}

#include "cmtz-interface.hh"

int auto_read_make_and_draw_maps(const char *mtz_file_name) {

   int imol = -1;
   
   if (! coot::file_exists(mtz_file_name)) {
      std::cout << "WARNING:: file " << mtz_file_name << " does not exist" << std::endl;
   } else { 
      if ( is_mtz_file_p(mtz_file_name) ) {
	 imol = auto_read_make_and_draw_maps_from_mtz(mtz_file_name);
      } else {
	 imol = auto_read_make_and_draw_maps_from_cns(mtz_file_name);
      }
   }
   return imol;
} 

int auto_read_make_and_draw_maps_from_cns(const char *mtz_file_name) {

   int imol1 = -1;
   int imol2 = -1;
   if (coot::util::file_name_extension(mtz_file_name) != ".mtz") { 
      
      // Otherwise try extended CNS format
      graphics_info_t g;
      float msr = graphics_info_t::map_sampling_rate;
      imol1 = g.create_molecule();
      bool success;
      success = g.molecules[imol1].map_fill_from_cns_hkl( mtz_file_name, "F2", 0, msr );
      if (success) { 
	 imol2 = g.create_molecule();
	 success = g.molecules[imol2].map_fill_from_cns_hkl( mtz_file_name, "F1", 1, msr );
	 if (success) { 
	    g.scroll_wheel_map = imol1;
	    g.activate_scroll_radio_button_in_display_manager(imol1);
	 } else {
	    g.erase_last_molecule();
	 } 
      } else {
	 g.erase_last_molecule();
      }
   }
   return imol2;
}

int auto_read_make_and_draw_maps_from_mtz(const char *mtz_file_name) {

   int imol1 = -1;
   int imol2 = -1;
   graphics_info_t g;

   std::vector<coot::mtz_column_trials_info_t> auto_mtz_pairs;

   // built-ins
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("FWT",     "PHWT",      false));
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("DELFWT",  "PHDELWT",   true ));
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("2FOFCWT", "PH2FOFCWT", false));
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("FOFCWT",  "PHFOFCWT",  true ));
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("FDM",     "PHIDM",     false));
   auto_mtz_pairs.push_back(coot::mtz_column_trials_info_t("FAN",     "PHAN",      true));

   for (unsigned int i=0; i<g.user_defined_auto_mtz_pairs.size(); i++)
      auto_mtz_pairs.push_back(g.user_defined_auto_mtz_pairs[i]);
   
   std::vector<int> imols;

   for (unsigned int i=0; i<auto_mtz_pairs.size(); i++) {
      const coot::mtz_column_trials_info_t &b = auto_mtz_pairs[i];
      if (valid_labels(mtz_file_name, b.f_col.c_str(), b.phi_col.c_str(), "", 0)) {
	 int imol = make_and_draw_map_with_reso_with_refmac_params(mtz_file_name, 
								   b.f_col.c_str(),
								   b.phi_col.c_str(),
								   "",
								   0,  //    use_weights
								   b.is_diff_map,   // is_diff_map,
								   0,     //   short int have_refmac_params,
								   "",    //   const char *fobs_col,
								   "",    //   const char *sigfobs_col,
								   "",    //   const char *r_free_col,
								   0,     //   short int sensible_f_free_col,
								   0,     //   short int is_anomalous_flag,
								   0,     //   short int use_reso_limits,
								   0,     //   float low_reso_limit,
								   0);    //   float high_reso_limit
	 if (is_valid_model_molecule(imol))
	    imols.push_back(imol);
      }
   }

   coot::mtz_column_types_info_t r = coot::get_mtz_columns(mtz_file_name);
   for (unsigned int i=0; i<r.f_cols.size(); i++) {
      std::string s = r.f_cols[i].column_label;
      std::string::size_type idx = s.find(".F_phi.F");
      if (idx != std::string::npos) {
	 std::string prefix = s.substr(0, idx);
	 std::string trial_phi_col = prefix + ".F_phi.phi";
	 for (unsigned int j=0; j<r.phi_cols.size(); j++) {
	    if (r.phi_cols[j].column_label == trial_phi_col) {
	       std::string f_col   = r.f_cols[i].column_label;
	       std::string phi_col = r.phi_cols[j].column_label;
	       int imol = make_and_draw_map_with_reso_with_refmac_params(mtz_file_name, 
									 f_col.c_str(),
									 phi_col.c_str(),
									 "",
									 0,     //   use_weights
									 0,     //   is_diff_map,
									 0,     //   short int have_refmac_params,
									 "",    //   const char *fobs_col,
									 "",    //   const char *sigfobs_col,
									 "",    //   const char *r_free_col,
									 0,     //   short int sensible_f_free_col,
									 0,     //   short int is_anomalous_flag,
									 0,     //   short int use_reso_limits,
									 0,     //   float low_reso_limit,
									 0);    //   float high_reso_limit
	    }
	 }
      }
   }

   
   return imol2;
}


int auto_read_make_and_draw_maps_old(const char *mtz_file_name) {


   int imol1 = -1;
   int imol2 = -1;
   graphics_info_t g;

   if (! coot::file_exists(mtz_file_name)) {
      std::cout << "WARNING:: file " << mtz_file_name << " does not exist" << std::endl;
      return -1;
   }
   
   if ( is_mtz_file_p(mtz_file_name) ) {

      // try MTZ file
      // list of standard column names
      const char coldefs[][40] = { "+FWT,PHWT,",
                                   "-DELFWT,PHDELWT,",
                                   "+2FOFCWT,PH2FOFCWT,",
                                   "-FOFCWT,PHFOFCWT,",
                                   "+FDM,PHIDM,",
                                   "+parrot.F_phi.F,parrot.F_phi.phi,",
                                   "+pirate.F_phi.F,pirate.F_phi.phi,",
                                   "-FAN,PHAN,"};
      /* "-DELFAN,PHDELAN," not sure about this last one,
         and maybe last one could be optional!? */
      const int nc = sizeof(coldefs)/sizeof(coldefs[0]);

      // make a list of column names to try: F, phase, and difference map flag
      std::vector<std::string> cols_f(2), cols_p(2), cols_w(2), cols_d(2);

      /* no longer compile this bit
      cols_f[0] = graphics_info_t::auto_read_MTZ_FWT_col;
      cols_p[0] = graphics_info_t::auto_read_MTZ_PHWT_col;
      cols_d[0] = "+";
      cols_f[1] = graphics_info_t::auto_read_MTZ_DELFWT_col;
      cols_p[1] = graphics_info_t::auto_read_MTZ_PHDELWT_col;
      cols_d[1] = "-";
      */
      
      for ( int ic = 0; ic < nc; ic++ ) {
	 std::string s( coldefs[ic] );
	 int c1 = s.find( "," );
	 int c2 = s.find( ",", c1+1 );
	 std::string f = s.substr(1,c1-1);
	 std::string p = s.substr(c1+1,c2-c1-1);
	 std::string w = s.substr(c2+1);
	 std::string d = s.substr(0,1);
	 if ( f != cols_f[0] && f != cols_f[1] ) {
	    cols_f.push_back(f);
	    cols_p.push_back(p);
	    cols_w.push_back(w);
	    cols_d.push_back(d);
	 }
      }

      // try each set of columns in turn
      std::vector<int> imols;
      for ( int ic = 0; ic < cols_f.size(); ic++ ) {
	 int imol = -1;
	 int w = (cols_w[ic] != "" ) ? 1 : 0;
	 int d = (cols_d[ic] != "+") ? 1 : 0;

	 if ( valid_labels( mtz_file_name, cols_f[ic].c_str(),
			    cols_p[ic].c_str(), cols_w[ic].c_str(), w ) )
	    imol = make_and_draw_map_with_reso_with_refmac_params(mtz_file_name, 
								  cols_f[ic].c_str(),
								  cols_p[ic].c_str(),
								  cols_w[ic].c_str(),
								  w, d,  //    use_weights,  is_diff_map,
								  0,     //   short int have_refmac_params,
								  "",    //   const char *fobs_col,
								  "",    //   const char *sigfobs_col,
								  "",    //   const char *r_free_col,
								  0,     //   short int sensible_f_free_col,
								  0,     //   short int is_anomalous_flag,
								  0,     //   short int use_reso_limits,
								  0,     //   float low_reso_limit,
								  0);    //   float high_reso_limit
	 if ( imol >= 0 ) imols.push_back( imol );
      }

      if ( imols.size() > 0 ) {
	 imol1 = imols.front();
	 imol2 = imols.back();
	 g.scroll_wheel_map = imol1;
	 g.activate_scroll_radio_button_in_display_manager(imol1);
      } else {
	 GtkWidget *w = wrapped_nothing_bad_dialog("Failed to find any suitable F/phi columns in the MTZ file");
	 gtk_widget_show(w);
      }
   
   } else {

      // don't try to read as a CNS file if it is called an .mtz file
      // (map_fill_from_cns_hkl can dump core if file is not properly
      // formatted).
      // 
      if (coot::util::file_name_extension(mtz_file_name) != ".mtz") { 
      
	 // Otherwise try extended CNS format
	 float msr = graphics_info_t::map_sampling_rate;
	 imol1 = g.create_molecule();
	 bool success;
	 success = g.molecules[imol1].map_fill_from_cns_hkl( mtz_file_name, "F2", 0, msr );
	 if (success) { 
	    imol2 = g.create_molecule();
	    success = g.molecules[imol2].map_fill_from_cns_hkl( mtz_file_name, "F1", 1, msr );
	    if (success) { 
	       g.scroll_wheel_map = imol1;
	       g.activate_scroll_radio_button_in_display_manager(imol1);
	    } else {
	       g.erase_last_molecule();
	    } 
	 } else {
	    g.erase_last_molecule();
	 }
      }
   }
   return imol2; 
}

int auto_read_do_difference_map_too_state() {

   add_to_history_simple("auto-read-do-difference-map-too-state");
   int i = graphics_info_t::auto_read_do_difference_map_too_flag; 
   return i;

} 
void set_auto_read_do_difference_map_too(int i) {

   graphics_info_t::auto_read_do_difference_map_too_flag = i;
   std::string cmd = "set-auto-read-do-dfference-map-too";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
} 

/*! \brief set the default inital contour for 2FoFc-style map

in sigma */
void set_default_initial_contour_level_for_map(float n_sigma) {

   graphics_info_t::default_sigma_level_for_map = n_sigma;
   std::string cmd = "set-default-initial-contour-level-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(n_sigma);
   add_to_history_typed(cmd, args);
} 

/*! \brief set the default inital contour for FoFc-style map

in sigma */
void set_default_initial_contour_level_for_difference_map(float n_sigma) {

   graphics_info_t::default_sigma_level_for_fofc_map = n_sigma;
   std::string cmd = "set-default-initial-contour-level-for-difference-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(n_sigma);
   add_to_history_typed(cmd, args);
} 




void set_map_line_width(int w) {
   graphics_info_t::map_line_width = w;
   // update the maps because they may be being draw as graphical
   // objects.
   for (int imol=0; imol<graphics_info_t::n_molecules(); imol++)
      graphics_info_t::molecules[imol].update_map();
   graphics_draw();
   std::string cmd = "set-map-line-width";
   std::vector<coot::command_arg_t> args;
   args.push_back(w);
   add_to_history_typed(cmd, args);
   
}

int map_line_width_state() {
   add_to_history_simple("map-line-width-state");
   return graphics_info_t::map_line_width;
} 

int swap_difference_map_colours_state() {
  int ret = graphics_info_t::swap_difference_map_colours;
  return ret;
}

/* return success status 0 = failure (imol does not have a map) */
int set_map_is_difference_map(int imol) { 

   int istatus = 0;
   if (imol< graphics_n_molecules()) { 
      if (graphics_info_t::molecules[imol].has_xmap()) { 
	 graphics_info_t::molecules[imol].set_map_is_difference_map();
	 istatus = 1;
	 graphics_draw();
      } else { 
	 std::cout << "WARNING:: molecule " << imol << " does not have a map." <<  std::endl;
      } 

   } else { 
      std::cout << "WARNING:: No such molecule as " << imol << std::endl;
   }
   std::string cmd = "set-map-is-difference-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   
   return istatus;
}

int map_is_difference_map(int imol) {

   int istat = 0;
   if (is_valid_map_molecule(imol)) {
      istat = graphics_info_t::molecules[imol].is_difference_map_p();
   }
   return istat;
} 


/* return the index of the new molecule or -1 on failure */
int another_level() {

   int istat = -1;
   int imap = -1;

   imap = imol_refinement_map();
   if (imap == -1) { 
      for (int i=0; i<graphics_info_t::n_molecules(); i++) {
	 if (is_valid_map_molecule(i)) {
	    if (! graphics_info_t::molecules[i].is_difference_map_p()) { 
	       imap = i;
	    }
	 }
      }
   }
   
   if (imap > -1) {
      istat = another_level_from_map_molecule_number(imap);
   }
   // history elsewhere
   return istat;
}

int another_level_from_map_molecule_number(int imap) {
   int istat = -1;
   if (is_valid_map_molecule(imap)) {
      // create another map with the same parameters as imap and then
      // push up the contour level a sigma.
//       std::cout << "DEBUG:: calling make_and_draw_map_with_reso_with_refmac_params"
// 		<< std::endl;
      istat = make_and_draw_map_with_reso_with_refmac_params(
	  graphics_info_t::molecules[imap].save_mtz_file_name.c_str(),
	  graphics_info_t::molecules[imap].save_f_col.c_str(),
	  graphics_info_t::molecules[imap].save_phi_col.c_str(),
          graphics_info_t::molecules[imap].save_weight_col.c_str(),
          graphics_info_t::molecules[imap].save_use_weights,
          graphics_info_t::molecules[imap].save_is_diff_map_flag,
	  0, "None", "None", "None", 0, // refmac params
          graphics_info_t::molecules[imap].save_is_anomalous_map_flag,
          graphics_info_t::molecules[imap].save_use_reso_limits,
          graphics_info_t::molecules[imap].save_low_reso_limit,
          graphics_info_t::molecules[imap].save_high_reso_limit);

//       std::cout << "DEBUG:: istat in another_level_from_map_molecule_number "
// 		<< istat << std::endl;
      
      if (istat != -1) { 

	 float map_sigma = graphics_info_t::molecules[istat].map_sigma();
	 float current_contour_level = graphics_info_t::molecules[istat].contour_level;
	 graphics_info_t::molecules[istat].set_contour_level(current_contour_level +
							     map_sigma*1.0);
	 graphics_info_t::molecules[istat].update_map();
	 graphics_draw();
      }
   }
   std::string cmd = "another-level-from-map-molecule-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(imap);
   add_to_history_typed(cmd, args);
   return istat;
}



void set_map_radius_slider_max(float f) {
   graphics_info_t::map_radius_slider_max = f;
   std::string cmd = "set-map-radius-slider-max";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
} 
void set_contour_level_absolute(int imol_map, float level) {

   if (is_valid_map_molecule(imol_map)) {
      graphics_info_t::molecules[imol_map].set_contour_level(level);
   }
   graphics_draw();

   std::string cmd = "set-contour-level-absolute";
   std::vector<coot::command_arg_t> args;
   args.push_back(level);
   add_to_history_typed(cmd, args);
}

void set_contour_level_in_sigma(int imol_map, float level) {
   
   if (is_valid_map_molecule(imol_map)) {
      graphics_info_t::molecules[imol_map].set_contour_level_by_sigma(level);
   }
   graphics_draw();
   std::string cmd = "set-contour-level-in-sigma";
   std::vector<coot::command_arg_t> args;
   args.push_back(level);
   add_to_history_typed(cmd, args);
}


void set_last_map_contour_level(float level) {

   graphics_info_t g;
   g.set_last_map_contour_level(level);
   std::string cmd = "set-last-map-contour-level";
   std::vector<coot::command_arg_t> args;
   args.push_back(level);
   add_to_history_typed(cmd, args);
}

void set_last_map_contour_level_by_sigma(float level) {

   graphics_info_t g;
   g.set_last_map_contour_level_by_sigma(level);
   std::string cmd = "set-last-map-contour-level-by-sigma";
   std::vector<coot::command_arg_t> args;
   args.push_back(level);
   add_to_history_typed(cmd, args);
}

void set_last_map_sigma_step(float f) { 

   graphics_info_t g;
   g.set_last_map_sigma_step(f);
   std::string cmd = "set-last-map-sigma-step";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);

} 

// -------------------------------------------------------------------------
//                        (density) iso level increment entry
// -------------------------------------------------------------------------
//

// imol is ignored.
//
char* get_text_for_iso_level_increment_entry(int imol) {

   char *text;
   graphics_info_t g;

   text = (char *) malloc (100);
   snprintf(text, 90, "%-6.4f", g.iso_level_increment);

   return text;

}

void set_iso_level_increment_from_text(const char *text, int imol) {

   float val;

   graphics_info_t g;

   val = atof(text);

   if ((val > 10000) || (val < -10000)) {
      cout << "Cannot interpret: " << text
	   << ".  Assuming 0.05 for increment" << endl;
      val  = 0.05;

   }

   cout << "setting iso_level_increment to " << val << endl; 
   g.iso_level_increment = val;

   graphics_draw();
}

void set_iso_level_increment(float val) { 
   graphics_info_t g;
   g.iso_level_increment = val;
} 

float get_iso_level_increment() {
  float ret = graphics_info_t:: iso_level_increment;
  return ret;
}

// imol is ignored.
//
char* get_text_for_diff_map_iso_level_increment_entry(int imol) {

   char *text;
   graphics_info_t g;

   text = (char *) malloc (100);
   snprintf(text, 90, "%-6.4f", g.diff_map_iso_level_increment);
   return text;

}

void set_diff_map_iso_level_increment_from_text(const char *text, int imol) {

   float val;
   graphics_info_t g;

   val = atof(text);

   if ((val > 10000) || (val < -10000)) {
      cout << "Cannot interpret: " << text
	   << ".  Assuming 0.005 for increment" << endl;
      val  = 0.005;
   } 
   g.diff_map_iso_level_increment = val;
   graphics_draw();
}

void set_diff_map_iso_level_increment(float val) { 
   graphics_info_t::diff_map_iso_level_increment = val;
} 

float get_diff_map_iso_level_increment() { 
   float ret =graphics_info_t::diff_map_iso_level_increment;
   return ret;
}

void set_map_sampling_rate_text(const char *text) {

   float val;
   val = atof(text);

   if ((val > 100) || (val < 1)) {
      cout << "Nonsense value: " << text
	   << ".  Assuming 1.5 for increment" << endl;
      val  = 1.5;
   }
   set_map_sampling_rate(val);

}

void set_map_sampling_rate(float r) {

   graphics_info_t g;
   g.map_sampling_rate = r;

}

char* get_text_for_map_sampling_rate_text() {

   char *text;
   graphics_info_t g;

   text = (char *) malloc (100);
   snprintf(text, 90, "%-5.4f", g.map_sampling_rate);
   return text;


}

float get_map_sampling_rate() {
   graphics_info_t g;
   return g.map_sampling_rate;
}


/* applies to the current map */
void change_contour_level(short int is_increment) { // else is decrement. 

   graphics_info_t g; 
   int s = g.scroll_wheel_map;

   if (is_valid_map_molecule(s)) {
      
      if (g.molecules[s].is_difference_map_p()) {
	 g.molecules[s].contour_level += g.diff_map_iso_level_increment;
      } else {
	 // normal case
	 if (is_increment) { 
	    g.molecules[s].contour_level += g.iso_level_increment;
	 } else {
	    g.molecules[s].contour_level -= g.iso_level_increment;
	 }
      }
      g.molecules[s].update_map();
      graphics_draw();
   }
} 

void set_initial_map_for_skeletonize() { 
   
   graphics_info_t::set_initial_map_for_skeletonize();

}

void set_max_skeleton_search_depth(int v) { 
   graphics_info_t g;
   g.set_max_skeleton_search_depth(v);
} 

/* Set the radio buttons in the frame to the be on or off for the map
   that is displayed in the optionmenu (those menu items "activate"
   callbacks (graphics_info::skeleton_map_select change
   g.map_for_skeletonize).  */
void set_on_off_skeleton_radio_buttons(GtkWidget *skeleton_frame) { 

   graphics_info_t g;
   g.set_on_off_skeleton_radio_buttons(skeleton_frame);
}

// imol is used imap is ignored.
// You can fix this anachronism one day if you like.  FIXME.
// 
void set_scrollable_map(int imol) {

   graphics_info_t g;
   if (is_valid_map_molecule(imol)) {
      int imap = 0; // ignored
      g.set_Scrollable_Map(imol, imap); // in graphics-info.h
   } else {
      std::cout << "WARNING:: " << imol << " is not a valid molecule"
		<< " in set_scrollable_map\n";
   }
     
}


int 
skeletonize_map(int prune_flag, int imol) {

   graphics_info_t::skeletonize_map(prune_flag, imol);
   return 0;
}

int unskeletonize_map(int imol) {
   graphics_info_t::unskeletonize_map(imol);
   return imol;
} 




void
do_skeleton_prune() { 

   graphics_info_t g;
   float map_cutoff  = g.skeleton_level;

   for (int imol=0; imol<g.n_molecules(); imol++) {
      if (g.molecules[imol].has_xmap() &&
	  !g.molecules[imol].xmap_is_diff_map) {
	    
	 if (g.molecules[imol].xskel_is_filled == 1) { 
	       
	    BuildCas bc(g.molecules[imol].xmap, map_cutoff); 

	    // mark segments by connectivity
	    // 
	    int nsegments = bc.count_and_mark_segments(g.molecules[imol].xskel_cowtan, 
						       g.molecules[imol].xmap,
						       map_cutoff); 

	    bc.transfer_segment_map(&g.molecules[imol].xskel_cowtan);
	    g.molecules[imol].update_clipper_skeleton();
	 }
      }
   }
} 
#ifdef USE_GUILE
SCM get_map_colour_scm(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_map_molecule(imol)) {
      std::vector<float> colour_v = graphics_info_t::molecules[imol].map_colours();
      if (colour_v.size() > 2) {
	 r = scm_list_3(scm_float2num(colour_v[0]),
			scm_float2num(colour_v[1]),
			scm_float2num(colour_v[2]));
      }
   }
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *get_map_colour_py(int imol) {

  PyObject *r = Py_False;
  if (is_valid_map_molecule(imol)) {
    std::vector<float> colour_v = graphics_info_t::molecules[imol].map_colours();
    if (colour_v.size() > 2) {
      r = PyList_New(colour_v.size());
      for (int i=0; i<colour_v.size(); i++) {
        PyList_SetItem(r, i, PyFloat_FromDouble(colour_v[i]));
      }
    }
  }
  if (PyBool_Check(r)) {
    Py_XINCREF(r);
  }
  return r;
} 
#endif



/* give a warning dialog if density it too dark (blue) */
void check_for_dark_blue_density() {

   if (graphics_info_t::use_graphics_interface_flag) {
      for (int i=0; i<graphics_info_t::n_molecules(); i++) { 
	 if (graphics_info_t::molecules[i].has_xmap()) { 
	    if (graphics_info_t::molecules[i].is_displayed_p()) {
	       if (background_is_black_p()) { 
		  if (graphics_info_t::molecules[i].map_is_too_blue_p()) {
		     std::string s = "I suggest that you increase the brightness of the map\n";
		     s += " if this is for a presentation (blue projects badly).";
		     info_dialog(s.c_str());
		     break; // only make the dialog once
		  }
	       }
	    }
	 }
      }
   } 
} 

int
handle_read_ccp4_map(const char* filename, int is_diff_map_flag) {

   int istate = -1;
   if (filename) { 
      std::string str(filename); 
      graphics_info_t g;
      int imol_new = graphics_info_t::create_molecule();

      istate = g.molecules[imol_new].read_ccp4_map(str, is_diff_map_flag,
						   *graphics_info_t::map_glob_extensions);

      if (istate > -1) { // not a failure
	 g.scroll_wheel_map = imol_new;  // change the current scrollable map.
	 g.activate_scroll_radio_button_in_display_manager(imol_new);
      } else {
	 g.erase_last_molecule();
	 std::cout << "Read map " << str << " failed" << std::endl;
	 std::string s = "Read map ";
	 s += str;
	 s += " failed.";
	 g.add_status_bar_text(s);
      } 
      graphics_draw();
   } else {
      // error
      std::cout << "ERROR:: filename null in handle_read_ccp4_map\n";
   }
   return istate;
}

void set_contour_by_sigma_step_by_mol(float f, short int state, int imol) { 
   
   if (imol < graphics_info_t::n_molecules()) { 
      if (imol >= 0) { 
	 if (graphics_info_t::molecules[imol].has_xmap()) {
	    // NXMAP-FIXME
	    graphics_info_t::molecules[imol].set_contour_by_sigma_step(f, state);
	 }
      }
   }
}

int export_map(int imol, const char *filename) {

   int rv = 0; // fail
   if (is_valid_map_molecule(imol)) {

      try { 
	 clipper::CCP4MAPfile mapout;
	 mapout.open_write(std::string(filename));
	 mapout.export_xmap(graphics_info_t::molecules[imol].xmap);
	 mapout.close_write();
	 rv = 1;
      }
      catch (...) {
	 std::cout << "CCP4 map writing error for " << filename << std::endl;
      }
      
   } else {
      graphics_info_t g;
      g.add_status_bar_text("Invalid map molecule number");
   }
   return rv; 
}

int export_map_fragment(int imol, float x, float y, float z, float radius, const char *filename) {

   int rv = 0;
   if (is_valid_map_molecule(imol)) {
      graphics_info_t g;
      clipper::Coord_orth pos(x,y,z);
      g.molecules[imol].export_map_fragment(radius, pos, filename);
      rv = 1;
   } 
   return rv;
}

/*! \brief export a fragment of the map about (x,y,z)  */
int export_map_fragment_with_origin_shift(int imol, float x, float y, float z, float radius, const char *filename) {

   int rv = 0;
   if (is_valid_map_molecule(imol)) {
      graphics_info_t g;
      clipper::Coord_orth pos(x,y,z);
      g.molecules[imol].export_map_fragment_with_origin_shift(radius, pos, filename);
      rv = 1;
   } 
   return rv;
} 



/* create a number of maps by segmenting the given map, above the
   (absolute) low_level.  New maps are on the same grid as the input
   map.  */
void segment_map(int imol_map, float low_level) {

   int max_segments = 300;
   
   if (is_valid_map_molecule(imol_map)) {
      clipper::Xmap<float> &xmap_in = graphics_info_t::molecules[imol_map].xmap;
      coot::util::segment_map s;
      std::pair<int, clipper::Xmap<int> > segmented_map = s.segment(xmap_in, low_level);
      float contour_level = graphics_info_t::molecules[imol_map].get_contour_level();

      for (int iseg=0; (iseg<segmented_map.first) && iseg<max_segments; iseg++) {
	 clipper::Xmap<float> xmap;
	 xmap.init(segmented_map.second.spacegroup(),
		   segmented_map.second.cell(),
		   segmented_map.second.grid_sampling());
	 clipper::Xmap_base::Map_reference_index ix;
	 for (ix = segmented_map.second.first(); !ix.last(); ix.next()) {
	    if (segmented_map.second[ix] == iseg)
	       xmap[ix] = xmap_in[ix];
	 }
	 int imol_new = graphics_info_t::create_molecule();
	 std::string name = "Map ";
	 name += coot::util::int_to_string(imol_map);
	 name += " Segment ";
	 name += coot::util::int_to_string(iseg);
	 graphics_info_t::molecules[imol_new].new_map(xmap, name);
	 graphics_info_t::molecules[imol_new].set_contour_level(contour_level);
      }
   }
   graphics_draw();
}

void segment_map_multi_scale(int imol_map, float low_level, float b_factor_inc, int n_rounds) {

   int max_segments = 300;
   if (is_valid_map_molecule(imol_map)) {
      clipper::Xmap<float> &xmap_in = graphics_info_t::molecules[imol_map].xmap;
      coot::util::segment_map s;
      std::pair<int, clipper::Xmap<int> > segmented_map = s.segment(xmap_in, low_level, b_factor_inc, n_rounds);
      float contour_level = graphics_info_t::molecules[imol_map].get_contour_level();
      for (int iseg=0; (iseg<segmented_map.first) && iseg<max_segments; iseg++) {
	 clipper::Xmap<float> xmap;
	 xmap.init(segmented_map.second.spacegroup(),
		   segmented_map.second.cell(),
		   segmented_map.second.grid_sampling());
	 clipper::Xmap_base::Map_reference_index ix;
	 int n_points_in_map = 0;
	 for (ix = segmented_map.second.first(); !ix.last(); ix.next()) {
	    if (segmented_map.second[ix] == iseg) { 
	       xmap[ix] = xmap_in[ix];
	       n_points_in_map++;
	    }
	 }
	 if (n_points_in_map) { 
	    int imol_new = graphics_info_t::create_molecule();
	    std::string name = "Map ";
	    name += coot::util::int_to_string(imol_map);
	    name += " Segment ";
	    name += coot::util::int_to_string(iseg);
	    graphics_info_t::molecules[imol_new].new_map(xmap, name);
	    graphics_info_t::molecules[imol_new].set_contour_level(contour_level);
	 }
      }
   }
   graphics_draw();
} 



// the cell is given in Angstroms and the angles in degrees.
int transform_map_raw(int imol, 
		      double r00, double r01, double r02, 
		      double r10, double r11, double r12, 
		      double r20, double r21, double r22, 
		      double t0, double t1, double t2,
		      double pt1, double pt2, double pt3, double box_size,
		      const char *ref_space_group,
		      double cell_a, double cell_b, double cell_c,
		      double alpha, double beta, double gamma) {

   int imol_new = -1;
   if (is_valid_map_molecule(imol)) {
      clipper::Mat33<double> m(r00, r01, r02, r10, r11, r12, r20, r21, r22);
      clipper::Coord_orth c(t0, t1, t2);
      clipper::RTop_orth rtop(m,c);
      // clipper::RTop_orth rtop_inv = rtop.inverse();
      clipper::Coord_orth pt(pt1, pt2, pt3);

      std::cout << "INFO:: in transforming map around target point "
		<< pt.format() << std::endl;

      clipper::Spgr_descr sg_descr(ref_space_group);
      clipper::Spacegroup new_space_group(sg_descr);
      
      clipper::Cell_descr cell_d(cell_a, cell_b, cell_c,
				 clipper::Util::d2rad(alpha),
				 clipper::Util::d2rad(beta),
				 clipper::Util::d2rad(gamma));
      clipper::Cell new_cell(cell_d);
	 
      
      clipper::Xmap<float> new_map =
	 coot::util::transform_map(graphics_info_t::molecules[imol].xmap,
				   new_space_group, new_cell,
				   rtop, pt, box_size);

      const coot::ghost_molecule_display_t ghost_info;
      // int is_diff_map_flag = graphics_info_t::molecules[imol].is_difference_map_p();
      // int swap_colours_flag = graphics_info_t::swap_difference_map_colours;
      mean_and_variance<float> mv = map_density_distribution(new_map, 0);
      std::string name = "Transformed map";
      imol_new = graphics_info_t::create_molecule();
      graphics_info_t::molecules[imol_new].new_map(new_map, name);
      graphics_draw();

   } else {
      std::cout << "WARNING:: molecule " << imol << " is not a valid map" << std::endl;
   }
   return imol_new;
}

/*! \brief make a difference map, taking map_scale * imap2 from imap1,
  on the grid of imap1.  Return the new molecule number.  
  Return -1 on failure. */
int difference_map(int imol1, int imol2, float map_scale) {

   int r = -1;

   if (is_valid_map_molecule(imol1)) {
      if (is_valid_map_molecule(imol2)) {
 	 std::pair<clipper::Xmap<float>, float> dm =
 	    coot::util::difference_map(graphics_info_t::molecules[imol1].xmap,
 				       graphics_info_t::molecules[imol2].xmap,
 				       map_scale);
	 int imol = graphics_info_t::create_molecule();
	 std::string name = "difference-map";
	 // int swpcolf = graphics_info_t::swap_difference_map_colours;
	 graphics_info_t::molecules[imol].new_map(dm.first, name);
	 graphics_info_t::molecules[imol].set_map_is_difference_map();
	 
	 r = imol;
	 graphics_draw();
      }
   }
   return r;
}


int reinterp_map(int map_no, int reference_map_no) {

   int r = -1;
   if (is_valid_map_molecule(map_no)) { 
      if (is_valid_map_molecule(reference_map_no)) {
	 graphics_info_t g;
	 clipper::Xmap<float> new_map =
	    coot::util::reinterp_map(g.molecules[map_no].xmap,
				     g.molecules[reference_map_no].xmap);
	 int imol = graphics_info_t::create_molecule();
	 std::string name = "map ";
	 name += coot::util::int_to_string(map_no);
	 name += " re-interprolated to match ";
	 name += coot::util::int_to_string(reference_map_no);
	 graphics_info_t::molecules[imol].new_map(new_map, name);
	 r = imol;
	 graphics_draw();
      }
   }
   return r;
} 

#ifdef USE_GUILE
/*! \brief make an average map from the map_number_and_scales (which
  is a list of pairs (list map-number scale-factor)) (the scale factor
  are typically 1.0 of course). */
int average_map_scm(SCM map_number_and_scales) {

   int r = -1;
   SCM n_scm = scm_length(map_number_and_scales);
   int n = scm_to_int(n_scm);
   std::vector<std::pair<clipper::Xmap<float>, float> > maps_and_scales_vec;
   for (unsigned int i=0; i<n; i++) {
      SCM number_and_scale = scm_list_ref(map_number_and_scales, SCM_MAKINUM(i));
      SCM ns_scm = scm_length(number_and_scale);
      int ns = scm_to_int(ns_scm);
      if (ns == 2) {
	 SCM map_number_scm = scm_list_ref(number_and_scale, SCM_MAKINUM(0));
	 SCM map_scale_scm  = scm_list_ref(number_and_scale, SCM_MAKINUM(1));
	 if (scm_is_true(scm_integer_p(map_number_scm))) {
	    if (scm_is_true(scm_number_p(map_scale_scm))) {
	       int map_number = scm_to_int(map_number_scm);
	       if (is_valid_map_molecule(map_number)) {
		  float scale = scm_to_double(map_scale_scm);
		  std::pair<clipper::Xmap<float>, float> p(graphics_info_t::molecules[map_number].xmap, scale);
		  maps_and_scales_vec.push_back(p);
	       } else {
		  std::cout << "Invalid map number " << map_number << std::endl;
	       } 
	    } else {
	       std::cout << "Bad scale "
			 << scm_to_locale_string(display_scm(map_scale_scm))
			 << " ignoring map " 
			 << scm_to_locale_string(display_scm(map_number_scm))
			 << std::endl;
	    }
	 } else {
	    std::cout << "Bad map number " << scm_to_locale_string(display_scm(map_number_scm))
		      << std::endl;
	 } 
      }
   }
   if (maps_and_scales_vec.size() > 0) {
      clipper::Xmap<float> average_map = coot::util::average_map(maps_and_scales_vec);
      int imol = graphics_info_t::create_molecule();
      std::string name = "averaged-map";
      graphics_info_t::molecules[imol].new_map(average_map, name);
      r = imol;
      graphics_draw();
   } 
   return r;
}
#endif


#ifdef USE_PYTHON
/*! \brief make an average map from the map_number_and_scales (which
  is a list of pairs [map-number, scale-factor]) (the scale factors
  are typically 1.0 of course). */
int average_map_py(PyObject *map_number_and_scales) {

   int r = -1;
   int n = PyObject_Length(map_number_and_scales);
   std::vector<std::pair<clipper::Xmap<float>, float> > maps_and_scales_vec;
   for (unsigned int i=0; i<n; i++) {
      PyObject *number_and_scale = PyList_GetItem(map_number_and_scales, i);
      int ns = PyObject_Length(number_and_scale);
      if (ns == 2) {
	 PyObject *map_number_py = PyList_GetItem(number_and_scale, 0);
	 PyObject *map_scale_py  = PyList_GetItem(number_and_scale, 1);
	 if (PyInt_Check(map_number_py)) {
	   if (PyFloat_Check(map_scale_py) || PyInt_Check(map_scale_py)) {
	       int map_number = PyInt_AsLong(map_number_py);
	       if (is_valid_map_molecule(map_number)) {
		  float scale = PyFloat_AsDouble(map_scale_py);
		  std::pair<clipper::Xmap<float>, float> p(graphics_info_t::molecules[map_number].xmap, scale);
		  maps_and_scales_vec.push_back(p);
	       } else {
		  std::cout << "Invalid map number " << map_number << std::endl;
	       } 
	    } else {
	     std::cout << "Bad scale " << PyString_AsString(display_python(map_scale_py))   // FIXME
	     		 << std::endl;
	    }
	 } else {
	   std::cout << "Bad map number " << PyString_AsString(display_python(map_number_py))  // FIXME
	         << std::endl;
	 } 
      }
   }
   if (maps_and_scales_vec.size() > 0) {
      clipper::Xmap<float> average_map = coot::util::average_map(maps_and_scales_vec);
      int imol = graphics_info_t::create_molecule();
      std::string name = "averaged-map";
      graphics_info_t::molecules[imol].new_map(average_map, name);
      r = imol;
      graphics_draw();
   } 
   return r;
} 
#endif 




/*  ----------------------------------------------------------------------- */
/*                  variance map                                            */
/*  ----------------------------------------------------------------------- */
//! \name Variance Map
//! \{
//! \brief Make a variance map, based on the grid of the first map.
//!
int make_variance_map(std::vector<int> map_molecule_number_vec) {

   int imol_map = -1;

   std::vector<std::pair<clipper::Xmap<float>, float> > xmaps;
   for (unsigned int i=0; i<map_molecule_number_vec.size(); i++) {
      int imol = map_molecule_number_vec[i];
      if (is_valid_map_molecule(imol)) {
	 float scale = 1.0;
	 xmaps.push_back(std::pair<clipper::Xmap<float>, float> (graphics_info_t::molecules[imol].xmap, scale));
      } 
   }
   std::cout << "debug:: map_molecule_number_vec size " << map_molecule_number_vec.size() << std::endl;
   std::cout << "debug:: xmaps size " << xmaps.size() << std::endl;
   if (xmaps.size()) {
      clipper::Xmap<float> variance_map = coot::util::variance_map(xmaps);
      int imol = graphics_info_t::create_molecule();
      std::string name = "variance-map";
      graphics_info_t::molecules[imol].new_map(variance_map, name);
      imol_map = imol;
      graphics_draw();
   } 
   return imol_map;
} 
//! \}


#ifdef USE_GUILE
int make_variance_map_scm(SCM map_molecule_number_list) {

   std::vector<int> v;
   SCM n_scm = scm_length(map_molecule_number_list);
   int n = scm_to_int(n_scm);
   for (unsigned int i=0; i<n; i++) { 
      SCM mol_number_scm = scm_list_ref(map_molecule_number_list, SCM_MAKINUM(i));
      if (scm_is_true(scm_integer_p(mol_number_scm))) {
	 int map_number = scm_to_int(mol_number_scm);
	 if (is_valid_map_molecule(map_number))
	    v.push_back(map_number);
      }
   }
   return make_variance_map(v);
}
#endif // USE_GUILE

#ifdef USE_PYTHON
int make_variance_map_py(PyObject *map_molecule_number_list) {

   std::vector<int> v;
   if (PyList_Check(map_molecule_number_list)) { 
      int n = PyObject_Length(map_molecule_number_list);
      for (unsigned int i=0; i<n; i++) { 
	 PyObject *mol_number_py = PyList_GetItem(map_molecule_number_list, i);
	 if (PyInt_Check(mol_number_py)) {
	    int map_number = PyInt_AsLong(mol_number_py);
	    if (is_valid_map_molecule(map_number)) {
	       v.push_back(map_number);
	    }
	 }
      }
   }
   return make_variance_map(v);
} 
#endif // USE_PYTHON


/* ------------------------------------------------------------------------- */
/*                      correllation maps                                    */
/* ------------------------------------------------------------------------- */

// 0: all-atoms
// 1: main-chain atoms if is standard amino-acid, else all atoms
// 2: side-chain atoms if is standard amino-acid, else all atoms
// 3: side-chain atoms-exclusing CB if is standard amino-acid, else all atoms
// 
float map_to_model_correlation(int imol,
			       const std::vector<coot::residue_spec_t> &specs,
			       const std::vector<coot::residue_spec_t> &neighb_specs,
			       unsigned short int atom_mask_mode,
			       int imol_map) {

   float ret_val = -2;
   float atom_radius = 1.5;
   if (is_valid_model_molecule(imol)) {
      if (is_valid_map_molecule(imol_map)) {
	 CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	 clipper::Xmap<float> xmap_reference = graphics_info_t::molecules[imol_map].xmap;
	 ret_val = coot::util::map_to_model_correlation(mol, specs, neighb_specs,
							atom_mask_mode,
							atom_radius, xmap_reference);
      }
   }

   return ret_val;
}
   

#ifdef USE_GUILE
SCM map_to_model_correlation_scm(int imol,
				 SCM residue_specs,
				 SCM neighb_residue_specs,
				 unsigned short int atom_mask_mode,
				 int imol_map) {

   std::vector<coot::residue_spec_t> residues    = scm_to_residue_specs(residue_specs);
   std::vector<coot::residue_spec_t> nb_residues = scm_to_residue_specs(neighb_residue_specs);
   float c = map_to_model_correlation(imol, residues, nb_residues, atom_mask_mode, imol_map);
   SCM ret_val = scm_double2num(c);
   return ret_val;
}
#endif 

#ifdef USE_PYTHON
PyObject *map_to_model_correlation_py(int imol,
				      PyObject *residue_specs,
				      PyObject *neighb_residue_specs,
				      unsigned short int atom_mask_mode,
				      int imol_map) {

   std::vector<coot::residue_spec_t> residues    = py_to_residue_specs(residue_specs);
   std::vector<coot::residue_spec_t> nb_residues = py_to_residue_specs(neighb_residue_specs);
   float c = map_to_model_correlation(imol, residues, nb_residues,
				      atom_mask_mode, imol_map);
   return PyFloat_FromDouble(c);
}
#endif 


std::vector<std::pair<coot::residue_spec_t,float> >
map_to_model_correlation_per_residue(int imol, const std::vector<coot::residue_spec_t> &specs,
				     unsigned short int atom_mask_mode,
				     int imol_map) {

   float atom_radius = 1.5; // user variable?
   std::vector<std::pair<coot::residue_spec_t,float> > v;
   if (is_valid_model_molecule(imol)) {
      if (is_valid_map_molecule(imol_map)) {
	 CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	 clipper::Xmap<float> xmap_reference = graphics_info_t::molecules[imol_map].xmap;
	 v = coot::util::map_to_model_correlation_per_residue(mol, specs, atom_mask_mode, atom_radius, xmap_reference);
      }
   }
   return v;
}

						   


#ifdef USE_GUILE
SCM
map_to_model_correlation_per_residue_scm(int imol, SCM specs_scm, unsigned short int atom_mask_mode, int imol_map) {

   SCM r = SCM_EOL;
   std::vector<coot::residue_spec_t> specs = scm_to_residue_specs(specs_scm);
   std::vector<std::pair<coot::residue_spec_t,float> >
      v = map_to_model_correlation_per_residue(imol, specs, atom_mask_mode, imol_map);
   for (unsigned int i=0; i<v.size(); i++) {
      SCM p1 = scm_residue(v[i].first);
      SCM p2 = scm_double2num(v[i].second);
      SCM item = scm_list_2(p1, p2);
      r = scm_cons(item, r);
   }
   r = scm_reverse(r);
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *
map_to_model_correlation_per_residue_py(int imol, PyObject *specs_py, unsigned short int atom_mask_mode, int imol_map) {

   std::vector<coot::residue_spec_t> specs = py_to_residue_specs(specs_py);
   std::vector<std::pair<coot::residue_spec_t,float> >
      v = map_to_model_correlation_per_residue(imol, specs, atom_mask_mode, imol_map);

   PyObject *r = PyList_New(v.size());
   for (unsigned int i=0; i<v.size(); i++) {
      PyObject *p0 = py_residue(v[i].first);
      PyObject *p1 = PyFloat_FromDouble(v[i].second);
      PyObject *item = PyList_New(2);
      PyList_SetItem(item, 0, p0);
      PyList_SetItem(item, 1, p1);
      PyList_SetItem(r, i, item);
   }
   return r;
}
#endif

