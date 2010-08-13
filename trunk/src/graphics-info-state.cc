/* src/graphics-info-state.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006 The University of York
 * Copyright 2007, 2009 by The University of Oxford.
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

#ifdef USE_PYTHON
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#if defined _MSC_VER
#include <windows.h>
#endif

#include <fstream>

#include <gtk/gtk.h>
#include "interface.h"
#include "globjects.h" // for GRAPHICS_WINDOW_X_START_SIZE and Y
#include "graphics-info.h"
#include "c-interface.h"
#include "cc-interface.hh"
#include "c-interface-scm.hh"

// save state
int
graphics_info_t::save_state_file(const std::string &filename) {

   // std::cout << "saving state" << std::endl;
   std::vector<std::string> commands;
   short int il = -1; // il: interface language, with some initial bogus value

#ifdef USE_PYTHON
   il = 2;
#endif // USE_PYTHON
   
#if defined USE_GUILE && ! defined WINDOWS_MINGW
   il = 1;
#endif // USE_GUILE

   std::string comment_str;
   if (il == 1) { 
      comment_str = "; These commands are the saved state of coot.  You can evaluate them\n";
      comment_str += "; using \"Calculate->Run Script...\".";
   } else { 
      comment_str = "# These commands are the saved state of coot.  You can evaluate them\n";
      comment_str += "# using \"Calculate->Run Script...\".";
   }
   commands.push_back(comment_str);
   
   std::vector<std::string> mod_data_start = save_state_data_and_models(il);
   for (unsigned int im=0; im<mod_data_start.size(); im++)
      commands.push_back(mod_data_start[im]);

   // the first thing:  The window position and size:

   // std::cout << "DEBUG in state: " << graphics_x_size << " " << graphics_y_size
   // << std::endl;
   
   if ( ! ((graphics_x_size == GRAPHICS_WINDOW_X_START_SIZE) &&
	   (graphics_y_size != GRAPHICS_WINDOW_Y_START_SIZE)) ) {
      commands.push_back(state_command("set-graphics-window-size",
				       graphics_x_size, graphics_y_size, il));
   }
   commands.push_back(state_command("set-graphics-window-position",
				    graphics_x_position, graphics_y_position, il));
   // now the positions of all the major dialogs:
   if (graphics_info_t::model_fit_refine_x_position > -1)
      commands.push_back(state_command("set-model-fit-refine-dialog-position",
				       model_fit_refine_x_position,
				       model_fit_refine_y_position, il));
   if (graphics_info_t::display_manager_x_position > -1)
      commands.push_back(state_command("set-display-control-dialog-position",
				       display_manager_x_position,
				       display_manager_y_position, il));

   if (graphics_info_t::go_to_atom_window_x_position > -1)
      commands.push_back(state_command("set-go-to-atom-window-position",
				       go_to_atom_window_x_position,
				       go_to_atom_window_y_position, il));
   if (graphics_info_t::delete_item_widget_x_position > -1)
      commands.push_back(state_command("set-delete-dialog-position",
				       delete_item_widget_x_position,
				       delete_item_widget_y_position, il));
   if (graphics_info_t::rotate_translate_x_position > -1)
      commands.push_back(state_command("set-rotate-translate-dialog-position",
				       rotate_translate_x_position,
				       rotate_translate_y_position, il));
   if (graphics_info_t::accept_reject_dialog_x_position > -1)
      commands.push_back(state_command("set-accept-reject-dialog-position",
				       accept_reject_dialog_x_position,
				       accept_reject_dialog_y_position, il));
   if (graphics_info_t::ramachandran_plot_x_position > -1)
      commands.push_back(state_command("set-ramachandran-plot-dialog-position",
				       ramachandran_plot_x_position,
				       ramachandran_plot_y_position, il));
   if (graphics_info_t::edit_chi_angles_dialog_x_position > -1)
      commands.push_back(state_command("set-edit-chi-angles-dialog-position",
				       edit_chi_angles_dialog_x_position,
				       edit_chi_angles_dialog_y_position, il));

   // Virtual trackball
   if (vt_surface_status() == 1)
      commands.push_back(state_command("vt-surface", 1, il));
   else
      commands.push_back(state_command("vt-surface", 2, il));

   if (sticky_sort_by_date)
      commands.push_back(state_command("set-sticky-sort-by-date",il));

   commands.push_back(state_command("set-clipping-front", clipping_front, il));
   commands.push_back(state_command("set-clipping-back",  clipping_back, il));

   commands.push_back(state_command("set-map-radius", box_radius, il));

   unsigned short int v = 4; // 4 dec pl. if float_to_string_using_dec_pl is fixed.
   // a "flag" to use a different function to generate the string from the float
   commands.push_back(state_command("set-iso-level-increment", iso_level_increment, il, v));
   commands.push_back(state_command("set-diff-map-iso-level-increment", diff_map_iso_level_increment, il, v));

   
   commands.push_back(state_command("set-colour-map-rotation-on-read-pdb",
				    rotate_colour_map_on_read_pdb, il));
   commands.push_back(state_command("set-colour-map-rotation-on-read-pdb-flag",
				    rotate_colour_map_on_read_pdb_flag, il));
   commands.push_back(state_command("set-colour-map-rotation-on-read-pdb-c-only-flag",
				    rotate_colour_map_on_read_pdb_c_only_flag, il));
   commands.push_back(state_command("set-swap-difference-map-colours",
				    swap_difference_map_colours, il));

   commands.push_back(state_command("set-background-colour",
				    background_colour[0],
				    background_colour[1],
				    background_colour[2], il));

   // set_density_size_from_widget (not from widget): box_size
   // show unit cell: per molecule (hmm)
   // commands.push_back(state_command("set-aniso-limit", show_aniso_atoms_radius_flag, il));

   
   commands.push_back(state_command("set-symmetry-size", symmetry_search_radius, il));
   commands.push_back(state_command("set-symmetry-colour-merge", float(symmetry_colour_merge_weight), il));
   commands.push_back(state_command("set-symmetry-colour",
				    symmetry_colour[0],
				    symmetry_colour[1],
				    symmetry_colour[2], il));
				    
   // FIXME
   //    commands.push_back(state_command("set-symmetry-whole-chain", symmetry_whole_chain_flag, il));
   commands.push_back(state_command("set-symmetry-atom-labels-expanded", symmetry_atom_labels_expanded_flag, il));
   commands.push_back(state_command("set-active-map-drag-flag", active_map_drag_flag, il));
   commands.push_back(state_command("set-show-aniso", show_aniso_atoms_flag, il));
   commands.push_back(state_command("set-aniso-probability", show_aniso_atoms_probability, il));
   commands.push_back(state_command("set-smooth-scroll-steps", smooth_scroll_steps, il));
   commands.push_back(state_command("set-smooth-scroll-limit", smooth_scroll_limit, il));
   commands.push_back(state_command("set-font-size", atom_label_font_size, il));
   commands.push_back(state_command("set-rotation-centre-size", rotation_centre_cube_size, il));
   commands.push_back(state_command("set-do-anti-aliasing", do_anti_aliasing_flag, il));
   commands.push_back(state_command("set-default-bond-thickness", default_bond_width, il));

   // cif dictionary
   if (cif_dictionary_filename_vec->size() > 0) { 
      for (unsigned int i=0; i<cif_dictionary_filename_vec->size(); i++) {
	 commands.push_back(state_command("read-cif-dictionary", 
					  single_quote((*cif_dictionary_filename_vec)[i]), il));
      }
   }

   // Torsion restraints were set?
   if (do_torsion_restraints)
      commands.push_back(state_command("set-refine-with-torsion-restraints", do_torsion_restraints, il));
   
   std::vector <std::string> command_strings;

   // because the goto_atom_molecule could be 11 with 8 closed
   // molecules, we need to find which one it will be when the script
   // is read in, so we make a count of the molecules... and update
   // local_go_to_atom_mol when we hit the current go_to_atom_molecule.
   int local_go_to_atom_mol = 0;

   // map sampling rate
   if (map_sampling_rate != 1.5) { // only set it if it is not the default
      command_strings.push_back("set-map-sampling-rate");
      command_strings.push_back(float_to_string(map_sampling_rate));
      commands.push_back(state_command(command_strings, il));
      command_strings.clear();
   }

   // goto atom stuff?

   // Now each molecule:
   //
   // We use molecule_count now so that we can use the toggle
   // functions, which need to know the molecule number.
   // molecule_count is the molecule number on execution of this
   // script.  This is a bit sleezy because it relies on the molecule
   // number not being disturbed by pre-existing molecules in coot.
   // Perhaps we should have toggle-last-map-display
   // toggle-last-mol-display toggle-last-mol-active functions.
   // 
   int molecule_count = 0;
   int scroll_wheel_map_for_state = -1; // unset
   for (int i=0; i<n_molecules(); i++) {
      if (molecules[i].has_map() || molecules[i].has_model()) { 
	 // i.e. it was not Closed...
	 command_strings = molecules[i].save_state_command_strings();
	 if (command_strings.size() > 0) {
	    commands.push_back(state_command(command_strings, il));
	    std::vector <std::string> display_strings;
	    std::vector <std::string>  active_strings;
	    // colour
	    if (molecules[i].has_model()) { 
	       display_strings.clear();
	       display_strings.push_back("set-molecule-bonds-colour-map-rotation");
	       display_strings.push_back(int_to_string(molecule_count));
	       display_strings.push_back(float_to_string(molecules[i].bonds_colour_map_rotation));
	       commands.push_back(state_command(display_strings, il));
	    }
	    if (molecules[i].has_model()) {
	       if (! molecules[i].drawit) {
		  display_strings.clear();
		  display_strings.push_back("set-mol-displayed");
		  display_strings.push_back(int_to_string(molecule_count));
		  display_strings.push_back(int_to_string(0));
		  commands.push_back(state_command(display_strings, il));
	       }
	       if (! molecules[i].atom_selection_is_pickable()) {
		  active_strings.clear();
		  active_strings.push_back("set-mol-active");
		  active_strings.push_back(int_to_string(molecule_count));
		  active_strings.push_back(int_to_string(0));
		  commands.push_back(state_command(active_strings, il));
	       }

	       if (molecules[i].bond_thickness() != default_bond_width) {
		  display_strings.clear();
		  display_strings.push_back("set-bond-thickness");
		  display_strings.push_back(int_to_string(molecule_count));
		  display_strings.push_back(int_to_string(molecules[i].bond_thickness()));
		  commands.push_back(state_command(display_strings, il));
	       }

	       // hydrogens?
	       display_strings.clear();
	       display_strings.push_back("set-draw-hydrogens");
	       display_strings.push_back(int_to_string(molecule_count));
	       display_strings.push_back(int_to_string(molecules[i].draw_hydrogens()));
	       commands.push_back(state_command(display_strings, il));
	       
	       // symmetry issues:
	       if (molecules[i].symmetry_as_calphas) {
		  // default would be not CAlphas
		  active_strings.clear();
		  active_strings.push_back("symmetry-as-calphas");
		  active_strings.push_back(int_to_string(molecule_count));
		  active_strings.push_back(int_to_string(1));
		  commands.push_back(state_command(active_strings, il));
	       }
	       if (!molecules[i].show_symmetry) {
		  // default would be to show symmetry
		  active_strings.clear();
		  active_strings.push_back("set-show-symmetry-molecule");
		  active_strings.push_back(int_to_string(molecule_count));
		  active_strings.push_back(int_to_string(0));
		  commands.push_back(state_command(active_strings, il));
	       }
	       if (molecules[i].symmetry_colour_by_symop_flag) {
		  // default is not to colour by symop
		  active_strings.clear();
		  active_strings.push_back("set-symmetry-colour-by-symop");
		  active_strings.push_back(int_to_string(molecule_count));
		  active_strings.push_back(int_to_string(1));
		  commands.push_back(state_command(active_strings, il));
	       }
	       if (molecules[i].symmetry_whole_chain_flag) {
		  // default is not to colour by symop
		  active_strings.clear();
		  active_strings.push_back("set-symmetry-whole-chain");
		  active_strings.push_back(int_to_string(molecule_count));
		  active_strings.push_back(int_to_string(1));
		  commands.push_back(state_command(active_strings, il));
	       }

// 	       std::cout << "molecules[i].Bonds_box_type() is "
// 			 << molecules[i].Bonds_box_type()
// 			 << std::endl;
	       
	       if (molecules[i].Bonds_box_type() != coot::NORMAL_BONDS) {
		  if (molecules[i].Bonds_box_type() == coot::CA_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-ca-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::COLOUR_BY_CHAIN_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("set-colour-by-chain");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::CA_BONDS_PLUS_LIGANDS) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-ca-plus-ligands-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::BONDS_NO_WATERS) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-bonds-no-waters-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::BONDS_SEC_STRUCT_COLOUR) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-sec-struct-bonds-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-ca-plus-ligands-sec-struct-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::COLOUR_BY_MOLECULE_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("set-colour-by-molecule");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::COLOUR_BY_RAINBOW_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-rainbow-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::COLOUR_BY_B_FACTOR_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("graphics_to_b_factor_representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
		  if (molecules[i].Bonds_box_type() == coot::COLOUR_BY_OCCUPANCY_BONDS) {
		     active_strings.clear();
		     active_strings.push_back("graphics-to-occupancy-representation");
		     active_strings.push_back(int_to_string(molecule_count));
		     commands.push_back(state_command(active_strings, il));
		  }
	       }

	       // Additional Representations
	       if (molecules[i].add_reps.size() > 0) {

		  // First the "all" status for the additional representation of this molecule.
		  // Ooops. This is not (yet) a separate state.  Currently, pressing the button
		  // simply turns on (or off) all representations.  The display manager does not
		  // save the state of this button (if it is off, closed and then openned then the
		  // "all" button is not shown!)
		  
// 		  active_strings.clear();
// 		  active_strings.push_back("set-show-all-additional-representations");
// 		  active_strings.push_back(int_to_string(molecule_count));
// 		  active_strings.push_back(int_to_string(0));
// 		  commands.push_back(state_command(active_strings, il));

		  
		  for (unsigned int iar=0; iar<molecules[i].add_reps.size(); iar++) {
		     active_strings.clear();
		     if (molecules[i].add_reps[iar].atom_sel_info.type ==
			 coot::atom_selection_info_t::BY_ATTRIBUTES) { 
			active_strings.push_back("additional-representation-by-attributes");
			active_strings.push_back(int_to_string(molecule_count));
			active_strings.push_back(single_quote(molecules[i].add_reps[iar].atom_sel_info.chain_id));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].atom_sel_info.resno_start));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].atom_sel_info.resno_end));
			active_strings.push_back( single_quote(molecules[i].add_reps[iar].atom_sel_info.ins_code));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].representation_type));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].bonds_box_type));
			active_strings.push_back(float_to_string(molecules[i].add_reps[iar].bond_width));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].draw_hydrogens_flag));
			commands.push_back(state_command(active_strings, il));
		     }
		     if (molecules[i].add_reps[iar].atom_sel_info.type ==
			 coot::atom_selection_info_t::BY_STRING) { 
			active_strings.push_back("additional-representation-by-string");
			active_strings.push_back(int_to_string(molecule_count));
			active_strings.push_back(single_quote(molecules[i].add_reps[iar].atom_sel_info.atom_selection_str));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].representation_type));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].bonds_box_type));
			active_strings.push_back(float_to_string(molecules[i].add_reps[iar].bond_width));
			active_strings.push_back(int_to_string(molecules[i].add_reps[iar].draw_hydrogens_flag));
			commands.push_back(state_command(active_strings, il));
		     }
		     
		     // now save the on/off state of the add. rep.  Default is on, so only write out
		     // the on/off status if the add rep is not shown.
		     // 
		     if (! molecules[i].add_reps[iar].show_it) {
			active_strings.clear();
			active_strings.push_back("set-show-additional-representation");
			active_strings.push_back(int_to_string(molecule_count));
			active_strings.push_back(int_to_string(iar));
			active_strings.push_back(int_to_string(0));
			commands.push_back(state_command(active_strings, il));
		     }
		  } 
	       }

	       // Is there a sequence associated with this model?
	       if (molecules[i].input_sequence.size() > 0) {
		  for (unsigned int iseq=0; iseq<molecules[i].input_sequence.size(); iseq++) {
		     active_strings.clear();
		     active_strings.push_back("assign-pir-sequence");
		     active_strings.push_back(int_to_string(molecule_count));
		     active_strings.push_back(single_quote(molecules[i].input_sequence[iseq].first));
		     std::string title = molecules[i].dotted_chopped_name();
		     title += " chain ";
		     title += molecules[i].input_sequence[iseq].first;
		     std::string pir_seq = coot::util::plain_text_to_pir(title,
									 molecules[i].input_sequence[iseq].second);
		     active_strings.push_back(single_quote(pir_seq));
		     commands.push_back(state_command(active_strings, il));
		  }
	       }
	    }

	    // Maps:
	    // 
	    if (molecules[i].has_map()) { 
	       command_strings = molecules[i].set_map_colour_strings();
	       commands.push_back(state_command(command_strings, il));
	       command_strings = molecules[i].get_map_contour_strings();
	       commands.push_back(state_command(command_strings, il));
	       if (molecules[i].contoured_by_sigma_p()) { 
		  command_strings = molecules[i].get_map_contour_sigma_step_strings();
		  commands.push_back(state_command(command_strings, il));
	       }
	       if (! molecules[i].is_displayed_p()) {
		  display_strings.clear();
		  display_strings.push_back("set-map-displayed");
		  display_strings.push_back(int_to_string(molecule_count));
		  display_strings.push_back(int_to_string(0));
		  commands.push_back(state_command(display_strings, il));
	       }

	       if (i == imol_refinement_map) {
		  display_strings.clear();
		  display_strings.push_back("set-imol-refinement-map");
		  display_strings.push_back(int_to_string(molecule_count));
		  commands.push_back(state_command(display_strings, il));
	       }

	       if (i == scroll_wheel_map) {
		  // save for after maps have been read.
		  scroll_wheel_map_for_state = molecule_count;
	       }
	    }
	    if (molecules[i].show_unit_cell_flag) {
	       display_strings.clear();
	       display_strings.push_back("set-show-unit-cell");
	       display_strings.push_back(int_to_string(molecule_count));
	       display_strings.push_back(int_to_string(1));
	       commands.push_back(state_command(display_strings, il));
	    } 
	 }

	 if (i==go_to_atom_molecule_)
	    local_go_to_atom_mol = molecule_count;

	 molecule_count++; 
      }
   }

   // last things to do:

   // did we find a map that was the scroll wheel map?
   // 
   if (scroll_wheel_map_for_state != -1) {
      std::vector <std::string> display_strings;
      display_strings.push_back("set-scroll-wheel-map");
      display_strings.push_back(int_to_string(scroll_wheel_map_for_state));
      commands.push_back(state_command(display_strings, il));
   }

   // was the default weight changed.  Whatever, just save the current
   // value already.
   commands.push_back(state_command("set-matrix", geometry_vs_map_weight, il));

   // planar peptide restraints?
   bool ps = Geom_p()->planar_peptide_restraint_state();
   if (! ps)
      commands.push_back(state_command("remove-planar-peptide-restraints", il));


   // environment distances?
   if (environment_show_distances) {
      commands.push_back(state_command("set-show-environment-distances",
				       int(1), il));
   }

   // show symmetry.  Turn this on after molecules have been read so
   // that we don't get the error popup.
   commands.push_back(state_command("set-show-symmetry-master", int(show_symmetry), il));
   // go to atom
   command_strings.resize(0);
   command_strings.push_back("set-go-to-atom-molecule");
   command_strings.push_back(int_to_string(local_go_to_atom_mol));
   commands.push_back(state_command(command_strings, il));

   command_strings.resize(0);
   if (go_to_atom_residue_ != -9999) { // magic unset value
      command_strings.push_back("set-go-to-atom-chain-residue-atom-name");
      command_strings.push_back(single_quote(go_to_atom_chain_));
      command_strings.push_back(int_to_string(go_to_atom_residue_));
      command_strings.push_back(single_quote(go_to_atom_atom_name_));
      commands.push_back(state_command(command_strings, il));
   }
   
   
   // view things: rotation centre and zoom. Sanity check the zoom first.
   //
   //
   float zoom_f = zoom/100.0;
   if (zoom_f < 0.1) zoom_f = 0.1;
   commands.push_back(state_command("scale-zoom", zoom_f, il)); 
   commands.push_back(state_command("set-rotation-centre", X(), Y(), Z(), il));

   // the orientation
   //
   if (use_graphics_interface_flag) {
      command_strings.clear();
      command_strings.push_back("set-view-quaternion");
      command_strings.push_back(float_to_string_using_dec_pl(quat[0], 5));
      command_strings.push_back(float_to_string_using_dec_pl(quat[1], 5));
      command_strings.push_back(float_to_string_using_dec_pl(quat[2], 5));
      command_strings.push_back(float_to_string_using_dec_pl(quat[3], 5));
      commands.push_back(state_command(command_strings, il));
   }

   // stereo mode
   // 
   // in_side_by_side_stereo_mode is not used, Baah.
   if (display_mode == coot::SIDE_BY_SIDE_STEREO) {
      int stereo_display_mode = 0;
      commands.push_back(state_command("side-by-side-stereo-mode", stereo_display_mode, il));
   }
   if (display_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) {
      int stereo_display_mode = 1;
      commands.push_back(state_command("side-by-side-stereo-mode", stereo_display_mode, il));
   }
   // 
   if (stereo_mode_state() == 1)
      commands.push_back(state_command("hardware-stereo-mode", il));


   // dialogs
   if (model_fit_refine_dialog)
      commands.push_back(state_command("post-model-fit-refine-dialog", il));
   if (go_to_atom_window)
      commands.push_back(state_command("post-go-to-atom-window", il));
   if (display_control_window_)
      commands.push_back(state_command("post-display-control-window", il));

   short int istat = 0;
   if (! disable_state_script_writing) { 
      istat = write_state(commands, filename);
      if (istat) {
	 std::string s = "Status file ";
	 s += filename;
	 s += " written.";
	 statusbar_text(s);
      } else {
	 std::string s = "WARNING:: failed to write status file ";
	 s += filename;
	 statusbar_text(s);
      }
   }
   return int(istat);
}


std::vector<std::string>
graphics_info_t::save_state_data_and_models(short int lang_flag) const {

   std::vector<std::string> v;
   for (int i=0; i<n_molecules(); i++) {
      if (molecules[i].has_map() || molecules[i].has_model()) {
	 // don't tell us that there are molecules in the state file
	 // that can't be read from a file (i.e. if
	 // save_state_command_strings() is blank).
	 std::vector <std::string> command_strings =
	    molecules[i].save_state_command_strings();
	 if (command_strings.size() > 0) {
	    std::string s = ";;molecule-info: ";
	    s += molecules[i].name_for_display_manager();
	    v.push_back(s);
	 }
      }
   }

   // add the cif dictionaries:
   if (cif_dictionary_filename_vec->size() > 0) { 
      for (unsigned int i=0; i<cif_dictionary_filename_vec->size(); i++) {
	 std::string sd = ";;molecule-info: Dictionary: ";
	 // sd += (*cif_dictionary_filename_vec)[i];
	 std::string s = (*cif_dictionary_filename_vec)[i];
	 std::string::size_type islash = s.find_last_of("/");
	 if (islash != std::string::npos) { 
	    s = s.substr(islash+1, s.length());
	 }
	 sd += s;
	 v.push_back(sd);
      }
   }
   
   // add a hash at the start for python comments
   if (lang_flag == 2) {
      for (unsigned int i=0; i<v.size(); i++) {
	 v[i] = "#" + v[i];
      }
   }
   return v;
}

// return a list of molecule names to be added to the "Run State
// script?" dialog. The state script is filename, of course
// 
std::vector<std::string>
graphics_info_t::save_state_data_and_models(const std::string &filename,
					    short int lang_flag) const { 

   std::vector<std::string> v;
   std::string mol_prefix = ";;molecule-info:";
   if (lang_flag == 2) { // python
      mol_prefix = "#" + mol_prefix; 
   }
   
   std::ifstream f(filename.c_str());
   if (f) {
      std::string s;
      while (! f.eof()) {
	 getline(f, s);
	 // f >> s;
	 std::string ss(s);
	 // xstd::cout << ss << std::endl;
	 std::string::size_type iprefix = ss.find(mol_prefix);
	 if (iprefix != std::string::npos) {
	    std::string::size_type ispace = ss.find(" ");
	    if (ispace != std::string::npos) {
	       std::string m = ss.substr(ispace);
	       // std::cout << "found molecule :" << m << ":" << std::endl;
	       v.push_back(m);
	    } else {
	       // std::cout << "no space found" << std::endl;
	    } 
	 } else {
	    // std::cout << "no prefix found" << std::endl;
	 } 
      }
   }
   f.close();
   return v;
} 



int
graphics_info_t::save_state() {

   if (run_state_file_status) { 
#ifdef USE_GUILE
      return save_state_file(save_state_file_name);
#else
#ifdef USE_PYTHON   
      return save_state_file("0-coot.state.py");
#else
      return 0;
#endif // USE_PYTHON   
#endif // USE_GUILE
   }
   return 0;
}

std::string 
graphics_info_t::state_command(const std::string &str,
			       int i1,
			       short int state_lang) const {

   std::vector<std::string> command_strings;
   command_strings.push_back(str);
   command_strings.push_back(int_to_string(i1));
   return state_command(command_strings,state_lang);
}

std::string 
graphics_info_t::state_command(const std::string &str,
			       int i1,
			       int i2,
			       short int state_lang) const {

   std::vector<std::string> command_strings;
   command_strings.push_back(str);
   command_strings.push_back(int_to_string(i1));
   command_strings.push_back(int_to_string(i2));
   return state_command(command_strings,state_lang);
}

std::string 
graphics_info_t::state_command(const std::string &str,
			       float f,
			       short int state_lang) const {

   std::vector<std::string> command_strings;
   command_strings.push_back(str);
   command_strings.push_back(float_to_string(f));
   return state_command(command_strings,state_lang);
}

std::string 
graphics_info_t::state_command(const std::string &str,
			       float f,
			       short int state_lang,
			       short unsigned int t) const {

   std::vector<std::string> command_strings;
   command_strings.push_back(str);
   command_strings.push_back(float_to_string_using_dec_pl(f,t));
   return state_command(command_strings,state_lang);
}

std::string 
graphics_info_t::state_command(const std::string &str,
			       float f1,
			       float f2,
			       float f3,
			       short int state_lang) const {

   std::vector<std::string> command_strings;
   command_strings.push_back(str);
   command_strings.push_back(float_to_string(f1));
   command_strings.push_back(float_to_string(f2));
   command_strings.push_back(float_to_string(f3));
   return state_command(command_strings,state_lang);
}


std::string 
graphics_info_t::state_command(const std::string &str, short int state_lang) const {

   std::vector<std::string> command;
   command.push_back(str);
   return state_command(command,state_lang);
}

// command arg interface
std::string
graphics_info_t::state_command(const std::string &str, 
			       const std::string &str2, 
			       short int state_lang) { 

   std::vector<std::string> command;
   command.push_back(str);
   command.push_back(str2);
   return state_command(command,state_lang);

} 

   
std::string 
graphics_info_t::state_command(const std::vector<std::string> &strs,
			       short int state_lang) const {

   std::string command = "";
   
   if (strs.size() > 0) { 
      if (state_lang == coot::STATE_SCM) {
	 command = "(";
	 for (int i=0; i<(int(strs.size())-1); i++) {
	    command += strs[i];
	    command += " ";
	 }
	 command += strs.back();
	 command += ")";
      }
   
      if (state_lang == coot::STATE_PYTHON) {
	 if (strs.size() > 0) { 
	    command = pythonize_command_name(strs[0]);
	    command += " (";
	    for (int i=1; i<(int(strs.size())-1); i++) {
	       command += strs[i];
	       command += ", ";
	    }
	    if (strs.size() > 1) 
	       command += strs.back();
	    command +=  ")";
	 }
      } 
   }
   return command;
} 

// Return success status.
// 
short int 
graphics_info_t::write_state(const std::vector<std::string> &commands,
			     const std::string &filename) const {

   bool do_c_mode = 1;

   short int istat = 1;
   if (do_c_mode) { 
      istat = write_state_c_mode(commands, filename);
   } else { 
      istat = write_state_fstream_mode(commands, filename);
   }
   return istat;
}

// 20090914-PE: this has been optioned out.  It was causing a crash on
// Mac OSX (of some kind - not all). I suspect a deeper issue.  The
// code seems clean to me.
// 
// Return success status.
// 
short int 
graphics_info_t::write_state_fstream_mode(const std::vector<std::string> &commands,
					  const std::string &filename) const {

   short int istat = 1;
   
   // std::cout << "writing state" << std::endl;
   std::ofstream f(filename.c_str());

   if (f) {
      for (unsigned int i=0; i<commands.size(); i++) {
	 f << commands[i] << std::endl;
      }
      f.flush();  // fixes valgrind problem?
      
      // f.close(); 20090914-PE comment out because it causes a crash
      // on Mac OS X (these days) (it didn't before I recently updated
      // the coot dependencies with fink).  However, given the above
      // uncertain remark about valgrind problems, I think that this
      // is only a temporary solution to deeper issue.  I think that
      // there is a memory problem elsewhere that is causing this
      // f.close() to give a problem.  Sounds pretty hellish to track
      // down until the problem exhibits itself on something where we
      // can run valgrind (if indeed even that will do any good).
      //
      // Note, of course, that the close() here is not necessary as
      // the ofstream f will get a flush() and close() when it goes
      // out of scope at the end of this function.
      
      std::cout << "State file " << filename << " written." << std::endl;

   } else {
      std::cout << "WARNING: couldn't write to state file " << filename
		<< std::endl;
      istat = 0;
   }
   return istat;
}


// Return success status.
// 
short int 
graphics_info_t::write_state_c_mode(const std::vector<std::string> &commands,
				    const std::string &filename) const {

   short int istat = 0;

   FILE *file = fopen(filename.c_str(), "w");
   if (file) { 
      for (unsigned int i=0; i<commands.size(); i++) {
	 fputs(commands[i].c_str(), file);
	 fputs("\n", file);
	 }
      istat = 1;
      fclose(file);
   } else {
      std::cout << "WARNING: couldn't write settings commands to file " << filename
		<< std::endl;
   } 
   return istat;
}



int
graphics_info_t::check_for_unsaved_changes() const {

   int iv = 0;
   for (int imol=0; imol<n_molecules(); imol++) {
      if (molecules[imol].Have_unsaved_changes_p()) {
	 GtkWidget *dialog = create_unsaved_changes_dialog();
	 fill_unsaved_changes_dialog(dialog);
	 gtk_widget_show(dialog);
	 iv = 1;
	 break;
      }
   }
   return iv;
}

void
graphics_info_t::fill_unsaved_changes_dialog(GtkWidget *dialog) const {

   GtkWidget *vbox = lookup_widget(GTK_WIDGET(dialog),
				   "unsaved_changes_molecule_vbox");

   int menu_index=0;
   for (int imol=0; imol<n_molecules(); imol++) {
      if (molecules[imol].Have_unsaved_changes_p()) {
	 std::string labelstr = int_to_string(imol);
	 labelstr += "  ";
	 labelstr += molecules[imol].name_;
	 GtkWidget *label = gtk_label_new(labelstr.c_str());
	 gtk_widget_show(label);
	 gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
	 gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      }
   }
}



/*  ------------------------------------------------------------------------ */
/*                         history                                           */
/*  ------------------------------------------------------------------------ */

