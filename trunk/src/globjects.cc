/* src/globjects.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2006 by Bernhard Lohkamp
 * Copyright 2007 by Paul Emsley
 * Copyright 2008, 2009 by The University of Oxford
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */
 
// we need the functions:
//
// draw, reshape, init, mouse_move and mouse_button press
// (and animate(for idle)).


#ifdef USE_PYTHON
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#ifndef NULL
#define NULL 0
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> // for keyboarding.

#if (GTK_MAJOR_VERSION == 1)
#include <gtkgl/gtkglarea.h>
#else
#include <gdk/gdkglconfig.h>
#include <gtk/gtkgl.h>
#endif // (GTK_MAJOR_VERSION == 1)

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h> // needed for wirecube and wiresphere.
#ifdef WINDOWS_MINGW
// in windows we need these for some newer openGL functions
#include <GL/glext.h>
//#include <GL/wglext.h>
#endif // WINDOWS_MINGW
#include "sleep-fixups.h"

#include <string.h> // strncmp

#include <math.h>
#ifndef HAVE_VECTOR
#include <vector>
#endif // HAVE_VECTOR

#ifndef HAVE_STRING
#include <string>
#endif // HAVE_STRING

#include "interface.h"

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "cos-sin.h"
#include "Cartesian.h"
#include "Bond_lines.h"

#include "graphics-info.h"

#include "pick.h"

#include "gl-matrix.h"

// #include "xmap-interface.h" // is this necessary? nope


#include "trackball.h"

#include "c-interface.h" // for toggle idle function
#include "cc-interface.hh"
#include "globjects.h"  // string

#include "coot-database.hh"

#include "rotate-translate-modes.hh"
#include "rotamer-search-modes.hh"

std::vector<molecule_class_info_t> graphics_info_t::molecules;


// Initialize the graphics_info_t mouse positions
// and rotation centre.

#if !defined WINDOWS_MINGW
bool graphics_info_t::prefer_python = 0; // prefer python scripts when
					 // scripting (if we have a
					 // choice). Default: no.
#else
bool graphics_info_t::prefer_python = 1; // Default: yes in Windows
#endif

short int graphics_info_t::python_at_prompt_flag = 0;

int graphics_info_t::show_paths_in_display_manager_flag = 0;
std::vector<std::string> *graphics_info_t::command_line_scripts;
coot::command_line_commands_t graphics_info_t::command_line_commands;
std::vector<std::string> graphics_info_t::command_line_accession_codes;

std::vector<coot::lsq_range_match_info_t> *graphics_info_t::lsq_matchers;
std::vector<coot::generic_text_object_t> *graphics_info_t::generic_texts_p = 0;
std::vector<coot::view_info_t> *graphics_info_t::views = 0;
bool graphics_info_t::do_expose_swap_buffers_flag = 1;

// Views 
float graphics_info_t::views_play_speed = 10.0;

// movies
std::string graphics_info_t::movie_file_prefix = "movie_";
int graphics_info_t::movie_frame_number = 0;
int graphics_info_t::make_movie_flag = 0;


// LSQ
short int graphics_info_t::in_lsq_plane_deviation = 0;
short int graphics_info_t::in_lsq_plane_define    = 0;
GtkWidget *graphics_info_t::lsq_plane_dialog = 0;
std::vector<clipper::Coord_orth> *graphics_info_t::lsq_plane_atom_positions;
std::string graphics_info_t::lsq_match_chain_id_ref;
std::string graphics_info_t::lsq_match_chain_id_mov;
int graphics_info_t::lsq_ref_imol = -1;
int graphics_info_t::lsq_mov_imol = -1;

// side by side stereo?
short int graphics_info_t::in_side_by_side_stereo_mode = 0;
short int graphics_info_t::in_wall_eyed_side_by_side_stereo_mode = 0;

// display list for maps?
short int graphics_info_t::display_lists_for_maps_flag = 1;

//
int graphics_info_t::save_imol = -1;

// accept/reject
GtkWidget *graphics_info_t::accept_reject_dialog = 0;

// refinement control dialog (so that we don't get multiple copies)
GtkWidget *graphics_info_t::refine_params_dialog = 0;

// flag to display the accep/reject dialog in the toolbar
int graphics_info_t::accept_reject_dialog_docked_flag = coot::DIALOG;

// flag to show/hide/sensitise the docked accep/reject dialog in the toolbar
int graphics_info_t::accept_reject_dialog_docked_show_flag = coot::DIALOG_DOCKED_SHOW;

// the refinement toolbar show/hide
short int graphics_info_t::model_toolbar_show_hide_state = 1;

// the refinement toolbar position
short int graphics_info_t::model_toolbar_position_state = coot::model_toolbar::RIGHT;

// the refinement toolbar style
short int graphics_info_t::model_toolbar_style_state = 1;

// the main toolbar show/hide
short int graphics_info_t::main_toolbar_show_hide_state = 1;

// the main toolbar position
// // not using (yet)
//short int graphics_info_t::main_toolbar_position_state = coot::main_toolbar::TOP;

// the main toolbar style
short int graphics_info_t::main_toolbar_style_state = 2;

// refmac option menu
int graphics_info_t::refmac_molecule = -1; 

//
short int graphics_info_t::active_map_drag_flag = 1; // true
long int graphics_info_t::Frames = 0;  // These 2 are to measure graphics speed.
long int graphics_info_t::T0 = 0;
int    graphics_info_t::show_fps_flag = 0;
int    graphics_info_t::control_is_pressed = 0 ; // false
short int graphics_info_t::control_key_for_rotate_flag = 1;
short int graphics_info_t::pick_pending_flag = 0;
int    graphics_info_t::a_is_pressed = 0;
int    graphics_info_t::shift_is_pressed = 0 ;       // false
int    graphics_info_t::y_is_pressed = 0 ;       // false
int    graphics_info_t::z_is_pressed = 0 ;       // false
double graphics_info_t::mouse_begin_x = 0.0;
double graphics_info_t::mouse_begin_y = 0.0;
float  graphics_info_t::rotation_centre_x = 0.0;
float  graphics_info_t::rotation_centre_y = 0.0;
float  graphics_info_t::rotation_centre_z = 0.0;
float  graphics_info_t::old_rotation_centre_x = 0.0;
float  graphics_info_t::old_rotation_centre_y = 0.0;
float  graphics_info_t::old_rotation_centre_z = 0.0;
float  graphics_info_t::zoom                = 100;
int    graphics_info_t::smooth_scroll       =   1; // flag: default is ..
int    graphics_info_t::smooth_scroll_steps =  40;
float  graphics_info_t::smooth_scroll_limit =  10.0; // A
float  graphics_info_t::smooth_scroll_zoom_limit = 30.0; // A
int    graphics_info_t::smooth_scroll_do_zoom = 0;  // initially no, too ugly ATM.
short int graphics_info_t::smooth_scroll_on = 0; 
int    graphics_info_t::mouse_just_cliked     = 0;
float  graphics_info_t::rotation_centre_cube_size = 0.1; // Angstroems
short int graphics_info_t::quanta_like_zoom_flag = 0;

// graphics display size:
int graphics_info_t::graphics_x_size = GRAPHICS_WINDOW_X_START_SIZE;
int graphics_info_t::graphics_y_size = GRAPHICS_WINDOW_Y_START_SIZE;
int graphics_info_t::graphics_x_position = 0; // gets overwritten at start
int graphics_info_t::graphics_y_position = 0;
int graphics_info_t::model_fit_refine_dialog_stays_on_top_flag = 1;

short int graphics_info_t::use_graphics_interface_flag = 1;

// display control size and position and the vboxes for the maps and
// the molecules in the display control
int graphics_info_t::display_manager_x_size = -1;
int graphics_info_t::display_manager_y_size = -1;
int graphics_info_t::display_manager_x_position = -1; 
int graphics_info_t::display_manager_y_position = -1;

int graphics_info_t::display_manager_molecules_vbox_x_size = -1;
int graphics_info_t::display_manager_molecules_vbox_y_size = -1; 
int graphics_info_t::display_manager_maps_vbox_x_size = -1;
int graphics_info_t::display_manager_maps_vbox_y_size = -1;

int graphics_info_t::display_manager_paned_position = -1;


int graphics_info_t::go_to_atom_window_x_position = -100;
int graphics_info_t::go_to_atom_window_y_position = -100;

int graphics_info_t::delete_item_widget_x_position = -100;
int graphics_info_t::delete_item_widget_y_position = -100;
   
int graphics_info_t::accept_reject_dialog_x_position = -100;
int graphics_info_t::accept_reject_dialog_y_position = -100;

int graphics_info_t::edit_chi_angles_dialog_x_position = -1;
int graphics_info_t::edit_chi_angles_dialog_y_position = -1;

short int graphics_info_t::draw_chi_angle_flash_bond_flag = 0;
std::pair<clipper::Coord_orth, clipper::Coord_orth> graphics_info_t::flash_bond = 
   std::pair<clipper::Coord_orth, clipper::Coord_orth> (clipper::Coord_orth(0,0,0),
							clipper::Coord_orth(0,0,0));
bool graphics_info_t::flash_intermediate_atom_pick_flag = 0;
clipper::Coord_orth graphics_info_t::intermediate_flash_point;


int graphics_info_t::default_bond_width = 5;

   
int graphics_info_t::rotamer_selection_dialog_x_position = -1;
int graphics_info_t::rotamer_selection_dialog_y_position = -1;

int graphics_info_t::model_fit_refine_x_position = -100;  // initially unset
int graphics_info_t::model_fit_refine_y_position = -100;

// save rotate/translate widget position:
int graphics_info_t::rotate_translate_x_position = -100;  // initially unset
int graphics_info_t::rotate_translate_y_position = -100;

// save Ramachandran widget position:
int graphics_info_t::ramachandran_plot_x_position = -100;  // initially unset
int graphics_info_t::ramachandran_plot_y_position = -100;

// save Distances and Angles dialog position
int graphics_info_t::distances_and_angles_dialog_x_position = -100;  // initially unset
int graphics_info_t::distances_and_angles_dialog_y_position = -100;

coot::Cartesian graphics_info_t::distance_pos_1 = coot::Cartesian(0.0, 0.0, 0.0);
coot::Cartesian graphics_info_t::angle_tor_pos_1 = coot::Cartesian(0.0, 0.0, 0.0);
coot::Cartesian graphics_info_t::angle_tor_pos_2 = coot::Cartesian(0.0, 0.0, 0.0);
coot::Cartesian graphics_info_t::angle_tor_pos_3 = coot::Cartesian(0.0, 0.0, 0.0);
coot::Cartesian graphics_info_t::angle_tor_pos_4 = coot::Cartesian(0.0, 0.0, 0.0);

// Shall we have file name filtering (i.e. before fileselection is
// displayed) on by default?
int graphics_info_t::filter_fileselection_filenames_flag = 0; // no
int graphics_info_t::file_selection_dialog_x_size = -1; // unset
int graphics_info_t::file_selection_dialog_y_size = -1; 


// things for quaternion rotation:
double graphics_info_t::mouse_current_x = 0.0; 
double graphics_info_t::mouse_current_y = 0.0;
float* graphics_info_t::quat = new float[4]; 
float graphics_info_t::trackball_size = 0.8; // for kevin

// things for baton quaternion rotation: Must use a c++ class at some
// stage:
// baton
float* graphics_info_t::baton_quat = new float[4];
float  graphics_info_t::baton_length = 3.8; // A;
int    graphics_info_t::baton_next_ca_options_index = 0;
int    graphics_info_t::user_set_baton_atom_molecule = -1; // initially unset
short int graphics_info_t::baton_tmp_atoms_to_new_molecule = 0;
short int graphics_info_t::draw_baton_flag = 0; // off initially
short int graphics_info_t::baton_mode = 0;      // rotation mode
coot::Cartesian graphics_info_t::baton_root = coot::Cartesian(0.0, 0.0, 0.0);
coot::Cartesian graphics_info_t::baton_tip =  coot::Cartesian(0.0, 0.0, 3.8);
std::vector<coot::scored_skel_coord> *graphics_info_t::baton_next_ca_options = NULL;
std::vector<clipper::Coord_orth> *graphics_info_t::baton_previous_ca_positions = NULL;
short int graphics_info_t::baton_build_direction_flag = 1; // forwards by default
int graphics_info_t::baton_build_start_resno = 1;
short int graphics_info_t::baton_build_params_active = 0; // not active initially.
std::string graphics_info_t::baton_build_chain_id = std::string("");


// double*  graphics_info_t::symm_colour_merge_weight = new double[10];
// double **graphics_info_t::symm_colour = new double*[10];

double graphics_info_t::symmetry_colour_merge_weight = 0.5; // 0.0 -> 1.0

std::vector<double> graphics_info_t::symmetry_colour = std::vector<double> (4, 0.5);

double*  graphics_info_t::skeleton_colour = new double[4]; 
int      graphics_info_t::map_for_skeletonize = -1; 
int      graphics_info_t::max_skeleton_search_depth = 10;

short int graphics_info_t::swap_pre_post_refmac_map_colours_flag = 0;

float     graphics_info_t::symmetry_search_radius = 13.0;
int       graphics_info_t::symmetry_shift_search_size = 1; // which_boxes search hack

// short int graphics_info_t::symmetry_as_calphas = 0; // moved to per molecule basis
// short int graphics_info_t::symmetry_rotate_colour_map_flag = 0; // moved to per molecule basis
float     graphics_info_t::symmetry_operator_rotate_colour_map = 37; //degrees
// int       graphics_info_t::symmetry_colour_by_symop_flag = 1; // moved to per molecule basis
// int       graphics_info_t::symmetry_whole_chain_flag = 0;  // moved to per molecule basis

// esoteric depth cue?
int       graphics_info_t::esoteric_depth_cue_flag = 1; // on by default.

// save coords fileselection dir
int graphics_info_t::save_coordinates_in_original_dir_flag = 0;

// save CONECT records, by default we dont
int graphics_info_t::write_conect_records_flag = 0;

// by default convert nucleic acid names to match the (currently v2)
// dictionary.
// 
bool graphics_info_t::convert_to_v2_atom_names_flag = 0; // changed 20110505

//
short int graphics_info_t::print_initial_chi_squareds_flag = 0;

short int graphics_info_t::show_symmetry = 0; 

float    graphics_info_t::box_radius = 10;


int      graphics_info_t::debug_atom_picking = 0;

int   graphics_info_t::imol_map_sharpening = -1;
float graphics_info_t::map_sharpening_scale_limit = 30.0;

//
int graphics_info_t::imol_remarks_browswer = -1;

// dragged moving atom:
int       graphics_info_t::moving_atoms_dragged_atom_index = -1;
short int graphics_info_t::in_moving_atoms_drag_atom_mode_flag = 0;

// validate moving atoms
int       graphics_info_t::moving_atoms_n_cis_peptides = -1;  // unset


std::string graphics_info_t::model_fit_refine_place_atom_at_pointer_string = "";
std::string graphics_info_t::model_fit_refine_rotate_translate_zone_string = "";

// Change of plan, so that we are more compatible with Stuart.
//
// We shall make maps and skeletons be objects of molecules.
// Hmmm... skeletons are objects of maps.
// 
// And molecules are instances of a molecule_class_info_t
//
// We need to store how many molecules we have and where to find them.

int graphics_info_t::n_molecules_max = 60; 
// int graphics_info_t::n_molecules = 0; // gets incremented on pdb reading

// generic display objects, gets set in init.
std::vector<coot::generic_display_object_t> *graphics_info_t::generic_objects_p = NULL;
bool graphics_info_t::display_generic_objects_as_solid_flag = 0;
bool graphics_info_t::display_environment_graphics_object_as_solid_flag = 0;


coot::console_display_commands_t graphics_info_t::console_display_commands;

// 20 molecules is enough for anyone, surely?
// FIXME - we should be using dynamic allocation, and allocating
// an new array of molecule_class_info_ts and moving over the pointers.
// 
// Fixed ( see graphics_info_t::initialize_graphics_molecules(); ) 
//
// molecule_class_info_t* graphics_info_t::molecules = NULL; yesterday's array

// The molecule for undoing
int graphics_info_t::undo_molecule = -1; 

// backup filenames
short int graphics_info_t::unpathed_backup_file_names_flag = 0;

// backup compress files (default: compress)
int graphics_info_t::backup_compress_files_flag = 1;

// Auto read
int graphics_info_t::auto_read_do_difference_map_too_flag = 1;


// Tip of the Day?
short int graphics_info_t::do_tip_of_the_day_flag = 1;

// Browser URL
// Also gets get in group? scheme code
std::string graphics_info_t::browser_open_command = "firefox -remote";

// should be: each map, each contour level, each colour
// (triple star)
// 

GtkWidget *graphics_info_t::glarea = NULL;
GtkWidget *graphics_info_t::glarea_2 = NULL;
GtkWidget *graphics_info_t::statusbar = NULL;
guint      graphics_info_t::statusbar_context_id = 0;

float graphics_info_t::clipping_front = 0.0;
float graphics_info_t::clipping_back  = 0.0;

//
int       graphics_info_t::atom_label_font_size = 2; // medium
void     *graphics_info_t::atom_label_font = GLUT_BITMAP_HELVETICA_12;
int       graphics_info_t::label_atom_on_recentre_flag = 1;
int       graphics_info_t::symmetry_atom_labels_expanded_flag = 0;
coot::colour_holder graphics_info_t::font_colour = coot::colour_holder(1.0, 0.8, 0.8);

short int graphics_info_t::brief_atom_labels_flag = 0;

// scroll wheel
int       graphics_info_t::scroll_wheel_map = -1; // (initial magic value) 
                                                  // updated on new read-map.

//
GLuint theMapContours = 0;

float graphics_info_t::iso_level_increment = 0.05; 
float graphics_info_t::diff_map_iso_level_increment =  0.005;
short int graphics_info_t::swap_difference_map_colours = 0; // default: not in Jan-Dohnalek-mode.

// No idle functions to start (but setting them to zero doesn't set that - the
// idle functions are added by gtk_idle_add()).
int   graphics_info_t::idle_function_spin_rock_token = 0;
long  graphics_info_t::time_holder_for_rocking = 0;
double graphics_info_t::idle_function_rock_amplitude_scale_factor = 1.0;
double graphics_info_t::idle_function_rock_freq_scale_factor = 1.0;
double graphics_info_t::idle_function_rock_angle_previous = 0;


// new style (20110505 ligand interactions)
// 
long  graphics_info_t::time_holder_for_ligand_interactions = 0;
int   graphics_info_t::idle_function_ligand_interactions_token = 0;
double graphics_info_t::ligand_interaction_pulse_previous = 0;


int   graphics_info_t::drag_refine_idle_function_token = -1; // magic unused value
coot::refinement_results_t graphics_info_t::saved_dragged_refinement_results(0, -2, "");
float graphics_info_t::idle_function_rotate_angle = 1.0; // degrees
bool  graphics_info_t::refinement_move_atoms_with_zero_occupancy_flag = 1; // yes

float graphics_info_t::map_sampling_rate = 1.5;

// Initialise the static atom_sel.
//
//We use this to store the atom selections of the molecules (not that
//an atom_selection_container_t contains the atom selection and the
//molecular manager (or at least, pointers to them).
// 
// Only one atom_selection_container_t at the moment.  Later perhaps
// we will make the atom_selection_container_t* for multiple
// molecules.
//
//
//atom_selection_container_t aaa;
//atom_selection_container_t molecule_class_info_t::atom_sel = aaa;

//

short int graphics_info_t::show_aniso_atoms_flag = 0; // initially don't show.
short int graphics_info_t::show_aniso_atoms_radius_flag = 0;
float     graphics_info_t::show_aniso_atoms_radius = 12.0;
float     graphics_info_t::show_aniso_atoms_probability = 50.0;

// initialise the molecule (scene) rotation axis statics.
//
float molecule_rot_t::x_axis_angle = 0.0;
float molecule_rot_t::y_axis_angle = 0.0;

// 0: never run it
// 1: ask to run it
// 2: alwasy run it
short int graphics_info_t::run_state_file_status = 1;

GtkWidget *graphics_info_t::preferences_widget = NULL;
int        graphics_info_t::mark_cis_peptides_as_bad_flag = 1;

std::vector<std::string> *graphics_info_t::preferences_general_tabs;
std::vector<std::string> *graphics_info_t::preferences_bond_tabs;
std::vector<std::string> *graphics_info_t::preferences_geometry_tabs;
std::vector<std::string> *graphics_info_t::preferences_colour_tabs;
std::vector<std::string> *graphics_info_t::preferences_map_tabs;
std::vector<std::string> *graphics_info_t::preferences_other_tabs;
std::vector<coot::preferences_icon_info_t> *graphics_info_t::model_toolbar_icons;
std::vector<coot::preferences_icon_info_t> *graphics_info_t::main_toolbar_icons;

std::vector<coot::preference_info_t> graphics_info_t::preferences_internal;
std::vector<coot::preference_info_t> graphics_info_t::preferences_internal_default;

// Torsions with hydrogens?
bool graphics_info_t::find_hydrogen_torsions_flag = 0; // no

// Which ccp4i project shall we put at the top of a fileselection optionmenu?
// 
int graphics_info_t::ccp4_projects_index_last = 0;

// phs reading
std::string graphics_info_t::phs_filename = "";

// Pointer Distances
float graphics_info_t::pointer_min_dist = 0.1;
float graphics_info_t::pointer_max_dist = 3.6;
int graphics_info_t::show_pointer_distances_flag = 0;


// Go to Atom widget:
// 
std::string graphics_info_t::go_to_atom_chain_     =  "A";
std::string graphics_info_t::go_to_atom_atom_name_ = "CA";
std::string graphics_info_t::go_to_atom_atom_altLoc_ = "";
std::string graphics_info_t::go_to_atom_inscode_ = "";
int         graphics_info_t::go_to_atom_residue_   = -9999; // magic
							    // number. unset
							    // initially.
int         graphics_info_t::go_to_atom_molecule_  = 0;
int         graphics_info_t::go_to_ligand_n_atoms_limit = 6;

int         graphics_info_t::go_to_atom_mol_menu_active_position = -1; // unset
                                                                       // initially.
GtkWidget  *graphics_info_t::go_to_atom_window = NULL;
int         graphics_info_t::go_to_atom_menu_label_n_chars_max = 30;

GtkWidget *graphics_info_t::model_fit_refine_dialog = NULL;
short int  graphics_info_t::model_fit_refine_dialog_was_sucked = 0;
GtkWidget *graphics_info_t::residue_info_dialog = NULL;
GtkWidget *graphics_info_t::rotamer_dialog = NULL;
GtkWidget *graphics_info_t::difference_map_peaks_dialog = NULL;
GtkWidget *graphics_info_t::checked_waters_baddies_dialog = NULL;

int graphics_info_t::in_base_paring_define = 0;

GtkWidget *graphics_info_t::other_modelling_tools_dialog = 0;

coot::residue_spec_t graphics_info_t::current_residue = coot::residue_spec_t();

std::string graphics_info_t::go_to_residue_keyboarding_string = "";
bool graphics_info_t::in_go_to_residue_keyboarding_mode = 0;


// Skeleton Widgets:
float graphics_info_t::skeleton_level = 0.2; 
float graphics_info_t::skeleton_box_radius = 40.0; 

// Autobuild control
short int graphics_info_t::autobuild_flag = 0; 

// Fileselection sorting:
short int graphics_info_t::sticky_sort_by_date = 0; // initally not.

// Maps and Molecule Display Control window:
GtkWidget *graphics_info_t::display_control_window_ = NULL;

int graphics_info_t::draw_axes_flag = 1; // on by default now.

// 
short int graphics_info_t::draw_crosshairs_flag = 0;

// For defining a range (to, say, regularize)
// 
int       graphics_info_t::refine_regularize_max_residues = 20;
int       graphics_info_t::refine_auto_range_step = 1; // +/- 1 about the clicked residue
short int graphics_info_t::in_range_define = 0; // regularization
short int graphics_info_t::in_range_define_for_refine = 0;// refine (i.e. with map)
short int graphics_info_t::in_distance_define = 0;
short int graphics_info_t::in_angle_define = 0;
short int graphics_info_t::in_torsion_define = 0;
short int graphics_info_t::fix_chiral_volume_before_refinement_flag = 1;
int       graphics_info_t::chiral_volume_molecule_option_menu_item_select_molecule = 0; // option menu
int       graphics_info_t::add_reps_molecule_option_menu_item_select_molecule = 0; // option menu
short int graphics_info_t::refinement_immediate_replacement_flag = 0;
short int graphics_info_t::show_chiral_volume_errors_dialog_flag = 1; // on by default


int graphics_info_t::residue_range_mol_no  = 0;
int graphics_info_t::residue_range_atom_index_1 = 0;
int graphics_info_t::residue_range_atom_index_2 = 0;
int graphics_info_t::geometry_atom_index_1 = 0;
int graphics_info_t::geometry_atom_index_2 = 0;
int graphics_info_t::geometry_atom_index_3 = 0;
int graphics_info_t::geometry_atom_index_4 = 0;
int graphics_info_t::geometry_atom_index_1_mol_no = -1; // must be set before use.
int graphics_info_t::geometry_atom_index_2_mol_no = -1;
int graphics_info_t::geometry_atom_index_3_mol_no = -1;
int graphics_info_t::geometry_atom_index_4_mol_no = -1;

// torsion general
int graphics_info_t::torsion_general_atom_index_1 = -1;
int graphics_info_t::torsion_general_atom_index_2 = -1;
int graphics_info_t::torsion_general_atom_index_3 = -1;
int graphics_info_t::torsion_general_atom_index_4 = -1;
int graphics_info_t::torsion_general_atom_index_1_mol_no = -1;
int graphics_info_t::torsion_general_atom_index_2_mol_no = -1;
int graphics_info_t::torsion_general_atom_index_3_mol_no = -1;
int graphics_info_t::torsion_general_atom_index_4_mol_no = -1;
short int graphics_info_t::in_edit_torsion_general_flag = 0;
std::vector<coot::atom_spec_t> graphics_info_t::torsion_general_atom_specs;
bool graphics_info_t::torsion_general_reverse_flag = 0;
Tree graphics_info_t::torsion_general_tree; 
std::vector<std::vector<int> > graphics_info_t::torsion_general_contact_indices;



//
short int graphics_info_t::in_residue_info_define = 0;
int graphics_info_t::residue_selection_flash_frames_number = 3;
short int graphics_info_t::in_torsion_general_define = 0;

short int graphics_info_t::in_save_symmetry_define = 0; 

short int graphics_info_t::in_rot_trans_object_define = 0;
short int graphics_info_t::rot_trans_object_type = ROT_TRANS_TYPE_ZONE;
short int graphics_info_t::rot_trans_zone_rotates_about_zone_centre = 0;
int graphics_info_t::rot_trans_atom_index_1 = -1;
int graphics_info_t::rot_trans_atom_index_2 = -1;
int graphics_info_t::imol_rot_trans_object = -1; 
// int graphics_info_t::rot_trans_atom_index_rotation_origin_atom = -1; // old
CAtom *graphics_info_t::rot_trans_rotation_origin_atom = NULL;

float *graphics_info_t::previous_rot_trans_adjustment = new float[6];

short int graphics_info_t::in_db_main_define = 0;
int graphics_info_t::db_main_imol = -1;
int graphics_info_t::db_main_atom_index_1 = -1;
int graphics_info_t::db_main_atom_index_2 = -1; 
coot::db_main graphics_info_t::main_chain;

coot::fixed_atom_pick_state_t graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_NO_PICK;
GtkWidget *graphics_info_t::fixed_atom_dialog = 0; 

std::vector<coot::simple_distance_object_t> *graphics_info_t::distance_object_vec = NULL;
std::vector<std::pair<clipper::Coord_orth, clipper::Coord_orth> > *graphics_info_t::pointer_distances_object_vec = NULL;
std::vector<coot::coord_orth_triple> *graphics_info_t::angle_object_vec = NULL;

int graphics_info_t::show_origin_marker_flag = 1;

//
float graphics_info_t::geometry_vs_map_weight = 60.0;
float graphics_info_t::rama_plot_restraint_weight = 1.0;
int   graphics_info_t::rama_n_diffs = 50;

atom_selection_container_t *graphics_info_t::moving_atoms_asc = NULL;
short int graphics_info_t::moving_atoms_asc_type = coot::NEW_COORDS_UNSET; // unset
int graphics_info_t::imol_moving_atoms = 0;
int graphics_info_t::imol_refinement_map = -1; // magic initial value
                                 // checked in graphics_info_t::refine()

graphical_bonds_container graphics_info_t::regularize_object_bonds_box;
graphical_bonds_container graphics_info_t::environment_object_bonds_box;
graphical_bonds_container graphics_info_t::symmetry_environment_object_bonds_box;
int graphics_info_t::mol_no_for_environment_distances = -1;

int   graphics_info_t::bond_parameters_molecule = -1; // unset


short int graphics_info_t::do_torsion_restraints = 0;
short int graphics_info_t::do_peptide_omega_torsion_restraints = 0;
bool      graphics_info_t::do_rama_restraints = 0; // No.
bool      graphics_info_t::do_numerical_gradients = 0; // No.
// for Kevin Keating 
bool      graphics_info_t::use_only_extra_torsion_restraints_for_torsions_flag = 0; 


// 
short int graphics_info_t::guile_gui_loaded_flag = FALSE;
short int graphics_info_t::python_gui_loaded_flag = FALSE;

//
int graphics_info_t::find_ligand_map_mol_ = -1;
int graphics_info_t::find_ligand_protein_mol_ = -1;
bool graphics_info_t::find_ligand_here_cluster_flag = 0;
int graphics_info_t::find_ligand_n_top_ligands = 10;
short int graphics_info_t::find_ligand_mask_waters_flag = 0;
float graphics_info_t::map_mask_atom_radius = -99; // unset
// std::vector<int> *mol_tmp;
std::vector<std::pair<int, bool> > *graphics_info_t::find_ligand_ligand_mols_;
float graphics_info_t::find_waters_sigma_cut_off = 1.8;
float graphics_info_t::ligand_acceptable_fit_fraction = 0.75;
float graphics_info_t::ligand_cluster_sigma_level = 1.0; // sigma
int   graphics_info_t::ligand_wiggly_ligand_n_samples = 50;
int   graphics_info_t::ligand_verbose_reporting_flag = 0; 
// std::vector<short int> *graphics_info_t::find_ligand_wiggly_ligands_; bye!
short int graphics_info_t::ligand_expert_flag = 0;

float graphics_info_t::ligand_water_to_protein_distance_lim_max = 3.2;
float graphics_info_t::ligand_water_to_protein_distance_lim_min = 2.4;
float graphics_info_t::ligand_water_variance_limit = 0.12;
int   graphics_info_t::ligand_water_n_cycles = 3;
int   graphics_info_t::find_ligand_ligand_atom_limit = 400;
// EJD wants it, but off by default:
short int graphics_info_t::ligand_water_write_peaksearched_atoms = 0; 
std::vector<clipper::Coord_orth> *graphics_info_t::ligand_big_blobs = NULL;


short int graphics_info_t::do_probe_dots_on_rotamers_and_chis_flag = 0;
short int graphics_info_t::do_probe_dots_post_refine_flag = 0;
coot::Cartesian graphics_info_t::probe_dots_on_chis_molprobity_centre = coot::Cartesian(0.0, 0.0, 0.0);
float graphics_info_t::probe_dots_on_chis_molprobity_radius = 6.0;

float* graphics_info_t::background_colour = new float[4];

//
short int graphics_info_t::delete_item_atom = 0;
short int graphics_info_t::delete_item_residue = 1;
short int graphics_info_t::delete_item_residue_zone = 0;
short int graphics_info_t::delete_item_residue_hydrogens = 0;
short int graphics_info_t::delete_item_water = 0;
short int graphics_info_t::delete_item_sidechain = 0;
GtkWidget *graphics_info_t::delete_item_widget = NULL;
int       graphics_info_t::keep_delete_item_active_flag = 0;
coot::residue_spec_t graphics_info_t::delete_item_residue_zone_1;
int graphics_info_t::delete_item_residue_zone_1_imol = -1;


GtkWidget *graphics_info_t::symmetry_controller_dialog = 0;

short int graphics_info_t::do_scroll_by_wheel_mouse_flag = 1;


// dummy atom typed (dialog) or dummy (forced/auto)
short int graphics_info_t::pointer_atom_is_dummy = 0;
int       graphics_info_t::user_pointer_atom_molecule = -1; // unset.

// read a pdb, shall we recentre?
short int graphics_info_t::recentre_on_read_pdb = 1;

// Buttons - now are configurable
//
GdkModifierType graphics_info_t::button_1_mask_ = GDK_BUTTON1_MASK;
GdkModifierType graphics_info_t::button_2_mask_ = GDK_BUTTON2_MASK;
GdkModifierType graphics_info_t::button_3_mask_ = GDK_BUTTON3_MASK;

// shall we show the density leven on screen?
short int graphics_info_t::display_density_level_on_screen = 1;
short int graphics_info_t::display_density_level_this_image = 1;
std::string graphics_info_t::display_density_level_screen_string =
   "Welcome to Coot";

// dynarama
//
// This kills the compiler:  Move the allocation to init.
// GtkWidget **graphics_info_t::dynarama_is_displayed = new GtkWidget *[graphics_info_t::n_molecules_max];
float       graphics_info_t::residue_density_fit_scale_factor = 1.0;

// cif dictionary
std::vector<std::string> *graphics_info_t::cif_dictionary_filename_vec = NULL;
int graphics_info_t::cif_dictionary_read_number = 1; 

// map radius slider
float graphics_info_t::map_radius_slider_max = 50.0;

// Rotate colour map
short int graphics_info_t::rotate_colour_map_on_read_pdb_flag = 1; // do it.
short int graphics_info_t::rotate_colour_map_on_read_pdb_c_only_flag = 1; // rotate Cs only by default
float     graphics_info_t::rotate_colour_map_on_read_pdb = 21.0;  // degrees
float     graphics_info_t::rotate_colour_map_for_map = 14.0;  // degrees

// cell colour
coot::colour_holder graphics_info_t::cell_colour =
   coot::colour_holder(0.8, 0.8, 0.2);


// regulariziation
//
coot::protein_geometry *graphics_info_t::geom_p = NULL;


// rotamer probabilities
int graphics_info_t::rotamer_search_mode = ROTAMERSEARCHAUTOMATIC;
coot::rotamer_probability_tables graphics_info_t::rot_prob_tables;
float graphics_info_t::rotamer_distortion_scale = 0.3;

// PHENIX support
std::string graphics_info_t::external_refinement_program_button_label = "*-*";


// pepflip
int graphics_info_t::atom_index_pepflip = 0;
int graphics_info_t::imol_pepflip = 0;
int graphics_info_t::iresno_pepflip = 0;
short int graphics_info_t::in_pepflip_define = 0; 

// rigid body refinement
short int graphics_info_t::in_rigid_body_define = 0;
int       graphics_info_t::imol_rigid_body_refine = 0;

// terminal residue define
short int graphics_info_t::in_terminal_residue_define = 0;
short int graphics_info_t::add_terminal_residue_immediate_addition_flag = 0;
short int graphics_info_t::add_terminal_residue_do_post_refine = 0;
float graphics_info_t::terminal_residue_addition_direct_phi = -135.0;
float graphics_info_t::terminal_residue_addition_direct_psi =  135.0;


// CIS <-> TRANS conversion 
int graphics_info_t::in_cis_trans_convert_define = 0;

// add OXT atom
int         graphics_info_t::add_OXT_molecule = -1;
std::string graphics_info_t::add_OXT_chain;

float graphics_info_t::default_new_atoms_b_factor = 30.0;

int graphics_info_t::reset_b_factor_moved_atoms_flag = 0;

// show environment
float graphics_info_t::environment_min_distance = 0.0;
float graphics_info_t::environment_max_distance = 3.2;
bool graphics_info_t::environment_show_distances = 0;
bool graphics_info_t::environment_distances_show_bumps = 1; 
bool graphics_info_t::environment_distances_show_h_bonds = 1;

short int graphics_info_t::environment_distance_label_atom = 0;

// dynamic distances to intermediate atoms:
short int graphics_info_t::in_dynamic_distance_define = 0;
coot::intermediate_atom_distance_t graphics_info_t::running_dynamic_distance;
std::vector<coot::intermediate_atom_distance_t> graphics_info_t::dynamic_distances;

// 
bool graphics_info_t::disable_state_script_writing = 0;

// Dynamic map resampling and sizing
short int graphics_info_t::dynamic_map_resampling = 0;
short int graphics_info_t::dynamic_map_size_display = 0;
int       graphics_info_t::graphics_sample_step = 1;
int       graphics_info_t::dynamic_map_zoom_offset = 0; 

// history
short int graphics_info_t::python_history = 1; // on
short int graphics_info_t::guile_history  = 1; // on
coot::history_list_t graphics_info_t::history_list;

// build one residue, n trials:
int graphics_info_t::add_terminal_residue_n_phi_psi_trials = 100;
int graphics_info_t::add_terminal_residue_add_other_residue_flag = 0; // no.
std::string graphics_info_t::add_terminal_residue_type = "auto"; // was "ALA" before 20080601
short int graphics_info_t::terminal_residue_do_rigid_body_refine = 1; // on by default
float graphics_info_t::rigid_body_fit_acceptable_fit_fraction = 0.75;

// rotamer
#ifdef USE_DUNBRACK_ROTAMERS
float graphics_info_t::rotamer_lowest_probability = 2.0; // percent
#else 
float graphics_info_t::rotamer_lowest_probability = 0.0; // compatibility.  Limit
                                                         // not used, practically.
#endif 
short int graphics_info_t::in_rotamer_define = 0;
int graphics_info_t::rotamer_residue_atom_index = -1; // unset initially.
int graphics_info_t::rotamer_residue_imol = -1;       // unset initially.
int graphics_info_t::rotamer_fit_clash_flag = 1;      //  check clashes initially.
short int graphics_info_t::in_auto_fit_define = 0;    // not in auto fit initially.

// mutation
short int graphics_info_t::in_mutate_define = 0; // not, initially.
int graphics_info_t::mutate_residue_atom_index = -1;
int graphics_info_t::mutate_residue_imol = -1;
atom_selection_container_t asc;
atom_selection_container_t graphics_info_t::standard_residues_asc = asc;
std::string graphics_info_t::mutate_sequence_chain_from_optionmenu;
int         graphics_info_t::mutate_sequence_imol;
int         graphics_info_t::align_and_mutate_imol;
std::string graphics_info_t::align_and_mutate_chain_from_optionmenu;

// Bob recommends:
realtype    graphics_info_t::alignment_wgap   = -0.5; // was -3.0;
realtype    graphics_info_t::alignment_wspace = -0.4;

//
short int graphics_info_t::in_reverse_direction_define = 0;

// user defined
short int graphics_info_t::in_user_defined_define = 0;
#ifdef USE_GUILE
SCM graphics_info_t::user_defined_click_scm_func = SCM_BOOL_F;
#endif // GUILE
#ifdef USE_PYTHON
PyObject *graphics_info_t::user_defined_click_py_func = NULL;
#endif // PYTHON
std::vector<coot::atom_spec_t> graphics_info_t::user_defined_atom_pick_specs;


// Miguel's axis orientation matrix ---------------
GL_matrix graphics_info_t::axes_orientation = GL_matrix();
short int graphics_info_t::use_axes_orientation_matrix_flag = 0;

// ---- NCS -----
float graphics_info_t::ncs_min_hit_ratio = 0.9;
short int graphics_info_t::ncs_maps_do_average_flag = 1;
short int graphics_info_t::ncs_matrix_flag = coot::NCS_LSQ;
// mutate auto fit
short int graphics_info_t::in_mutate_auto_fit_define = 0; 
int graphics_info_t::mutate_auto_fit_residue_atom_index = -1;
int graphics_info_t::mutate_auto_fit_residue_imol = -1;
short int graphics_info_t::residue_type_chooser_auto_fit_flag = 0;
short int graphics_info_t::residue_type_chooser_stub_flag = 0;
short int graphics_info_t::mutate_auto_fit_do_post_refine_flag = 0; 
short int graphics_info_t::rotamer_auto_fit_do_post_refine_flag = 0; 

short int graphics_info_t::in_add_alt_conf_define = 0;
GtkWidget *graphics_info_t::add_alt_conf_dialog = NULL; 
short int graphics_info_t::alt_conf_split_type = 1; // usually it is after Ca?
int graphics_info_t::add_alt_conf_atom_index = -1;
int graphics_info_t::add_alt_conf_imol = -1;
float graphics_info_t::add_alt_conf_new_atoms_occupancy = 0.5;
short int graphics_info_t::show_alt_conf_intermediate_atoms_flag = 0;
float graphics_info_t::ncs_homology_level = 0.7;