void 
graphics_info_t::add_history_command(const std::vector<std::string> &command_strings) { 
   
   history_list.history_strings.push_back(command_strings);
} 

// Being a maniac, I thought to write out the history in scm and py.
// On reflection, I think I might be a nutter (200402xx?).
//
// Well, it seems I am still a nutter, but instead of keeping 2
// histories, I will keep one history that can be formatted as either
// (20050328).
// 
int
graphics_info_t::save_history() const { 

   int istate = 0; 
   std::string history_file_name("0-coot-history");
   std::vector<std::vector<std::string> > raw_command_strings = history_list.history_list();
   std::vector<std::string> languaged_commands;
   if (python_history) { 
      for (unsigned int i=0; i<raw_command_strings.size(); i++)
	 languaged_commands.push_back(pythonize_command_strings(raw_command_strings[i]));
      std::string file = history_file_name + ".py";
      istate =  write_state(languaged_commands, file);
   }
   if (guile_history) {
      languaged_commands.resize(0);
      for (unsigned int i=0; i<raw_command_strings.size(); i++)
	 languaged_commands.push_back(schemize_command_strings(raw_command_strings[i]));
      std::string file = history_file_name + ".scm";
      istate =  write_state(languaged_commands, file);
   }
   return istate;
}

// static
std::string
graphics_info_t::pythonize_command_strings(const std::vector<std::string> &command_strings) {

   std::string command;
   if (command_strings.size() > 0) { 
      command = pythonize_command_name(command_strings[0]);
      command += " (";
      for (int i=1; i<(int(command_strings.size())-1); i++) {
	 command += command_strings[i];
	 command += ", ";
      }
      if (command_strings.size() > 1)
         command += command_strings.back();
      command += ")";
   }
   // std::cout << "INFO:: python history command: " << command << std::endl;
   return command;
}


// static
std::string
graphics_info_t::schemize_command_strings(const std::vector<std::string> &command_strings) {

   std::string command;
   bool done = 0; 
#ifdef USE_GUILE
   if (command_strings.size() == 2) {
      if (command_strings[0] == DIRECT_SCM_STRING) {
	 command = command_strings[1];
	 done = 1;
      }
   }
#endif // USE_GUILE   

   if (! done) { 
      command = "(";
      for (int i=0; i<(int(command_strings.size())-1); i++) {
	 command += command_strings[i];
	 command += " ";
      }
      command += command_strings.back();
      command += ")";
      // std::cout << "INFO:: history command: " << command << std::endl;
   }
   return command;
}