// edit phi/psi
short int graphics_info_t::in_edit_phi_psi_define = 0;
int graphics_info_t::edit_phi_psi_atom_index = -1;
int graphics_info_t::edit_phi_psi_imol = -1;
short int graphics_info_t::in_backbone_torsion_define = 0;
#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)
coot::rama_plot  *graphics_info_t::edit_phi_psi_plot = NULL;
#endif // HAVE_GTK_CANVAS
float graphics_info_t::rama_level_prefered = 0.02;
float graphics_info_t::rama_level_allowed = 0.002;
float graphics_info_t::rama_plot_background_block_size = 10; // divisible into 360 preferably.
coot::ramachandran_points_container_t graphics_info_t::rama_points = coot::ramachandran_points_container_t();
// edit chi
short int graphics_info_t::in_edit_chi_angles_define = 0;
int       graphics_info_t::edit_chi_current_chi = 0; // real values start at 1.
short int graphics_info_t::in_edit_chi_mode_flag = 0;
short int graphics_info_t::in_edit_chi_mode_view_rotate_mode = 0;
bool      graphics_info_t::edit_chi_angles_reverse_fragment = 0;
short int graphics_info_t::moving_atoms_move_chis_flag = 0;
coot::atom_spec_t graphics_info_t::chi_angles_clicked_atom_spec;
std::string graphics_info_t::chi_angle_alt_conf = "";

// 180 degree flip
short int graphics_info_t::in_180_degree_flip_define = 0;

int graphics_info_t::ramachandran_plot_differences_imol1 = -1;
int graphics_info_t::ramachandran_plot_differences_imol2 = -1;
std::string graphics_info_t::ramachandran_plot_differences_imol1_chain = "";
std::string graphics_info_t::ramachandran_plot_differences_imol2_chain = "";


// state language variable:
short int graphics_info_t::state_language = 1; // scheme

// directory saving:
std::string graphics_info_t::directory_for_fileselection = ""; 
std::string graphics_info_t::directory_for_saving_for_fileselection = ""; 
#if (GTK_MAJOR_VERSION > 1)
std::string graphics_info_t::directory_for_filechooser = "";
std::string graphics_info_t::directory_for_saving_for_filechooser = "";
#endif // GTK_MAJOR_VERSION


// Residue info
short int graphics_info_t::residue_info_pending_edit_b_factor = 0;
short int graphics_info_t::residue_info_pending_edit_occ = 0;
int       graphics_info_t::residue_info_n_atoms = -1; 
std::vector<coot::select_atom_info> *graphics_info_t::residue_info_edits = NULL; 

//  Backbone torsions:
clipper::Coord_orth graphics_info_t::backbone_torsion_end_ca_1;
clipper::Coord_orth graphics_info_t::backbone_torsion_end_ca_2;
int graphics_info_t::backbone_torsion_peptide_button_start_pos_x;
int graphics_info_t::backbone_torsion_peptide_button_start_pos_y;
int graphics_info_t::backbone_torsion_carbonyl_button_start_pos_x;
int graphics_info_t::backbone_torsion_carbonyl_button_start_pos_y;

// show citation notice?
// short int graphics_info_t::show_citation_notice = 1; // on by default :)
short int graphics_info_t::show_citation_notice = 0; // on by default :)

// we have dragged shear fixed points?
short int graphics_info_t::have_fixed_points_sheared_drag_flag = 0;
int       graphics_info_t::dragged_refinement_steps_per_frame = 80;
short int graphics_info_t::dragged_refinement_refine_per_frame_flag = 0;

// save the restraints:
//
#ifdef HAVE_GSL
coot::restraints_container_t graphics_info_t::last_restraints;
#endif // HAVE_GSL
// 
// 
short int graphics_info_t::draw_zero_occ_spots_flag = 1; // on by default

int   graphics_info_t::check_waters_molecule = -1; // unset initially.
float graphics_info_t::check_waters_b_factor_limit  = 80.0;
float graphics_info_t::check_waters_map_sigma_limit = 1.0;
float graphics_info_t::check_waters_min_dist_limit = 2.3;
float graphics_info_t::check_waters_max_dist_limit = 3.5;
float graphics_info_t::check_waters_by_difference_map_sigma_level = 3.5;
int   graphics_info_t::check_waters_by_difference_map_map_number = -1; 

// default map sigma level:
float graphics_info_t::default_sigma_level_for_map = 1.5;
float graphics_info_t::default_sigma_level_for_fofc_map = 3.0;


// geometry widget:
GtkWidget *graphics_info_t::geometry_dialog = NULL;

// run refmac widget:
std::string graphics_info_t::refmac_ccp4i_project_dir = std::string("");
std::string graphics_info_t::libcheck_ccp4i_project_dir = std::string("");

std::vector<int> *graphics_info_t::preset_number_refmac_cycles;
coot::refmac::refmac_refinement_method_type graphics_info_t::refmac_refinement_method = coot::refmac::RESTRAINED;
coot::refmac::refmac_phase_input_type graphics_info_t::refmac_phase_input   = coot::refmac::NO_PHASES;
coot::refmac::refmac_use_tls_type     graphics_info_t::refmac_use_tls_flag  = coot::refmac::TLS_ON;
coot::refmac::refmac_use_twin_type    graphics_info_t::refmac_use_twin_flag = coot::refmac::TWIN_OFF;
coot::refmac::refmac_use_sad_type     graphics_info_t::refmac_use_sad_flag  = coot::refmac::SAD_OFF;
coot::refmac::refmac_use_ncs_type     graphics_info_t::refmac_use_ncs_flag  = coot::refmac::NCS_ON;
coot::refmac::refmac_use_intensities_type graphics_info_t::refmac_use_intensities_flag  = coot::refmac::AMPLITUDES;
coot::refmac::refmac_used_mtz_file_type graphics_info_t::refmac_used_mtz_file_flag = coot::refmac::MTZ;
const gchar *graphics_info_t::saved_refmac_file_filename = NULL;
int graphics_info_t::refmac_ncycles = 5;
GtkWidget *graphics_info_t::refmac_dialog_mtz_file_label = NULL;
std::vector<coot::refmac::sad_atom_info_t> graphics_info_t::refmac_sad_atoms;
short int graphics_info_t::have_sensible_refmac_params = 0;
std::string graphics_info_t::refmac_mtz_file_filename = "";
std::string graphics_info_t::refmac_fobs_col = "";
std::string graphics_info_t::refmac_sigfobs_col = "";
std::string graphics_info_t::refmac_r_free_col = "";
int graphics_info_t::refmac_r_free_flag_sensible = 0;

// scrollin' scrollin' scrollin'... Shall we stop? When shall we stop?
short int graphics_info_t::stop_scroll_diff_map_flag = 1; // stop on
short int graphics_info_t::stop_scroll_iso_map_flag  = 1; // ditto.
float     graphics_info_t::stop_scroll_diff_map_level = 0.0; 
float     graphics_info_t::stop_scroll_iso_map_level = 0.0; 

// globing
// 
std::vector<std::string> *graphics_info_t::coordinates_glob_extensions;
std::vector<std::string> *graphics_info_t::data_glob_extensions;
std::vector<std::string> *graphics_info_t::map_glob_extensions;
std::vector<std::string> *graphics_info_t::dictionary_glob_extensions;

// superposition
int graphics_info_t::superpose_imol1 = -1;
int graphics_info_t::superpose_imol2 = -1;
std::string graphics_info_t::superpose_imol1_chain = "";
std::string graphics_info_t::superpose_imol2_chain = "";

// unbonded star size [doesn't work yet]
float graphics_info_t::unbonded_atom_star_size = 0.5;

// Raster3D
float graphics_info_t::raster3d_bond_thickness    = 0.18;
float graphics_info_t::raster3d_atom_radius    = 0.25;
float graphics_info_t::raster3d_density_thickness = 0.015;
bool  graphics_info_t::raster3d_enable_shadows = 1;
int   graphics_info_t::renderer_show_atoms_flag = 1;
float graphics_info_t::raster3d_bone_thickness    = 0.05;
int   graphics_info_t::raster3d_water_sphere_flag = 0;

// map (density) line thickness:
int graphics_info_t::map_line_width = 1;

// bonding stuff
int   graphics_info_t::bond_thickness_intermediate_value = -1;
float graphics_info_t::bond_thickness_intermediate_atoms = 5; // thick white atom bonds

// merge molecules
int graphics_info_t::merge_molecules_master_molecule = -1;
std::vector<int> *graphics_info_t::merge_molecules_merging_molecules;

// change chain ids:
int graphics_info_t::change_chain_id_molecule = -1;
std::string graphics_info_t::change_chain_id_from_chain = "";

// renumber residues
int         graphics_info_t::renumber_residue_range_molecule = -1;
std::string graphics_info_t::renumber_residue_range_chain;

// antialiasing:
short int graphics_info_t::do_anti_aliasing_flag = 0;

// lighting
short int graphics_info_t::do_lighting_flag = 0;
bool      graphics_info_t::do_flat_shading_for_solid_density_surface = 1;


// stereo?
int graphics_info_t::display_mode = coot::MONO_MODE;
float graphics_info_t::hardware_stereo_angle_factor = 1.0;

// remote controlled coot
int graphics_info_t::try_port_listener = 0;
int graphics_info_t::remote_control_port_number;
std::string graphics_info_t::remote_control_hostname;
int graphics_info_t::coot_socket_listener_idle_function_token = -1; //  default (off)
// Did we get a good socket when we tried to open it?  If so, set
// something non-zero here (which is done as a scheme command).
int graphics_info_t::listener_socket_have_good_socket_state = 0;
std::string graphics_info_t::socket_string_waiting = "";
volatile bool graphics_info_t::have_socket_string_waiting_flag = 0;
volatile bool graphics_info_t::socket_string_waiting_mutex_lock = 0;


// validation
std::vector<clipper::Coord_orth> *graphics_info_t::diff_map_peaks =
   new std::vector<clipper::Coord_orth>;
int   graphics_info_t::max_diff_map_peaks = 0;
float graphics_info_t::difference_map_peaks_sigma_level = 5.0;
float graphics_info_t::difference_map_peaks_max_closeness = 2.0; // A

// save state file name
#ifdef USE_GUILE
std::string graphics_info_t::save_state_file_name = "0-coot.state.scm";
#else 
std::string graphics_info_t::save_state_file_name = "0-coot.state.py";
#endif

// auto-read mtz columns
std::string graphics_info_t::auto_read_MTZ_FWT_col = "FWT";
std::string graphics_info_t::auto_read_MTZ_PHWT_col = "PHWT";
std::string graphics_info_t::auto_read_MTZ_DELFWT_col = "DELFWT";
std::string graphics_info_t::auto_read_MTZ_PHDELWT_col = "PHDELWT";

// fffearing
float graphics_info_t::fffear_angular_resolution = 15.0; // degrees

// move molecule here
int graphics_info_t::move_molecule_here_molecule_number = -1;

#ifdef HAVE_GSL
// pseudo bond for sec str restraints
coot::pseudo_restraint_bond_type graphics_info_t::pseudo_bonds_type = coot::NO_PSEUDO_BONDS;
#endif // HAVE_GSL


// MYSQL database
#ifdef USE_MYSQL_DATABASE
MYSQL *graphics_info_t::mysql = 0;
int    graphics_info_t::query_number = 1;
std::string graphics_info_t::sessionid = "";
std::pair<std::string, std::string> graphics_info_t::db_userid_username("no-userid","no-user-name");
std::string graphics_info_t::mysql_host   = "localhost";
std::string graphics_info_t::mysql_user   = "cootuser";
std::string graphics_info_t::mysql_passwd = "password";
#endif // USE_MYSQL_DATABASE

//
int graphics_info_t::ncs_next_chain_skip_key = GDK_o;
int graphics_info_t::ncs_prev_chain_skip_key = GDK_O;
int graphics_info_t::update_go_to_atom_from_current_residue_key = GDK_p;

//
GdkCursorType graphics_info_t::pick_cursor_index = GDK_CROSSHAIR;

// update self?
bool graphics_info_t::update_self = 0;  // Set by command line arg --update-self

float graphics_info_t::electrostatic_surface_charge_range = 0.5;


// surface

#if (GTK_MAJOR_VERSION == 1)

GtkWidget*
gl_extras(GtkWidget* vbox1, short int try_stereo_flag) {


 /* Attribute list for gtkglarea widget. Specifies a
     list of Boolean attributes and enum/integer
     attribute/value pairs. The last attribute must be
     GDK_GL_NONE. See glXChooseVisual manpage for further
     explanation.  */

   GtkWidget *glarea = NULL;
   graphics_info_t g;
   gchar *info_str;

   int *attrlist;
   int mono_attrlist[]  = {
      GDK_GL_RGBA,
      GDK_GL_RED_SIZE,   1,
      GDK_GL_GREEN_SIZE, 1,
      GDK_GL_BLUE_SIZE,  1,
      GDK_GL_DEPTH_SIZE, 1,
      GDK_GL_DOUBLEBUFFER,
      //      GDK_GL_MODE_MULTISAMPLE | /* 2x FSAA */ (2 << GDK_GL_MODE_SAMPLE_SHIFT),
      // (1 << 7) | /* 2x FSAA */ (2 << 24),
      GDK_GL_NONE
   };
   attrlist = mono_attrlist;

   if (try_stereo_flag == coot::HARDWARE_STEREO_MODE) { 
     int hardware_attrlist[] = {
	GDK_GL_RGBA,
	GDK_GL_RED_SIZE,   1,
	GDK_GL_GREEN_SIZE, 1,
	GDK_GL_BLUE_SIZE,  1,
	GDK_GL_DEPTH_SIZE, 1,
	GDK_GL_STEREO,
	GDK_GL_DOUBLEBUFFER,
	GDK_GL_NONE
     };
     attrlist = hardware_attrlist;
  }

   if (try_stereo_flag == coot::ZALMAN_STEREO) { 
     int hardware_attrlist[] = {
	GDK_GL_RGBA,
	GDK_GL_RED_SIZE,   1,
	GDK_GL_GREEN_SIZE, 1,
	GDK_GL_BLUE_SIZE,  1,
	GDK_GL_DEPTH_SIZE, 1,
	GDK_GL_STENCIL_SIZE, 1,  // BL test this
	GDK_GL_DOUBLEBUFFER,
	GDK_GL_NONE
     };
     attrlist = hardware_attrlist;
  }


  /* Check if OpenGL is supported. */
  if (gdk_gl_query() == FALSE) {
    g_print("OpenGL not supported\n");
  }

  gtk_container_set_border_width(GTK_CONTAINER(vbox1), 1);

  /* Quit form main if got delete event */
  gtk_signal_connect(GTK_OBJECT(vbox1), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);


  /* You should always delete gtk_gl_area widgets before exit or else
     GLX contexts are left undeleted, this may cause problems (=core dump)
     in some systems.
     Destroy method of objects is not automatically called on exit.
     You need to manually enable this feature. Do gtk_quit_add_destroy()
     for all your top level windows unless you are certain that they get
     destroy signal by other means.
  */
  GtkWidget *window1 = lookup_widget(vbox1, "window1");
  if (window1)
     // Perhaps this should simply be glarea and placed after glarea
     // is inited.  BTW, gtk_exit() is normally used to end coot session.
     gtk_quit_add_destroy(1, GTK_OBJECT(window1));
  else
     // this shouldn't happen
     std::cout << "ERROR:: NULL top_level in gl_extras" << std::endl;

  int n_gl_contexts = 1;
  if ((try_stereo_flag == coot::SIDE_BY_SIDE_STEREO) ||
      (try_stereo_flag == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ||
      (try_stereo_flag == coot::DTI_SIDE_BY_SIDE_STEREO)){
     n_gl_contexts = 2;
  }

  for (int i_gl_contexts=0; i_gl_contexts < n_gl_contexts; i_gl_contexts++) { 
     /* create new OpenGL widget. */
     GtkWidget *glarea_a = 0;

     if (i_gl_contexts == 0)
	glarea_a = gtk_gl_area_new(attrlist);
     if (i_gl_contexts == 1)
	glarea_a = gtk_gl_area_share_new(attrlist, GTK_GL_AREA(glarea));
     

     // This is kludgey.
     if (i_gl_contexts == 0)
	glarea = glarea_a;

     if (glarea_a) {

	if (i_gl_contexts == 1) { 
	   graphics_info_t::glarea_2 = glarea_a;
	   graphics_info_t::display_mode = try_stereo_flag;
	}
	
	if (try_stereo_flag == coot::HARDWARE_STEREO_MODE) {
	   std::cout << "INFO:: Hardware stereo widget opened successfully"
		     << std::endl;
	   graphics_info_t::display_mode = coot::HARDWARE_STEREO_MODE;
	}

	if (try_stereo_flag == coot::ZALMAN_STEREO) {
	   std::cout << "INFO:: Zalman stereo widget opened successfully?!"
		     << std::endl;
	   graphics_info_t::display_mode = coot::ZALMAN_STEREO;
	}

	/* Events for widget must be set before X Window is created */
#if !defined(WINDOWS_MINGW)
	gtk_widget_set_events(GTK_WIDGET(glarea_a),
			      GDK_EXPOSURE_MASK      |
			      GDK_BUTTON_PRESS_MASK  |
			      GDK_BUTTON_RELEASE_MASK|
			      GDK_POINTER_MOTION_MASK|
			      GDK_KEY_PRESS_MASK     |
			      GDK_KEY_RELEASE_MASK   |
			      GDK_POINTER_MOTION_HINT_MASK);
#else 	
     gtk_widget_set_events(GTK_WIDGET(glarea_a),
                           GDK_EXPOSURE_MASK      |
                           GDK_BUTTON_PRESS_MASK  |
                           GDK_BUTTON_RELEASE_MASK|
                           GDK_SCROLL_MASK        |
                           GDK_POINTER_MOTION_MASK|
                           GDK_KEY_PRESS_MASK     |
                           GDK_KEY_RELEASE_MASK   |
			   GDK_POINTER_MOTION_HINT_MASK);
#endif      
 
	/* Connect signal handlers */
	/* Redraw image when exposed. */
	gtk_signal_connect(GTK_OBJECT(glarea_a), "expose_event",
			   GTK_SIGNAL_FUNC(expose), NULL);
	/* When window is resized viewport needs to be resized also. */
	gtk_signal_connect(GTK_OBJECT(glarea_a), "configure_event",
			   GTK_SIGNAL_FUNC(reshape), NULL);
	/* Do initialization when widget has been realized. */
	gtk_signal_connect(GTK_OBJECT(glarea_a), "realize",
			   GTK_SIGNAL_FUNC(init), NULL);

	/* pressed a button? */
	gtk_signal_connect (GTK_OBJECT(glarea_a), "button_press_event",
			    GTK_SIGNAL_FUNC(glarea_button_press), NULL);
	gtk_signal_connect (GTK_OBJECT(glarea_a), "button_release_event",
			    GTK_SIGNAL_FUNC(glarea_button_release), NULL);
	/* mouse in motion! */
	gtk_signal_connect (GTK_OBJECT(glarea_a), "motion_notify_event",
			    GTK_SIGNAL_FUNC(glarea_motion_notify), NULL);

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
	// scroll wheel changed on windows is not a button press event
	// (as is the case elsewhere) it is a scrollevent.  So says FR.
	//
	// This is conditional on Windows GTK2.
	// Proper GTK2 code path is elsewhere.
     
	gtk_signal_connect (GTK_OBJECT(glarea_a), "scroll_event",
			    GTK_SIGNAL_FUNC(glarea_scroll_event), NULL);
#endif

	/* set minimum size */
	gtk_widget_set_usize(GTK_WIDGET(glarea_a), 300, 300);

	// Not today.
	//   std::cout << "setting window size to " << g. graphics_x_size
	// 	    << " " << g. graphics_x_size << std::endl;

	GtkWindow *window1 = GTK_WINDOW(lookup_widget(vbox1,"window1"));
	gtk_window_set_default_size(window1, g.graphics_x_size, g.graphics_y_size);

	/* put glarea into vbox */
	GtkWidget *main_window_graphics_hbox =
	   lookup_widget(vbox1, "main_window_graphics_hbox");
	// gtk_container_add(GTK_CONTAINER(main_window_graphics_hbox), GTK_WIDGET(glarea_a));
	gtk_box_pack_start(GTK_BOX(main_window_graphics_hbox),
			   GTK_WIDGET(glarea_a), TRUE, TRUE, 0);
	
	/* Capture keypress events */
	gtk_signal_connect(GTK_OBJECT(glarea_a), "key_press_event",
			   GTK_SIGNAL_FUNC(key_press_event), NULL);
	gtk_signal_connect(GTK_OBJECT(glarea_a), "key_release_event",
			   GTK_SIGNAL_FUNC(key_release_event), NULL);

	/* set focus to glarea widget - we need this to get key presses. */
	GTK_WIDGET_SET_FLAGS(glarea_a, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(GTK_WIDGET(glarea_a));

	/* vendor dependent version info string */
#if !defined(WINDOWS_MINGW) && !defined(_MSC_VER)
	info_str = gdk_gl_get_info();
	g_print("%s\n", info_str);
	g_free(info_str);
#endif
     } else {
	std::cout << "CATASTROPHIC ERROR:: in gl_extras no GtkGL widget!"
		  << std::endl; 
     }
  }
  return glarea; 
}

#else

// Defaults for the file chooser
int graphics_info_t::gtk2_file_chooser_selector_flag = coot::CHOOSER_STYLE;
int graphics_info_t::gtk2_chooser_overwrite_flag = coot::CHOOSER_OVERWRITE_PROTECT;

// GTK2 code
// 
// if try_stereo_flag is 1, then hardware stereo.
// if try_stereo_flag is 2, the side-by-side stereo
// 
GtkWidget *
gl_extras(GtkWidget* vbox1, short int try_stereo_flag) {

   graphics_info_t g;
   GdkGLConfig *glconfig = 0;
   bool got_hardware_stereo_flag = 0; 
   
   GdkGLConfigMode mode = static_cast<GdkGLConfigMode>
      (GDK_GL_MODE_RGB    |
       GDK_GL_MODE_DEPTH  |
       // GDK_GL_MODE_MULTISAMPLE |
       GDK_GL_MODE_DOUBLE
       );

   if (try_stereo_flag == coot::HARDWARE_STEREO_MODE) {
      mode = static_cast<GdkGLConfigMode>
	 (GDK_GL_MODE_RGB    |
	  GDK_GL_MODE_DEPTH  |
	  GDK_GL_MODE_DOUBLE |
	  GDK_GL_STEREO);
      /* Try double-buffered visual */
      glconfig = gdk_gl_config_new_by_mode(mode);
      if (glconfig) {
	 got_hardware_stereo_flag = 1;// for message later
      } else {
	 std::cout << "WARNING:: Can't enable stereo visual - falling back"
		   << std::endl;
	 mode = static_cast<GdkGLConfigMode>
	    (GDK_GL_MODE_RGB    |
	     GDK_GL_MODE_DEPTH  |
	     GDK_GL_MODE_DOUBLE);
      }
   }


   if (try_stereo_flag == coot::ZALMAN_STEREO) {
     mode = static_cast<GdkGLConfigMode>
       (GDK_GL_MODE_RGB    |
	GDK_GL_MODE_DEPTH  |
	// GDK_GL_MODE_MULTISAMPLE |
	GDK_GL_MODE_STENCIL |
	GDK_GL_MODE_DOUBLE
	);
      /* Try stencil buffer */
      glconfig = gdk_gl_config_new_by_mode(mode);
      if (glconfig == NULL) {
	g_print ("\n*** Cannot use stencil buffer.\n");
	g_print ("\n*** Sorry no Zalman setting possible.\n");
	mode = static_cast<GdkGLConfigMode>
	  (GDK_GL_MODE_RGB    |
	   GDK_GL_MODE_DEPTH  |
	   GDK_GL_MODE_DOUBLE);
      } else {
        graphics_info_t::display_mode = coot::ZALMAN_STEREO;
      }
   }


   
   /* Try double-buffered visual */
   glconfig = gdk_gl_config_new_by_mode(mode);
   if (glconfig == NULL) {
      g_print ("\n*** Cannot find the double-buffered visual.\n");
      g_print ("\n*** Trying single-buffered visual.\n");

      mode =static_cast<GdkGLConfigMode>
	 (GDK_GL_MODE_RGB   |
	  GDK_GL_MODE_DEPTH);
      
      /* Try single-buffered visual */
      glconfig = gdk_gl_config_new_by_mode (mode);
      if (glconfig == NULL) {
	 g_print ("*** No appropriate OpenGL-capable visual found.\n");
	 exit (1);
      }
   }

   // BL debugging
   //if (gdk_gl_config_has_stencil_buffer(glconfig)) {
   //  g_print("BL DEBUG:: have stencil\n");
   //} else {
   //  g_print("BL DEBUG:: dont have stencil\n");
   //}

  /*
   * Drawing area to draw OpenGL scene.
   */
   GtkWidget *drawing_area = NULL; // the returned thing

  int n_contexts = 1;
  if ((try_stereo_flag == coot::SIDE_BY_SIDE_STEREO) ||
      (try_stereo_flag == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ||
      (try_stereo_flag == coot::DTI_SIDE_BY_SIDE_STEREO)) {
     n_contexts = 2;
     graphics_info_t::display_mode = try_stereo_flag;
  }

  int context_count = 0;
  
  while(context_count < n_contexts) {

     context_count++;
  
     GtkWidget *drawing_area_tmp = gtk_drawing_area_new ();
     if (context_count == 1) {
	drawing_area = drawing_area_tmp;
     } else {
	graphics_info_t::glarea_2 = drawing_area_tmp;
     }

     { 
//	int gl_context_x_size = GRAPHICS_WINDOW_X_START_SIZE;
//	int gl_context_y_size = GRAPHICS_WINDOW_Y_START_SIZE;
	int gl_context_x_size = graphics_info_t::graphics_x_size;
	int gl_context_y_size = graphics_info_t::graphics_y_size;
	// int gl_context_y_size = g.graphics_y_size; old

 	if (context_count > 1) { // more than the first context
	   // std::cout << " =============== " << context_count << std::endl;
	  if (graphics_info_t::glarea) {
 	   gl_context_x_size = graphics_info_t::glarea->allocation.width;
 	   gl_context_y_size = graphics_info_t::glarea->allocation.height;
	  }
	   // std::cout << " ===============" << gl_context_x_size
	   // << " "<< gl_context_y_size << std::endl;
 	}

// 	gtk_widget_set_size_request (drawing_area_tmp,
// 				     gl_context_x_size,
// 				     gl_context_y_size);

	GtkWindow *window1 = GTK_WINDOW(lookup_widget(vbox1,"window1"));
	gtk_window_set_default_size(window1,
				    n_contexts * gl_context_x_size,
				    n_contexts * gl_context_y_size);
	
     }
     
     /* Set OpenGL-capability to the widget */
     gtk_widget_set_gl_capability (drawing_area_tmp,
				   glconfig,
				   NULL,
				   TRUE,
				   GDK_GL_RGBA_TYPE); // render_type (only choice)
     if (drawing_area_tmp) {

	if (try_stereo_flag == coot::HARDWARE_STEREO_MODE) {
	   if (got_hardware_stereo_flag) { 
	      std::cout << "INFO:: Hardware stereo widget opened successfully"
			<< std::endl;
	      graphics_info_t::display_mode = coot::HARDWARE_STEREO_MODE;
	   }
	}

	/* Events for widget must be set before X Window is created */
	gtk_widget_set_events(GTK_WIDGET(drawing_area_tmp),
			      GDK_EXPOSURE_MASK      |
			      GDK_BUTTON_PRESS_MASK  |
			      GDK_BUTTON_RELEASE_MASK|
			      GDK_SCROLL_MASK        |
			      GDK_POINTER_MOTION_MASK|
			      GDK_KEY_PRESS_MASK     |
			      GDK_KEY_RELEASE_MASK   |
			      GDK_POINTER_MOTION_HINT_MASK);

	/* Connect signal handlers */
	/* Redraw image when exposed. */
	gtk_signal_connect(GTK_OBJECT(drawing_area_tmp), "expose_event",
			   GTK_SIGNAL_FUNC(expose), NULL);
	/* When window is resized viewport needs to be resized also. */
	gtk_signal_connect(GTK_OBJECT(drawing_area_tmp), "configure_event",
			   GTK_SIGNAL_FUNC(reshape), NULL);
	/* Do initialization when widget has been realized. */
	gtk_signal_connect(GTK_OBJECT(drawing_area_tmp), "realize",
			   GTK_SIGNAL_FUNC(init), NULL);

	/* pressed a button? */
	gtk_signal_connect (GTK_OBJECT(drawing_area_tmp), "button_press_event",
			    GTK_SIGNAL_FUNC(glarea_button_press), NULL);
	gtk_signal_connect (GTK_OBJECT(drawing_area_tmp), "button_release_event",
			    GTK_SIGNAL_FUNC(glarea_button_release), NULL);
	/* mouse in motion! */
	gtk_signal_connect (GTK_OBJECT(drawing_area_tmp), "motion_notify_event",
			    GTK_SIGNAL_FUNC(glarea_motion_notify), NULL);
	// mouse wheel scrolled:
	gtk_signal_connect (GTK_OBJECT(drawing_area_tmp), "scroll_event",
			    GTK_SIGNAL_FUNC(glarea_scroll_event), NULL);

	/* put glarea into vbox */
	GtkWidget *main_window_graphics_hbox =
	   lookup_widget(vbox1, "main_window_graphics_hbox");
	gtk_container_add(GTK_CONTAINER(main_window_graphics_hbox),
			  GTK_WIDGET(drawing_area_tmp));
  
	/* Capture keypress events */
	gtk_signal_connect(GTK_OBJECT(drawing_area_tmp), "key_press_event",
			   GTK_SIGNAL_FUNC(key_press_event), NULL);
	gtk_signal_connect(GTK_OBJECT(drawing_area_tmp), "key_release_event",
			   GTK_SIGNAL_FUNC(key_release_event), NULL);

	/* set focus to glarea widget - we need this to get key presses. */
	GTK_WIDGET_SET_FLAGS(drawing_area_tmp, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(GTK_WIDGET(drawing_area_tmp));

     } else {
	std::cout << "CATASTROPHIC ERROR:: in gl_extras  no GtkGL widget!"
		  << std::endl;
     }
  } // end while
  //   std::cout << "DEBUG:: gl_extras returns " << drawing_area << std::endl;
  return drawing_area;
   
}
#endif // #if (GTK_MAJOR_VERSION == 1)


gint
init(GtkWidget *widget)
{

   graphics_info_t g;

   if (g.background_colour == NULL) { 
      g.background_colour = new float[4];
      g.background_colour[0] = 0.0; 
      g.background_colour[1] = 0.0; 
      g.background_colour[2] = 0.0; 
      g.background_colour[3] = 1.0; 
   }

   // The cosine->sine lookup table, used in picking.
   //
   // The data in it are static, so we can get to them anywhere
   // now that we have run this
   cos_sin cos_sin_table(1000);

#if (GTK_MAJOR_VERSION == 1)
   
   /* Check if OpenGL (GLX extension) is supported. */
   if (gdk_gl_query() == FALSE) {
      g_print("OpenGL not supported\n");
    return 0;
   }
  /* OpenGL functions can be called only if make_current returns true */
   if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
      init_gl_widget(widget);
#else

   // 
   // what about glXMakeCurrent(dpy, Drawable, Context) here?
   // Bool glXMakeCurrent(Display * dpy,
   //                     GLXDrawable  Drawable,
   //                     GLXContext  Context)

   init_gl_widget(widget);

#endif
   return GL_TRUE;
}


gint
init_gl_widget(GtkWidget *widget) { 

   glViewport(0,0, widget->allocation.width, widget->allocation.height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-10,30, 10,-20, -20,20); // change clipping
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   graphics_info_t g;
   
   // set the background colour
   // glClearColor(0,0,0,1);
   // these statics are set at the begining.
   glClearColor(g.background_colour[0],
		g.background_colour[1],
		g.background_colour[2],
		1); 
   
   // Enable fog
   //
   glEnable(GL_FOG);

   // Linear fog
   glFogi(GL_FOG_MODE, GL_LINEAR); 
   glFogf(GL_FOG_START, -20.0);
   // glFogf(GL_FOG_START, 0.0);
   glFogf(GL_FOG_END, 20.0);
   
   glFogfv(GL_FOG_COLOR, g.background_colour); 
   
   // Exponential fog
   // 
   // glFogi(GL_FOG_MODE, GL_EXP2);
   // glFogf(GL_FOG_DENSITY, 0.10);
   
   // 
   glEnable(GL_DEPTH_TEST);
   
   if (g.do_anti_aliasing_flag)
      glEnable(GL_LINE_SMOOTH);
   
   
   // Solid model lighting
   //
   setup_lighting(graphics_info_t::do_lighting_flag);

   
   
   // set the skeleton to initially be yellow
   graphics_info_t::skeleton_colour[0] = 0.7;
   graphics_info_t::skeleton_colour[1] = 0.7;
   graphics_info_t::skeleton_colour[2] = 0.2;


   // last value does not matter because zero rotation
   trackball(graphics_info_t::quat , 0.0, 0.0, 0.0, 0.0, 0.0);

   // initialize also the baton quaternion
   trackball(graphics_info_t::baton_quat , 0.0, 0.0, 0.0, 0.0, 0.0);


   // gtk_idle_add((GtkFunction)animate, widget);

  return TRUE;
}

void
setup_lighting(short int do_lighting_flag) {


   if (do_lighting_flag) { // set this to 1 to light a surface currently.
//       GLfloat  mat_specular[]   = {1.0, 0.3, 0.2, 1.0};
//       GLfloat  mat_ambient[]    = {0.8, 0.1, 0.1, 1.0};
//       GLfloat  mat_diffuse[]    = {0.2, 1.0, 0.0, 1.0};
//       GLfloat  mat_shininess[]  = {50.0};
      GLfloat  light_position[] = {1.0, 1.0, 1.0, 0.0};

      glClearColor(0.0, 0.0, 0.0, 0.0);
      glShadeModel(GL_SMOOTH);

//       glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
//       glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
//       glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
//       glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
      glLightfv(GL_LIGHT0,   GL_POSITION, light_position);
      // glLightfv(GL_LIGHT2,   GL_POSITION, light_position);

      glEnable(GL_LIGHT0);
      glEnable(GL_DEPTH_TEST);
   } else {
      glDisable(GL_LIGHT0);
   }
}


/* When glarea widget size changes, viewport size is set to match the
   new size */

gint reshape(GtkWidget *widget, GdkEventConfigure *event) {

#if (GTK_MAJOR_VERSION == 1)
   
   /* OpenGL functions can be called only if make_current returns true */

   if (gtk_gl_area_make_current(GTK_GL_AREA(widget))) {
      glViewport(0,0, widget->allocation.width, widget->allocation.height);
      graphics_info_t g;
      g.graphics_x_size = widget->allocation.width;
      g.graphics_y_size = widget->allocation.height;
   }
   graphics_info_t::graphics_draw();
   return TRUE;
#else

   // GTK2 code
   if (graphics_info_t::make_current_gl_context(widget)) {
      glViewport(0,0, widget->allocation.width, widget->allocation.height);
      graphics_info_t g;
      g.graphics_x_size = widget->allocation.width;
      g.graphics_y_size = widget->allocation.height;
   } 
   graphics_info_t::graphics_draw(); // Added 20080408, needed?
#endif
   return TRUE;
}

/* When widget is exposed it's contents are redrawn. */
#define DEG_TO_RAD .01745327
gint expose(GtkWidget *widget, GdkEventExpose *event) {

   // std::cout << "expose "  << widget << std::endl;
   draw(widget, event);

   // RR Broke   
   // if (graphics_info_t::do_expose_swap_buffers_flag) 
   // graphics_info_t::coot_swap_buffers(widget, 0);
   
   graphics_info_t::do_expose_swap_buffers_flag = 1;
   return TRUE;
}

   
/* When widget is exposed it's contents are redrawn. */
#define DEG_TO_RAD .01745327
gint draw(GtkWidget *widget, GdkEventExpose *event) {

//    GtkWidget *w = graphics_info_t::glarea;
//    GdkGLContext *glcontext = gtk_widget_get_gl_context (w);
//    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (w);
//    int i = gdk_gl_drawable_gl_begin (gldrawable, glcontext);
//    std::cout << "DEBUG gdk_gl_drawable_gl_begin returns state: "
// 	     << i << std::endl;
//    if (i == 0)
//       return TRUE;


   if (graphics_info_t::display_mode == coot::HARDWARE_STEREO_MODE) {
      draw_hardware_stereo(widget, event);
   } else {
      if (graphics_info_t::display_mode == coot::ZALMAN_STEREO) {
	 draw_zalman_stereo(widget, event);
      } else {
	 if (graphics_info_t::display_mode_use_secondary_p()) { 
	    if (widget == graphics_info_t::glarea_2) {
	       // std::cout << "DEBUG:: draw other window" << std::endl;
	       graphics_info_t g; // is this a slow thing?
	       float tbs =  g.get_trackball_size(); 
	       float spin_quat[4];
	       // 0.0174 = 1/(2*pi)
	       float eye_fac = 1.0;
	       if (graphics_info_t::display_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE)
		  eye_fac = -1.0;
	       trackball(spin_quat, 0, 0, eye_fac*g.hardware_stereo_angle_factor*0.07, 0.0, tbs);
	       add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);
	       // std::cout << "drawing gl_area_2 (right)" << std::endl;
	       draw_mono(widget, event, IN_STEREO_SIDE_BY_SIDE_RIGHT);
	       // reset the viewing angle:
	       trackball(spin_quat, 0, 0, -eye_fac*g.hardware_stereo_angle_factor*0.07, 0.0, tbs);
	       add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);
	    } else {
	       // std::cout << "drawing regular gl_area (left) " << std::endl;
	       draw_mono(widget, event, IN_STEREO_SIDE_BY_SIDE_LEFT);
	    }
	 } else { 
	    draw_mono(widget, event, IN_STEREO_MONO);
	 }
      }
   }
   return TRUE;
}


gint draw_hardware_stereo(GtkWidget *widget, GdkEventExpose *event) {

   // tinker with graphics_info_t::quat, rotate it left, draw it,
   // rotate it right, draw it.
   graphics_info_t g; // is this a slow thing?
   float tbs =  g.get_trackball_size(); 
   float spin_quat[4];
   // 0.0174 = 1/(2*pi)
   trackball(spin_quat, 0, 0, -g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   // draw right:
   glDrawBuffer(GL_BACK_RIGHT);
   draw_mono(widget, event, IN_STEREO_HARDWARE_STEREO);
   
   trackball(spin_quat, 0, 0, 2.0*g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   // draw left:
   glDrawBuffer(GL_BACK_LEFT);
   draw_mono(widget, event, IN_STEREO_HARDWARE_STEREO);

   // reset the viewing angle:
   trackball(spin_quat, 0, 0, -g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   // draw it
#if (GTK_MAJOR_VERSION == 1)
   gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
#else
   GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
   gdk_gl_drawable_swap_buffers(gldrawable);
#endif
   return TRUE;
}

gint draw_zalman_stereo(GtkWidget *widget, GdkEventExpose *event) {

   // tinker with graphics_info_t::quat, rotate it left, draw it,
   // rotate it right, draw it.
   graphics_info_t g; // is this a slow thing?
   float tbs =  g.get_trackball_size(); 
   float spin_quat[4];
   // 0.0174 = 1/(2*pi)
   if (graphics_info_t::display_mode == coot::ZALMAN_STEREO) {
     GLint viewport[4];
     glGetIntegerv(GL_VIEWPORT,viewport);
    
     glPushAttrib(GL_ENABLE_BIT);
     glMatrixMode(GL_PROJECTION);
     glPushMatrix();
     glLoadIdentity();
     glOrtho(0,viewport[2],0,viewport[3],-10.0,10.0);
     glMatrixMode(GL_MODELVIEW);
     glPushMatrix();
     glLoadIdentity();
     glTranslatef(0.33F,0.33F,0.0F); 
     glDisable(GL_STENCIL_TEST);
     /* We/you may not need all of these... FIXME*/

     glDisable(GL_ALPHA_TEST);
     glDisable(GL_LIGHTING);
     glDisable(GL_FOG);
     glDisable(GL_NORMALIZE);
     glDisable(GL_DEPTH_TEST);
     glDisable(GL_COLOR_MATERIAL);
     glDisable(GL_LINE_SMOOTH);
     glDisable(GL_DITHER);
     glDisable(GL_BLEND);
     glShadeModel(GL_SMOOTH);
     //glDisable(0x809D); /* GL_MULTISAMPLE_ARB */ 
     glDisable(0x809D); /* GL_MULTISAMPLE_ARB */ 

     //glDisable(GL_STENCIL_TEST);
     glClearStencil(0);
     glColorMask(false,false,false,false);
     glDepthMask(false);
     glClear(GL_STENCIL_BUFFER_BIT);

     glEnable(GL_STENCIL_TEST);
     glStencilFunc(GL_ALWAYS, 1, 1);
     glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

     glLineWidth(1.0);
     glBegin(GL_LINES);
     int h = viewport[3], w=viewport[2];
     int y;
     for(y=0;y<h;y+=2) {
       glVertex2i(0,y);
       glVertex2i(w,y);
     }
     glEnd();

     glColorMask(true,true,true,true);
     glDepthMask(true);

     glMatrixMode(GL_MODELVIEW);
     glPopMatrix();
     glMatrixMode(GL_PROJECTION);
     glPopMatrix();
     //
     glPopAttrib();
   }

   trackball(spin_quat, 0, 0, -g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   // draw right (should maybe be left)??:
   draw_mono(widget, event, IN_STEREO_ZALMAN_RIGHT); // 5 for right
   
   trackball(spin_quat, 0, 0, 2.0*g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   // draw left (should maybe be right)??:
   draw_mono(widget, event, IN_STEREO_ZALMAN_LEFT); // 6 for left

   // reset the viewing angle:
   trackball(spin_quat, 0, 0, -g.hardware_stereo_angle_factor*0.0358, 0.0, tbs);
   add_quats(spin_quat, graphics_info_t::quat, graphics_info_t::quat);

   return TRUE;
}

void gdkglext_finish_frame(GtkWidget *widget) {

   // should not even be called by GTK.
#if (GTK_MAJOR_VERSION == 1)
#else   
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
  gdk_gl_drawable_gl_end (gldrawable);
#endif
  
} 


gint
draw_mono(GtkWidget *widget, GdkEventExpose *event, short int in_stereo_flag) {


   // std::cout << "draw_mono() with widget " << widget << std::endl;

   if ((event-1) != 0) { 
      /* Draw only last expose. */
      if (event->count > 0) {
	 //       cout << "event->count is " << event->count << endl;
	 //       cout << "chucking an event" << endl;
	 return TRUE;
      }
   }

   // graphics_info_t info;          // static members
   // Those two classes should be rationalised.

   if (graphics_info_t::GetFPSFlag()) {
      graphics_info_t::ShowFPS();
   }

   // GLCONTEXT
   int gl_context = GL_CONTEXT_MAIN;
   if (in_stereo_flag == IN_STEREO_SIDE_BY_SIDE_RIGHT)
      gl_context = GL_CONTEXT_SECONDARY;

   gl_context_info_t gl_info(graphics_info_t::glarea, graphics_info_t::glarea_2);
   

// void glDepthRange(GLclampd near, GLclampd far); Defines an encoding
// for z coordinates that's performed during the viewport
// transformation. The near and far values represent adjustments to the
// minimum and maximum values that can be stored in the depth buffer. By
// default, they're 0.0 and 1.0, respectively, which work for most
// applications. These parameters are clamped to lie within [0,1].
   
   /* OpenGL functions can be called only if make_current returns true */
   if (graphics_info_t::make_current_gl_context(widget)) {

      float aspect_ratio = float (widget->allocation.width)/
	 float (widget->allocation.height);

      if (graphics_info_t::display_mode == coot::DTI_SIDE_BY_SIDE_STEREO) {
	 aspect_ratio *= 2.0; // DTI side by side stereo mode
      } 

      // Clear the scene
      // 
      // BL says:: another hack!? FIXME
      // dont clear when we want to draw the 2 Zalman views
      
      if (in_stereo_flag != IN_STEREO_ZALMAN_LEFT) 
      	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


      // From Bernhard
#if (GTK_MAJOR_VERSION > 1) 
      glEnable (GL_FOG);
      glFogi(GL_FOG_MODE, GL_LINEAR);
      glFogf(GL_FOG_START, -20.0);
      glFogf(GL_FOG_END, 20.0);
      glDepthFunc (GL_LESS);
      glFogfv(GL_FOG_COLOR, graphics_info_t::background_colour);
      glEnable(GL_DEPTH_TEST);
#endif


      // glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

      // BL:: this is code for Zalman monitor. Maybe can be somewhere else!?
      // Zalman works here?! but crap lighting!?
      if (in_stereo_flag == IN_STEREO_ZALMAN_RIGHT) {
	// draws one Zalman lines
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);

	/* red triangle for testing*/
	//glColor3ub(200, 0, 0);
	//glBegin(GL_POLYGON);
	//glVertex3i(-4, -4, 0);
	//glVertex3i(4, -4, 0);
	//glVertex3i(0, 4, 0);
	//glEnd();

	//glDisable(GL_STENCIL_TEST);
      }

      if (in_stereo_flag == IN_STEREO_ZALMAN_LEFT) {
	// g_print("BL DEBUG:: now draw 'right'\n");
	// draws the other Zalman lines
	glStencilFunc(GL_EQUAL, 0, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);
	/* green square for testing */
	//glColor3ub(0, 200, 0);
	//glBegin(GL_POLYGON);
	//glVertex3i(3, 3, 0);
	//glVertex3i(-3, 3, 0);
	//glVertex3i(-3, -3, 0);
	//glVertex3i(3, -3, 0);
	//glEnd();
      }


      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      // 	 glOrtho(GLdouble left,   GLdouble right, 
      //               GLdouble bottom, GLdouble top,  
      //               GLdouble near,   GLdouble far);

      GLdouble near_scale = 0.1;
//       if (! graphics_info_t::esoteric_depth_cue_flag)
// 	 near_scale = 0.3;
      
      GLdouble near = -near_scale*graphics_info_t::zoom * (graphics_info_t::clipping_front*-0.1 + 1.0);
      GLdouble far  =        0.30*graphics_info_t::zoom * (graphics_info_t::clipping_back* -0.1 + 1.0);

      //	 if (graphics_info_t::esoteric_depth_cue_flag) 
      glOrtho(-0.3*graphics_info_t::zoom*aspect_ratio, 0.3*graphics_info_t::zoom*aspect_ratio,
	      -0.3*graphics_info_t::zoom,  0.3*graphics_info_t::zoom,
	      near, far);
      // 	 else
      // 	    glOrtho(-0.3*info.zoom*aspect_ratio, 0.3*info.zoom*aspect_ratio,
      // 		    -0.3*info.zoom,  0.3*info.zoom,
      // 		    -0.10*info.zoom,
      //   		    +0.30*info.zoom);

      //glFogf(GL_FOG_START, -0.00*info.zoom);
      //glFogf(GL_FOG_END,    0.3*info.zoom);


      if (graphics_info_t::esoteric_depth_cue_flag) {
	 glFogf(GL_FOG_START,  0.0f);
	 glFogf(GL_FOG_END, far);
      } else {
	 glFogf(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
	 glFogf(GL_FOG_DENSITY, 1.0);
	 GLdouble fog_start = 0;
	 GLdouble fog_end =  far;
	 glFogf(GL_FOG_START,  fog_start);
	 glFogf(GL_FOG_END,    fog_end);
	 // std::cout << "GL_FOG_START " << fog_start << " with far  " << far  << std::endl;
	 // std::cout << "GL_FOG_END "   << fog_end   << " with near " << near << std::endl;

      }

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      // Scene Rotation
      GL_matrix m;
      m.from_quaternion(graphics_info_t::quat); // consider a constructor.
      glMultMatrixf(m.get());
      

      // Translate the scene to the the view centre
      // i.e. the screenrotation center is at (X(), Y(), Z())
      //
      glTranslatef(-graphics_info_t::RotationCentre_x(),
		   -graphics_info_t::RotationCentre_y(),
		   -graphics_info_t::RotationCentre_z());

      if (! graphics_info_t::esoteric_depth_cue_flag) { 
      	 coot::Cartesian front = unproject(0.0);
	 coot::Cartesian back  = unproject(1.0);
	 coot::Cartesian front_to_back = back - front;
	 coot::Cartesian fbs = front_to_back.by_scalar(-0.2);
	 // glTranslatef(fbs.x(), fbs.y(), fbs.z());
      }


      // do we need to turn on the lighting?
      int n_display_list_objects = 0;

      for (int ii=graphics_info_t::n_molecules()-1; ii>=0; ii--) {

	 // Molecule stuff
	 //
	 graphics_info_t::molecules[ii].draw_molecule(graphics_info_t::draw_zero_occ_spots_flag);

	 //
	 graphics_info_t::molecules[ii].draw_dipoles();

	 // draw display list objects
	 if (graphics_info_t::molecules[ii].has_display_list_objects()) {
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    n_display_list_objects +=
	       graphics_info_t::molecules[ii].draw_display_list_objects(gl_context);

	    glDisable(GL_LIGHT0);
	    glDisable(GL_LIGHTING);
	 }

	 if (graphics_info_t::molecules[ii].draw_animated_ligand_interactions_flag) { 
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    glEnable(GL_LIGHT1);
	    // glEnable(GL_LIGHT2);
	    graphics_info_t::molecules[ii].draw_animated_ligand_interactions(gl_info,
									     graphics_info_t::time_holder_for_ligand_interactions);
	    glDisable(GL_LIGHTING);
	 } 

	 // draw anisotropic atoms maybe
	 graphics_info_t::molecules[ii].anisotropic_atoms();

	 // We need to (also) pass whether we are drawing the first or
	 // secondary window, so that, when display lists are being
	 // used we use the correct part of theMapContours.
	 //
         // BL says:: bad hack FIXME
         if (in_stereo_flag == IN_STEREO_ZALMAN_LEFT || in_stereo_flag == IN_STEREO_ZALMAN_RIGHT) {
	    graphics_info_t::molecules[ii].draw_density_map(graphics_info_t::display_lists_for_maps_flag,
							    0);
         } else {
	    graphics_info_t::molecules[ii].draw_density_map(graphics_info_t::display_lists_for_maps_flag,
							    in_stereo_flag);
         }

	 // Turn the light(s) on and after off, if needed.
	 // 
	 graphics_info_t::molecules[ii].draw_surface();

	 // extra restraints - thin blue lines or some such
	 graphics_info_t::molecules[ii].draw_extra_restraints_representation();

	 // Label the atoms in the atoms label list.
	 //
	 graphics_info_t::molecules[ii].label_atoms(graphics_info_t::brief_atom_labels_flag);

	 // Draw the dotted atoms:
	 graphics_info_t::molecules[ii].draw_dots();

	 // Draw Unit cell maybe.
	 graphics_info_t::molecules[ii].draw_coord_unit_cell(graphics_info_t::cell_colour);

	 // Draw Map unit cell maybe;
	 graphics_info_t::molecules[ii].draw_map_unit_cell(graphics_info_t::cell_colour);

	 //
	 graphics_info_t::molecules[ii].draw_skeleton();

      }


      // regularize object 
      graphics_info_t::moving_atoms_graphics_object();

      // environment object
      graphics_info_t::environment_graphics_object();

      // flash the picked intermediate atom (Erik-mode)
      graphics_info_t::picked_intermediate_atom_graphics_object();

      //
      graphics_info_t::baton_object();

      //
      graphics_info_t::geometry_objects(); // angles and distances

      // pointer distances
      graphics_info_t::pointer_distances_objects();

      // lsq atom blobs
      if (graphics_info_t::lsq_plane_atom_positions->size() > 0) {
	 graphics_info_t g;
	 g.render_lsq_plane_atoms();
      }

      // ligand flash bond
      graphics_info_t::draw_chi_angles_flash_bond();
	 
      // draw reference object, which sits at the model origin.
      //
      if (graphics_info_t::show_origin_marker_flag) { 
	 glLineWidth(1.0);
	 glColor3f(0.8,0.8,0.2);
	 myWireCube (1.0);
      }

      //
      // test_object();

      graphics_info_t::draw_generic_objects();
      graphics_info_t::draw_generic_text();

      // Put a wirecube at the rotation centre.
      //
      glPushMatrix();
      glTranslatef(graphics_info_t::RotationCentre_x(),
		   graphics_info_t::RotationCentre_y(),
		   graphics_info_t::RotationCentre_z());

      draw_axes(m);

      glScalef (graphics_info_t::rotation_centre_cube_size, 
		graphics_info_t::rotation_centre_cube_size, 
		graphics_info_t::rotation_centre_cube_size);
	 
      if (! graphics_info_t::smooth_scroll_on) { 
	 glLineWidth(2.0);
	 glColor3f(0.8,0.6,0.7);
	 myWireCube (1.0);
      }

     // Now we have finished displaying our objects and making
      // transformations, lets put the matrix back how it used
      // to be.
      glPopMatrix();

      // Note that is important to leave with the modelling and
      // view matrices that are the same as those that were when
      // the molecule was drawn.  Atom picking depends on this.

      // transparent objects:
      // 
      for (int ii=graphics_info_t::n_molecules()-1; ii>=0; ii--) {
	 if (is_valid_map_molecule(ii)) {
	    // enable lighting internal to this function
	    bool do_flat =
	       graphics_info_t::do_flat_shading_for_solid_density_surface;
	    graphics_info_t::molecules[ii].draw_solid_density_surface(do_flat);
	 }
      }


      // 
      draw_crosshairs_maybe();

      // 
      display_density_level_maybe();

// This causes weird lighting effects.
// commented out 20080322, seems to work proper now.      
// Turn on the light(s)?  // moved here from end of loop 20051014
//       if (n_display_list_objects > 0) {
// 	 glPushMatrix();
// 	 glMatrixMode(GL_MODELVIEW);
// 	 glLoadIdentity();
// 	 GLfloat  light_position_0[] = {1.0, 1.0, 1.0, 0.0};
// 	 GLfloat  light_position_1[] = {0.0, 0.0, 1.0, 0.0};
// 	 glLightfv(GL_LIGHT0,  GL_POSITION, light_position_0);
// 	 glEnable(GL_LIGHTING);
// 	 glEnable(GL_LIGHT0);
// 	 glPopMatrix();
//       }

      // BL says:: not sure if we dont need to do this for 2nd Zalman view
      //      if (! in_stereo_flag) {
      if (in_stereo_flag != IN_STEREO_HARDWARE_STEREO && in_stereo_flag != IN_STEREO_ZALMAN_RIGHT) {
         /* Swap backbuffer to front */
#if (GTK_MAJOR_VERSION == 1)
         gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
#else
         GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
         if (gdk_gl_drawable_is_double_buffered (gldrawable)) {
            gdk_gl_drawable_swap_buffers (gldrawable);
         } else {
            glFlush ();
         }

#endif
         graphics_info_t::Increment_Frames();
      }

      if (graphics_info_t::display_mode == coot::ZALMAN_STEREO)
	glDisable(GL_STENCIL_TEST);
  
   } // gtkgl make area current test

#if (GTK_MAJOR_VERSION == 1)
#else
   gdkglext_finish_frame(widget);
#endif    
   
   return TRUE;
}

void
display_density_level_maybe() {

   if (graphics_info_t::display_density_level_on_screen == 1) {
      if (graphics_info_t::display_density_level_this_image == 1) {

	 //	 std::cout << "DEBUG:: screen label: "
	 // << graphics_info_t::display_density_level_screen_string
	 // << std::endl;
	 
	 GLfloat white[3] = {1.0, 1.0, 1.0};
	 GLfloat black[3] = {0.0, 0.0, 0.0};

	 if (background_is_black_p())
	    glColor3fv(white);
	 else 
	    glColor3fv(black);

	 glPushMatrix();
	 glLoadIdentity();

 	 glMatrixMode(GL_PROJECTION);
	 glPushMatrix();
 	 glLoadIdentity();

	 // Disable the fix so that the density leve will not change
	 // intensity in a zoom-dependent way:
	 glPushAttrib(GL_ENABLE_BIT);
	 glDisable(GL_FOG);

      	    glRasterPos3f(0.0, 0.95, -0.9);
 	    printString(graphics_info_t::display_density_level_screen_string);

         glPopAttrib();
	 glPopMatrix();
	 glMatrixMode(GL_MODELVIEW);
	 
	 glPopMatrix();

      }
   }
   graphics_info_t::display_density_level_this_image = 0;

   // glScalef (2.0, 2.0, 2.0);
   // glRasterPos3f(2.0, 28.0,0.0);

   // This does resonable things when changing the contour level (it
   // waits until each change has been made before returning (rather
   // than combining them into one change), but terrible things to
   // regular rotation (unusable jerkiness).
//    while (gtk_events_pending())
//       gtk_main_iteration();

}

void 
update_things_on_move_and_redraw() {

   graphics_info_t g;
   g.update_things_on_move_and_redraw();
}

void myWireCube(float size) { 

	 float corners[8][3] = {
	    {-0.5,-0.5,-0.5}, //0 
	    {-0.5,-0.5,0.5}, //1 
	    {-0.5,0.5,-0.5}, //2 
	    {-0.5,0.5,0.5}, //3 
	    {0.5,-0.5,-0.5}, //4 
	    {0.5,-0.5,0.5}, //5 
	    {0.5,0.5,-0.5}, //6 
	    {0.5,0.5,0.5}};//7 

	 glBegin(GL_LINES);
	 
	 // bottom left connections
	   glVertex3f(corners[0][0], corners[0][1], corners[0][2]);
	   glVertex3f(corners[1][0], corners[1][1], corners[1][2]);

	   glVertex3f(corners[0][0], corners[0][1], corners[0][2]);
	   glVertex3f(corners[2][0], corners[2][1], corners[2][2]);
	 
	   glVertex3f(corners[0][0], corners[0][1], corners[0][2]);
	   glVertex3f(corners[4][0], corners[4][1], corners[4][2]);

	 // top right front connections
	   glVertex3f(corners[6][0], corners[6][1], corners[6][2]);
	   glVertex3f(corners[4][0], corners[4][1], corners[4][2]);
	 
	   glVertex3f(corners[6][0], corners[6][1], corners[6][2]);
	   glVertex3f(corners[2][0], corners[2][1], corners[2][2]);
	 
	   glVertex3f(corners[6][0], corners[6][1], corners[6][2]);
	   glVertex3f(corners[7][0], corners[7][1], corners[7][2]);
	 
	 // from 5
	   glVertex3f(corners[5][0], corners[5][1], corners[5][2]);
	   glVertex3f(corners[7][0], corners[7][1], corners[7][2]);

	   glVertex3f(corners[5][0], corners[5][1], corners[5][2]);
	   glVertex3f(corners[4][0], corners[4][1], corners[4][2]);

	   glVertex3f(corners[5][0], corners[5][1], corners[5][2]);
	   glVertex3f(corners[1][0], corners[1][1], corners[1][2]);

	 // from 3
	   glVertex3f(corners[3][0], corners[3][1], corners[3][2]);
	   glVertex3f(corners[1][0], corners[1][1], corners[1][2]);

	   glVertex3f(corners[3][0], corners[3][1], corners[3][2]);
	   glVertex3f(corners[7][0], corners[7][1], corners[7][2]);

	   glVertex3f(corners[3][0], corners[3][1], corners[3][2]);
	   glVertex3f(corners[2][0], corners[2][1], corners[2][2]);

	   glEnd();
   
} 

void
draw_crosshairs_maybe() { 

   if (graphics_info_t::draw_crosshairs_flag) {
      glPushMatrix();
      float s = 0.033 * 100/graphics_info_t::zoom; 
      //       std::cout << " zoom: " << graphics_info_t::zoom << std::endl;

      glLoadIdentity();
      GLfloat grey[3] = {0.7, 0.7, 0.7};
      glColor3fv(grey);
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();

      glLoadIdentity();

      glBegin(GL_LINES);

      // screen y axis
      
      float val = 3.8;
      glVertex3f(0.0, -val*s, 0.0);
      glVertex3f(0.0,  val*s, 0.0);

      val = 3.8;
      glVertex3f( 0.0,   -val*s, 0.0);
      glVertex3f(-0.3*s, -val*s, 0.0);
      glVertex3f( 0.0,    val*s, 0.0);
      glVertex3f(-0.3*s,  val*s, 0.0);

      val = 1.54;
      glVertex3f( 0.0,   -val*s, 0.0);
      glVertex3f(-0.3*s, -val*s, 0.0);
      glVertex3f( 0.0,    val*s, 0.0);
      glVertex3f(-0.3*s,  val*s, 0.0);

      val = 2.7;
      glVertex3f( 0.0,   -val*s, 0.0);
      glVertex3f(-0.3*s, -val*s, 0.0);
      glVertex3f( 0.0,    val*s, 0.0);
      glVertex3f(-0.3*s,  val*s, 0.0);


      // screen x axis
      // 
      // adjust for the width being strange
      float adjustment = float(graphics_info_t::glarea->allocation.height) /
          	         float(graphics_info_t::glarea->allocation.width);
      s *= adjustment;
      
      val = 3.8;
      glVertex3f(-val*s, 0.0, 0.0);
      glVertex3f( val*s, 0.0, 0.0);

      val = 3.8;
      glVertex3f(-val*s,  0.0,   0.0);
      glVertex3f(-val*s, -0.3*s, 0.0);
      glVertex3f( val*s,  0.0,   0.0);
      glVertex3f( val*s, -0.3*s, 0.0);

      val = 2.7;
      glVertex3f(-val*s,  0.0,   0.0);
      glVertex3f(-val*s, -0.3*s, 0.0);
      glVertex3f( val*s,  0.0,   0.0);
      glVertex3f( val*s, -0.3*s, 0.0);

      val = 1.54;
      glVertex3f(-val*s,  0.0,   0.0);
      glVertex3f(-val*s, -0.3*s, 0.0);
      glVertex3f( val*s,  0.0,   0.0);
      glVertex3f( val*s, -0.3*s, 0.0);

      glEnd();
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
   } 
} 

void
debug_draw_rotation_axes(float y_x, float y_z, float x_y, float x_z) {

   glBegin(GL_LINES);
   // x
   glVertex3f(0.0, 0.0, 0.0); 
   glVertex3f(y_x*10, 0.0, y_z*10);
   // y
   glVertex3f(0.0, 0.0, 0.0); 
   glVertex3f(x_y*10, 0.0, x_z*10);
   // z
   glVertex3f(0.0, 0.0, 0.0); 
   glVertex3f(0.0, 1.0*10, 0.0);
   // 
   glEnd();

   glRasterPos3f(12, 0.0, 0.0);
   printString("x");
   glRasterPos3f(0, 12.0, 0.0);
   printString("y");
   glRasterPos3f(0, 0.0, 12.0);
   printString("z");
} 

gint glarea_motion_notify (GtkWidget *widget, GdkEventMotion *event) {

   graphics_info_t info;
   gdouble x;
   gdouble y;
   int x_as_int=0, y_as_int=0;

   gdouble x_diff, y_diff;

   //GdkRectangle area; // needed for redrawing a fraction of the area
//    area.x = 0;
//    area.y = 0;
//    area.width  = widget->allocation.width;
//    area.height = widget->allocation.height;

   GdkModifierType my_button1_mask = info.gdk_button1_mask();
   GdkModifierType my_button2_mask = info.gdk_button2_mask();
   GdkModifierType my_button3_mask = info.gdk_button3_mask();
 
   GdkModifierType state;
   short int button_was_pressed = 0; 

   if (event->is_hint) {
      gdk_window_get_pointer(event->window, &x_as_int, &y_as_int, &state);
      x = x_as_int;
      y = y_as_int;
   } else {
      // when does this happen?  Never as far as I can see.
      // Actually Bernie says that it happens for him on WindowsXX
      // 
      // std::cout << "!!!!!!!!! changing state!" << std::endl;
      x_as_int = int(event->x);
      y_as_int = int(event->y);
      x = event->x;
      y = event->y;
      state = (GdkModifierType) event->state;
   }

   // Try to correct cntrl and shift anomalies:
   // 
   if (event->state & GDK_CONTROL_MASK)
      info.control_is_pressed = 1;
   else
      info.control_is_pressed = 0;
   if (event->state & GDK_SHIFT_MASK)
      info.shift_is_pressed = 1;
   else
      info.shift_is_pressed = 0;
    
   if (state & my_button1_mask) {

      short int local_rotate_view_mode = 0;

      button_was_pressed = 1;
      if (info.baton_mode) {

	 info.rotate_baton(x,y);

      } else {

	 if (info.in_edit_chi_mode_flag || info.in_edit_torsion_general_flag) {
	    if (info.in_edit_chi_mode_flag) {
	       
	       if (info.in_edit_chi_mode_view_rotate_mode) { 
		  local_rotate_view_mode = 1;
	       } else { 
		  if (graphics_info_t::do_probe_dots_on_rotamers_and_chis_flag) {
		     graphics_info_t g;
		     g.do_probe_dots_on_rotamers_and_chis();
		  } 
		  info.rotate_chi(x, y);
	       }
	    }
	    if (info.in_edit_torsion_general_flag) {
	       if (info.in_edit_chi_mode_view_rotate_mode) { 
		  local_rotate_view_mode = 1;
	       } else {
		  info.rotate_chi_torsion_general(x,y);
	       }
	    }
	    
	 } else {
	    // not rotating chi or torsion_general
	    if (info.in_moving_atoms_drag_atom_mode_flag) {

	       if (info.control_is_pressed) {

		  // single atom move, except if we are in
		  // rotate/translate zone, where a Ctrl-click does a
		  // intermediate-atom-screen-z-rotate not a single
		  // atom move.

// 		  std::cout << "Moving single atom " << x_as_int << " " 
// 				<< y_as_int << " " << x << " " << y << std::endl;

		  info.move_single_atom_of_moving_atoms(x_as_int, y_as_int);


	       } else {

		  // multi atom move

#ifdef HAVE_GSL
		  if (info.last_restraints_size() > 0) {

		     if (graphics_info_t::a_is_pressed) {

			 // We can't use shift, because this never
			 // happens because
			// in_moving_atoms_drag_atom_mode_flag is
			// unset on shift-left-click ("label atom").
			info.move_moving_atoms_by_shear(x_as_int, y_as_int, 1);
			if (graphics_info_t::dragged_refinement_refine_per_frame_flag) {
			   info.drag_refine_refine_intermediate_atoms();
			}
		     } else {
			info.move_moving_atoms_by_shear(x_as_int, y_as_int, 0);
			if (graphics_info_t::dragged_refinement_refine_per_frame_flag) {
			   info.drag_refine_refine_intermediate_atoms();
			}
		     }
		  } else {
		     // don't allow translation drag of the
		     // intermediate atoms when they are a rotamer:
		     // 
		     if (! info.rotamer_dialog) {
			// e.g. translate an added peptide fragment.
			info.move_moving_atoms_by_simple_translation(x_as_int,
								     y_as_int);
		     }
		  }
#endif // HAVE_GSL		  
	       }

	    } else {
	       // info.in_moving_atoms_drag_atom_mode_flag test

	       short int handled_non_atom_drag_event = 0; 
	       x_diff = x - info.GetMouseBeginX();
	       y_diff = y - info.GetMouseBeginY();
	       handled_non_atom_drag_event =
		  info.rotate_intermediate_atoms_maybe(0, x_diff * 0.01);
	       info.rotate_intermediate_atoms_maybe(1, y_diff * 0.01);
	       
	       if (! handled_non_atom_drag_event) {

		  short int do_drag = 0;
		  if (info.static_graphics_pick_pending()) {
		     if (info.control_key_for_rotate_flag) {
			if (event->state & GDK_CONTROL_MASK) { 
			   local_rotate_view_mode = 1;
			} else {
			   // 
			}
		     } else {
			// ctrl is needed for picking, not mouse motion
			local_rotate_view_mode = 1;
		     }
		  } else {
		     if (event->state & GDK_CONTROL_MASK) {
			do_drag = 1;
		     } else {
			local_rotate_view_mode = 1; 
		     }
		  }

		  if (do_drag) { 
		     // ----- DRAG -----

		     // We are in (density) drag mode:
		     // 
		     // We need to do two things:
		     // i) change the rotation axis centre.
		     //    Question: which direction are we dragging in...?
		     //    That is, what is the vector in coordinate space
		     //    that corresponds to the screen x axis?
		     // 
		     // ii) update the map about the new rotation centre
		     //

		     x_diff = x - info.GetMouseBeginX();
		     y_diff = y - info.GetMouseBeginY();

		     coot::CartesianPair vec_x_y = screen_x_to_real_space_vector(widget);

		     // x_diff and y_diff are the scale factors to the x and y
		     // drag vectors.
		     // 
		     info.add_to_RotationCentre(vec_x_y, -x_diff*0.02, -y_diff*0.02);

		     if (info.GetActiveMapDrag() == 1) {
			for (int ii=0; ii<info.n_molecules(); ii++) { 
			   info.molecules[ii].update_map(); // to take account
			   // of new rotation centre.
			}
		     }
		     for (int ii=0; ii<info.n_molecules(); ii++) { 
			info.molecules[ii].update_symmetry();
		     }
		     info.graphics_draw();

		  } // control_is_pressed
	       }
	    }
	 }

      } // baton_mode

      // This is factored out and put here because there are 2 ways of
      // getting here now: either by conventional left mouse drag or
      // by cntl left mouse drag when in edit_chi_mode.
      // 
      // Perhaps it should have its own function.

      if (local_rotate_view_mode) {

	 /* Mouse button 1 is engaged (and we are in motion) */

	 // save it for the quaternion construction
	 // 
	 info.mouse_current_x = x;
	 info.mouse_current_y = y;

	 // 	 x_diff = x - info.GetMouseBeginX();
	 // 	 y_diff = y - info.GetMouseBeginY();

	 // Stop the nasty orientation glitch that we get when
	 // dragging the mouse from one gl context to the other in
	 // side-by-side stereo.
	 //
	 if (fabs(info.GetMouseBeginX()-info.mouse_current_x) < 300) { 

	    float spin_quat[4];

	    // map the coordinates of the mouse to the range -1:1, (so
	    // top right is at (1,1) and the centre of the window is at
	    // (0,0). 
	    // 
	    // modify spin_quat:
	    trackball(spin_quat,
		      (2.0*info.GetMouseBeginX() - widget->allocation.width) /widget->allocation.width,
		      (widget->allocation.height - 2.0*info.GetMouseBeginY())/widget->allocation.height,
		      (2.0*info.mouse_current_x - widget->allocation.width)  /widget->allocation.width,
		      (widget->allocation.height -  2.0*info.mouse_current_y)/widget->allocation.height,
		      info.get_trackball_size() );

	    // 	 cout << (2.0*info.GetMouseBeginX() - widget->allocation.width) /widget->allocation.width
	    // 	      << " "
	    // 	      << (2.0*info.mouse_current_x - widget->allocation.width)  /widget->allocation.width
	    // 	      <<  "      "
	    // 	      << (widget->allocation.height - 2.0*info.GetMouseBeginY())/widget->allocation.height
	    // 	      << " "
	    // 	      << (widget->allocation.height -  2.0*info.mouse_current_y)/widget->allocation.height
	    // 	      << endl; 
	    
	    // args: q1, q2, destination
	    add_quats(spin_quat, info.quat, info.quat);

	    // 	 cout << "spin_quat " << spin_quat[0] << " " << spin_quat[1] << " "
	    // 	      << spin_quat[2] << " " << spin_quat[3] << endl;
	    // 	 cout << "info.quat " << info.quat[0] << " " << info.quat[1] << " "
	    // 	      << info.quat[2] << " " << info.quat[3] << endl;

      
	    // 	 mol_rot.x_axis_angle += y_diff*0.14;  // we don't like x
	    //                                                // rotation much.  
	    // 	 mol_rot.y_axis_angle += x_diff*0.3;

	    // info.SetMouseBegin(x,y);  // for next round
      
	    // redraw because the scene rotation centre has changed.
	    //
	    info.graphics_draw();
	 }

      }
      
   } // my_button1_mask

   if (state & my_button3_mask) {

      button_was_pressed = 1;
      /* Mouse button 3 is engaged */
//       cout << "Button 3 motion: Zoom related "
//        << event->x << " " << event->y << endl;

      if (fabs(info.GetMouseBeginX()-x) < 300) { 
	 if (info.control_is_pressed) { 
	    if (info.shift_is_pressed) {
	       do_screen_z_rotate(x, y); // z-rotation
	    } else {
	       // Pymol-like z-translation or slab adjustment
	       // requested by FvD 20050304
	       //
	       // Diagonal up-right/down-left changes the clipping
	       //
	       // Diagonal up-left/down-righ changes the z-translation
	       // (c.f keypad 3 and .: keypad_translate_xyz(3, 1)).
	       // 
	       do_ztrans_and_clip(x, y);
	    }
	 } else { 
	    do_button_zoom(x, y);
	 }
      }
   }

   if (state & my_button2_mask) {
      button_was_pressed = 1;
      /* Mouse button 2 is engaged */
      // cout << "Button 2 motion " << event->x << " " << event->y;
   }
   
   if (! button_was_pressed) { 
      if (info.quanta_like_zoom_flag) { 
	 if (info.shift_is_pressed) { 
	    do_button_zoom(x,y);
	 }
      } 
   }

   // because we are only really interested in how the mouse has
   // moved from the previous position, not from when the mouse was
   // clicked.
   // 
   info.SetMouseBegin(x,y);

   return TRUE; 

}

void
do_button_zoom(gdouble x, gdouble y) { 
      /* Mouse button 3 is engaged */
      //cout << "Button 3 motion: Zoom related "
      // << event->x << " " << event->y << endl;

   graphics_info_t info;

   gdouble x_diff = x - info.GetMouseBeginX(); 
   scale_zoom_internal(1 -  x_diff/200.0); 

   gdouble y_diff = y - info.GetMouseBeginY(); 
   scale_zoom_internal(1 -  y_diff/200.0); 

   //cout << "difference on zoom: " << x << " - "
   //	   << info.GetMouseBeginX() << " = " << x_diff << endl;
   //cout << "Zoom : " << info.zoom << endl;

   if (info.dynamic_map_resampling == 1) {
	 
      // is a new new resampling triggered by this new zoom?
      // 
      // int iv = 1 + int (0.009*info.zoom) + info.dynamic_map_zoom_offset;
      int iv = 1 + int (0.009*(info.zoom + info.dynamic_map_zoom_offset)); 
      if (iv != info.graphics_sample_step) {
	    
	 for (int imap=0; imap<info.n_molecules(); imap++) {
	    info.molecules[imap].update_map(); // uses g.zoom
	 }
	 info.graphics_sample_step = iv;
      }
   }
   // redraw it
   info.graphics_draw();
}

void 
do_screen_z_rotate(gdouble x, gdouble y) { 

   // c. f. animate_idle()

   float spin_quat[4];
   graphics_info_t g;


   // int x0 = graphics_info_t::glarea->allocation.width;
   // int y0 = graphics_info_t::glarea->allocation.height;

   // gdouble x_diff = x - g.GetMouseBeginX(); 
   // gdouble y_diff = y - g.GetMouseBeginY(); 
   // double y2 = 1.0 + (x_diff + y_diff)*0.01;

//    double a_x = x - x0;
//    double a_y = y - y0;
//    double b_x = g.GetMouseBeginX() - x0;
//    double b_y = g.GetMouseBeginY() - y0;
   // double alen = sqrt(a_x*a_x + a_y*a_y);
   // double blen = sqrt(b_x*b_x + b_y*b_y);
   // double y2 = 0.001 * (a_x*b_x + a_y*b_y)/(alen*blen);

   gdouble x_diff = 0.01 * (x - g.GetMouseBeginX()); 
   gdouble y_diff = 0.01 * (y - g.GetMouseBeginY());
   x_diff += y_diff;
   
//    trackball(spin_quat, 
// 	     1, 1.0,
// 	     y2, ,
// 	     9.0);
//    trackball(spin_quat,
// 	     (2.0*g.GetMouseBeginX() - x0)                    /x0,
// 	     (y0                     - 2.0*g.GetMouseBeginY())/y0,
// 	     (2.0*x                  - x0)                    /x0,
// 	     (y0                     - 2.0*y)                 /y0,
// 	     1.0);
   trackball(spin_quat,  0.0, 1.0, x_diff, 1.0, 0.5); 
   add_quats(spin_quat, g.quat, g.quat);
   g.graphics_draw();
}

void 
do_ztrans_and_clip(gdouble x, gdouble y) { 

   graphics_info_t g;
   gdouble x_diff = x - g.GetMouseBeginX(); 
   gdouble y_diff = y - g.GetMouseBeginY();

   coot::Cartesian v = screen_z_to_real_space_vector(graphics_info_t::glarea);

//    // Like Frank showed me in Pymol?
//    double slab_change = 0.05  * (x_diff + y_diff); // about 0.2?
//    double ztr_change  = 0.005 * (x_diff - y_diff);

   // I prefer straight left/right and up and down:
   // (Up and down move the individual clipping planes in PyMol.
   // I'm not keen on allowing that in Coot).
   double slab_change = 0.02  * y_diff; // about 0.2?
   double ztr_change  = 0.001 * x_diff;

   // Do *either* z-trans or clipping, not both.  Doing both confuses
   // me.
     
   v *= ztr_change;
   if (fabs(x_diff) > fabs(y_diff))
      g.add_vector_to_RotationCentre(v);
   else 
      adjust_clipping(slab_change);

   g.graphics_draw();
   
}

void
adjust_clipping(double d) {

   if (d>0) {
      if (graphics_info_t::clipping_back < 15.0) { 
	 set_clipping_front(graphics_info_t::clipping_front + d);
	 set_clipping_back (graphics_info_t::clipping_front + d);
      }
   } else {
      if (graphics_info_t::clipping_back > -15.2) { 
	 set_clipping_front(graphics_info_t::clipping_front + d);
	 set_clipping_back (graphics_info_t::clipping_front + d);
      }
   }
}


gint key_press_event(GtkWidget *widget, GdkEventKey *event)
{

   // Try to correct cntrl and shift anomalies:
   //
   // 
   if (event->state & GDK_CONTROL_MASK)
      graphics_info_t::control_is_pressed = 1;
   else
      graphics_info_t::control_is_pressed = 0;
   if (event->state & GDK_SHIFT_MASK)
      graphics_info_t::shift_is_pressed = 1;
   else
      graphics_info_t::shift_is_pressed = 0;

   gint handled = 0; // initially unhandled

   switch (event->keyval) {
   case GDK_Control_L:
   case GDK_Control_R:

//       std::cout << "DEBUG ctrl key press  : graphics_info_t::control_is_pressed "
// 		<< graphics_info_t::control_is_pressed
// 		<< " graphics_info_t::pick_pending_flag "
// 		<< graphics_info_t::pick_pending_flag << std::endl;
      
      graphics_info_t::control_is_pressed = 1; // TRUE.
      if (graphics_info_t::in_edit_chi_mode_flag)
	 graphics_info_t::in_edit_chi_mode_view_rotate_mode = 1;
      if (graphics_info_t::in_edit_torsion_general_flag)
	 graphics_info_t::in_edit_chi_mode_view_rotate_mode = 1;

      if (graphics_info_t::control_key_for_rotate_flag) {
	 normal_cursor();
      } else {
	 // control key is for pick
	 if (graphics_info_t::pick_pending_flag) {
	    pick_cursor_maybe();
	 }
      }
      handled = TRUE; 
      break;
      
   case GDK_Alt_L:
   case GDK_Alt_R:
   case GDK_Meta_L:
   case GDK_Meta_R:

      handled = TRUE; // stops ALT key getting through to key-press-hook
      break;

   case GDK_Shift_L: // stops Shift key getting through to key-press-hook
   case GDK_Shift_R:

      handled = TRUE;
      break;
      
   case GDK_Return:

      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 if (graphics_info_t::go_to_residue_keyboarding_string != "")
	    graphics_info_t::apply_go_to_residue_keyboading_string();
      } else {
	 if (graphics_info_t::accept_reject_dialog) {
	    accept_regularizement();
	    if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG) {
	       save_accept_reject_dialog_window_position(graphics_info_t::accept_reject_dialog);
	       gtk_widget_destroy(graphics_info_t::accept_reject_dialog);
	    } else {
          // have docked dialog
          if (graphics_info_t::accept_reject_dialog_docked_show_flag == coot::DIALOG_DOCKED_HIDE) {
            gtk_widget_hide(graphics_info_t::accept_reject_dialog);
          } else {
            gtk_widget_set_sensitive(graphics_info_t::accept_reject_dialog, FALSE);
          }
	    }
	    graphics_info_t::accept_reject_dialog = 0;
	 }
      }
      graphics_info_t::go_to_residue_keyboarding_string = ""; // reset.
      graphics_info_t::in_go_to_residue_keyboarding_mode = 0;
      handled = TRUE;
      break;

   case GDK_Escape:

      std::cout << "GDK_Escape pressed" << std::endl;

      clear_up_moving_atoms();
      
      if (graphics_info_t::accept_reject_dialog) {
	 if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG) {
	   save_accept_reject_dialog_window_position(graphics_info_t::accept_reject_dialog);
	   gtk_widget_destroy(graphics_info_t::accept_reject_dialog);
       graphics_info_t::accept_reject_dialog = 0;
	 } else {
	   gtk_widget_set_sensitive(graphics_info_t::accept_reject_dialog, FALSE);
	 }
      }
      
      graphics_info_t::go_to_residue_keyboarding_string = ""; // reset.
      graphics_info_t::in_go_to_residue_keyboarding_mode = 0;
      handled = TRUE;
      break;

   case GDK_space:
      handled = TRUE; 
      break; // stops Space key getting through to key-press-hook
      
   case GDK_g:
      // say I want to go to residue 1G: first time Ctrl-G (second if)
      // and then the first if.
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "G";
	 handled = TRUE;
      }
      if (graphics_info_t::control_is_pressed) {
	 graphics_info_t::in_go_to_residue_keyboarding_mode = 1;
	 handled = TRUE;
      }
      break;
      
   case GDK_i:
      // throw away i key pressed (we act on i key released).
      if (! graphics_info_t::control_is_pressed) { 
	 if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	    graphics_info_t::go_to_residue_keyboarding_string += "i";
	 }
      }
      handled = TRUE; 
      break;

   case GDK_a:

      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "A";
	 handled = TRUE;
      } else { 

	 if (graphics_info_t::in_range_define_for_refine == 2) {
	    
	    graphics_info_t::a_is_pressed = 1;

	    int rot_trans_rotation_origin_atom = 0; // flag for Ctrl left
	    // mouse behaviour (we don't want to rotate
	    // the atoms)
	    graphics_info_t::watch_cursor();
	    int auto_range_flag = 1;
	    graphics_info_t g;
	    g.refine(graphics_info_t::residue_range_mol_no,
		     auto_range_flag,
		     graphics_info_t::residue_range_atom_index_1,
		     graphics_info_t::residue_range_atom_index_1);
	    g.in_range_define_for_refine = 0;
	    normal_cursor();
	    g.pick_pending_flag = 0;
	    g.model_fit_refine_unactive_togglebutton("model_refine_dialog_refine_togglebutton");
	    handled = TRUE; 
	 }
      }
      break;

   case GDK_b:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "B";
	 handled = TRUE; 
      }
      break;
      
   case GDK_c:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "C";
	 handled = TRUE; 
      }
      break; // stops C key getting through to key-press-hook

   case GDK_u:
      undo_last_move();
      handled = TRUE; 
      break;

   case GDK_s:
      if (graphics_info_t::control_is_pressed) {
	 quick_save();
	 handled = TRUE;
      }
      break;

   case GDK_d:
      
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "D";
	 handled = TRUE; 
      } else { 
	 
	 if (graphics_info_t::clipping_back < 10.0) { 
	    set_clipping_front(graphics_info_t::clipping_front + 0.2);
	    set_clipping_back (graphics_info_t::clipping_front + 0.2);
	    // std::cout << graphics_info_t::clipping_front << " " << graphics_info_t::clipping_back << std::endl;
	 }
      }
      handled = TRUE; 
      break;

   case GDK_e:
   case GDK_E:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "E";
	 handled = TRUE; 
      }
      break;
      
   case GDK_f:
      
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "E";
	 handled = TRUE; 
      } else { 
	 if (graphics_info_t::clipping_back > -10.2) { 
	    set_clipping_front(graphics_info_t::clipping_front - 0.2);
	    set_clipping_back (graphics_info_t::clipping_front - 0.2);
	    // std::cout << graphics_info_t::clipping_front << " " << graphics_info_t::clipping_back << std::endl;
	 }
      }
      handled = TRUE; 
      break;

   case GDK_n:
      
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "n";
      } else { 
	 for (int i=0; i<5; i++) {
	    graphics_info_t::zoom *= 1.01;
	    graphics_info_t::graphics_draw();
	 }
      }
      handled = TRUE; 
      break;

   case GDK_m:

      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "m";
      } else { 
	 for (int i=0; i<5; i++) {
	    graphics_info_t::zoom *= 0.99;
	    graphics_info_t::graphics_draw();
	 }
      }
      handled = TRUE; 
      break;

   case GDK_1:
   case GDK_KP_1:

      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "1";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 1;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 }
      }
      handled = TRUE; 
      break;
   case GDK_2:
   case GDK_KP_2:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "2";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 2;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 }
      }
      handled = TRUE; 
      break;
   case GDK_3:
   case GDK_KP_3:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "3";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 3;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 } else { 
	    keypad_translate_xyz(3, 1);
	 }
      }
      handled = TRUE; 
      break;
   case GDK_4:
   case GDK_KP_4:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "4";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 4;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 }
      }
      handled = TRUE; 
      break;
   case GDK_5:
   case GDK_KP_5:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "5";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 5;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 }
      }
      handled = TRUE; 
      break;
   case GDK_6:
   case GDK_KP_6:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "6";
      } else { 
	 if (graphics_info_t::moving_atoms_move_chis_flag) { 
	    graphics_info_t::edit_chi_current_chi = 6;
	    graphics_info_t::in_edit_chi_mode_flag = 1; // on
	 }
      }
      handled = TRUE; 
      break;

   case GDK_7:
   case GDK_KP_7:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "7";
      }
      handled = TRUE;
      break;
      
   case GDK_8:
   case GDK_KP_8:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "8";
      }
      handled = TRUE;
      break;
      
   case GDK_9:
   case GDK_KP_9:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "9";
      }
      handled = TRUE;
      break;
      
   case GDK_0:
   case GDK_KP_0:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "0";
      } else { 
	 graphics_info_t::edit_chi_current_chi = 0;
	 graphics_info_t::in_edit_chi_mode_flag = 0; // off
      }
      handled = TRUE; 
      break;
      
   case GDK_l:
      {
	 if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	    graphics_info_t::go_to_residue_keyboarding_string += "l";
	 } else { 
	    graphics_info_t g;
	    std::pair<int, int> cl_at = g.get_closest_atom();
	    if (is_valid_model_molecule(cl_at.second)) {
	       g.molecules[cl_at.second].add_to_labelled_atom_list(cl_at.first);
           // shall add to status_bar ? maybe this should be a function?
           // it is now
           std::string ai;
           ai = atom_info_as_text_for_statusbar(cl_at.first, cl_at.second);
           g.statusbar_text(ai);
           // insert end
	    }
	    graphics_info_t::graphics_draw();
	 }
      }
      handled = TRUE; 
      break;

   case GDK_z:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "z";
      } else { 
	 graphics_info_t::z_is_pressed = 1;
	 if (graphics_info_t::control_is_pressed) { 
	    if (graphics_info_t::draw_baton_flag)
	       baton_build_delete_last_residue();
	    else 
	       apply_undo();
	    handled = TRUE; 
	 }
      }
      break;

   case GDK_y:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 graphics_info_t::go_to_residue_keyboarding_string += "y";
      } else { 
	 graphics_info_t::y_is_pressed = 1;
	 if (graphics_info_t::control_is_pressed) { 
	    apply_redo();
	    handled = TRUE; 
	 }
      }
      break;

   case GDK_F5:
      post_model_fit_refine_dialog();
      handled = TRUE; 
      break;
   case GDK_F6:
      post_go_to_atom_window();
      handled = TRUE; 
      break;
   case GDK_F7:
      post_display_control_window();
      handled = TRUE; 
      break;
   case GDK_F8:
      // case GDK_3270_PrintScreen: // that the gnome screenshot 
      raster_screen_shot();
      handled = TRUE; 
      break;
   case GDK_KP_Down:
   case GDK_KP_Page_Down:
      keypad_translate_xyz(3, 1);
      handled = TRUE; 
      break;
   case GDK_KP_Up:
   case GDK_KP_Page_Up:
      keypad_translate_xyz(3, 1);
      handled = TRUE; 
      break;
   case GDK_KP_Decimal:
   case GDK_KP_Delete:
      keypad_translate_xyz(3, -1);
      handled = TRUE; 
      break;

   case GDK_period:
      // std::cout << "Got a .\n";
      if (graphics_info_t::rotamer_dialog) {
	 // We have to make a synthetic keypress on the "next" rotamer.
	 // So, what is the next rotamer (bear in mind wrapping).
	 graphics_info_t::rotamer_dialog_next_rotamer();
      } else {
	 if (graphics_info_t::difference_map_peaks_dialog) {
	    graphics_info_t::difference_map_peaks_next_peak();
	 } else {
	    if (graphics_info_t::checked_waters_baddies_dialog) {
	       graphics_info_t::checked_waters_next_baddie(+1);
	    } else { 
#ifdef USE_GUILE
	       std::string scheme_command("(graphics-dot-key-pressed-hook)");
	       safe_scheme_command(scheme_command);
#endif // USE_GUILE
#ifdef USE_PYTHON
	       std::string python_command("graphics_dot_key_pressed_hook()");
	       safe_python_command(python_command);   
#endif // PYTHON
	    }
	 }
      }
      handled = TRUE; 
      break;
      
   case GDK_comma:
      //       std::cout << "Got an ,\n";
      if (graphics_info_t::rotamer_dialog) {
	 graphics_info_t::rotamer_dialog_previous_rotamer();
      } else {
	 if (graphics_info_t::difference_map_peaks_dialog) {
	    graphics_info_t::difference_map_peaks_previous_peak();
	 } else { 
	    if (graphics_info_t::checked_waters_baddies_dialog) {
	       graphics_info_t::checked_waters_next_baddie(-1); // prev
	    } else { 
#ifdef USE_GUILE
	       std::string scheme_command("(graphics-comma-key-pressed-hook)");
	       safe_scheme_command(scheme_command);
#endif // USE_GUILE
#ifdef USE_PYTHON
	       std::string python_command("graphics_comma_key_pressed_hook()");
	       safe_python_command(python_command);   
#endif // USE_PYTHON
	    }
	 }
      }
      handled = TRUE; 
      break;
      
   case GDK_minus:
      handled = TRUE; 
      break;
   case GDK_plus:
      handled = TRUE; 
      break;
   case GDK_equal:
      handled = TRUE; 
      break;

   case GDK_Left:
      if (graphics_info_t::control_is_pressed) {
	 graphics_info_t::nudge_active_residue(GDK_Left);
	 handled = TRUE;
	 break;
      } 
   case GDK_Right:
      if (graphics_info_t::control_is_pressed) {
	 graphics_info_t::nudge_active_residue(GDK_Right);
	 handled = TRUE;
	 break;
      } 
   case GDK_Up:
      if (graphics_info_t::control_is_pressed) {
	 graphics_info_t::nudge_active_residue(GDK_Up);
	 handled = TRUE;
	 break;
      } 
   case GDK_Down:
      if (graphics_info_t::control_is_pressed) {
	 graphics_info_t::nudge_active_residue(GDK_Down);
	 handled = TRUE;
	 break;
      }
      
   }

   // Now to test event->keyval against dynamic "reprogramable" key
   // bindings.

   if (handled == 0) { // initial value
      if (event->keyval == graphics_info_t::ncs_next_chain_skip_key) {
	 if (graphics_info_t::prefer_python) { 
#if defined USE_PYTHON
	    std::string python_command("skip_to_next_ncs_chain('forward')");
	    safe_python_command(python_command);
#endif // USE_PYTHON
	 } else { 
#if defined USE_GUILE
	    std::string scheme_command("(skip-to-next-ncs-chain 'forward)");
	    safe_scheme_command(scheme_command);
#endif // USE_GUILE
	 }
	 handled = TRUE;
      }

      if (event->keyval == graphics_info_t::ncs_prev_chain_skip_key) {
	 if (graphics_info_t::prefer_python) { 
#if defined USE_PYTHON
	    std::string python_command("skip_to_next_ncs_chain('backward')");
	    safe_python_command(python_command);
#endif // USE_PYTHON
	 } else { 
#if defined USE_GUILE
	    std::string scheme_command("(skip-to-next-ncs-chain 'backward)");
	    safe_scheme_command(scheme_command);
#endif // USE_GUILE
	 }
	 handled = TRUE;
      }

      
      if (event->keyval == graphics_info_t::update_go_to_atom_from_current_residue_key) {
	 update_go_to_atom_from_current_position();
	 handled = TRUE;
      }

      if (handled == 0) { 
	 if (event->keyval != GDK_backslash) {
	    int ikey = event->keyval;

	    if (graphics_info_t::prefer_python) {
#ifdef USE_PYTHON
#endif 	       
	    } else {
#ifdef USE_GUILE
#endif 	       
	    } 

#if defined USE_GUILE && !defined WINDOWS_MINGW
	    std::string scheme_command("(graphics-general-key-press-hook ");
	    // scheme_command += "\"";
	    scheme_command += graphics_info_t::int_to_string(ikey);
	    // scheme_command += "\"";
	    scheme_command += ")";
	    // std::cout << "running scheme command: " << scheme_command << std::endl;
	    safe_scheme_command(scheme_command);
#else // not GUILE
#ifdef USE_PYTHON	    
	    std::string python_command("graphics_general_key_press_hook(");
	    python_command += graphics_info_t::int_to_string(ikey);
	    python_command += ",";
	    python_command += graphics_info_t::int_to_string(graphics_info_t::control_is_pressed);
	    python_command += ")";
	    safe_python_command(python_command);
#endif // USE_PYTHON	    
#endif // USE_GUILE
	    
	 } else {
	    std::cout << "Ignoring GDK_backslash key press event" << std::endl;
	 }
      }
   } 
   return TRUE;
}

// Direction is either +1 or -1 (in or out)
// 
void keypad_translate_xyz(short int axis, short int direction) { 

   coot::Cartesian v = screen_z_to_real_space_vector(graphics_info_t::glarea);
   v *= 0.05 * float(direction);
   graphics_info_t g;
   g.add_vector_to_RotationCentre(v);

} 


gint key_release_event(GtkWidget *widget, GdkEventKey *event)
{

   // try to correct cntrl and shift anomalies:
   // 
   if (event->state & GDK_CONTROL_MASK)
      graphics_info_t::control_is_pressed = 1;
   else
      graphics_info_t::control_is_pressed = 0;
   if (event->state & GDK_SHIFT_MASK)
      graphics_info_t::shift_is_pressed = 1;
   else
      graphics_info_t::shift_is_pressed = 0;

   // we are getting key release events as the key is pressed (but not
   // released) :-).
   // 
   // std::cout << "key release event" << std::endl;
   graphics_info_t g; 
   int s = graphics_info_t::scroll_wheel_map;
   if (s < graphics_info_t::n_molecules()) { 
      if (s >= 0) { 
	 if (! graphics_info_t::molecules[s].has_map()) {
	    s = -1; // NO MAP
	 }
      }
   } else {
      s = -1; // NO MAP
   }
   
   
   std::vector<int> num_displayed_maps = g.displayed_map_imols();
   if (num_displayed_maps.size() == 1) 
      s = num_displayed_maps[0];
   short int istate = 0;
   
   switch (event->keyval) {
   case GDK_Control_L:
   case GDK_Control_R:

//       std::cout << "DEBUG ctrl key release: graphics_info_t::control_is_pressed "
// 		<< graphics_info_t::control_is_pressed
// 		<< " graphics_info_t::pick_pending_flag "
// 		<< graphics_info_t::pick_pending_flag << std::endl;
      
      graphics_info_t::control_is_pressed = 0; // FALSE.
      if (graphics_info_t::in_edit_chi_mode_flag)
	 graphics_info_t::in_edit_chi_mode_view_rotate_mode = 0;
      if (graphics_info_t::in_edit_torsion_general_flag)
	 graphics_info_t::in_edit_chi_mode_view_rotate_mode = 0;

      if (graphics_info_t::control_key_for_rotate_flag) {
	 if (graphics_info_t::pick_pending_flag) {
	    pick_cursor_maybe();
	 } else {
	    normal_cursor();
	 }
      } else {
	 // control key is for pick
	 if (graphics_info_t::pick_pending_flag) {
	    normal_cursor();
	 } else {
	    pick_cursor_maybe();
	 }
      }


      // When we don't have active map dragging, we want to renew the map at
      // control release, not anywhere else.
      //
      for (int ii=0; ii<graphics_info_t::n_molecules(); ii++) { 
	 graphics_info_t::molecules[ii].update_map();
	 graphics_info_t::molecules[ii].update_clipper_skeleton(); 
      }
      g.make_pointer_distance_objects();
      g.graphics_draw();
      break;
   case GDK_minus:
      //
      // let the object decide which level change it needs (and if it needs it)
      // using graphics_info_t static members
      if (s >= 0) {

	 istate = graphics_info_t::molecules[s].change_contour(-1);
	 
	 if (istate)
	    graphics_info_t::molecules[s].update_map();
// 	 std::cout << "contour level of molecule [" << s << "]:  "
// 		   << graphics_info_t::molecules[s].contour_level[0] << std::endl;
	 
	 g.set_density_level_string(s, g.molecules[s].contour_level[0]);
	 g.display_density_level_this_image = 1;
	 
	 g.graphics_draw();
      } else {
	 std::cout << "WARNING: No map - Can't change contour level.\n";
      }
      break;
   case GDK_plus:
   case GDK_equal:  // unshifted plus, usually.
      //
      

      // let the object decide which level change it needs:
      //
      if (s >= 0) { 
	 graphics_info_t::molecules[s].change_contour(1); // positive change
      
	 graphics_info_t::molecules[s].update_map();
// 	 std::cout << "contour level of molecule [" << s << "]:  "
// 		   << graphics_info_t::molecules[s].contour_level[0] << std::endl;

	 g.set_density_level_string(s, g.molecules[s].contour_level[0]);
	 g.display_density_level_this_image = 1;

	 g.graphics_draw();
      } else {
	 std::cout << "WARNING: No map - Can't change contour level.\n";
      }
      break;
      
   case GDK_A:
   case GDK_a:
      graphics_info_t::a_is_pressed = 0;
      break;

   case GDK_B:
   case GDK_b:
      // Only toggle baton mode if we are showing a baton!
      // (Otherwise confusion reigns!)
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 // do nothing B was added on key press
      } else { 
	 if (g.draw_baton_flag)
	    g.toggle_baton_mode();
      }
      break;
      
   case GDK_c:
   case GDK_C:
      if (graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 // do nothing C was added on key press
      } else { 
	 g.draw_crosshairs_flag = 1 - g.draw_crosshairs_flag; 
	 g.crosshairs_text();
	 g.graphics_draw();
      }
      break;

   case GDK_s:
   case GDK_S:
      if (graphics_info_t::control_is_pressed) {
	 // nothing yet
      } else {
	 // as it used to be
	 for (int ii = 0; ii< graphics_info_t::n_molecules(); ii++)
	    graphics_info_t::molecules[ii].update_clipper_skeleton();
	 g.graphics_draw();
      }
      break;
      
   case GDK_i:
   case GDK_I:
      if (! graphics_info_t::in_go_to_residue_keyboarding_mode) {
	 // toggle the idle function using the I key.
	 //
	 if (! graphics_info_t::control_is_pressed) 
	    toggle_idle_spin_function();
      }
      break; 
      
   case GDK_l:
   case GDK_L:
   case GDK_Shift_L:
   case GDK_Shift_R:
      // something here, L is released.
      break;

   case GDK_z:
   case GDK_Z:
      graphics_info_t::z_is_pressed = 0;
      break;

   case GDK_y:
   case GDK_Y:
      graphics_info_t::y_is_pressed = 0;
      break;

   case GDK_space:
      // go to next residue
//       int next = 1;
//       if (graphics_info_t::shift_is_pressed == 1) 
// 	 next = -1;  // i.e. previous
      
      // std::cout << "got a space acting on " << g.go_to_atom_chain()
// 		<< " " << g.go_to_atom_residue()+ next
// 		<<  " " << g.go_to_atom_atom_name() << std::endl;

      // commented 030805:
//       set_go_to_atom_chain_residue_atom_name(g.go_to_atom_chain(),
// 					     g.go_to_atom_residue()+next,
// 					     g.go_to_atom_atom_name());

      if (graphics_info_t::shift_is_pressed) {
	 g.intelligent_previous_atom_centring(g.go_to_atom_window);
      } else {
	 g.intelligent_next_atom_centring(g.go_to_atom_window);
      }
      break;
   }
   
   /* prevent the default handler from being run */
   gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_release_event");
   return TRUE;

  return TRUE;
}

// widget is the glarea.
// 
gint
animate_idle_spin(GtkWidget *widget) {

   float spin_quat[4];
   graphics_info_t g;

   // spin it 1 degree * user angle setting
   trackball(spin_quat, 0, 0, g.idle_function_rotate_angle*0.0174,
	     0.0, g.get_trackball_size());
   add_quats(spin_quat, g.quat, g.quat);

   g.graphics_draw();

   return 1; 
}

// widget is the glarea.
// 
gint
animate_idle_rock(GtkWidget *widget) {

   graphics_info_t g; 
   double target_angle = get_idle_function_rock_target_angle();
   double curr_angle = g.idle_function_rock_angle_previous;

   double angle_diff = target_angle - curr_angle;

   if (0) 
      std::cout << "target_angle: " << target_angle
		<< "   curr_angle: " << curr_angle
		<< "   angle_diff: " << angle_diff
		<< std::endl;

   // we don't need to see every angle - that is fine-grained and
   // full-on, we just need to do an animation where the angle
   // difference is a big bigger than tiny, otherwise sleep.
   
   if (fabs(angle_diff) > 0.0004) { 

      float spin_quat[4];
      trackball(spin_quat, 0, 0, angle_diff, 0.0, g.get_trackball_size());
      add_quats(spin_quat, g.quat, g.quat);
      g.graphics_draw();
      g.idle_function_rock_angle_previous = target_angle; // for next round

   } 
   return 1;  // keep going
}

double
get_idle_function_rock_target_angle() {

   graphics_info_t g;

   long t = glutGet(GLUT_ELAPSED_TIME);
   long delta_t = t - g.time_holder_for_rocking;
   double rock_sf = 0.001 * g.idle_function_rock_freq_scale_factor;

   double theta = delta_t * rock_sf;

   while (theta > 2 * M_PI)
      theta -= 2*M_PI;
   while (theta < -2 * M_PI)
      theta += 2*M_PI;   

   double target_angle = g.idle_function_rock_amplitude_scale_factor *
      0.015 * 2 * M_PI * sin(theta);

   return target_angle;
}


// widget is the glarea.
// 
gint
animate_idle_ligand_interactions(GtkWidget *widget) {

   graphics_info_t g;
   for (int imol=0; imol<graphics_n_molecules(); imol++) {
      if (is_valid_model_molecule(imol)) {
	 if (g.molecules[imol].is_displayed_p()) {
	    g.molecules[imol].draw_animated_ligand_interactions_flag = 1;
	 }
      }
   }
   g.graphics_draw();
   return 1; // don't stop calling this idle function
}


void
draw_axes(GL_matrix &m) {

   graphics_info_t g;

   
   float mat_p[16];
   
   if (g.draw_axes_flag) {

      glPushMatrix();
      glMultMatrixf(m.transpose().get());
      glTranslatef(-0.88, +0.85, 0);
      glMultMatrixf(m.get());
      if (graphics_info_t::use_axes_orientation_matrix_flag)
	 glMultMatrixf(graphics_info_t::axes_orientation.get());

      glMatrixMode(GL_MODELVIEW);
      glGetFloatv(GL_MODELVIEW_MATRIX, mat_p);
      glPushMatrix();
      glLoadIdentity();

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();

      glLoadIdentity();
      
      glTranslatef(0.1, 0.1, 0.1);
      
      glLoadMatrixf(mat_p);


      glScalef(0.4, 0.4, 0.4);
      glLineWidth(1.0);
      
//       std::cout << endl;
//       std::cout << "( " << mat_p[0] << " " << mat_p[1]
// 		<< " " <<  mat_p[2] << " " << mat_p[3] << " )\n";
//       std::cout << "( " << mat_p[4] << " " << mat_p[5]
// 		<< " " <<  mat_p[6] << " " << mat_p[7] << " )\n";
//       std::cout << "( " << mat_p[8] << " " << mat_p[9]
// 		<< " " <<  mat_p[10] << " " << mat_p[11] << " )\n";
//       std::cout << "( " << mat_p[12] << " " << mat_p[13]
// 		<< " " <<  mat_p[14] << " " << mat_p[15] << " )\n";
//       std::cout << endl;
	 
      GLfloat col[3] = { 0.55, 0.7, 0.55 };
      glColor3fv(col);

      glBegin(GL_LINES);

      // axes
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.2, 0.0, 0.0);

      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, 0.2, 0.0);

      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, 0.0, 0.2);

      // arrowheads:
      glVertex3f(0.2,  0.0,  0.0);
      glVertex3f(0.18, 0.02, 0.0);
      glVertex3f(0.2,  0.0,  0.0);
      glVertex3f(0.18, -0.02, 0.0);

      glVertex3f(0.0,  0.2,   0.0);
      glVertex3f(0.0,  0.18,  0.02);
      glVertex3f(0.0,  0.2,   0.0);
      glVertex3f(0.0,  0.18, -0.02);

      glVertex3f(0.0,   0.0,   0.2);
      glVertex3f(0.02,  0.0,   0.18);
      glVertex3f(0.0,   0.0,   0.2);
      glVertex3f(-0.02,  0.0,   0.18);
      
      glEnd();

      glRasterPos3f(0.22, 0.0, 0.0);
      printString("x");
      glRasterPos3f(0.0, 0.22, 0.0);
      printString("y");
      glRasterPos3f(0.0, 0.0, 0.22);
      printString("z");

      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glPopMatrix();
   }
}



float r_50(std::string ele) {

   // fix later
   //
   return 1.2;
} 

float rad_50_and_prob_to_radius(float rad_50, float prob) {

   // Note the prob*(prob-100.0) part is scaled up by a factor of
   // 100*100... and the (50-prob) is scaled by a factor of 100, so
   // multiply the top by 100.0 to compensate.
   //
   // The leading factor is to stop the radius moving too fast.
   // 
   return rad_50*exp(0.3*100.0*(50.0-prob)/(prob*(prob-100.0)));

}

// This function is way too long.  It needs to be split it into
// internal bits.
// 
gint glarea_button_press(GtkWidget *widget, GdkEventButton *event) {
   
   graphics_info_t info; // declared at the top of the file
   
   int x_as_int, y_as_int;
   GdkModifierType state;
   gdk_window_get_pointer(event->window, &x_as_int, &y_as_int, &state);

   info.SetMouseBegin(event->x, event->y);

   GdkModifierType my_button1_mask = info.gdk_button1_mask();
   GdkModifierType my_button2_mask = info.gdk_button2_mask();
   GdkModifierType my_button3_mask = info.gdk_button3_mask();

   //cout << "mouse button " << event->button << " was pressed at ("
   //<< event->x << "," << event->y << ")" << endl;

   // cout << gtk_events_pending() << " events pending..." << endl;

   // On refinement atom selection it seems double clicks generate an
   // extra event (i.e. we get 3 click events of which the last one
   // seems to be a synthetic double click event).  Let's bin the last
   // event.
   // 
   if (event->type==GDK_2BUTTON_PRESS ||
       event->type==GDK_3BUTTON_PRESS) { 
//       printf("INFO:: Ignoring this %s click on button %d\n",
// 	     event->type==GDK_2BUTTON_PRESS ? "double" : "triple", 
// 	     event->button);

      if (state & my_button1_mask) {
	 // instead do a label atom:
	 pick_info nearest_atom_index_info = atom_pick(event);
	 if ( nearest_atom_index_info.success == GL_TRUE ) {
	    int im = nearest_atom_index_info.imol;
	    graphics_info_t g;
	    g.molecules[im].add_to_labelled_atom_list(nearest_atom_index_info.atom_index);
	 } else {
	    if (graphics_info_t::show_symmetry) {
	       coot::Symm_Atom_Pick_Info_t symm_atom_info = info.symmetry_atom_pick();
	       if (symm_atom_info.success == GL_TRUE) {
		  info.molecules[symm_atom_info.imol].add_atom_to_labelled_symm_atom_list(symm_atom_info.atom_index,
											  symm_atom_info.symm_trans,
											  symm_atom_info.pre_shift_to_origin);
	       }
	    }
	 }
	 graphics_draw();
      }
      return TRUE;
   }
   



   // if (event->button == 1) {
   if (state & my_button1_mask) {

      if (info.shift_is_pressed == 1) {
 
	 // label atom
	 // 
	 pick_info nearest_atom_index_info; 
	 nearest_atom_index_info = atom_pick(event);
	       
	 if ( nearest_atom_index_info.success == GL_TRUE ) {
	    
	    int im = nearest_atom_index_info.imol; 
	    info.molecules[im].add_to_labelled_atom_list(nearest_atom_index_info.atom_index);
	    info.graphics_draw();

	 } else {

	    // we should only try to pick symmetry atoms if they are being
	    // displayed.
	    // (note we are still in shift left mouse mode)

	    if ( info.show_symmetry == 1 ) {
	    
	       std::cout << "Trying symmetry label\n";
	       coot::Symm_Atom_Pick_Info_t symm_atom_info = info.symmetry_atom_pick();
	       
	       if (symm_atom_info.success == GL_TRUE) {
		  
		  info.molecules[symm_atom_info.imol].add_atom_to_labelled_symm_atom_list(symm_atom_info.atom_index,
											  symm_atom_info.symm_trans,
											  symm_atom_info.pre_shift_to_origin);
		  info.graphics_draw();
		  
		  // need equivalent for this?
		  // clear_symm_atom_info(symm_atom_info);

	       } else {
		  std::cout << "Symmetry label failed\n";
	       }
	    }
	 }

      } else { 

	 // Left mouse, but not shift-left-mouse:

	 // (this is the conventional case)
	 // 
	 // and if so, do the regularization or refinement
	 // or angle and distance geometries.
	 //
	 int iv = info.check_if_in_range_defines(event, state);
	 if (! iv) 
	    info.check_if_moving_atom_pull(); // and if so, set it up (it
	 
	 // executes on *motion* not a button press event).  Also,
	 // remove any on-going drag-refine-idle-function.
	 //
	 // check_if_moving_atom_pull sets
	 // in_moving_atoms_drag_atom_mode_flag.
	 
      }  // shift is pressed
   }     // button 1

   
   if (state & my_button2_mask) {

      // Atom picking (recentre)
      
      pick_info nearest_atom_index_info; 
      nearest_atom_index_info = atom_pick(event);
      
      if ( nearest_atom_index_info.success == GL_TRUE) {

	 int im = nearest_atom_index_info.imol; 
	 std::cout << "INFO:: recentre: clicked on imol: " << im << std::endl;
	 
	 info.setRotationCentre(nearest_atom_index_info.atom_index,
				nearest_atom_index_info.imol);

	 // Lets display the coordinate centre change
	 // *then* update the map, so we can see how fast
	 // the map updating is.
	 // 
	 info.graphics_draw();

	 info.post_recentre_update_and_redraw();

      } else {

	 std::cout << "Model atom pick failed. " << std::endl; 

	 // Only try to pick symmetry atoms if the symmetry atoms
	 // are being displayed. 
	 
	 if ( info.show_symmetry == 1 ) {
	    
	    std::cout << "Trying symmetry pick" << std::endl;
	    coot::Symm_Atom_Pick_Info_t symm_atom_info = info.symmetry_atom_pick();
	    if (symm_atom_info.success == GL_TRUE) {
	    
	       std::cout << "Found Symmetry atom pick" << std::endl;

	       // info.setRotationCentre(symm_atom_info.Hyb_atom());
	       std::pair<symm_trans_t, Cell_Translation> symtransshiftinfo(symm_atom_info.symm_trans,
									   symm_atom_info.pre_shift_to_origin);
	       info.setRotationCentre(translate_atom_with_pre_shift(info.molecules[symm_atom_info.imol].atom_sel, 
								    symm_atom_info.atom_index,
								    symtransshiftinfo));

	       
	       // clear_symm_atom_info(symm_atom_info);

	       for (int ii=0; ii<info.n_molecules(); ii++) {
		  info.molecules[ii].update_symmetry();
	       }
	       info.graphics_draw();
	       for (int ii=0; ii<info.n_molecules(); ii++) {
		  info.molecules[ii].update_clipper_skeleton();
		  info.molecules[ii].update_map();
	       }
	       info.graphics_draw();

	    } else {

	       std::cout << "Symmetry atom pick failed." << std::endl;

	    }
	 }
      }
   }
   
   if (event->button == 4) {
      handle_scroll_density_level_event(1); // up
   }
   
   if (event->button == 5) { 
      handle_scroll_density_level_event(0); // down
   }
   return TRUE;
}

gint glarea_button_release(GtkWidget *widget, GdkEventButton *event) {

   if (graphics_info_t::in_moving_atoms_drag_atom_mode_flag) {
      graphics_info_t g;
      g.do_post_drag_refinement_maybe();
   }
   graphics_info_t::in_moving_atoms_drag_atom_mode_flag = 0;
   return TRUE;
}

// Francois says to remove this:
// 
// #if defined(WINDOWS_MINGW) || defined(_MSC_VER) 

// gint glarea_scroll_event(GtkWidget *widget, GdkEventScroll *event) {

//    if (event->direction == SCROLL_UP)
//       handle_scroll_event(1);
//    if (event->direction == SCROLL_DOWN)
//       handle_scroll_event(0);
//    return TRUE;
// } 
// #endif


#if (GTK_MAJOR_VERSION > 1)
gint glarea_scroll_event(GtkWidget *widget, GdkEventScroll *event) {

   graphics_info_t info;
   bool handled = 0;
   if (info.control_is_pressed) {
      if (info.shift_is_pressed) {
	 if (event->direction == GDK_SCROLL_UP)
	    change_model_molecule_representation_mode(1); // up
	 if (event->direction == GDK_SCROLL_DOWN)
	    change_model_molecule_representation_mode(0); // up
	 handled = 1;
      }
   }

   if (! handled) {
      if (event->direction == GDK_SCROLL_UP)
	 handle_scroll_density_level_event(1);
      if (event->direction == GDK_SCROLL_DOWN)
	 handle_scroll_density_level_event(0);
   } 
   return TRUE;
}
#endif

void handle_scroll_density_level_event(int scroll_up_down_flag) {

   graphics_info_t info;
   
   if (scroll_up_down_flag == 1) {
      //
      // consider using
      // change_contour_level(1); // is_increment

      if (graphics_info_t::do_scroll_by_wheel_mouse_flag) { 

	 int s = info.scroll_wheel_map;

	 std::vector<int> num_displayed_maps = info.displayed_map_imols();
	 if (num_displayed_maps.size() == 1) 
	    s = num_displayed_maps[0];

	 if (s >= 0) { // i.e. not as yet unassigned 
	    short int istate = info.molecules[s].change_contour(1);

	    if (istate) { 
	       info.set_density_level_string(s, info.molecules[s].contour_level[0]);
	       info.display_density_level_this_image = 1;

	       // if (gtk_events_pending() == 0 ) { // there is always this event
	       info.molecules[s].update_map();
	       info.graphics_draw();
	       // 	 while (gtk_events_pending())
	       // 	    gtk_main_iteration();

	       // 	 } else {
	       // 	    std::cout << "events pending " << gtk_events_pending() << std::endl; 
	       // 	 } 
// 	       std::cout << "contour level of molecule [" << s << "]:  "
// 			 << info.molecules[s].contour_level[0] << std::endl;
	    }
	 } else {
	    std::cout << "WARNING: No map - Can't change contour level.\n";
	 }
      }
   }
   
   if (scroll_up_down_flag == 0) {

      if (graphics_info_t::do_scroll_by_wheel_mouse_flag) { 
	 int s = info.scroll_wheel_map;
	 short int happened = 0;

	 std::vector<int> num_displayed_maps = info.displayed_map_imols();
	 if (num_displayed_maps.size() == 1) 
	    s = num_displayed_maps[0];
	 if (s >= 0) { 

	    happened = info.molecules[s].change_contour(-1);

	    info.set_density_level_string(s, info.molecules[s].contour_level[0]);
	    info.display_density_level_this_image = 1;
	    if (happened) { 
	       info.molecules[s].update_map();
	    }
	    info.graphics_draw();
// 	    std::cout << "contour level of molecule [" << s << "]:  "
// 		      << info.molecules[s].contour_level[0] << std::endl;
	 } else {
	    std::cout << "WARNING: No map - Can't change contour level.\n";
	 }
      }
   }
} 


void test_object() {

   // a solid triangle
   // 
   if (1) {

      
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

      glBegin(GL_TRIANGLE_STRIP);

      glColor4f(0.6, 0.6, 0.7, 0.8);
      glNormal3f(1.0, 0.0, 0.0);
      glVertex3f(10.0,  0.0,  0.0);
      glVertex3f( 0.0, 10.0,  0.0);
      glVertex3f(10.0, 10.0,  0.0);
      
      glNormal3f(0.0, 1.0, 0.0);
      glVertex3f(-5.0, 0.0,  -10.0);
      glVertex3f( 0.0, 10.0,  0.0);
      glVertex3f(10.0, 10.0,  0.0);
      glEnd();

   } 

   if (0) { 
      int n = 40;
      float f = 1.0;

      std::vector<int> colours;
      colours.push_back(GREEN_BOND);
      colours.push_back(BLUE_BOND);
      colours.push_back(GREY_BOND);

      for (unsigned int ic=0; ic<colours.size(); ic++) {
	 set_bond_colour(colours[ic]);
	 if (ic == 2)
	    glLineWidth(2.0);
	 else 
	    glLineWidth(5.0);
      
	 glBegin(GL_LINES);
	 for (int i=0; i<n; i++) { 
	    for (int j=0; j<n; j++) {
	       glVertex3f(i*f, j*f, ic);
	       glVertex3f(i*f, j*f, ic+1);
	    }
	 }
	 glEnd();
      }
   }
}


void
set_bond_colour(int i) {

   switch (i) {
   case GREEN_BOND:
      glColor3f (0.1, 0.8, 0.1);
      break;
   case BLUE_BOND: 
      glColor3f (0.2, 0.2, 0.8);
      break;
   case RED_BOND: 
      glColor3f (0.8, 0.1, 0.1);
      break;
   case YELLOW_BOND: 
      glColor3f (0.7, 0.7, 0.0);
      break;
   case GREY_BOND: 
      glColor3f (0.7, 0.7, 0.7);
      break;
   case HYDROGEN_GREY_BOND: 
      glColor3f (0.6, 0.6, 0.6);
      break;
   case MAGENTA_BOND: 
      glColor3f (0.99, 0.2, 0.99);
      break;
      
   default:
      glColor3f (0.7, 0.8, 0.8);

   } 
}

// amount is not in degrees, it is in fractions of a circle, e.g. 10/360.
// 
std::vector<float> rotate_rgb(std::vector<float> &rgb, float amount) {

   std::vector<float> hsv = coot::convert_rgb_to_hsv(rgb);

   // add 20 degrees to hue (or whatever)
   // std::cout << "adding " << amount << " to hue " << std::endl; 
   hsv[0] += amount;
   while  (hsv[0] > 1.0) {
      hsv[0] -= 1.0;
   }

   std::vector<float> r = coot::convert_hsv_to_rgb(hsv);
   //     std::cout << "rotate from ("
   // << rgb[0] << " " << rgb[1] << " " << rgb[2] << ")\n"
   //  	     << "         to ("
   // << rgb[0] << " " << rgb[1] << " " << rgb[2] << ")\n";
   return r;
   // return convert_hsv_to_rgb(hsv);

}



// // we get passed a number that is 0.0 - 1.0 inclusive (the higher the
// // number, the more "core" is the skeleton).
// // 
// void set_skeleton_bond_colour(float f)
// {
//    // glColor3f (0.2+0.8*f, 0.4+0.5*f, 0.8-0.8*f);
//    // glColor3f (0.4+0.6*f, f, 0.3);

//    graphics_info_t g;
//    glColor3f(0.1+0.6*f*g.skeleton_colour[0],
// 	     0.1+0.9*f*g.skeleton_colour[1], 
// 	     0.1+0.2*f*g.skeleton_colour[2]);
 
// //    glColor3f(0.2+0.8*g.skeleton_colour[0],
// // 	     0.3+0.5*g.skeleton_colour[1],
// // 	     0.0+0.2*g.skeleton_colour[2]); 
// }

// i goes upto bond_box.num_colours
// 
void set_skeleton_bond_colour_random(int i, const vector< vector<float> > &colour_table) { 

   glColor3f(0.2+0.8*colour_table[i][0],
	     0.2+0.8*colour_table[i][1],
	     0.2+0.8*colour_table[i][2]); 

}

// double
// combine_colour(double v, int col_part_index) {
//    // col_part_index is 0,1,2 for red gree blue components of the colour

//    graphics_info_t g;
//    double w = g.symm_colour_merge_weight[0];
   
//    return w*g.symm_colour[0][col_part_index] + v*(1.0-w);
// }


// // redundant now, I hope.  It's internal to molecule_class_info_t.
// void
// set_symm_bond_colour(int i) {

//    switch (i) {
//    case green:
//       glColor3f (combine_colour(0.1,0),
// 		 combine_colour(0.8,1),
// 		 combine_colour(0.1,2));
//       break;
//    case blue: 
//       glColor3f (combine_colour(0.2,0),
// 		 combine_colour(0.2,1),
// 		 combine_colour(0.8,2));
//       break;
//    case red: 
//       glColor3f (combine_colour(0.8,0),
// 		 combine_colour(0.1,1),
// 		 combine_colour(0.1,2));
//       break;
//    case yellow: 
//       glColor3f (combine_colour(0.7,0),
// 		 combine_colour(0.7,1),
// 		 combine_colour(0.0,2));
//       break;
      
//    default:
//       glColor3f (combine_colour(0.7, 0),
// 		 combine_colour(0.8, 1),
// 		 combine_colour(0.8, 2));
//    } 
// }


// ----------------------------------------------------------
//
// Remember, being in GL_LINES mode will cause this to fail silently.
// 
void printString(std::string s) {
   glPushAttrib (GL_LIST_BIT);
   for (unsigned int i = 0; i < s.length(); i++)
      glutBitmapCharacter (graphics_info_t::atom_label_font, s[i]);
   glPopAttrib ();
}


