/* src/c-interface.h
 * 
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
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

/* svn $Id: c-interface.h 1458 2007-01-26 20:20:18Z emsley $ */

/*! \file 
  \brief Coot Scripting Interface

  Here is a list of all the scripting interface functions. They are
  described/formatted in c/python format.

  Usually coot is compiled with the guile interpreter, and in this
  case these function names and usage are changed a little, e.g.:

  c-format:
  chain_n_residues("A", 1)

  scheme format:
  (chain-n-residues "A" 1)

  Note the prefix usage of the parenthesis and the lack of comma to
  separate the arguments.

*/

#ifndef C_INTERFACE_H
#define C_INTERFACE_H

/*
  The following extern stuff here because we want to return the
  filename from the file entry box.  That code (e.g.) 
  on_ok_button_coordinates_clicked (callback.c), is written and
  compiled in c.
 
  But, we need that function to set the filename in mol_info, which 
  is a c++ class.
 
  So we need to have this function external for c++ linking.
 
*/

/* Francois says move this up here so that things don't get wrapped
   twice in C-declarations inside gmp library. Hmm! */
#ifdef __cplusplus
#ifdef USE_GUILE
#include <libguile.h>		/* for SCM type (returned by safe_scheme_command) */
#endif // USE_GUILE
#endif 

#ifndef BEGIN_C_DECLS

#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }

#else
#define BEGIN_C_DECLS extern
#define END_C_DECLS     
#endif
#endif /* BEGIN_C_DECLS */

BEGIN_C_DECLS

/* Fix this on a rainy day. */
/* #ifdef __cplusplus */
/* #include "mini-mol.hh" */
/* #endif //  __cplusplus */

/* For similar reason to above, we can't have this here (mmdb poisoning) */
/* #ifdef __cplusplus */
/* #include "sequence-view.hh" */
/* #endif */

#define COOT_SCHEME_DIR "COOT_SCHEME_DIR"

/*  ------------------------------------------------------------------------ */
/*                         File system Functions:                            */
/*  ------------------------------------------------------------------------ */
/*  File system Utility function: maybe there is a better place for it... */

 
/*  Return like mkdir: mkdir returns zero on success, or -1 if an error */
/*  occurred */

/*  if it already exists as a dir, return 0 of course.  Perhaps should */
/*  be called "is_directory?_if_not_make_it". */

/* section File System Functions */
/*!  \name File System Functions */
/* \{ */

/*! 
   \brief make a directory dir (if it doesn't exist) and return error code

   If it can be created, create the directory dir, return the success status
   like mkdir: mkdir 

   @return zero on success, or -1 if an  error  occurred.
   If dir already exists as a directory, return 0 of course.
 */
int make_directory_maybe(const char *dir);

/*! \brief Show Paths in Display Manager?

    Some people don't like to see the full path names in the display manager
    here is the way to turn them off, with an argument of 1.
*/
void set_show_paths_in_display_manager(int i);

/*! \brief return the internal state

   What is the internal flag? 

   @return 1 for "yes, display paths" , 0 for not
 */
int show_paths_in_display_manager_state();

/* 20050316 Ooooh! Hairy hairy! */
GSList **gslist_for_scroll_in_display_manager_p();

/*! \brief add an extension to be treated as coordinate files 
*/
void add_coordinates_glob_extension(const char *ext);
/*! \brief add an extension to be treated as data (reflection) files 
*/
void add_data_glob_extension(const char *ext);

/*! \brief add an extension to be treated as geometry dictionary files 
*/
void add_dictionary_glob_extension(const char *ext);

/*! \brief add an extension to be treated as geometry map files 
*/
void add_map_glob_extension(const char *ext);


/*! \brief sort files in the file selection by date?

  some people like to have their files sorted by date by default */
void set_sticky_sort_by_date(); 

/*! \brief on opening a file selection dialog, pre-filter the files.

set to 1 to pre-filter, [0 (off, non-pre-filtering) is the default */
void set_filter_fileselection_filenames(int istate);

void set_file_selection_dialog_size(GtkWidget *w);

/*! \brief, return the state of the above variable */
int filter_fileselection_filenames_state();

#ifdef COOT_USE_GTK2_INTERFACE
void on_filename_filter_toggle_button_toggled (GtkButton       *button,
					      gpointer         user_data);
#else
void on_filename_filter_toggle_button_toggled_gtk1 (GtkButton       *button,
						    gpointer         user_data);
#endif 
void add_filename_filter(GtkWidget *fileselection);

// where data type:
// 0 coords
// 1 mtz etc
// 2 maps
// (return the button)
GtkWidget *add_filename_filter_button(GtkWidget *fileselection, 
				      short int type);
gboolean on_filename_filter_key_press_event (GtkWidget       *widget,
					     GdkEventKey     *event,
					     gpointer         user_data);

/* a c callable wrapper to the graphics_info_t function */
void fill_option_menu_with_coordinates_options(GtkWidget *option_menu, 
					       GtkSignalFunc signal_func,
					       int imol_active_position);
GtkWidget *coot_file_chooser();

void set_directory_for_coot_file_chooser(GtkWidget *w);

const char *coot_file_chooser_file_name(GtkWidget *widget);

/* \} */


/*  ---------------------------------------------------------------------- */
/*                     widget utilities                                    */
/*  ---------------------------------------------------------------------- */
/* section Widget Utilities */
/*! \name Widget Utilities */
/* \{ */
/* return negative if fail */
float get_positive_float_from_entry(GtkEntry *w); 

#ifdef COOT_USE_GTK2_INTERFACE
void handle_filename_filter_gtk2(GtkWidget *widget);
#else 
void handle_filename_filter_gtk1(GtkWidget *widget);
#endif 

void set_transient_and_position(int window_type, GtkWidget *window);

/*! \brief create a dialog with information

  create a dialog with information string txt.  User has to click to
  dismiss it, but it is not modal (nothing in coot is modal). */
void info_dialog(const char *txt); 

GtkWidget *main_menubar();
GtkWidget *main_statusbar();

/* \} */

/*  -------------------------------------------------------------------- */
/*                   mtz and data handling utilities                     */
/*  -------------------------------------------------------------------- */
/* section Widget Utilities */
/*! \name Widget Utilities */
/* \{ */
/* We try as .phs and .cif files first */

/*! \brief given a filename, try to read it as a data file 

   We try as .phs and .cif files first */
void manage_column_selector(const char *filename);
void fill_f_optionmenu_with_expert_options(GtkWidget *f_optionmenu);
void handle_column_label_make_fourier(GtkWidget *column_label_window);

/* \} */

/*  -------------------------------------------------------------------- */
/*                     Molecule Functions       :                        */
/*  -------------------------------------------------------------------- */
/* section Molecule Info Functions */
/*! \name Molecule Info Functions */
/* \{ */

/*! \brief  the number of residues in chain chain_id and molecule number imol
  @return the number of residues 
*/
int chain_n_residues(const char *chain_id, int imol); 
/* @return status, less than -9999 is for failure (eg. bad imol); */
float molecule_centre_internal(int imol, int iaxis);
/*! \brief return the rename from a residue serial number

   @return NULL (#f) on failure. */
char *resname_from_serial_number(int imol, const char *chain_id, 
				 int serial_num);

/*! \brief a residue seqnum (normal residue number) from a residue
  serial number

   @return < -9999 on failure */
int  seqnum_from_serial_number(int imol, const char *chain_id, 
			       int serial_num);

/*! \brief the insertion code of the residue.

   @return NULL (#f) on failure. */
char *insertion_code_from_serial_number(int imol, const char *chain_id, int serial_num);

/*! \brief the chain_id (string) of the ichain-th chain
  molecule number imol  
   @return the chain-id */
char *chain_id(int imol, int ichain);

/*! \brief  number of chains in molecule number imol 

   @return the number of chains*/
int n_chains(int imol);

/*! \brief is this a solvent chain? [Raw function]

   This is a raw interface function, you should generally not use
   this, but instead use (is-solvent-chain? imol chain-id)

   @return -1 on error, 0 for no, 1 for is "a solvent chain".  We
   wouldn't want to be doing rotamer searches and the like on such a
   chain.
 */
int is_solvent_chain_p(int imol, const char *chain_id);

/*! \brief copy molecule imol

@return the new molecule number.
Return -1 on failure to copy molecule (out of range, or molecule is
closed) */
int copy_molecule(int imol);

/*! \brief Experimental interface for Ribosome People. 

Ribosome People have many chains in their pdb file, they prefer segids
to chainids (chainids are only 1 character).  But coot uses the
concept of chain ids and not seg-ids.  mmdb allow us to use more than
one char in the chainid, so after we read in a pdb, let's replace the
chain ids with the segids. Will that help? */
int exchange_chain_ids_for_seg_ids(int imol);

/* \} */

/*  -------------------------------------------------------------------- */
/*                     Library/Utility Functions:                        */
/*  -------------------------------------------------------------------- */

/* section Library and Utility Functions */
/*! \name Library and Utility Functions */
/* \{ */

/*! \brief the coot version string 

   @return something like "coot-0.1.3".  New versions of coot will
   always be lexographically greater than previous versions. */
char *coot_version();

/*! \brief return the name of molecule number imol

 @return 0 if not a valid name ( -> #f in scheme) 
 e.g. "/a/b/c.pdb" for "d/e/f.mtz FWT PHWT" */ 
const char *molecule_name(int imol);
/*! \brief set the molecule name of the imol-th molecule */
void set_molecule_name(int imol, const char *new_name);
GtkWidget *main_window(); 
gboolean coot_checked_exit(int retval); 
/*! \brief exit from coot, give return value retval back to invoking
  process. */
void coot_real_exit(int retval); 
void fill_about_window(GtkWidget *widget);
 
/*! \brief What is the molecule number of first coordinates molecule?

   return -1 when there is none. */
int first_coords_imol(); 	

/* \} */

/*  -------------------------------------------------------------------- */
/*                     Library/Utility Functions:                        */
/*  -------------------------------------------------------------------- */
/* section Graphics Utility Functions */
/*! \name Graphics Utility Functions */
/* \{ */

/*! \brief set the bond lines to be antialiased */
void set_do_anti_aliasing(int state);
/*! \brief return the flag for antialiasing the bond lines */
int do_anti_aliasing_state();

/*! \brief turn the GL lighting on (state = 1) or off (state = 0) 

   slows down the display of simple lines
*/
void set_do_GL_lighting(int state);
/*! \brief return the flag for GL lighting */
int do_GL_lighting_state();

/*! \brief shall we start up the Gtk and the graphics window? 

   if passed the command line argument --no-graphics, coot will not start up gtk
   itself.

   An interface function for Ralf.
*/
short int use_graphics_interface_state();

/*! \brief start Gtk (and graphics) 

   This function is useful if it was not started already (which can be
   achieved by using the command line argument --no-graphics).

   An interface for Ralf */
void start_graphics_interface(); 

/*! \brief "Reset" the view
 

  return 1 if we moved, else return 0.

   centre on last-read molecule with zoom 100. If we are there, then
   go to the previous molecule, if we are there, then go to the origin. */
int reset_view();

/*! \brief return the number of molecules (coordinates molecules and
  map molecules combined) that are currently in coot

  @return the number of molecules (closed molecules are not counted) */
int graphics_n_molecules(); 

int next_map_for_molecule(int imol); /* return a map number */

/*! \brief Spin spin spin (or not) */
void toggle_idle_function(); 

/*! \brief how far should we rotate when (auto) spinning? Fast
  computer? set this to 0.1  */
void set_idle_function_rotate_angle(float f);  // degrees

float idle_function_rotate_angle();

/* pass back the newly created molecule number */
/*! \brief a synonym for read-pdb.  Read the coordinates from
  filename (can be pdb, cif or shelx format)  */
int handle_read_draw_molecule(const char *filename);

/*! \brief read coordinates from filename with option to not recentre.

   set recentre_on_read_pdb_flag to 0 if you don't want the view to
   recentre on the new coordinates. */
int handle_read_draw_molecule_with_recentre(const char *filename, 
					    int recentre_on_read_pdb_flag);

/*! \brief read coordinates from filename */
int read_pdb(const char *filename); // cc4mg function name

/*! \brief replace the parts of molecule number imol that are
  duplicated in molecule number imol_frag */
int replace_fragment(int imol_target, int imol_fragment, const char *atom_selection);

/*! \brief replace pdb.  Fail if molecule_number is not a valid model molecule.
  Return -1 on failure.  Else return molecule_number  */
int clear_and_update_model_molecule_from_file(int molecule_number, 
					      const char *file_name);

/* Used in execute_rigid_body_refine */
/* Fix this on a rainy day. */
/* atom_selection_container_t  */
/* make_atom_selection(int imol, const coot::minimol::molecule &mol);  */

/*! \brief dump the current screen image to a file.  Format ppm 

You can use this, in conjunction with spinning and view moving functions to 
make movies */
void screendump_image(const char *filename);

void add_is_difference_map_checkbutton(GtkWidget *fileselection); 
/* the callback for the above: */
void
on_read_map_difference_map_toggle_button_toggled (GtkButton       *button,
						  gpointer         user_data);

void add_recentre_on_read_pdb_checkbutton(GtkWidget *fileselection); 
/* the callback for the above: */
void
on_recentre_on_read_pdb_toggle_button_toggled (GtkButton       *button,
					       gpointer         user_data);

/* \} */

/*  --------------------------------------------------------------------- */
/*                      Interface Preferences                             */
/*  --------------------------------------------------------------------- */
/* section Interface Preferences */
/*! \name   Interface Preferences */
/*! \{ */

/*! \brief Some people (like Phil Evans) don't want to scroll their
  map with the mouse-wheel.

  To turn off mouse wheel recontouring call this with istate value of 0  */
void set_scroll_by_wheel_mouse(int istate);
/*! \brief return the internal state of the scroll-wheel map contouring */
int scroll_by_wheel_mouse_state();

/*! \brief set the default inital contour for 2FoFc-style map

in sigma */
void set_default_initial_contour_level_for_map(float n_sigma);

/*! \brief set the default inital contour for FoFc-style map

in sigma */
void set_default_initial_contour_level_for_difference_map(float n_sigma);

/*! \brief print the view matrix to the console, useful for molscript,
  perhaps */
void print_view_matrix();		/* print the view matrix */

float get_view_matrix_element(int row, int col); /* used in (view-matrix) command */

/*! \brief internal function to get an element of the view quaternion.
  The whole quaternion is returned by the scheme function
  view-quaternion  */
float get_view_quaternion_internal(int element);

/*! \brief Set the view quaternion */
void set_view_quaternion(float i, float j, float k, float l);

/*! \brief Given that we are in chain current_chain, apply the NCS
  operator that maps current_chain on to next_ncs_chain, so that the
  relative view is preserved.  For NCS skipping. */
void apply_ncs_to_view_orientation(int imol, const char *current_chain, const char *next_ncs_chain);

void set_fps_flag(int t);
int  get_fps_flag();

/*! \brief set a flag: is the origin marker to be shown? 1 for yes, 0
  for no. */
void set_show_origin_marker(int istate);
/*! \brief return the origin marker shown? state */
int  show_origin_marker_state();

/*! \brief reparent the Model/Fit/Refine dialog so that it becomes
  part of the main window, next to the GL graphics context */
int suck_model_fit_dialog();

/* return the dialog if it exists, else null */
GtkWidget *close_model_fit_dialog(GtkWidget *dialog_hbox);
/* use this from the scripting layer to say something to the user (popup). */
GtkWidget *popup_window(const char *s);

/*! \brief Put text s into the status bar.

  use this to put info for the user in the statusbar (less intrusive
  than popup). */
void add_status_bar_text(const char *s);

void set_model_fit_refine_dialog_stays_on_top(int istate);
int model_fit_refine_dialog_stays_on_top_state();

void save_accept_reject_dialog_window_position(GtkWidget *acc_reg_dialog);
void set_accept_reject_dialog(GtkWidget *w); /* used by callbacks to unset the widget */

/*! \} */

/*  ----------------------------------------------------------------------- */
/*                           mouse buttons                                  */
/*  ----------------------------------------------------------------------- */
/* section Mouse Buttons */
/*! \name   Mouse Buttons */
/* \{ */

/* Note, when you have set these, there is no way to turn them of
   again (other than restarting). */
void quanta_buttons(); 
void quanta_like_zoom();


/* -------------------------------------------------------------------- */
/*    Ctrl for rotate or pick: */
/* -------------------------------------------------------------------- */
/*! \brief Alternate mode for rotation

Prefered by some, including Dirk Kostrewa.  I don't think this mode
works properly yet */
void set_control_key_for_rotate(int state);
/*! \brief return the control key rotate state */
int control_key_for_rotate_state();

/* Put the blob under the cursor to the screen centre.  Check only
positive blobs.  Useful function if bound to a key.

The refinement map must be set.  (We can't check all maps because they
are not (or may not be) on the same scale).

   return 1 if successfully found a blob and moved there.
   return 0 if no move.
*/
int blob_under_pointer_to_screen_centre();

/* \} */

/*  --------------------------------------------------------------------- */
/*                      Cursor Functions:                                 */
/*  --------------------------------------------------------------------- */
/* section Cursor Function */
/*! \name Cursor Function */
/*! \{ */
void normal_cursor();
void fleur_cursor();
void pick_cursor_maybe();
void rotate_cursor();

/*! \brief let the user have a different pick cursor

sometimes (the default) GDK_CROSSHAIR is hard to see, let the user set
their own */
void set_pick_cursor_index(int icursor_index);

/*! \} */


/*  --------------------------------------------------------------------- */
/*                      Model/Fit/Refine Functions:                       */
/*  --------------------------------------------------------------------- */
/* section Model/Fit/Refine Functions  */
/*! \name Model/Fit/Refine Functions  */
/* \{ */
/*! \brief display the Model/Fit/Refine dialog */
void post_model_fit_refine_dialog();
GtkWidget *wrapped_create_model_fit_refine_dialog(); 
void unset_model_fit_refine_dialog();
void unset_refine_params_dialog();
/*! \brief display the Display Manager dialog */
void show_select_map_dialog();
/*! \brief Allow the changing of Model/Fit/Refine button label from
  "Rotate/Translate Zone" */
void set_model_fit_refine_rotate_translate_zone_label(const char *txt);
/*! \brief Allow the changing of Model/Fit/Refine button label from
  "Place Atom at Pointer" */
void set_model_fit_refine_place_atom_at_pointer_label(const char *txt);

/* other tools */
GtkWidget *wrapped_create_other_model_tools_dialog();
void unset_other_modelling_tools_dialog();

/*! \brief display the Other Modelling Tools dialog */
void post_other_modelling_tools_dialog();

/*! \} */

/*  --------------------------------------------------------------------- */
/*                      backup/undo functions:                            */
/*  --------------------------------------------------------------------- */
/* section Backup Functions */
/*! \name Backup Functions */
/* \{ */
/* c-interface-build functions */

/*! \brief make backup for molecule number imol */
void make_backup(int imol);

/*! \brief turn off backups for molecule number imol */
void turn_off_backup(int imol); 
/*! \brief turn on backups for molecule number imol */
void turn_on_backup(int imol);
/*! \brief return the backup state for molecule number imol 

 return 0 for backups off, 1 for backups on, -1 for unknown */
int  backup_state(int imol);
void apply_undo();		/* "Undo" button callback */
void apply_redo();

/*! \brief set the molecule number imol to be marked as having unsaved changes */
void set_have_unsaved_changes(int imol);

/* \brief does molecule number imol have unsaved changes?

return -1 on bad imol, 0 on no unsaved changes, 1 on has unsaved changes */
int have_unsaved_changes_p(int imol);


/*! \brief set the molecule to which undo operations are done to
  molecule number imol */
void set_undo_molecule(int imol);

/*! \brief show the Undo Molecule chooser - i.e. choose the molecule
  to which the "Undo" button applies. */
void show_set_undo_molecule_chooser(); 

/*! \brief set the state for adding paths to backup file names

  by default directories names are added into the filename for backup
  (with / to _ mapping).  call this with state=1 to turn off directory
  names  */
void set_unpathed_backup_file_names(int state);
/*! \brief return the state for adding paths to backup file names*/
int  unpathed_backup_file_names_state();
/* \} */

/*  --------------------------------------------------------------------- */
/*                         recover session:                               */
/*  --------------------------------------------------------------------- */
/* section Recover Session Function */
/*! \name  Recover Session Function */
/* \{ */
/*! \brief recover session

   After a crash (shock horror!) we provide this convenient interface
   to restore the session.  It runs through all the molecules with
   models and looks at the coot backup directory looking for related
   backup files that are more recent that the read file. */
void recover_session();
void execute_recover_session(GtkWidget *w);
/* \} */
   
/*  ---------------------------------------------------------------------- */
/*                       map functions:                                    */
/*  ---------------------------------------------------------------------- */
/* section Map Functions */
/*! \name  Map Functions */
/* \{ */

/*! \brief fire up a GUI, which asks us which model molecule we want
  to calc phases from.  On "OK" button there, we call
  map_from_mtz_by_refmac_calc_phases() */
void calc_phases_generic(const char *mtz_file_name);

/*! \brief Calculate SFs (using refmac optionally) from an MTZ file
  and generate a map. Get F and SIGF automatically (first of their
  type) from the mtz file.

@return the new molecule number, -1 on a problem. */
int map_from_mtz_by_refmac_calc_phases(const char *mtz_file_name, 
				       const char *f_col, 
				       const char *sigf_col, 
				       int imol_coords);


/*! \brief Calculate SFs from an MTZ file and generate a map. 
 @return the new molecule number. */
int map_from_mtz_by_calc_phases(const char *mtz_file_name, 
				const char *f_col, 
				const char *sigf_col, 
				int imol_coords);

gdouble* get_map_colour(int imol);
void add_on_map_colour_choices(GtkWidget *w);
/* the callback set on the submenu items in the above function */
void map_colour_mol_selector_activate (GtkMenuItem     *menuitem,
				       gpointer         user_data);
void my_delete_menu_items(GtkWidget *widget, void *data);

/* similarly for the scrollwheel */
void add_on_map_scroll_whell_choices(GtkWidget *menu);
void map_scroll_wheel_mol_selector_activate (GtkMenuItem     *menuitem,
					     gpointer         user_data);
/*! \brief return the molecule number to which the mouse scroll wheel
  is attached */
int scroll_wheel_map();
/*! \brief save previous colour map for molecule number imol */
void save_previous_map_colour(int imol);
/*! \brief restore previous colour map for molecule number imol */
void restore_previous_map_colour(int imol);

/*! \brief set the state of immediate map upate on map drag.

By default, it is on (t=1).  On slower computers it might be better to
set t=0. */
void set_active_map_drag_flag(int t);
/*! \brief return the state of the dragged map flag  */
short int get_active_map_drag_flag(); 

/*! \brief set the colour of the last (highest molecule number) map */
void set_last_map_colour(double f1, double f2, double f3);
/*! \brief set the colour of the imolth map */
void set_map_colour(int imol, float red, float green, float blue);

void handle_map_colour_change     (int map_no, gdouble[4]);
void handle_symmetry_colour_change(int mol,    gdouble[4]);
void fill_single_map_properties_dialog(GtkWidget *window, int imol);

/*! \brief set the sigma step of the last map to f sigma */
void set_last_map_sigma_step(float f);
void set_contour_sigma_button_and_entry(GtkWidget *window, int imol);
void set_contour_by_sigma_step_maybe(GtkWidget *window, int imol);
/*! \brief set the contour level step

   set the contour level step of molecule number imol to f and
   variable state (setting state to 0 turns off contouring by sigma
   level)  */
void set_contour_by_sigma_step_by_mol(float f, short int state, int imol); 

/*! \brief return the resolution of the data for molecule number imol.
   Return negative number on error, otherwise resolution in A (eg. 2.0) */
float data_resolution(int imol);

void solid_surface(int imap, short int on_off_flag);

/*! \brief export (write to disk) the map of molecule number imol to
  filename */
void export_map(int imol, const char *filename);

/* return the new molecule number */
int transform_map_raw(int imol, 
		      double r00, double r01, double r02, 
		      double r10, double r11, double r12, 
		      double r20, double r21, double r22, 
		      double t0, double t1, double t2, 
		      double pt0, double pt1, double pt2, 
		      double box_half_size);

/* return the new molecule number (or should I do it in place?) */
int rotate_map_round_screen_axis_x(float r_degrees); 
int rotate_map_round_screen_axis_y(float r_degrees); 
int rotate_map_round_screen_axis_z(float r_degrees); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                         (density) iso level increment entry */
/*  ----------------------------------------------------------------------- */
/* section Density Increment */
/*! \name  Density Increment */
/* \{ */

char* get_text_for_iso_level_increment_entry(int imol); /* const gchar *text */
char* get_text_for_diff_map_iso_level_increment_entry(int imol); /* const gchar *text */

/* void set_iso_level_increment(float val); */
/*! \brief set the contour scroll step (in absolute e/A3) for
  2Fo-Fc-style maps to val

The is only activated when scrolling by sigma is turned off */
void set_iso_level_increment(float val); 
void set_iso_level_increment_from_text(const char *text, int imol);
/*! \brief set the contour scroll step for difference map (in absolute
  e/A3) to val

The is only activated when scrolling by sigma is turned off */
void set_diff_map_iso_level_increment(float val);
void set_diff_map_iso_level_increment_from_text(const char *text, int imol);

void set_map_sampling_rate_text(const char *text);

/*! \brief set the map sampling rate (default 1.5)

Set to something like 2.0 or 2.5 for more finely sampled maps.  Useful
for baton-building low resolution maps. */
void set_map_sampling_rate(float r);
char* get_text_for_map_sampling_rate_text();

/*! \brief return the map sampling rate */
float get_map_sampling_rate();

/*! \brief set the map that has its contour level changed by the
  scrolling the mouse wheel to molecule number imol */
void set_scrollable_map(int imol); 
/*! \brief change the contour level of the current map by a step

if is_increment=1 the contour level is increased.  If is_increment=0
the map contour level is decreased.
 */
void change_contour_level(short int is_increment); /* else is decrement.  */

/*! \brief set the contour level of the map with the highest molecule
    number to level */
void set_last_map_contour_level(float level);
/*! \brief set the contour level of the map with the highest molecule
    number to n_sigma sigma */
void set_last_map_contour_level_by_sigma(float n_sigma);

/*! \brief create a lower limit to the "2Fo-Fc-style" map contour level changing 

  (default 1 on) */
void set_stop_scroll_diff_map(int i);
/*! \brief create a lower limit to the difference map contour level changing 

  (default 1 on) */
void set_stop_scroll_iso_map(int i);

/*! \brief set the actual map level changing limit 

   (default 0.0) */
void set_stop_scroll_iso_map_level(float f); 

/*! \brief set the actual difference map level changing limit 

   (default 0.0) */
void set_stop_scroll_diff_map_level(float f);

/*! \brief set the scale factor for the Residue Density fit analysis */
void set_residue_density_fit_scale_factor(float f);


/*! \} */

/*  ------------------------------------------------------------------------ */
/*                         density stuff                                     */
/*  ------------------------------------------------------------------------ */
/* section Density Functions */
/*! \name  Density Functions */
/*! \{ */
/*! \brief draw the lines of the chickenwire density in width w */
void set_map_line_width(int w);
/*! \brief return the width in which density contours are drawn */
int map_line_width_state();

/*! \brief make a map from an mtz file (simple interface)

 given mtz file mtz_file_name and F column f_col and phases column
 phi_col and optional weight column weight_col (pass use_weights=0 if
 weights are not to be used).  Also mark the map as a difference map
 (is_diff_map=1) or not (is_diff_map=0) because they are handled
 differently inside coot.

 @return -1 on error, else return the new molecule number */
int make_and_draw_map(const char *mtz_file_name, 
		      const char *f_col, const char *phi_col, 
		      const char *weight,
		      int use_weights, int is_diff_map); 

/*! \brief as the above function, execpt set refmac parameters too

 pass along the refmac column labels for storage (not used in the
 creation of the map)

 @return -1 on error, else return imol */
int  make_and_draw_map_with_refmac_params(const char *mtz_file_name, 
		       const char *a, const char *b, const char *weight,
					  int use_weights, int is_diff_map,
					  short int have_refmac_params,
					  const char *fobs_col,
					  const char *sigfobs_col,
					  const char *r_free_col,
					  short int sensible_f_free_col);

/*! \brief as the above function, except set expert options too.

*/
/* Note to self, we need to save the reso limits in the state file  */
int make_and_draw_map_with_reso_with_refmac_params(const char *mtz_file_name, 
						   const char *a, const char *b, 
						   const char *weight,
						   int use_weights, int is_diff_map,
						   short int have_refmac_params,
						   const char *fobs_col,
						   const char *sigfobs_col,
						   const char *r_free_col,
						   short int sensible_f_free_col,
						   short int is_anomalous,
						   short int use_reso_limits,
						   float low_reso_limit,
						   float high_reso_lim);

/*! \brief does the mtz file have the columms that we want it to have? */
int valid_labels(const char *mtz_file_name, const char *f_col, 
		 const char *phi_col, 
		 const char *weight_col, 
		 int use_weights);

/* We need to know if an mtz file has phases.  If it doesn't then we */
/*  go down a (new 20060920) different path. */
int mtz_file_has_phases_p(const char *mtz_file_name); 

int is_mtz_file_p(const char *filename);

/*! \brief read MTZ file filename and from it try to make maps

Useful for reading the output of refmac.  The default labels (FWT/PHWT
and DELFWT/PHDELFWT) can be changed using ...[something] 

 @return the molecule number for the new map 
*/
int auto_read_make_and_draw_maps(const char *filename); 
/*! \brief set the flag to do a difference map (too) on auto-read MTZ */
void set_auto_read_do_difference_map_too(int i);
/*! \brief return the flag to do a difference map (too) on auto-read MTZ 

   @return 0 means no, 1 means yes. */
int auto_read_do_difference_map_too_state();
/*! \brief set the exected MTZ columns for Auto-reading MTZ file. 

  Not every program uses the default refmac labels (FWT/PHWT) for its
  MTZ file.  Here we can tell coot to expect other labels, 

  e.g. (set-auto-read-column-labels "2FOFCWT" "PH2FOFCWT" 0) */
void set_auto_read_column_labels(const char *fwt, const char *phwt, 
				 int is_for_diff_map_flag);

char* get_text_for_density_size_widget(); /* const gchar *text */
void set_density_size_from_widget(const char *text);

/*! \brief set the extent of the box/radius of electron density contours */
void set_map_radius(float f);

/*! \brief another (old) way of setting the radius of the map */
void set_density_size(float f);

void set_map_radius_slider_max(float f); 

/*! \brief Give me this nice message str when I start coot */
void set_display_intro_string(const char *str); 

/*! \brief not everone likes coot's esoteric depth cueing system

  Pass an argument istate=1 to turn it off

 (this function is currently disabled). */
void set_esoteric_depth_cue(int istate);

/*! \brief native depth cueing system

  return the state of the esoteric depth cueing flag */
int  esoteric_depth_cue_state();

/*! \brief not everone lies coot's default difference map colouring.  

   Pass an argument i=1 to swap the difference map colouring so that
   red is positve and green is negative. */
void set_swap_difference_map_colours(int i);
/*! \brief post-hoc set the map of molecule number imol to be a
  difference map
  @return success status, 0 -> failure (imol does not have a map) */
int set_map_is_difference_map(int imol);

int map_is_difference_map(int imol);

/*! \brief Add another contour level for the last added map.  

  Currently, the map must have been generated from an MTZ file.
  @return the molecule number of the new molecule or -1 on failure */
int another_level();

/*! \brief Add another contour level for the given map.

  Currently, the map must have been generated from an MTZ file.
  @return the molecule number of the new molecule or -1 on failure */
int another_level_from_map_molecule_number(int imap);

/*! \brief return the scale factor for the Residue Density fit analysis */
float residue_density_fit_scale_factor();

/* \} */
 

/*  ------------------------------------------------------------------------ */
/*                         Parameters from map:                              */
/*  ------------------------------------------------------------------------ */
/* section Parameters from map */
/*! \name  Parameters from map */
/* \{ */

/*! \brief return the mtz file that was use to generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_hklin_for_map(int imol_map);

/*! \brief return the FP column in the file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_fp_for_map(int imol_map);

/*! \brief return the phases column in mtz file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_phi_for_map(int imol_map);

/*! \brief return the weight column in the mtz file that was use to
  generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say) or no weights were used. */
const char *mtz_weight_for_map(int imol_map);

/*! \brief return flag for whether weights were used that was use to
  generate the map

  return 0 when no weights were used or there is no mtz file
  associated with that map. */
short int mtz_use_weight_for_map(int imol_map);

/*! \} */


/*  ------------------------------------------------------------------------ */
/*                         Write PDB file:                                   */
/*  ------------------------------------------------------------------------ */
/* section PDB Functions */
/*! \name  PDB Functions */
/* \{ */

/*! \brief write molecule number imol as a PDB to file file_name */
/*  return 0 on success, 1 on error. */
int write_pdb_file(int imol, const char *file_name); 

/*! \brief write molecule number imol's residue range as a PDB to file
  file_name */
/*  return 0 on success, 1 on error. */
int write_residue_range_to_pdb_file(int imol, const char *chainid, 
				    int resno_start, int resno_end, 
				    const char *filename);
/*! \} */


/*  ------------------------------------------------------------------------ */
/*                         refmac stuff                                      */
/*  ------------------------------------------------------------------------ */
/* section Refmac Functions */
/*! \name  Refmac Functions */
/* \{ */
void execute_refmac(GtkWidget *window); /* lookup stuff here. */
/*  this is the option menu callback - does nothing. */
void refmac_molecule_button_select(GtkWidget *item, GtkPositionType pos); 
int set_refmac_molecule(int imol); /* used by callback.c */
void fill_option_menu_with_refmac_options(GtkWidget *optionmenu);


void free_memory_run_refmac(GtkWidget *window); 

/*! \brief set counter for runs of refmac so that this can be used to
  construct a unique filename for new output */
void set_refmac_counter(int imol, int refmac_count);
/*! \brief the name for refmac 

 @return a stub name used in the construction of filename for refmac output */
const char *refmac_name(int imol);

/*! \brief swap the colours of maps 

  swap the colour of maps imol1 and imol2.  Useful to some after
  running refmac, so that the map to be build into is always the same
  colour*/
void swap_map_colours(int imol1, int imol2);
/*! \brief flag to enable above

call this with istate=1 */
void set_keep_map_colour_after_refmac(int istate);

/*! \brief the keep-map-colour-after-refmac internal state

  @return 1 for "yes", 0 for "no"  */
int keep_map_colour_after_refmac_state();

/* \} */

/*  --------------------------------------------------------------------- */
/*                      symmetry                                          */
/*  --------------------------------------------------------------------- */
/* section Symmetry Functions */
/*! \name Symmetry Functions */
/* \{ */
char* get_text_for_symmetry_size_widget(); /* const gchar *text */
void set_symmetry_size_from_widget(const char *text); 
/*! \brief set the size of the displayed symmetry */
void set_symmetry_size(float f); 
double* get_symmetry_bonds_colour(int imol);
/*! \brief is symmetry master display control on? */
short int get_show_symmetry(); /* master */
/*! \brief set display symmetry, master controller */
void set_show_symmetry_master(short int state);
/*! \brief set display symmetry for molecule number mol_no

   pass with state=0 for off, state=1 for on */
void set_show_symmetry_molecule(int mol_no, short int state);
/*! \brief display symmetry as CAs?


   pass with state=0 for off, state=1 for on */
void symmetry_as_calphas(int mol_no, short int state); 
/*! \brief what is state of display CAs for molecule number mol_no?

   return state=0 for off, state=1 for on
*/
short int get_symmetry_as_calphas_state(int imol);

/*! \brief set the colour map rotation (i.e. the hue) for the symmetry
    atoms of molecule number imol  */
void set_symmetry_molecule_rotate_colour_map(int imol, int state);

/*! \brief should there be colour map rotation (i.e. the hue) change
    for the symmetry atoms of molecule number imol?

   return state=0 for off, state=1 for on
*/
int symmetry_molecule_rotate_colour_map_state(int imol);

void set_symmetry_colour_by_symop(int imol, int state);
void set_symmetry_whole_chain(int imol, int state);
void set_symmetry_atom_labels_expanded(int state);
GtkWidget *wrapped_create_show_symmetry_window();
void symmetry_colour_adjustment_changed (GtkAdjustment *adj, 
					 GtkWidget *window); 
GtkWidget *symmetry_molecule_controller_dialog();

/*! \brief molecule number imol has a unit cell? 

/* @return 1 on "yes, it has a cell", 0 for "no" */
int has_unit_cell_state(int imol); 

/*! \brief save the symmetry coordinates of molecule number imol to
  filename 

Allow a shift of the coordinates to the origin before symmetry
expansion is apllied (this is how symmetry works in Coot
internals). */
void save_symmetry_coords(const char *filename, 
			  int imol,
			  int symop_no, 
			  int shift_a, 
			  int shift_b, 
			  int shift_c,
			  int pre_shift_to_origin_na,
			  int pre_shift_to_origin_nb,
			  int pre_shift_to_origin_nc);

void setup_save_symmetry_coords();

void save_symmetry_coords_from_fileselection(GtkWidget *fileselection);

/*! \brief set the space group for a coordinates molecule

 for shelx FA pdb files, there is no space group.  So allow the user
   to set it.  This can be initted with a HM symbol or a symm list for
   clipper */
void set_space_group(int imol, const char *spg);

/*! \brief set the cell shift search size for symmetry searching.

When the coordinates for one (or some) symmetry operator are missing
(which happens sometimes, but rarely), try changing setting this to 2
(default is 1).  It slows symmetry searching, which is why it is not
set to 2 by default.  */
void set_symmetry_shift_search_size(int shift);

/* \} */ /* end of symmetry functions */

/*  ------------------------------------------------------------------- */
/*                    file selection                                    */
/*  ------------------------------------------------------------------- */
/* section File Selection Functions */
/*! \name File Selection Functions */
/* \{ */ /* start of file selection functions */

/* so that we can save/set the directory for future fileselections
   (i.e. the new fileselection will open in the directory that the
   last one ended in) */
void set_directory_for_fileselection(GtkWidget *coords_fileselection1); 
void save_directory_from_fileselection(const GtkWidget *fileselection);
void save_directory_for_saving_from_fileselection(const GtkWidget *fileselection);
void set_file_for_save_fileselection(GtkWidget *fileselection);

/* Eleanor likes to sort her files by date when selecting a file */

/* return the button. */
GtkWidget *add_sort_button_fileselection(GtkWidget *fileselection); 

void add_ccp4i_project_optionmenu(GtkWidget *fileselection);
void add_ccp4i_projects_to_optionmenu(GtkWidget *optionmenu, GtkSignalFunc func);
void option_menu_refmac_ccp4i_project_signal_func(GtkWidget *item, GtkPositionType pos);
void run_refmac_ccp4i_option_menu_signal_func(GtkWidget *item, GtkPositionType pos);
void clear_refmac_ccp4i_project();
GtkWidget *lookup_file_selection_widgets(GtkWidget *item);

/* We wrote this button/callback by hand, most of the rest are in
   callbacks.c  */

#ifdef COOT_USE_GTK2_INTERFACE
void fileselection_sort_button_clicked( GtkWidget *sort_button,
					GtkWidget *file_list); 
#else
void fileselection_sort_button_clicked_gtk1( GtkWidget *sort_button,
					     GtkCList  *file_list); 
#endif

void push_the_buttons_on_fileselection(GtkWidget *filter_button, 
				       GtkWidget *sort_button,
				       GtkWidget *fileselection);

/* \} */ /* end of file selection functions */

/*  -------------------------------------------------------------------- */
/*                     history                                           */
/*  -------------------------------------------------------------------- */
/* section History Functions */
/*! \name  History Functions */

/* \{ */ /* end of file selection functions */
/* We don't want this exported to the scripting level interface,
   really... (that way lies madness, hehe). oh well... */

/*! \brief print the history in scheme format */
void print_all_history_in_scheme();
/*! \brief print the history in python format */
void print_all_history_in_python();

/*! \brief set a flag to show the text command equivalent of gui
  commands in the console as they happen. 

  1 for on, 0 for off. */
void set_console_display_commands_state(short int istate);

/* \} */

/*  --------------------------------------------------------------------- */
/*                  state (a graphics_info thing)                         */
/*  --------------------------------------------------------------------- */
/* info */
/*! \name State Functions */
/* \{ */

/*! \brief save the current state to the default filename */
void save_state(); 

/*! \brief save the current state to file filename */
void save_state_file(const char *filename);

/*! \brief set the default state file name (default 0-coot.state.scm) */
/* set the filename */
void set_save_state_file_name(const char *filename);
/*! \brief the save state file name 

  @return the save state file name*/
char *save_state_file_name();

/*! \brief set run state file status

0: never run it
1: ask to run it
2: run it, no questions */
void set_run_state_file_status(short int istat);
/*! \brief run the state file (reading from default filenname) */
void run_state_file();		/* just do it */
/*! \brief run the state file depending on the state variables */
void run_state_file_maybe();	/* depending on the above state variables */

GtkWidget *wrapped_create_run_state_file_dialog();

/* \} */

/*  -------------------------------------------------------------------- */
/*                     virtual trackball                                 */
/*  -------------------------------------------------------------------- */
/* subsection Virtual Trackball */
/*! \name The Virtual Trackball */
/* \{ */

#define VT_FLAT 1
#define VT_SPHERICAL 2
/*! \brief How should the mouse move the view?

mode=1 for "Flat", mode=2 for "Spherical Surface"  */
void vt_surface(int mode); 
/*! \brief return the mouse view status mode

mode=1 for "Flat", mode=2 for "Spherical Surface"  */
int  vt_surface_status();

/* \} */


/*  --------------------------------------------------------------------- */
/*                      clipping                                          */
/*  --------------------------------------------------------------------- */
/* section Clipping Functions */
/*! \name  Clipping Functions */
/* \{ */
void do_clipping1_activate();
void clipping_adjustment_changed (GtkAdjustment *adj, GtkWidget *window);

void set_clipping_back( float v);
void set_clipping_front(float v);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Unit Cell                                        */
/*  ----------------------------------------------------------------------- */
/* section Unit Cell */
/*! \name  Unit Cell */
/* \{ */

/*! \brief return the stage of show unit cell for molecule number imol */
short int get_show_unit_cell(int imol);

/*! \brief set the state of show unit cell for all molecules

1 for displayed   
0 for undisplayed */
void set_show_unit_cells_all(short int istate);

/*! \brief set the state of show unit cell for the particular molecule number imol 

1 for displayed   
0 for undisplayed */
void set_show_unit_cell(int imol, short int istate);

void set_unit_cell_colour(float red, float green, float blue);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Colour                                           */
/*  ----------------------------------------------------------------------- */
/* section Colour */
/*! \name  Colour */
/* \{ */

void set_symmetry_colour_merge(int mol_no, float v);

/*! \brief set the hue change step on reading a new molecule */
void set_colour_map_rotation_on_read_pdb(float f); 

/*! \brief shall the hue change step be used?

 @param i 0 for no, 1 for yes */
void set_colour_map_rotation_on_read_pdb_flag(short int i); 

/*! \brief shall the colour map rotation apply only to C atoms?

 @param i 0 for no, 1 for yes */
void set_colour_map_rotation_on_read_pdb_c_only_flag(short int i); 

/*! \brief colour molecule number imol by chain type */
void set_colour_by_chain(int imol); 

/*! \brief colour molecule number imol by molecule */
void set_colour_by_molecule(int imol); 

/* \} */

/*  Section Map colour*/
/*! \name   Map colour*/
/* \{ */
/*! \brief set the colour map rotation (hue change) for maps

   default: for maps is 14 degrees. */
void set_colour_map_rotation_for_map(float f); /* "global"/default */

/* widget work */
GtkWidget *wrapped_create_coords_colour_control_dialog();

/*! \brief set the colour map rotation for molecule number imol

theta is in degrees */
void  set_molecule_bonds_colour_map_rotation(int imol, float theta);

/*! \brief Get the colour map rotation for molecule number imol */
float get_molecule_bonds_colour_map_rotation(int imol); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Anisotropic Atoms */
/*  ----------------------------------------------------------------------- */
/* section Anisotropic Atoms */
/*! \name  Anisotropic Atoms */
/* \{ */
/*  we use the text interface to this in callback.c rather */
/*  than getting the float directly. */
 
float get_limit_aniso();           /* not a function of the molecule */

short int get_show_limit_aniso();  /* not a function of the molecule */

short int get_show_aniso();       /*  not a function of the molecule */

void set_limit_aniso(short int state);

void set_aniso_limit_size_from_widget(const char *text);

void set_show_aniso(int state); 

char *get_text_for_aniso_limit_radius_entry();

void set_aniso_probability(float f);

float get_aniso_probability();

/* \} */

/*  ---------------------------------------------------------------------- */
/*                         Display Functions                               */
/*  ---------------------------------------------------------------------- */
/* section Display Functions */
/*! \name  Display Functions */
/* \{ */
/*  currently doesn't get seen when the window starts due to */
/*  out-of-order issues. */

/*! \brief set the window size */
void   set_graphics_window_size(int x_size, int y_size);
/*! \brief set the window position */
void   set_graphics_window_position(int x_pos, int y_pos);
void store_graphics_window_position(int x_pos, int y_pos); /*  "configure_event" callback */

/* a general purpose version of the above, where we pass a widget flag */
void store_window_position(int window_type, GtkWidget *w);
void store_window_size(int window_type, GtkWidget *w);

/*! \brief draw a frame */
void graphics_draw(); 	/* and wrapper interface to gtk_widget_draw(glarea)  */

/*! \brief try to turn on stereo mode  */
void hardware_stereo_mode();
/*! \brief what is the stero state?

  @return 1 for in hardware stereo, 2 for side by side stereo, else return 0. */
int  stereo_mode_state();
/*! \brief try to turn on mono mode  */
void mono_mode();
/*! \brief turn on side bye side stereo mode */
void side_by_side_stereo_mode(short int use_wall_eye_mode);

/*! \brief how much should the eyes be separated in stereo mode? 

   @param f the angular difference (in multiples of 4.5 degrees) */
void set_hardware_stereo_angle_factor(float f);
/*! \brief return the hardware stereo angle factor */
float hardware_stereo_angle_factor_state();

/*! \brief set position of Model/Fit/Refine dialog */
void set_model_fit_refine_dialog_position(int x_pos, int y_pos);
/*! \brief set position of Display Control dialog */
void set_display_control_dialog_position(int x_pos, int y_pos);
/*! \brief set position of Go To Atom dialog */
void set_go_to_atom_window_position(int x_pos, int y_pos);
/*! \brief set position of Delete dialog */
void set_delete_dialog_position(int x_pos, int y_pos);
/*! \brief set position of the Rotate/Translate Residue Range dialog */
void set_rotate_translate_dialog_position(int x_pos, int y_pos);
/*! \brief set position of the Accept/Reject dialog */
void set_accept_reject_dialog_position(int x_pos, int y_pos);
/*! \brief set position of the Ramachadran Plot dialog */
void set_ramachandran_plot_dialog_position(int x_pos, int y_pos);

/* \} */

/*  ---------------------------------------------------------------------- */
/*                         Smooth "Scrolling" */
/*  ---------------------------------------------------------------------- */
/* section Smooth Scrolling */
/*! \name  Smooth Scrolling */
/* \{ */

/*! \brief set smooth scrolling 

  @param v use v=1 to turn on smooth scrolling, v=0 for off (default on). */
void set_smooth_scroll_flag(int v);

/*! \brief return the smooth scrolling state */
int  get_smooth_scroll();

void set_smooth_scroll_steps_str(const char * t);

/*  useful exported interface */
/*! \brief set the number of steps in the smooth scroll

   Set more steps (e.g. 50) for more smoothness (default 10).*/
void set_smooth_scroll_steps(int i); 

char  *get_text_for_smooth_scroll_steps();

void  set_smooth_scroll_limit_str(const char *t);

/*  useful exported interface */
/*! \brief do not scroll for distances greater this limit */
void  set_smooth_scroll_limit(float lim); 

char *get_text_for_smooth_scroll_limit();

/* \} */


/*  ---------------------------------------------------------------------- */
/*                         Font Size */
/*  ---------------------------------------------------------------------- */
/* section Font Size */
/*! \name  Font Size */
/* \{ */

/*! \brief set the font size

  @param i 1 (small) 2 (medium, default) 3 (large) */
void set_font_size(int i);

/*! \brief return the font size

  @return 1 (small) 2 (medium, default) 3 (large) */
int get_font_size();

/*! \brief set the colour of the atom label font - the arguments are
  in the range 0->1 */
void set_font_colour(float red, float green, float blue);
/* \} */

/*  ---------------------------------------------------------------------- */
/*                         Rotation Centre                                 */
/*  ---------------------------------------------------------------------- */
/* section Rotation Centre */
/*! \name  Rotation Centre */
/* \{ */
void set_rotation_centre_size_from_widget(const gchar *text); /* and redraw */
void set_rotation_centre_size(float f); /* and redraw (maybe) */
gchar *get_text_for_rotation_centre_cube_size(); 
short int recentre_on_read_pdb(); 
void set_recentre_on_read_pdb(short int);
void set_rotation_centre(float x, float y, float z);
// The redraw happens somewhere else...
void set_rotation_centre_internal(float x, float y, float z); 
float rotation_centre_position(int axis); /* only return one value: x=0, y=1, z=2 */
/* \} */

/*  ---------------------------------------------------------------------- */
/*                         orthogonal axes                                 */
/*  ---------------------------------------------------------------------- */

/* section Orthogonal Axes */
/*! \name Orthogonal Axes */
/* \{ */
/* Draw the axes in the top left? 
  
0 off, 1 on */
void set_draw_axes(int i);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  utility function                                        */
/*  ----------------------------------------------------------------------- */
/* section Atom Selection Utilities */
/*! \name  Atom Selection Utilities */
/* \{ */

/* does not account for alternative conformations properly */
/* return -1 if atom not found. */
int atom_index(int imol, const char *chain_id, int iresno, const char *atom_id); 
/* Refine zone needs to be passed atom indexes (which it then converts */
/* to residue numbers - sigh).  So we need a function to get an atom */
/* index from a given residue to use with refine_zone() */
int atom_index_first_atom_in_residue(int imol, const char *chain_id, 
				     int iresno, const char *ins_code);
float median_temperature_factor(int imol);
float average_temperature_factor(int imol);
void clear_pending_picks(); 
char *centre_of_mass_string(int imol);
/*! \brief set the default temperature factor for newly created atoms
  (initial default 20) */
void set_default_temperature_factor_for_new_atoms(float new_b);
/*! \brief return the default temperature factor for newly created atoms */
float default_new_atoms_b_factor();

/*! \brief set a numberical attibute to the atom with the given specifier.

Attributes can be "x", "y","z", "B", "occ" and the attribute val is a floating point number*/
int set_atom_attribute(int imol, const char *chain_id, int resno, const char *ins_code, const char *atom_name, const char*alt_conf, const char *attribute_name, float val);

/*! \brief set a string attibute to the atom with the given specifier.

Attributes can be "atom-name", "alt-conf" or "element". */
int set_atom_string_attribute(int imol, const char *chain_id, int resno, const char *ins_code, const char *atom_name, const char*alt_conf, const char *attribute_name, const char *attribute_value);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                            skeletonization                               */
/*  ----------------------------------------------------------------------- */
/* section Skeletonization */
/*! \name  Skeletonization */
/* \{ */
void skel_greer_on(); 
void skel_greer_off(); 

void skel_foadi_on(); 
void skel_foadi_off(); 

void skeletonize_map_by_optionmenu(GtkWidget *optionmenu);
void skeletonize_map_single_map_maybe(GtkWidget *window, int imol); 

/*! \brief skeletonize molecule number imol

   the prune_flag should almost  always be 0.  */
int skeletonize_map(int prune_flag, int imol);

/*! \brief undisplay the skeleton on molecule number imol */
int unskeletonize_map(int imol); 


void fill_option_menu_with_skeleton_options(GtkWidget *option_menu); /* a wrapper */
void set_initial_map_for_skeletonize(); /* set graphics_info variable
					   for use in callbacks.c */

/*! \brief set the skeleton search depth, used in baton building

  For high resolution maps, you need to search deeper down the skeleton tree.  This 
  limit needs to be increased to 20 or so for high res maps (it is 10 by default)  */
void set_max_skeleton_search_depth(int v); /* for high resolution
					      maps, change to 20 or
					      something (default 10). */

/* set the radio buttons in the frame to the be on or off for the map
   that is displayed in the optionmenu (those menu items "active"
   callbacks (graphics_info::skeleton_map_select change
   g.map_for_skeletonize).  */
void set_on_off_skeleton_radio_buttons(GtkWidget *skeleton_frame);
void set_on_off_single_map_skeleton_radio_buttons(GtkWidget *skeleton_frame, int imol);


/*  ----------------------------------------------------------------------- */
/*                  skeletonization level widgets                           */
/*  ----------------------------------------------------------------------- */

gchar *get_text_for_skeletonization_level_entry(); 

void set_skeletonization_level_from_widget(const char *txt); 

gchar *get_text_for_skeleton_box_size_entry(); 

void set_skeleton_box_size_from_widget(const char *txt); 

/*! \brief the box size (in Angstroms) for which the skeleton is displayed */
void set_skeleton_box_size(float f);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                        Skeleton                                          */
/*  ----------------------------------------------------------------------- */
/* section Skeleton Colour */
/*! \name  Skeleton Colour */
/* \{ */
void handle_skeleton_colour_change(int mol, gdouble* map_col);
void set_skeleton_colour(int imol, float r, float g, float b);

gdouble* get_skeleton_colour(); 

/* \} */

/*  ----------------------------------------------------------------------- */
/*                         read a ccp4 map                                  */
/*  ----------------------------------------------------------------------- */
/* section Read CCP4 Map */
/*! \name  Read CCP4 Map */
/* \{ */

/*! \brief read a CCP4 map */
void handle_read_ccp4_map(const char* filename, int is_diff_map_flag); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                        save coordintes                                   */
/*  ----------------------------------------------------------------------- */
/* section Save Coordinates */
/*! \name  Save Coordinates */
/* \{ */


void save_coordinates_using_widget(GtkWidget *widget); /* do a get_user_data for
					     the molecule and a lookup
					     of the entry? to find the
					     filename in c-interface,
					     not in the callback.c  */
/*! \brief save coordinates of molecule number imol in filename

  @return status 1 is good (success), 0 is fail. */
int save_coordinates(int imol, const char *filename);

void set_save_coordinates_in_original_directory(int i);

/* not really a button select, its a menu item select */
/* not productive */
void save_molecule_coords_button_select(GtkWidget *item, GtkPositionType pos); 

/* access to graphics_info_t::save_imol for use in callback.c */
int save_molecule_number_from_option_menu();
/* access from callback.c, not to be used in scripting, I suggest.
   Sets the *save* molecule number */
void set_save_molecule_number(int imol);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                        .phs file reading                                 */
/*  ----------------------------------------------------------------------- */
/* section Read Phases File Functions */
/*! \name  Read Phases File Functions */
/* \{ */

/*! \brief read phs file use coords to get cell and symm to make map 

uses pending data to make the map.

*/
void
read_phs_and_coords_and_make_map(const char *pdb_filename);

/*! \brief read a phs file, the cell and symm information is from
  previously read (most recently read) coordinates file

 For use with phs data filename provided on the command line */
int 
read_phs_and_make_map_using_cell_symm_from_previous_mol(const char *phs_filename);


/*! \brief read phs file and use a previously read molecule to provide
  the cell and symmetry information

@return the new molecule number, return -1 if problem creating the map
(e.g. not phs data, file not found etc).  */
int
read_phs_and_make_map_using_cell_symm_from_mol(const char *phs_filename, int imol);

int
read_phs_and_make_map_using_cell_symm_from_mol_using_implicit_phs_filename(int imol);

/*! \brief read phs file use coords to use cell and symm to make map */
int
read_phs_and_make_map_using_cell_symm(const char *phs_file_name,
				      const char *hm_spacegroup, float a, float b, float c,
				      float alpha, float beta, float gamma); /*!< in degrees */

/*! \brief read a phs file and use the cell and symm in molecule
  number imol and use the resolution limits reso_lim_high (in Angstroems).

@param imol is the molecule number of the reference (coordinates)
molecule from which the cell and symmetry can be obtained.

@param phs_file_name is the name of the phs data file.

@param reso_lim_high is the high resolution limit in Angstroems.

@param reso_lim_low the low resoluion limit (currently ignored).  */
int 
read_phs_and_make_map_with_reso_limits(int imol, const char* phs_file_name,
				       float reso_lim_low, float reso_lim_high); 

/* work out the spacegroup from the given symm operators, e.g. return "P 1 21 1" 
given "x,y,z ; -x,y+1/2,-z" */
/* char * */
/* spacegroup_from_operators(const char *symm_operators_in_clipper_format);  */

void
graphics_store_phs_filename(const gchar *phs_filename); 

const char* graphics_get_phs_filename();

short int possible_cell_symm_for_phs_file(); 

gchar *get_text_for_phs_cell_chooser(int imol, char *field); 

/* \} */

/*  ----------------------------------------------------------------------- */
/*                                  Movement                                */
/*  ----------------------------------------------------------------------- */
/* section Graphics Move */
/*! \name Graphics Move */
/* \{ */
/*! \brief undo last move  */
void undo_last_move(); // suggested by Frank von Delft

/*! \brief translate molecule number imol by (x,y,z) in Angstroms  */
void translate_molecule_by(int imol, float x, float y, float z);

/*! \brief transform molecule number imol by the given rotation
  matrix, then translate by (x,y,z) in Angstroms  */
void transform_molecule_by(int imol, 
			   float m11, float m12, float m13,
			   float m21, float m22, float m23,
			   float m31, float m32, float m33,
			   float x, float y, float z);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                        go to atom widget                                 */
/*  ----------------------------------------------------------------------- */
/* section Go To Atom Widget Functions */
/*! \name Go To Atom Widget Functions */
/* \{ */
GtkWidget *wrapped_create_goto_atom_window();

/*! \brief Post the Go To Atom Window */
void post_go_to_atom_window();
void fill_go_to_atom_window(GtkWidget *widget);

gchar *get_text_for_go_to_atom_chain_entry(); 
gchar *get_text_for_go_to_atom_residue_entry();
gchar *get_text_for_go_to_atom_atom_name_entry();

int go_to_atom_molecule_number();
char *go_to_atom_chain_id();
char *go_to_atom_atom_name();
int go_to_atom_residue_number();
char *go_to_atom_ins_code();
char *go_to_atom_alt_conf();


/*! \brief set the go to atom specification

   It seems important for swig that the char * arguments are const
   char *, not const gchar * (or else we get wrong type of argument
   error on (say) "A"*/
int set_go_to_atom_chain_residue_atom_name(const char *t1_chain_id, int iresno, 
					   const char *t3_atom_name);
int set_go_to_atom_chain_residue_atom_name_no_redraw(const char *t1, int iresno, const char *t3);

/* FIXME one day */
/* #ifdef __cplusplus */
/* int set_go_to_atom_from_spec(const coot::atom_spec_t &atom_spec); */
/* #endif // __cplusplus */

int set_go_to_atom_chain_residue_atom_name_strings(const gchar *t1, 
						   const gchar *t2, 
						   const gchar *txt); 

/* int goto_next_atom_maybe(const gchar *t1, const gchar *t2, const gchar *t3,  */
/* 			 GtkEntry *res_entry);  */

int goto_prev_atom_maybe(const gchar *t1, const gchar *t2, const gchar *t3, 
			 GtkEntry *res_entry); 

int goto_near_atom_maybe(const char *t1, int ires, const char *t3,
			 GtkEntry *res_entry, int idiff);

int goto_next_atom_maybe_new(GtkWidget *window);
int goto_previous_atom_maybe_new(GtkWidget *window);

/*! \brief update the Go To Atom widget entries to atom closest to
  screen centre. */
void update_go_to_atom_from_current_position(); 

/* used by keypress (return) callbacks */

/*  read the widget values and apply them to the graphics */
 
int apply_go_to_atom_values(GtkWidget * window);

/*  return an atom index */
/*! \brief what is the atom index of the given atom? */
int atom_spec_to_atom_index(int mol, char *chain, int resno, char *atom_name); 

/*! \brief what is the atom index of the given atom? */
int full_atom_spec_to_atom_index(int imol, const char *chain, int resno,
				 const char *inscode, const char *atom_name,
				 const char *altloc); 

/*! \brief update the Go To Atom window */
void update_go_to_atom_window_on_changed_mol(int imol);

/*! \brief update the Go To Atom window.  This updates the option menu
  for the molecules. */
void update_go_to_atom_window_on_new_mol();

void update_go_to_atom_window_on_other_molecule_chosen(int imol);

/*! \brief set the molecule for the Go To Atom

   For dynarama callback sake. The widget/class knows which mapview
   molecule that it was generated from, so in order to go to the
   molecule from dynarama, we first need to the the molecule - because
   set_go_to_atom_chain_residue_atom_name() does not mention the
   molecule (see "Next/Previous Residue" for reasons for that).  This
   function simply calls the graphics_info_t function of the same
   name.

   Also used in scripting, where go-to-atom-chain-residue-atom-name
   does not mention the molecule number. */
void set_go_to_atom_molecule(int imol); 

int go_to_atom_molecule_optionmenu_active_molecule(GtkWidget *widget); 


void save_go_to_atom_widget(GtkWidget *widget); /* store in a static */
void unset_go_to_atom_widget(); /* unstore the static go_to_atom_window */


void clear_atom_list(GtkWidget *atom_gtklist);

void apply_go_to_atom_from_widget(GtkWidget *widget);

void
on_go_to_atom_residue_list_select_child (GtkList         *list,
					 GtkWidget       *widget,
					 gpointer         user_data);

#ifdef COOT_USE_GTK2_INTERFACE
/* stuff moved into graphics_info_t */
#else
void on_go_to_atom_residue_tree_selection_changed_gtk1 (GtkList         *gtktree,
							gpointer         user_data);
#endif

/* Nothing calls this? */
/* void */
/* on_go_to_atom_residue_list_selection_changed (GtkList         *list, */
/* 						   gpointer         user_data); */


#ifdef COOT_USE_GTK2_INTERFACE

#else
void on_go_to_atom_atom_list_selection_changed_gtk1 (GtkList         *list,
						gpointer         user_data);
void
on_go_to_atom_residue_list_unselect_child (GtkList         *list,
					   GtkWidget       *widget,
					   gpointer         user_data);
#endif
/* \} */


/*  ----------------------------------------------------------------------- */
/*                  autobuilding control                                    */
/*  ----------------------------------------------------------------------- */
/* section AutoBuilding functions (Defunct) */
/* void autobuild_ca_on();  - moved to junk */

void autobuild_ca_off(); 

void test_fragment(); 

void do_skeleton_prune();

int test_function(int i, int j);


/*  ----------------------------------------------------------------------- */
/*                  map and molecule control                                */
/*  ----------------------------------------------------------------------- */
/* section Map and Molecule Control */
/*! \name Map and Molecule Control */
/* \{ */
void save_display_control_widget_in_graphics(GtkWidget *widget); 

GtkWidget *wrapped_create_display_control_window();

/*! \brief display the Display Constrol window  */
void post_display_control_window(); 

void add_map_display_control_widgets(); 
void add_mol_display_control_widgets(); 
void add_map_and_mol_display_control_widgets(); 

void reset_graphics_display_control_window(); 
void close_graphics_display_control_window(); /* destroy widget */

/*! \brief make the map displayed/undisplayed, 0 for off, 1 for on */
void set_map_displayed(int imol, int state);
/*! \brief make the coordinates molecule displayed/undisplayed, 0 for off, 1 for on */
void set_mol_displayed(int imol, int state);
/*! \brief make the coordinates molecule active/inactve (clickable), 0
  for off, 1 for on */
void set_mol_active(int imol, int state);


/*! \brief return the display state of molecule number imol 

 @return 1 for on, 0 for off
*/
int mol_is_displayed(int imol); 
/*! \brief return the active state of molecule number imol
 @return 1 for on, 0 for off */
int mol_is_active(int imol); 
/*! \brief return the display state of molecule number imol
 @return 1 for on, 0 for off */
int map_is_displayed(int imol); 

/*! \brief if on_or_off is 0 turn off all maps displayed, for other
  values of on_or_off turn on all maps */
void set_all_maps_displayed(int on_or_off);

/*! \brief if on_or_off is 0 turn off all models displayed and active,
  for other values of on_or_off turn on all models. */
void set_all_models_displayed_and_active(int on_or_off);

/*! \brief return the spacegroup of molecule number imol 

@return "No Spacegroup" when the spacegroup of a molecule has not been
set.*/
char *show_spacegroup(int imol);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Merge Molecules                                  */
/*  ----------------------------------------------------------------------- */
/* section Merge Molecules */
GtkWidget *wrapped_create_merge_molecules_dialog();
void do_merge_molecules_gui();
void do_merge_molecules(GtkWidget *dialog);
void fill_vbox_with_coordinates_options(GtkWidget *vbox,
					GtkSignalFunc checkbox_callback_func);
void merge_molecules_menu_item_activate(GtkWidget *item, 
					GtkPositionType pos);
void on_merge_molecules_check_button_toggled (GtkToggleButton *togglebutton,
					      gpointer         user_data);

#ifdef __cplusplus/* protection from use in callbacks.c, else compilation probs */
#ifdef USE_GUILE
SCM merge_molecules(SCM add_molecules, int imol);
#endif

#ifdef USE_PYTHON
// Bernhard, fill me in...
#endif 
#endif	/* c++ */


/*  ----------------------------------------------------------------------- */
/*                         Mutate Sequence and Loops GUI                    */
/*  ----------------------------------------------------------------------- */
/* section Mutate Sequence and Loops GUI */
GtkWidget *wrapped_create_mutate_sequence_dialog();
void do_mutate_sequence(GtkWidget *dialog); 
void mutate_sequence_molecule_menu_item_activate(GtkWidget *item, 
						 GtkPositionType pos);
/* void fill_chain_option_menu(GtkWidget *chain_option_menu, int imol); */
/* the generic form of the above - also used by superpose chain optionmenu */
/* void fill_chain_option_menu_with_callback(GtkWidget *chain_option_menu, 
					  int imol,
					  GtkSignalFunc callback); */
void mutate_sequence_chain_option_menu_item_activate (GtkWidget *item,
						      GtkPositionType pos);

GtkWidget *wrapped_fit_loop_dialog();
void fit_loop_from_widget(GtkWidget *w);

/*  ----------------------------------------------------------------------- */
/*                         Align and Mutate GUI                             */
/*  ----------------------------------------------------------------------- */
/* section Align and Mutate */
/*! \name  Align and Mutate */
/* \{ */
GtkWidget *wrapped_create_align_and_mutate_dialog();
/* return the handled_state, so that we know if we should kill the dialog or not */
int do_align_mutate_sequence(GtkWidget *w);
void align_and_mutate_molecule_menu_item_activate(GtkWidget *item, 
						  GtkPositionType pos);
void align_and_mutate_chain_option_menu_item_activate (GtkWidget *item,
						       GtkPositionType pos);

void align_and_mutate(int imol, const char *chain_id, const char *fasta_maybe);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Renumber residue range                           */
/*  ----------------------------------------------------------------------- */
/* section Renumber Residue Range */
/*! \name Renumber Residue Range */

/* \{ */
/*! \brief renumber the given residue range by offset residues */
int renumber_residue_range(int imol, const char *chain_id, 
			   int start_res, int last_res, int offset);

GtkWidget *wrapped_create_renumber_residue_range_dialog();
void renumber_residues_from_widget(GtkWidget *window);

/*! \brief change chain id for given residue number range  */
int change_residue_number(int imol, const char *chain_id, int current_resno, const char *current_inscode, int new_resno, const char *new_inscode);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                         Change chain id                                  */
/*  ----------------------------------------------------------------------- */
/* section Change Chain ID */
GtkWidget *wrapped_create_change_chain_id_dialog();
void change_chain_id_by_widget(GtkWidget *w);
void change_chain_ids_mol_option_menu_item_activate(GtkWidget *item,
						    GtkPositionType pos);
void change_chain_ids_chain_menu_item_activate(GtkWidget *item,
					       GtkPositionType pos);

void  change_chain_id(int imol, const char *from_chain_id, const char *to_chain_id, 
		      short int use_res_range_flag, int from_resno, int to_resno);

/*  ----------------------------------------------------------------------- */
/*                  scripting                                               */
/*  ----------------------------------------------------------------------- */
/* section Scripting */

/*! \name Scripting */
/* \{ */

/*! \brief pop-up a scripting window */
void post_scripting_window(); 

/* called from c-inner-main */
void run_command_line_scripts();

void setup_guile_window_entry(GtkWidget *entry); 
void setup_python_window_entry(GtkWidget *entry); 

/*  Check if this is needed still, I think not. */
#ifdef USE_GUILE
void guile_window_enter_callback( GtkWidget *widget,
				  GtkWidget *entry ); 
#endif /* USE_GUILE */

#ifdef USE_PYTHON
void python_window_enter_callback( GtkWidget *widget,
				   GtkWidget *entry ); 
#endif /* USE_PYTHON */

void set_guile_gui_loaded_flag(); 
void set_found_coot_gui(); 

/* Accession code */
void handle_get_accession_code(GtkWidget *widget); 

/* Libcheck monomer code */
void handle_get_libcheck_monomer_code(GtkWidget *widget); 

/*! \brief import libcheck monomer give the 3-letter code. 

@return the new molecule number, if not -1 (error). */
int get_monomer(const char *three_letter_code);


int
handle_make_monomer_search(const char *text, GtkWidget *viewport);



/*  Don't let this be seen by standard c, since I am using a std::string */
/*  and now we make it return a value, which we can decode in the calling
    function. I make a dummy version for when GUILE is not being used in case
    there are functions in the rest of the code that call safe_scheme_command
    without checking if there is USE_GUILE first.*/

/*! \brief run script file */
void run_script       (const char *filename);
void run_guile_script (const char *filename);
void run_python_script(const char *filename);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  regularize/refine                                       */
/*  ----------------------------------------------------------------------- */
/* section Regularization and Refinement */
/*! \name  Regularization and Refinement */
/* \{ */

void do_regularize(short int state); /* pass 0 for off (unclick togglebutton) */
void do_refine(short int state);

/*! \brief add a restraint on peptides to make them planar 

  This adds a 5 atom restraint that includes both CA atoms of the
  peptide.  Use this rather than editting the mon_lib_list.cif file. */
void add_planar_peptide_restraints(); 

/*! \brief remove restraints on peptides to make them planar. */
void remove_planar_peptide_restraints();

/*! \brief add restraints on the omega angle of the peptides

  (that is the torsion round the peptide bond).  Omega angles that are
  closer to 0 than to 180 will be refined as cis peptides (and of
  course if omega is greater than 90 then the peptide will be refined
  as a trans peptide (this is the normal case). */
void add_omega_torsion_restriants(); 

/*! \brief remove omega restraints on CIS and TRANS linked residues. */
void remove_omega_torsion_restriants(); 

/*! \brief set immediate replacement mode for refinement and
  regularization.  You need this (call with istate=1) if you are
  scripting refinement/regularization  */
void set_refinement_immediate_replacement(int istate);

/*! \brief query the state of the immediate replacement mode */
int  refinement_immediate_replacement_state();

/*! \brief set the number of frames for which the selected residue
  range flashes 

 On fast computers, this can be set to higher than the default for
 more aesthetic appeal. */
void set_residue_selection_flash_frames_number(int i);

/*! \brief accept the new positions of the regularized or refined residues 

    If you are scripting refinement and/or regularization, this is the
    function that you need to call after refine-zone or regularize-zone.  */
void accept_regularizement();
void clear_up_moving_atoms();	/* remove the molecule and bonds */
void clear_moving_atoms_object(); /* just get rid of just the bonds (redraw done here). */
/* void fill_option_menu_with_map_options(GtkWidget *option_menu); old */
/* now we use */
void fill_option_menu_with_refine_options(GtkWidget *option_menu);

void do_torsions_toggle(GtkWidget *button);
void do_peptide_torsions_toggle();

/*! \brief turn on (or off) torsion restraints 

   Pass with istate=1 for on, istate=0 for off.
*/
void set_refine_with_torsion_restraints(int istate); 
void set_refine_params_toggle_buttons(GtkWidget *button);

/*! \brief set the type or phi/psi restraints

either alpha helix, beta strand or ramachandran goodness
(simple_restraint.hh link torsions) */
/*    enum link_torsion_restraints_type { NO_LINK_TORSION = 0,  */
/* 					  LINK_TORSION_RAMACHANDRAN_GOODNESS = 1, */
/* 					  LINK_TORSION_ALPHA_HELIX = 2, */
/* 					  LINK_TORSION_BETA_STRAND = 3 };  */
void set_refine_params_phi_psi_restraints_type(int restraints_type);

/*! \brief set the relative weight of the geometric terms to the map terms 

 The default is 60.

 The higher the number the more weight that is given to the map terms
 but the resulting @math{\chi^2} values are higher).  This will be
 needed for maps generated from data not on (or close to) the absolute
 scale or maps that have been scaled (for example so that the sigma
 level has been scaled to 1.0).

*/
void set_matrix(float f);

/*! \brief return the relative weight of the geometric terms to the map terms. */
float matrix_state();

/*! \brief change the +/- step for autoranging (default is 1)

Auto-ranging alow you to select a range from one button press, this
allows you to set the number of residues either side of the clicked
residue that becomes the selected zone */
void set_refine_auto_range_step(int i);

/*! \brief set the heuristic fencepost for the maximum number of
  residues in the refinement/regularization residue range  

  Default is 20

*/
void set_refine_max_residues(int n);

/*! \brief refine a zone based on atom indexing */
void refine_zone_atom_index_define(int imol, int ind1, int ind2); 

/*! \brief refine a zone

 presumes that imol_Refinement_Map has been set */
void refine_zone(int imol, const char *chain_id, int resno1, int resno2, const char *altconf);

/*! \brief refine a zone using auto-range

 presumes that imol_Refinement_Map has been set */
void refine_auto_range(int imol, const char *chain_id, int resno1, const char *altconf);

/*! \brief regularize a zone
  */
void regularize_zone(int imol, const char *chain_id, int resno1, int resno2, const char *altconf);

/*! \brief set the number of refinement steps applied to the
  intermediate atoms each frame of graphics.

  smaller numbers make the movement of the intermediate atoms slower,
  smoother, more elegant.

  Default: 50. */
void set_dragged_refinement_steps_per_frame(int v);

/*! \brief return the number of steps per frame in dragged refinement */
int dragged_refinement_steps_per_frame();

/*! \brief allow refinement of intermediate atoms after dragging,
  before displaying (default: 0, off).

   An attempt to do something like xfit does, at the request of Frank
   von Delft.

   Pass with istate=1 to enable this option. */
void set_refinement_refine_per_frame(int istate);

/*! \brief query the state of the above option */
int refinement_refine_per_frame_state(); 

/*! \brief correct the sign of chiral volumes before commencing refinement?
   
   Do we want to fix chiral volumes (by moving the chiral atom to the
   other side of the chiral plane if necessary).  Default yes
   (1). Note: doesn't work currently. */
void set_fix_chiral_volumes_before_refinement(int istate);

/*! \brief query the state of the above option */
void check_chiral_volumes(int imol);

void check_chiral_volumes_from_widget(GtkWidget *window); 
void fill_chiral_volume_molecule_option_menu(GtkWidget *w);
void chiral_volume_molecule_option_menu_item_select(GtkWidget *item, GtkPositionType pos);

/*! \brief For experienced Cooters who don't like Coot nannying about
  chiral volumes during refinement. */
void set_show_chiral_volume_errors_dialog(short int istate);

/*! \brief set the type of secondary structure restraints

0 no sec str restraints

1 alpha helix restraints

2 beta strand restraints */
void set_secondary_structure_restraints_type(int itype);

/*! \brief return the secondary structure restraints type */
int secondary_structure_restraints_type();

/*! \brief the molecule number of the map used for refinement 

   @return the map number, if it has been set or there is only one
   map, return -1 on no map set (ambiguous) or no maps.
*/
   
int imol_refinement_map();	/* return -1 on no map */

/*! \brief set the molecule number of the map to be used for
  refinement/fitting.

   @return imol on success, -1 on failure*/
int set_imol_refinement_map(int imol);	/* returns imol on success, otherwise -1 */

/*! \brief Does the residue exist? (Raw function) 

   @return 0 on not-exist, 1 on does exist.
*/
int does_residue_exist_p(int imol, char *chain_id, int resno, char *inscode); 

/* \} */

/*  ----------------------------------------------------------------------- */
/*               Simplex Refinement                                         */
/*  ----------------------------------------------------------------------- */
/*! \name Simplex Refinement */
/*! \{ */

/*! \brief refine residue range using simplex optimization */
void
fit_residue_range_to_map_by_simplex(int res1, int res2, char *altloc, char *chain_id, int imol, int imol_for_map); 

/*! \brief simply score the residue range fit to map */
float
score_residue_range_fit_to_map(int res1, int res2, char *altloc, char *chain_id, int imol, int imol_for_map); 
/*! \} */

/*  ----------------------------------------------------------------------- */
/*               Nomenclature Errors                                        */
/*  ----------------------------------------------------------------------- */
/*! \name Nomenclature Errors */
/* \{ */
/*! \brief fix nomenclature errors in molecule number imol

   @return the number of resides altered. */
int fix_nomenclature_errors(int imol);

/* \} */

/*  ----------------------------------------------------------------------- */
/*               Move Molecule Here                                        */
/*  ----------------------------------------------------------------------- */
/*! \name move molecule here (wrapper to scheme function) */
/* { */
GtkWidget *wrapped_create_move_molecule_here_dialog();
void move_molecule_here_by_widget(GtkWidget *w);
/* } */

/*  ----------------------------------------------------------------------- */
/*               Atom info                                                  */
/*  ----------------------------------------------------------------------- */
/* section Atom Info */
/*! \name Atom Info */
/* \{ */

/*! \brief output Atom Info for the give atom specs

   Actually I want to return a scheme object with occ, pos, b-factor
   info */
void
output_atom_info_as_text(int imol, const char *chain_id, int resno,
			 const char *ins_code, const char *atname,
			 const char *altconf);

/* \} */

/*  ----------------------------------------------------------------------- */
/*               (Eleanor's) Residue info                                   */
/*  ----------------------------------------------------------------------- */
/* section Residue Info */
/*! \name Residue Info */
/* \{ */
/* Similar to above, we need only one click though. */
void do_residue_info();
void output_residue_info    (int atom_index, int imol); /* widget version */
void output_residue_info_as_text(int atom_index, int imol); /* text version */
/* functions that uses mmdb_manager functions/data types moved to graphics_info_t */

void apply_residue_info_changes(GtkWidget *widget);
void do_distance_define();
void do_angle_define();
void do_torsion_define();
void residue_info_apply_all_checkbutton_toggled();
GtkWidget *wrapped_create_residue_info_dialog();
void clear_residue_info_edit_list();

/* a graphics_info_t function wrapper: */
void residue_info_release_memory(GtkWidget *widget); 
void unset_residue_info_widget(); 
void clear_simple_distances();
void clear_last_simple_distance();
void store_geometry_dialog(GtkWidget *w);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  residue enviroment                                      */
/*  ----------------------------------------------------------------------- */
/* section Residue Environment Functions */
void fill_environment_widget(GtkWidget *widget);
void execute_environment_settings(GtkWidget *widget);
void toggle_environment_show_distances(GtkToggleButton *button); 
/*! \brief show environment distances.  If state is 0, distances are
  turned off, otherwise distances are turned on. */
void set_show_environment_distances(int state);
int show_environment_distances_state();
/*! \brief min and max distances for the environment distances */
void set_environment_distances_distance_limits(float min_dist, float max_dist);



/*  ----------------------------------------------------------------------- */
/*                  pointer distances                                      */
/*  ----------------------------------------------------------------------- */
/* section Pointer Functions */
/*! \name Pointer Functions */
/* \{ */
void fill_pointer_distances_widget(GtkWidget *widget);
void execute_pointer_distances_settings(GtkWidget *widget);
void toggle_pointer_distances_show_distances(GtkToggleButton *button); 
void set_show_pointer_distances(int istate);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  zoom                                                    */
/*  ----------------------------------------------------------------------- */
/* section Zoom Functions */
/*! \name Zoom Functions */
/* \{ */
/*! \brief scale the view by f 

   external (scripting) interface (with redraw) 
    @param f the smaller f, the bigger the zoom, typical value 1.3*/
void scale_zoom(float f);  
/* internal interface */
void scale_zoom_internal(float f);  
/*! \brief return the current zoom factor */
float zoom_factor(); 

/*! \brief set smooth scroll with zoom 
   @param i 0 means no, 1 means yes: (default 0) */
void set_smooth_scroll_do_zoom(int i);
/* default 1 (on) */
/*! \brief return the state of the above system */
int      smooth_scroll_do_zoom();   
float    smooth_scroll_zoom_limit(); 
void set_smooth_scroll_zoom_limit(float f);
void set_zoom_adjustment(GtkWidget *w);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  CNS data stuff                                          */
/*  ----------------------------------------------------------------------- */
/*! \name CNS Data Functions */
/* \{ */
/*! \brief read CNS data (currently only a placeholder)  */
int handle_cns_data_file(const char *filename, int imol);

/*! \brief read CNS data (currently only a placeholder)

a, b,c are in Angstroems.  alpha, beta, gamma are in degrees.  spg is
the space group info, either ;-delimited symmetry operators or the
space group name*/
int handle_cns_data_file_with_cell(const char *filename, int imol, float a, float b, float c, float alpha, float beta, float gamma, const char *spg_info);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  cif stuff                                               */
/*  ----------------------------------------------------------------------- */
/* section mmCIF Functions */
/*! \name mmCIF Functions */
/* dataset stuff */
/* \{ */
int auto_read_cif_data_with_phases(const char *filename); 
int read_cif_data_with_phases_sigmaa(const char *filename); 
int read_cif_data_with_phases_diff_sigmaa(const char *filename); 
int read_cif_data(const char *filename, int imol_coords);
int read_cif_data_2fofc_map(const char *filename, int imol_coords);  
int read_cif_data_fofc_map(const char *filename, int imol_coords);  
int read_cif_data_with_phases_fo_fc(const char *filename); 
int read_cif_data_with_phases_2fo_fc(const char *filename);
int read_cif_data_with_phases_nfo_fc(const char *filename,
				     int map_type);
int read_cif_data_with_phases_fo_alpha_calc(const char *filename);


/*                  cif (geometry) dictionary                            */
void handle_cif_dictionary(const char *filename);
void read_cif_dictionary(const char *filename);
int write_connectivity(const char* monomer_name, const char *filename);

/* Use the environment variable COOT_REFMAC_LIB_DIR to find cif files
   in subdirectories and import them all. */
void import_all_refmac_cifs(); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  SHELX stuff                                             */
/*  ----------------------------------------------------------------------- */
/* section SHELXL Functions */
/*! \name SHELXL Functions */
/* \{ */
int read_shelx_ins_file(const char *filename);
int write_shelx_ins_file(int imol, const char *filename);
/* for shelx fcf file that needs to be filtered: */
int handle_shelx_fcf_file_internal(const char *filename);
#ifdef __cplusplus/* protection from use in callbacks.c, else compilation probs */
#ifdef USE_GUILE
/*! \brief @return the chain id for the given residue.  Return #f if
  can't do it/fail. */
SCM chain_id_for_shelxl_residue_number(int imol, int resno);
#endif 
void add_shelx_string_to_molecule(int imol, const char *string);

#ifdef USE_PYTHON
/* Fill me in Bernhard */
#endif 
#endif 

/* \} */
/*  ------------------------------------------------------------------------ */
/*                         Validation:                                       */
/*  ------------------------------------------------------------------------ */
/* section Validation Functions */
/*! \name Validation Functions */
/* \{ */
void deviant_geometry(int imol);
short int is_valid_model_molecule(int imol);
short int is_valid_map_molecule(int imol);

void free_geometry_graph(GtkWidget *dialog); /* free the lines in the widget  */
void unset_geometry_graph(GtkWidget *dialog); /* set the graphics info
						 static to NULL, so
						 that we on longer try
						 to update the
						 widget*/

void add_on_validation_graph_mol_options(GtkWidget *menu, const char *type_in);
void my_delete_validaton_graph_mol_option(GtkWidget *widget, void *);
void validation_graph_b_factor_mol_selector_activate (GtkMenuItem     *menuitem,
						      gpointer         user_data);
void validation_graph_geometry_mol_selector_activate (GtkMenuItem     *menuitem,
						      gpointer         user_data);
void validation_graph_omega_mol_selector_activate (GtkMenuItem     *menuitem,
						   gpointer         user_data);
void validation_graph_rotamer_mol_selector_activate (GtkMenuItem     *menuitem,
						   gpointer         user_data);
void validation_graph_density_fit_mol_selector_activate (GtkMenuItem     *menuitem,
						   gpointer         user_data);
void gln_and_asn_b_factor_outlier_mol_selector_activate (GtkMenuItem     *menuitem,
							 gpointer         user_data);

void probe_mol_selector_activate (GtkMenuItem     *menuitem,
				  gpointer         user_data);

/* These are called right at the beginning (main) */
/* old style not-generic menu initialization */
/* void create_initial_validation_graph_b_factor_submenu(GtkWidget *window1); */
/* void create_initial_validation_graph_geometry_submenu(GtkWidget *window1); */
/* void create_initial_validation_graph_omega_submenu(GtkWidget *window1); */

/*! \brief generate a list of difference map peaks */
void difference_map_peaks(int imol, int imol_coords, float level, int do_positive_level_flag, int do_negative_level_flag); 

void difference_map_peaks_by_widget(GtkWidget *dialog);
void set_difference_map_peaks_widget(GtkWidget *w);

void clear_diff_map_peaks();
GtkWidget *wrapped_create_generate_diff_map_peaks_dialog();

/*! \brief Make a gui for GLN adn ASN B-factor outiers, compairing the
  O and N temperature factore idfference to the distribution of
  temperature factors from the other atoms.  */
void gln_asn_b_factor_outliers(int imol);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  ramachandran plot                                       */
/*  ----------------------------------------------------------------------- */
/* section Ramachandran Plot Functions */
/*! \name Ramachandran Plot Functions */
/* \{ */
/* Note for optionmenu from the main window menubar, we should use
   code like this, rather than the map_colour/attach_scroll_wheel code
   (actually, they are mostly the same, differing only the container
   delete code). */

/*! \brief Ramachandran plot for molecule number imol */
void do_ramachandran_plot(int imol);
/*  the the menu */
void add_on_rama_choices();

/*! \brief set the contour levels for theremachandran plot, default
  values are 0.02 (prefered) 0.002 (allowed) */
void set_ramachandran_plot_contour_levels(float level_prefered, float level_allowed);
/*! \brief set the ramachandran plot background block size. Smaller is
  smoother but slower.  Should be divisible exactly into 360.  Default value is 10. */
void set_ramachandran_plot_background_block_size(float blocksize) ;

void my_delete_ramachandran_mol_option(GtkWidget *widget, void *);

/* call with value non-zero for on, 0 for off/not. */

/* This should not be used for scripting. */
 
/*  If called with 0, it checks to see if it was previously non-zero, */
/*  if so, then it does a get_user_data to find the pointer to the */
/*  object and deletes it. */
void set_dynarama_is_displayed(GtkWidget *dynarama_widget, int imol);
GtkWidget *dynarama_is_displayed_state(int imol);

/*  return -1 on error. */
int get_mol_from_dynarama(GtkWidget *window);

void set_moving_atoms(double phi, double psi);

void accept_phi_psi_moving_atoms();
void setup_edit_phi_psi(short int state);	/* a button callback */

void destroy_edit_backbone_rama_plot();

/* and the 2 molecule ramachandran plot */
void ramachandran_plot_differences(int imol1, int imol2);

void ramachandran_plot_differences_by_chain(int imol1, int imol2, 
					    const char *a_chain, const char *b_chain);

GtkWidget *wrapped_ramachandran_plot_differences_dialog();
int  do_ramachandran_plot_differences_by_widget(GtkWidget *w); /* return status */
void fill_ramachandran_plot_differences_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
								       int is_first_mol_flag);
void ramachandran_plot_differences_mol_option_menu_activate_first(GtkWidget *item, GtkPositionType pos);
void ramachandran_plot_differences_mol_option_menu_activate_second(GtkWidget *item, GtkPositionType pos);
void ramachandran_plot_differences_chain_option_menu_activate_first(GtkWidget *item, GtkPositionType pos);
void ramachandran_plot_differences_chain_option_menu_activate_second(GtkWidget *item, GtkPositionType pos);
/* \} */

/*  ----------------------------------------------------------------------- */
/*           sequence_view                                                  */
/*  ----------------------------------------------------------------------- */
/*! \name Sequence View  */
/* \{ */
void do_sequence_view(int imol);
void add_on_sequence_view_choices();
void set_sequence_view_is_displayed(GtkWidget *widget, int imol); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*           rotate moving atoms peptide                                    */
/*  ----------------------------------------------------------------------- */
void change_peptide_carbonyl_by(double angle);/*  in degrees. */
void change_peptide_peptide_by(double angle);  /* in degress */
void execute_setup_backbone_torsion_edit(int imol, int atom_index);
void setup_backbone_torsion_edit(short int state);

void set_backbone_torsion_peptide_button_start_pos(int ix, int iy);
void change_peptide_peptide_by_current_button_pos(int ix, int iy);
void set_backbone_torsion_carbonyl_button_start_pos(int ix, int iy);
void change_peptide_carbonyl_by_current_button_pos(int ix, int iy);

/*  ----------------------------------------------------------------------- */
/*                  atom labelling                                          */
/*  ----------------------------------------------------------------------- */
/* The guts happens in molecule_class_info_t, here is just the
   exported interface */
/* section Atom Labelling */
/*! \name Atom Labelling */
/* \{ */
/*  Note we have to search for " CA " etc */
int    add_atom_label(int imol, char *chain_id, int iresno, char *atom_id); 
int remove_atom_label(int imol, char *chain_id, int iresno, char *atom_id); 
void remove_all_atom_labels();

void set_label_on_recentre_flag(int i); /* 0 for off, 1 or on */

int centre_atom_label_status();

/*! \brief use brief atom names for on-screen labels

 call with istat=1 to use brief labels, istat=0 for normal labels */
void set_brief_atom_labels(int istat);

/*! \brief the brief atom label state */
int brief_atom_labels_state();
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  scene rotation                                          */
/*  ----------------------------------------------------------------------- */
/* section Screen Rotation */
/*! \name Screen Rotation */
/* stepsize in degrees */
/* \{ */
/*! \brief rotate view round y axis stepsize degrees for nstep such steps */
void rotate_y_scene(int nsteps, float stepsize); 
/*! \brief rotate view round x axis stepsize degrees for nstep such steps */
void rotate_x_scene(int nsteps, float stepsize);
/*! \brief rotate view round z axis stepsize degrees for nstep such steps */
void rotate_z_scene(int nsteps, float stepsize);

/*! \brief Bells and whistles rotation 

    spin, zoom and translate.

    where axis is either x,y or z,
    stepsize is in degrees, 
    zoom_by and x_rel etc are how much zoom, x,y,z should 
            have changed by after nstep steps.
*/
void spin_zoom_trans(int axis, int nstep, float stepsize, float zoom_by, 
		     float x_rel, float y_rel, float z_rel);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Views                                                   */
/*  ----------------------------------------------------------------------- */
/*! \brief return the view number */
int add_view_here(const char *view_name); 
/*! \brief return the view number */
int add_view_raw(float rcx, float rcy, float rcz, float quat1, float quat2, 
		 float quat3, float quat4, float zoom, const char *view_name);
void play_views();
void remove_this_view();
/*! \brief the view with the given name */
int remove_named_view(const char *view_name);
/*! \brief the given view number */
void remove_view(int view_number);
/* go to the first view.  if snap_to_view_flag, go directly, else
   smooth twisty path.*/
int go_to_first_view(int snap_to_view_flag);
int go_to_view_number(int view_number, int snap_to_view_flag);
int add_spin_view(const char *view_name, int n_steps, float degrees_total);
/*! \brief return the number of views */
void add_view_description(int view_number, const char *description);
/*! \brief add a view (not add to an existing view) that *does*
  something (e.g. displays or undisplays a molecule) rather than move
  the graphics.

  @return the view number for this (new) view.
 */
int add_action_view(const char *view_name, const char *action_function);
/*! \brief add an action view after the view of the given view number

  @return the view number for this (new) view.
 */
int insert_action_view_after_view(int view_number, const char *view_name, const char *action_function);
int n_views(); 
/*! \brief return the name of the given view, if view_number does not
  specify a view return #f */

/*! \brief save views to view_file_name */
void save_views(const char *view_file_name);

float views_play_speed(); 
void set_views_play_speed(float f);

#ifdef __cplusplus/* protection from use in callbacks.c, else compilation probs */
#ifdef USE_GUILE
SCM view_name(int view_number);
SCM view_description(int view_number);
void go_to_view(SCM view);
#endif	/* USE_GUILE */
#endif	/* __cplusplus */
/*! \brief Clear the view list */
void clear_all_views();

/* movies */
void set_movie_file_name_prefix(const char *file_name);
void set_movie_frame_number(int frame_number);
#ifdef __cplusplus/* protection from use in callbacks.c, else compilation probs */
#ifdef USE_GUILE
SCM movie_file_name_prefix();
#endif
#endif
int movie_frame_number();
void set_make_movie_mode(int make_movies_flag);

/*  ----------------------------------------------------------------------- */
/*                  graphics background colour                              */
/*  ----------------------------------------------------------------------- */
/* section Background Colour */
/*! \name Background Colour */
/* \{ */

/*! \brief set the background colour

 red, green and blue are numbers between 0.0 and 1.0 */
void set_background_colour(double red, double green, double blue); 

/*! \brief is the background black (or nearly black)?

@return 1 if the background is black (or nearly black), 
else return 0. */
int  background_is_black_p();
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  ligand fitting stuff                                    */
/*  ----------------------------------------------------------------------- */
/* section Ligand Fitting Functions */
/*! \name   Ligand Fitting Functions */
/* \{ */

/*! \brief set the fraction of atoms which must be in positive density
  after a ligand fit */
void set_ligand_acceptable_fit_fraction(float f);

/*! \brief set the default sigma level that the map is searched to
  find potential ligand sites */
void set_ligand_cluster_sigma_level(float f); /* default 2.2 */

/*! \brief set the number of conformation samples 

    big ligands require more samples.  Default 10.*/
void set_ligand_flexible_ligand_n_samples(int i); /* default 50: Really? */
void set_ligand_verbose_reporting(int i); /* 0 off (default), 1 on */

/*! \brief search the top n sites for ligands.

   Default 10. */
void set_find_ligand_n_top_ligands(int n); /* fit the top n ligands,
					      not all of them, default
					      10. */
/*! \brief how shall we treat the waters during ligand fitting? 

   pass with istate=1 for waters to mask the map in the same way that
   protein atoms do.
   */
void set_find_ligand_mask_waters(int istate);

/* get which map to search, protein mask and ligands from button and
   then do it*/

/*  extract the sigma level and stick it in */
/*  graphics_info_t::ligand_cluster_sigma_level */
void set_ligand_cluster_sigma_level_from_widget(GtkWidget *button);

/*! \brief set the protein molecule for ligand searching */
void set_ligand_search_protein_molecule(int imol);
/*! \brief set the map molecule for ligand searching */
void set_ligand_search_map_molecule(int imol_map);
/*! \brief add a rigid ligand molecule to the list of ligands to search for
  in ligand searching */
void add_ligand_search_ligand_molecule(int imol_ligand);
/*! \brief add a flexible ligand molecule to the list of ligands to search for
  in ligand searching */
void add_ligand_search_wiggly_ligand_molecule(int imol_ligand);

#ifdef __cplusplus
#ifdef USE_GUILE
SCM execute_ligand_search();  
#else 
// Fixme Bernhard
void execute_ligand_search();  
#endif 
#endif // __cplusplus
void free_ligand_search_user_data(GtkWidget *button); 
void add_ligand_clear_ligands(); 

/*! \brief this sets the flag to have expert option ligand entries in
  the Ligand Searching dialog */
void ligand_expert(); 

/*! \brief display the find ligands dialog, if maps, coords and ligands are available */
void do_find_ligands_dialog();

/* Widget functions */

int fill_ligands_dialog(GtkWidget *dialog);
int fill_ligands_dialog_map_bits(GtkWidget *dialog, short int diff_maps_only_flag);	
int fill_ligands_dialog_protein_bits(GtkWidget *dialog);	
int fill_ligands_dialog_ligands_bits(GtkWidget *dialog);	
/*  we need to delete the find_ligand_dialog when we are done, so  */
/*  add this pointer as user data. */
void do_find_ligand_many_atoms_in_ligands(GtkWidget *find_ligand_dialog); 
/* these I factored out, they can be used for the waters dialog too */
int fill_ligands_dialog_map_bits_by_dialog_name(GtkWidget *find_ligand_dialog,
						const char *dialog_name, 
						short int diff_maps_only_flag); 
int fill_ligands_dialog_protein_bits_by_dialog_name(GtkWidget *find_ligand_dialog,
						    const char *dialog_name); 
int fill_vbox_with_coords_options_by_dialog_name(GtkWidget *find_ligand_dialog,
						 const char *dialog_name,
						 short int have_ncs_flag);

void fill_ligands_sigma_level_entry(GtkWidget *dialog);
void fill_ligands_expert_options(GtkWidget *find_ligand_dialog);
void set_ligand_expert_options_from_widget(GtkWidget *button);

/*! \brief "Template"-based matching.  Overlap the first residue in
  imol_ligand onto the residue specified by the reference parameters.
  Use graph matching, not atom names.  

@return success status, #f = failed to find residue in either
imol_ligand or imo_ref, 

otherwise return the RT operator */
#ifdef __cplusplus
#ifdef USE_GUILE
SCM overlap_ligands(int imol_ligand, int imol_ref, const char *chain_id_ref, int resno_ref);
SCM analyse_ligand_differences(int imol_ligand, int imol_ref, const char *chain_id_ref,
			       int resno_ref);
#endif 
#endif	/* __cplusplus */

void execute_get_mols_ligand_search(GtkWidget *button); 
/*  info is stored in graphics_info_t beforehand */

/* This has pointers to Coord_orths poked into it, let's clear them
   up. */
void  free_blob_dialog_memory(GtkWidget *w);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  water fitting                                           */
/*  ----------------------------------------------------------------------- */
/* section Water Fitting Functions */
/*! \name Water Fitting Functions */
/* \{ */

/*! Renumber the waters of molecule number imol with consecutive numbering */
void renumber_waters(int imol); 
void fill_find_waters_dialog(GtkWidget *find_ligand_dialog);
/* interface fluff */
void execute_find_waters(GtkWidget *ok_button);  

/*! \brief find waters */
void execute_find_waters_real(int imol_for_map,
			      int imol_for_protein,
			      short int new_waters_mol_flag, 
			      float sigma_cut_off);

void find_waters(int imol_for_map,
		 int imol_for_protein,
		 short int new_waters_mol_flag, 
		 float sigma_cut_off,
		 short int show_blobs_dialog);

char *get_text_for_find_waters_sigma_cut_off();
void set_value_for_find_waters_sigma_cut_off(float f); 
/* #ifdef __cplusplus */
/* Just too painful... */
/* void wrapped_create_big_blobs_dialog(const std::vector<Cartesian> &blobs); */
/* #endif */
void on_big_blob_button_clicked(GtkButton *button,
				gpointer user_data);

/* default 0.07, I think. */
void set_ligand_water_spherical_variance_limit(float f); 

/*! \brief set ligand to protein distance limits

 f1 is the minimum distance, f2 is the maximum distance */
void set_ligand_water_to_protein_distance_limits(float f1, float f2);

/*! \brief set the number of cycles of water searching */
void set_ligand_water_n_cycles(int i); 
void set_write_peaksearched_waters(); 

/*! \brief find blobs  */
void execute_find_blobs(int imol_model, int imol_for_map, float cut_off, short int interactive_flag);

void execute_find_blobs_from_widget(GtkWidget *dialog);

GtkWidget *wrapped_create_unmodelled_blobs_dialog();
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  bond representation                                     */
/*  ----------------------------------------------------------------------- */
/* section Bond Representation */
/*! \name Bond Representation */
/* \{ */

/*! \brief set the default thickness for bonds (e.g. in ~/.coot)  */
void set_default_bond_thickness(int t);

/*! \brief set the thickness of the bonds in molecule number imol to t pixels  */
void set_bond_thickness(int imol, float t);
/*! \brief set the thickness of the bonds of the intermediate atoms to t pixels  */
void set_bond_thickness_intermediate_atoms(float t);
void set_unbonded_atom_star_size(float f);

/*! \brief set status of drawing zero occupancy markers.

  default status is 1. */
void set_draw_zero_occ_markers(int status);

/* istat = 0 is hydrogens off, istat = 1: show hydrogens */
void set_draw_hydrogens(int imol, int istat);

/*! \brief draw molecule number imol as CAs */
void graphics_to_ca_representation   (int imol);
/*! \brief draw molecule number imol as CA + ligands */
void graphics_to_ca_plus_ligands_representation   (int imol);
/*! \brief draw molecule number imol with no waters */
void graphics_to_bonds_no_waters_representation(int imol);
/*! \brief draw molecule number imol with normal bonds */
void graphics_to_bonds_representation(int mol);
/*! \brief draw molecule number imol with CA bonds in secondary
  structure representation and ligands */
void graphics_to_ca_plus_ligands_sec_struct_representation(int imol);
/*! \brief draw molecule number imol with bonds in secondary structure
  representation */
void graphics_to_sec_struct_bonds_representation(int imol); 
/*! \brief draw molecule number imol in Jones' Rainbow */
void graphics_to_rainbow_representation(int imol);
/*! \brief draw molecule number imol coloured by B-factor */
void graphics_to_b_factor_representation(int imol);
/*! \brief draw molecule number imol coloured by occupancy */
void graphics_to_occupancy_represenation(int imol);
/*! \brief what is the bond drawing state of molecule number imol  */
int graphics_molecule_bond_type(int imol); 
/*! \brief scale the colours for colour by b factor representation */
int set_b_factor_bonds_scale_factor(int imol, float f);


GtkWidget *wrapped_create_bond_parameters_dialog();
void apply_bond_parameters(GtkWidget *w);

/*! \brief make a ball and stick representation of imol given atom selection

e.g. (make-ball-and-stick 0 "/1" 0.15 0.25 1) */
int make_ball_and_stick(int imol,
			const char *atom_selection_str,
			float bond_thickness, float sphere_size,
			int do_spheres_flag);
/*! \brief clear ball and stick representation of molecule number imol */
int clear_ball_and_stick(int imol);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  dots display                                            */
/*  ----------------------------------------------------------------------- */

/*! \name Dots Representation */
/* \{ */
/*! \brief display dotted surface

return a generic objects handle (which can be used to remove later) */
int dots(int imol,
	  const char *atom_selection_str,
	  float dot_density, float sphere_size_scale);

/*! \brief clear dots in imol with dots_handle */
void clear_dots(int imol, int dots_handle);

/*! \brief return the number of dots sets for molecule number imol */
int n_dots_sets(int imol); 
/* \} */


/*  ----------------------------------------------------------------------- */
/*                  pepflip                                                 */
/*  ----------------------------------------------------------------------- */
/* section Pep-flip */
/*! \name Pep-flip */
/* \{ */
void do_pepflip(short int state); /* sets up pepflip, ready for atom pick. */
/*! \brief pepflip the given residue */
void pepflip(int ires, const char *chain_id, int imol); /* the residue with CO,
							   for scripting interface. */
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  rigid body refinement                                   */
/*  ----------------------------------------------------------------------- */
/* section Rigid Body Refinement */
/*! \name Rigid Body Refinement */
/* \{ */
/* a gui-based interface: setup the rigid body refinement.*/
void do_rigid_body_refine(short int state);	/* set up for atom picking */

/*! \brief setup rigid body refine zone

   where we set the atom selection
   holders according to the arguments and then call
   execute_rigid_body_refine() */
void rigid_body_refine_zone(int reso_start, int resno_end, 
			    const char *chain_id, int imol);

/*! \brief actually do the rigid body refinement

   various atom selection holders in graphics-info have been set, actually do it.  */
void execute_rigid_body_refine(short int auto_range_flag); /* atom picking has happened.
				     Actually do it */

/*! \brief set rigid body fraction of atoms in positive density

 @param f in the range 0.0 -> 1.0 (default 0.75) */
void set_rigid_body_fit_acceptable_fit_fraction(float f);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  dynamic map                                             */
/*  ----------------------------------------------------------------------- */
/* section Dynamic Map */
/*! \name Dynamic Map */
/* \{ */
void   toggle_dynamic_map_display_size();
void   toggle_dynamic_map_sampling();
void   set_map_dynamic_map_sampling_checkbutton(GtkWidget *checkbutton);
void   set_map_dynamic_map_display_size_checkbutton(GtkWidget *checkbutton);
/* scripting interface: */
void set_dynamic_map_size_display_on();
void set_dynamic_map_size_display_off();
void set_dynamic_map_sampling_on();
void set_dynamic_map_sampling_off();
void set_dynamic_map_zoom_offset(int i);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  build one residue by phi/psi search                     */
/*  ----------------------------------------------------------------------- */
/* section Add Terminal Residue Functions */
/*! \name Add Terminal Residue Functions */
/* \{ */
void do_add_terminal_residue(short int state);
/*  execution of this is in graphics_info_t because it uses a CResidue */
/*  in the interface and we can't have that in c-interface.h */
/*  (compilation of coot_wrap_guile goes mad on inclusion of */
/*   mmdb_manager.h) */
void set_add_terminal_residue_n_phi_psi_trials(int n); 
/* Add Terminal Residues actually build 2 residues, this allows us to
   see both residues - default is 0 (off). */
void set_add_terminal_residue_add_other_residue_flag(int i);
void set_terminal_residue_do_rigid_body_refine(short int v); 
int add_terminal_residue_immediate_addition_state(); 

/*! \brief set immediate addition of terminal residue

call with i=1 for immediate addtion */
void set_add_terminal_residue_immediate_addition(int i);

/*! \brief Add a terminal residue

return 0 on failure, 1 on success */
int add_terminal_residue(int imol, char *chain_id, int residue_number,
			 char *residue_type, int immediate_add); 

/*! \brief set the residue type of an added terminal residue.   */
void set_add_terminal_residue_default_residue_type(const char *type);
/*! \brief set a flag to run refine zone on terminal residues after an
  addition.  */
void set_add_terminal_residue_do_post_refine(short int istat); 

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  delete residue                                          */
/*  ----------------------------------------------------------------------- */
/* section Delete Residues */
/*! \name Delete Residues */
/* in build */
/* by graphics */
/* \{ */
void delete_atom_by_atom_index(int imol, int index, short int do_delete_dialog);
void delete_residue_by_atom_index(int imol, int index, short int do_delete_dialog);
void delete_residue_hydrogens_by_atom_index(int imol, int index, short int do_delete_dialog);

/*! \brief delete residue range */
void delete_residue_range(int imol, const char *chain_id, int resno_start, int end_resno);

/*! \brief delete residue  */
void delete_residue(int imol, const char *chain_id, int resno, const char *inscode); 
/*! \brief delete residue with altconf  */
void delete_residue_with_altconf(int imol, const char *chain_id, int resno, const char *inscode, const char *altloc); 
/*! \brief delete hydrogen atoms in residue  */
void delete_residue_hydrogens(int imol, const char *chain_id, int resno, const char *inscode, const char *altloc); 
/*! \brief delete atom in residue */
void delete_atom(int imol, const char *chain_id, int resno, const char *ins_code, const char *at_name, const char *altloc);
/*! \brief delete all atoms in residue that are not main chain or CB */
void delete_residue_sidechain(int imol, const char *chain_id, int resno, const char*ins_code);
/* toggle callbacks */
void set_delete_atom_mode();
void set_delete_residue_mode();
void set_delete_residue_zone_mode();
void set_delete_residue_hydrogens_mode();
void set_delete_water_mode();
void set_delete_sidechain_mode();
short int delete_item_mode_is_atom_p(); /* (predicate) a boolean */
short int delete_item_mode_is_residue_p(); /* predicate again */
short int delete_item_mode_is_water_p();
short int delete_item_mode_is_sidechain_p();
void store_delete_item_widget(GtkWidget *widget);
void clear_pending_delete_item(); /* for when we cancel with picking an atom */
void clear_delete_item_widget();
void store_delete_item_widget_position();
short int delete_item_widget_is_being_shown();
short int delete_item_widget_keep_active_on();

/* We need to set the pending delete flag and that can't be done in
   callback, so this wrapper does it */
GtkWidget *wrapped_create_delete_item_dialog();

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  rotate/translate buttons                                */
/*  ----------------------------------------------------------------------- */
/* section Rotate/Translate Buttons */
/*  sets flag for atom selection clicks */
void do_rot_trans_setup(short int state); 
void do_rot_trans_adjustments(GtkWidget *dialog);
void rot_trans_reset_previous();
void set_rotate_translate_zone_rotates_about_zone_centre(int istate);

/*  ----------------------------------------------------------------------- */
/*                  cis <-> trans conversion                                */
/*  ----------------------------------------------------------------------- */
void do_cis_trans_conversion_setup(int istate);
void cis_trans_convert(int imol, const char *chain_id, int resno, const char *altconf);

/*  ----------------------------------------------------------------------- */
/*                  db-main                                                 */
/*  ----------------------------------------------------------------------- */
/* section Mainchain Building Functions */

/*! \name Mainchain Building Functions */
/* \{ */
void do_db_main(short int state); 
/*! \brief CA -> mainchain conversion */
void db_mainchain(int imol,
		  const char *chain_id,
		  int iresno_start,
		  int iresno_end,
		  const char *direction_string);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  close molecule                                          */
/*  ----------------------------------------------------------------------- */
/* section Close Molecule FUnctions */
/*! \name Close Molecule FUnctions */
/* \{ */

void close_molecule(int imol);

/* get the molecule to delete from the optionmenu */
void close_molecule_by_widget(GtkWidget *optionmenu);
void fill_close_option_menu_with_all_molecule_options(GtkWidget *optionmenu);
/* The callback for the above menuitems */
void close_molecule_item_select(GtkWidget *item, GtkPositionType pos); 

/* New version of close molecule */
void new_close_molecules(GtkWidget *window);
GtkWidget *wrapped_create_new_close_molecules_dialog();

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  rotamers                                                */
/*  ----------------------------------------------------------------------- */
/* section Rotatmer Functions */
/*! \name Rotatmer Functions */
/* \{ */

/* functions defined in c-interface-build */

void setup_rotamers(short int state);

/*  display the rotamer option and display the most likely in the graphics as a */
/*  moving_atoms_asc */
void do_rotamers(int atom_index, int imol);

 /* as a percentage i.e. 1.00 is quite low */
void set_rotamer_lowest_probability(float f);

/*! \brief set a flag: 0 is off, 1 is on */
void set_rotamer_check_clashes(int i); 

/*! \brief auto fit by rotamer search.

   return the score, for some not very good reason.  clash_flag
   determines if we use clashes with other residues in the score for
   this rotamer (or not).  It would be cool to call this from a script
   that went residue by residue along a (newly-built) chain (now available). */
float auto_fit_best_rotamer(int resno, 
			    const char *altloc, 
			    const char *insertion_code, 
			    const char *chain_id, int imol_coords, int imol_map, 
			    int clash_flag, float lowest_probability);

/*! \brief set the clash flag for rotamer search

   And this functions for [pre-setting] the variables for
   auto_fit_best_rotamer called interactively (using a graphics_info_t
   function). 0 off, 1 on.*/
void set_auto_fit_best_rotamer_clash_flag(int i); /*  */
/* currently stub function only */
float rotamer_score(int resno, const char *insertion_code,
		    const char *chain_id, int imol_coords, int imol_map,
		    int rotamer_number); 
void setup_auto_fit_rotamer(short int state);	/* called by the Auto Fit button call
				   back, set's in_auto_fit_define. */

/*! \brief fill all the residues of molecule number imol that have
   missing atoms.

To be used to remove the effects of chainsaw.  */
void fill_partial_residues(int imol);

void fill_partial_residue(int imol, const char *chain_id, int resno, const char* inscode);

/* Used for unsetting the rotamer dialog when it gets destroyed. */
void
set_graphics_rotamer_dialog(GtkWidget *w);


#ifdef __cplusplus	/* need this wrapper, else gmp.h problems in callback.c */
#ifdef USE_GUILE 
/*! \brief Activate rotamer graph analysis for molecule number imol.  

Return rotamer info - function used in testing.  */
SCM rotamer_graphs(int imol);
#else
/* FIXME Bernhard */
#endif 
#endif 

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  180 degree flip                                         */
/*  ----------------------------------------------------------------------- */
/*! \name 180 Flip Side chain */
/* \{ */

/*! \brief rotate 180 degrees round the last chi angle */
void do_180_degree_side_chain_flip(int imol, const char* chain_id, int resno, 
				   const char *inscode, const char *altconf);

void setup_180_degree_flip(short int state); 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  mutate                                                  */
/*  ----------------------------------------------------------------------- */
/* section Mutate Functions */
/*! \name Mutate Functions */
/* \{ */

/* c-interface-build */
void setup_mutate(short int state);
/*! \brief Mutate then fit to map

 that we have a map define is checked first */
void setup_mutate_auto_fit(short int state);

void do_mutation(const char *type, short int is_stub_flag);

/* auto-mutate stuff */
short int progressive_residues_in_chain_check(const char *chain_id, int imol);

/*! \brief mutate a given residue 

target_res_type is a three-letter-code. */
int mutate(int ires, const char *chain_id, int imol,  const char *target_res_type);

/*! \brief Do you want Coot to automatically run a refinement after
  every mutate and autofit?

 1 for yes, 0 for no. */
void set_mutate_auto_fit_do_post_refine(short int istate);

/*! \brief what is the value of the previous flag? */
int mutate_auto_fit_do_post_refine_state();


/*! \brief an alternate interface to mutation of a singe residue.

 @return 1 on success, 0 on failure 

  ires-ser is the serial number of the residue, not the seqnum 
  There 2 functions don't make backups, but mutate() does - CHECKME
   Hence mutate() is for use as a "one-by-one" type and the following
   2 by wrappers that muate either a residue range or a whole chain

   Note that the target_res_type is a char, not a string (or a char *).  So from the scheme 
   interface you'd use (for example) #\A {hash backslash A} for ALA.
*/
int mutate_single_residue_by_serial_number(int ires_ser, const char *chain_id, 
					   int imol, char target_res_type);
/* ires is the seqnum of the residue (conventional) */
int mutate_single_residue_by_seqno(int ires, const char *inscode,
				   const char *chain_id,
				   int imol, char target_res_type);

/* an internal function - not useful for scripting: */

void do_base_mutation(const char *type);

/*! \brief set a flag saying that the residue chosen by mutate or
  auto-fit mutate should only be added as a stub (mainchain + CB) */
void set_residue_type_chooser_stub_state(short int istat);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  alternate conformation                                  */
/*  ----------------------------------------------------------------------- */
/* section Alternative Conformation */
/*! \name Alternative Conformation */
/* c-interface-build function */
/*! \{ */

short int alt_conf_split_type_number();
void set_add_alt_conf_split_type_number(short int i);
void setup_alt_conf_with_dialog(GtkWidget *dialog); 
void unset_add_alt_conf_dialog(); /* set the static dialog holder in
				     graphics info to NULL */
void unset_add_alt_conf_define(); /* turn off pending atom pick */
void altconf();			/* temporary debugging interface. */
void set_add_alt_conf_new_atoms_occupancy(float f); /* default 0.5 */
void set_show_alt_conf_intermediate_atoms(int i);
int  show_alt_conf_intermediate_atoms_state();
void zero_occupancy_residue_range(int imol, const char *chain_id, int ires1, int ires2);
void fill_occupancy_residue_range(int imol, const char *chain_id, int ires1, int ires2);
/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  pointer atoms                                           */
/*  ----------------------------------------------------------------------- */
/* section Pointer Atom Functions */
/*! \name Pointer Atom Functions */
/* c-interface-build */
/*! \{ */

void place_atom_at_pointer();
void place_typed_atom_at_pointer(const char *type);

/* ! \brief set pointer atom is a water (HOH) */
void set_pointer_atom_is_dummy(int i);
void fill_place_atom_molecule_option_menu(GtkWidget *optionmenu);
void display_where_is_pointer(); /* print the coordinates of the
				    pointer to the console */

/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  baton mode                                              */
/*  ----------------------------------------------------------------------- */
/* section Baton Build Functions */
/* c-interface-build */
void set_baton_mode(short int i); /* Mouse movement moves the baton not the view? */
void set_draw_baton(short int i); /* draw the baton or not */
void accept_baton_position();	/* put an atom at the tip */
void baton_try_another();
void shorten_baton();
void lengthen_baton();
void baton_build_delete_last_residue();
void set_baton_build_params(int istart_resno, const char *chain_id, const char *backwards); 
void baton_mode_calculate_skeleton(GtkWidget *window);


/*  ----------------------------------------------------------------------- */
/*                  post baton mode                                         */
/*  ----------------------------------------------------------------------- */
/* section Post-Baton Functions */
/* c-interface-build */
/* Reverse the direction of a the fragment of the clicked on
   atom/residue.  A fragment is a consequitive range of residues -
   where there is a gap in the numbering, that marks breaks between
   fragments in a chain.  There also needs to be a distance break - if
   the CA of the next/previous residue is more than 5A away, that also
   marks a break. Thow away all atoms in fragment other than CAs.*/
void reverse_direction_of_fragment(int imol, const char *chain_id, int resno);
void setup_reverse_direction(short int i);


/*  ----------------------------------------------------------------------- */
/*                  terminal OXT atom                                       */
/*  ----------------------------------------------------------------------- */
/* section Terminal OXT Atom */
/*! \name Terminal OXT Atom */
/* c-interface-build */
/*! \{ */
short int add_OXT_to_residue(int imol, int reso, const char *insertion_code, const char *chain_id);
GtkWidget *wrapped_create_add_OXT_dialog();
void apply_add_OXT_from_widget(GtkWidget *w);

/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  crosshairs                                              */
/*  ----------------------------------------------------------------------- */
/* section Crosshairs  */
/*! \name Crosshairs  */
/*! \{ */
void set_draw_crosshairs(short int i);
/* so that we display the crosshairs with the radiobuttons in the
   right state, return draw_crosshairs_flag */
short int draw_crosshairs_state();
/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  Edit Chi Angles                                         */
/*  ----------------------------------------------------------------------- */
/* section Edit Chi Angles */
/*! \name  Edit Chi Angles */
/* \{ */
/* c-interface-build functions */
void setup_edit_chi_angles(short int state); 
void set_find_hydrogen_torsion(short int state);
void set_graphics_edit_current_chi(int ichi); /* button callback */
void unset_moving_atom_move_chis();

/*! \brief fire up the edit chi angles gui for the given residue 

 return a status of 0 if it failed to fined the residue, 
 return a value of 1 if it worked. */
int edit_chi_angles(int imol, const char *chain_id, int resno, 
		     const char *ins_code, const char *altconf);

int set_show_chi_angle_bond(int imode);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Mask                                                    */
/*  ----------------------------------------------------------------------- */
/* section Masks */
/*! \name Masks */
/*! \{ */
/* The idea is to generate a new map that has been masked by some
   coordinates. */
/*! \brief  generate a new map that has been masked by some coordinates

        (mask-map-by-molecule map-no mol-no invert?)  creates and
        displays a masked map, cuts down density where the coordinates
        are (invert is 0).  If invert? is 1, cut the density down
        where there are no atoms atoms.  */
int mask_map_by_molecule(int map_mol_no, int coord_mol_no, short int invert_flag);

int mask_map_by_atom_selection(int map_mol_no, int coords_mol_no, const char *mmdb_atom_selection, short int invert_flag); 

/*! \brief set the atom radius for map masking */
void set_map_mask_atom_radius(float rad);

/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  check waters interface                                  */
/*  ----------------------------------------------------------------------- */
/* section Check Waters Interface */
/*! \name check Waters Interface */
/* interactive check by b-factor, density level etc. */
/*! \{ */
GtkWidget *wrapped_create_check_waters_dialog();
void set_check_waters_b_factor_limit(float f);
void set_check_waters_map_sigma_limit(float f);
void set_check_waters_min_dist_limit(float f);
void set_check_waters_max_dist_limit(float f);
void check_waters_molecule_menu_item_activate(GtkWidget *item, 
					      GtkPositionType pos);
void do_check_waters_by_widget(GtkWidget *dialog);
GtkWidget *wrapped_checked_waters_baddies_dialog(int imol, float b_factor_lim, float map_sigma_lim, 
						 float min_dist, float max_dist,
						 short int part_occ_contact_flag,
						 short int zero_occ_flag,
						 short int logical_operator_and_or_flag);
void delete_checked_waters_baddies(int imol, float b_factor_lim, float map_sigma_lim, 
				   float min_dist, float max_dist,
				   short int part_occ_contact_flag,
				   short int zero_occ_flag,
				   short int logical_operator_and_or_flag);

/* difference map variance check  */
void check_waters_by_difference_map(int imol_waters, int imol_diff_map, 
				    int interactive_flag); 
void check_waters_by_difference_map_by_widget(GtkWidget *window);
GtkWidget *wrapped_create_check_waters_diff_map_dialog();
/* results widget are in graphics-info.cc  */
/* Let's give access to the sigma level (default 4) */
float check_waters_by_difference_map_sigma_level_state();
void set_check_waters_by_difference_map_sigma_level(float f);
/*! \} */

/*  ----------------------------------------------------------------------- */
/*                  Least squares                                           */
/*  ----------------------------------------------------------------------- */
/* section Least-Squares matching */
/*! \name Least-Squares matching */
/*! \{ */
void clear_lsq_matches();
void add_lsq_match(int reference_resno_start, 
		   int reference_resno_end,
		   const char *chain_id_reference,
		   int moving_resno_start, 
		   int moving_resno_end,
		   const char *chain_id_moving,
		   int match_type); /* 0: all
				       1: main
				       2: CA 
				     */

/* Return an rtop pair (proper list) on good match, else #f */
#ifdef __cplusplus	/* need this wrapper, else gmp.h problems in callback.c */
#ifdef USE_GUILE
SCM apply_lsq_matches(int imol_reference, int imol_moving);
#endif
#endif

/* poor old python programmers... */
int apply_lsq_matches_simple(int imol_reference, int imol_moving);
		    
/* section Least-Squares plane interface */
void setup_lsq_deviation(int state);
void setup_lsq_plane_define(int state); 
GtkWidget *wrapped_create_lsq_plane_dialog();
void unset_lsq_plane_dialog(); /* callback from destroy of widget */
void remove_last_lsq_plane_atom();

GtkWidget *wrapped_create_least_squares_dialog();
int apply_lsq_matches_by_widget(GtkWidget *lsq_dialog); /* return 1 for good fit */
void lsq_ref_mol_option_menu_changed(GtkWidget *item, GtkPositionType pos);
void lsq_mov_mol_option_menu_changed(GtkWidget *item, GtkPositionType pos);
void lsq_reference_chain_option_menu_item_activate(GtkWidget *item,
						   GtkPositionType pos);
void lsq_moving_chain_option_menu_item_activate(GtkWidget *item,
						GtkPositionType pos);
void fill_lsq_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
					     int is_reference_structure_flag);
/*! \} */


/*  ----------------------------------------------------------------------- */
/*                  trim                                                    */
/*  ----------------------------------------------------------------------- */
/* section Trim */
/*! \name Trim */
/*! \{ */

/* a c-interface-build function */
void trim_molecule_by_map(int imol_coords, int imol_map, 
			  float map_level, int delete_or_zero_occ_flag);

/*! \} */


/*  ------------------------------------------------------------------------ */
/*                       povray/raster3d interface                           */
/*  ------------------------------------------------------------------------ */
/* make the text input to external programs */
/*! \name External Ray-Tracing */
/* \{ */
/* \name Povray and Raster3D Interface */

/* \brief create a r3d file for the current view */
void raster3d(const char *rd3_filename);
void povray(const char *filename);
/* a wrapper for the (scheme) function that makes the image, callable
   from callbacks.c  */
void make_image_raster3d(const char *filename);
void make_image_povray(const char *filename);

/* set the bond thickness for the Raster3D representation  */
void set_raster3d_bond_thickness(float f);
/* set the atom radius for the Raster3D representation  */
void set_raster3d_atom_radius(float f);
/* set the density line thickness for the Raster3D representation  */
void set_raster3d_density_thickness(float f);
/* set the flag to show atoms for the Raster3D representation  */
void set_renderer_show_atoms(int istate);
void raster_screen_shot(); /* run raster3d or povray and guile */
                           /* script to render and display image */

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  citation notice                                         */
/*  ----------------------------------------------------------------------- */
void citation_notice_off();

/*  ----------------------------------------------------------------------- */
/*                  Superpose                                               */
/*  ----------------------------------------------------------------------- */
/* section Superposition (SSM) */
/*! \name Superposition (SSM) */
/* \{ */

/*! \brief simple interface to superposition. 

Superpose all residues of imol2 onto imol1.  imol1 is reference, we
can either move imol2 or copy it to generate a new molecule depending
on the vaule of move_imol2_flag (1 for move 0 for copy). */
void superpose(int imol1, int imol2, short int move_imol2_flag); 


/*! \brief chain-based interface to superposition. 

Superpose the given chains of imol2 onto imol1.  imol1 is reference,
we can either move imol2 or copy it to generate a new molecule
depending on the vaule of move_imol2_flag (1 for move 0 for copy). */
void superpose_with_chain_selection(int imol1, int imol2, 
				    const char *chain_imol1,
				    const char *chain_imol2,
				    int chain_used_flag_imol1,
				    int chain_used_flag_imol2,
				    short int move_imol2_copy_flag);

/*! \brief detailed interface to superposition. 

Superpose the given atom selection (specified by the mmdb atom
selection strings) of imol2 onto imol1.  imol1 is reference, we can
either move imol2 or copy it to generate a new molecule depending on
the vaule of move_imol2_flag (1 for move 0 for copy). */
void superpose_with_atom_selection(int imol1, int imol2,
				   const char *mmdb_atom_sel_str_1, 
				   const char *mmdb_atom_sel_str_2,
				   short int move_imol2_copy_flag);

void execute_superpose(GtkWidget *w);
GtkWidget *wrapped_create_superpose_dialog(); /* used by callback */
void fill_superpose_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
 						   int is_reference_structure_flag);
 
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  NCS                                                     */
/*  ----------------------------------------------------------------------- */
/* section NCS */
/*! \name NCS */

/* \{ */
/*! \brief set drawing state of NCS ghosts for molecule number imol   */
void set_draw_ncs_ghosts(int imol, int istate);
/*! \brief set bond thickness of NCS ghosts for molecule number imol   */
void set_ncs_ghost_bond_thickness(int imol, float f);
/*! \brief update ghosts for molecule number imol */
void ncs_update_ghosts(int imol); // update ghosts
/*! \brief make NCS map */
int make_dynamically_transformed_ncs_maps(int imol_model, int imol_map);
int make_dynamically_transformed_ncs_maps_by_widget(GtkWidget *dialog);
GtkWidget *wrapped_create_ncs_maps_dialog();
void make_ncs_ghosts_maybe(int imol);
/*! \brief Add NCS matrix */
void add_ncs_matrix(int imol, const char *this_chain_id, const char *target_chain_id,
		    float m11, float m12, float m13, 
		    float m21, float m22, float m23, 
		    float m31, float m32, float m33, 
		    float t1,  float t2,  float t3);

void clear_ncs_ghost_matrices(int imol);

/*! \brief add an NCS matrix for strict NCS molecule representation

for CNS strict NCS usage: expand like normal symmetry does  */
int add_strict_ncs_matrix(int imol,
			  const char *this_chain_id,
			  const char *target_chain_id,
			  float m11, float m12, float m13, 
			  float m21, float m22, float m23, 
			  float m31, float m32, float m33, 
			  float t1,  float t2,  float t3);

/*! \brief return the state of NCS ghost molecules for molecule number imol   */
int show_strict_ncs_state(int imol);
/*! \brief set display state of NCS ghost molecules for molecule number imol   */
void set_show_strict_ncs(int imol, int state);
/*! \brief At what level of homology should we say that we can't see homology
   for NCS calculation? (default 0.8) */
void set_ncs_homology_level(float flev);
/* for a single copy */
/*! \brief Copy single NCS chain */
void copy_chain(int imol, const char *from_chain, const char *to_chain);
/* do multiple copies */
/*! \brief Copy chain from master to all related NCS chains */
void copy_from_ncs_master_to_others(int imol, const char *chain_id);
/* void copy_residue_range_from_ncs_master_to_others(int imol, const char *master_chain_id, int residue_range_start, int residue_range_end); */
/*! \brief Copy residue range to all related NCS chains */
void copy_residue_range_from_ncs_master_to_others(int imol, const char *master_chain_id, 
						  int residue_range_start, int residue_range_end);
GtkWidget *wrapped_create_ncs_control_dialog();	
/*! \brief change the NCS master chain  */
void ncs_control_change_ncs_master_to_chain(int imol, int ichain); 
void ncs_control_change_ncs_master_to_chain_update_widget(GtkWidget *w, int imol, int ichain); 
/*! \brief display the NCS master chain  */
void ncs_control_display_chain(int imol, int ichain, int state);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Autobuild helices and strands                           */
/*  ----------------------------------------------------------------------- */
/*! \name Helices and Strands*/
/*! \{ */
/*! \brief add a helix 

   Add a helix somewhere close to this point in the map, try to fit
   the orientation. Add to a molecule called "Helix", create it if
   needed.  Create another moecule called "Reverse Helix" if the helix
   orientation isn't completely unequivocal.

   @return the index of the new molecule.*/
int place_helix_here();

/*! \brief add a strands

   Add a strand close to this point in the map, try to fit
   the orientation. Add to a molecule called "Strand", create it if needed.
   n_residues is the estimated number of residues in the strand.

   n_sample_strands is the number of strands from the database tested
   to fit into this strand density.  8 is a suggested number.  20 for
   a more rigourous search, but it will be slower.

   @return the index of the new molecule.*/
int place_strand_here(int n_residues, int n_sample_strands);

/* \} */

/*  ----------------------------------------------------------------------- */
/*             New Molecule by Various Selection                            */
/*  ----------------------------------------------------------------------- */
/*! \brief create a new molecule that consists of only the residue of
  type residue_type in molecule number imol

@return the new molecule number, -1 means an error. */
int new_molecule_by_residue_type_selection(int imol, const char *residue_type);

/*! \brief create a new molecule that consists of only the atoms specified
  by the mmdb atoms selection string in molecule number imol

@return the new molecule number, -1 means an error. */
int new_molecule_by_atom_selection(int imol, const char* atom_selection);

/*! \brief create a new molecule that consists of only the atoms 
  within the given radius (r) of the given position.

@return the new molecule number, -1 means an error. */
int new_molecule_by_sphere_selection(int imol, float x, float y, float z, float r);


/*  ----------------------------------------------------------------------- */
/*                  Miguel's orientation axes matrix                         */
/*  ----------------------------------------------------------------------- */
/* section Miguel's orientation axes matrix */

void
set_axis_orientation_matrix(float m11, float m12, float m13,
			    float m21, float m22, float m23,
			    float m31, float m32, float m33);

void
set_axis_orientation_matrix_usage(int state);



/*  ----------------------------------------------------------------------- */
/*                  RNA/DNA                                                 */
/*  ----------------------------------------------------------------------- */
/* section RNA/DNA */
/*! \name RNA/DNA */

/* \{ */
/*!  \brief create a molecule of idea nucleotides 

use the given sequence (single letter code)

RNA_or_DNA is either "RNA" or "DNA"

form is either "A" or "B"

@return the new molecule number or -1 if a problem */
int ideal_nucleic_acid(const char *RNA_or_DNA, const char *form,
		       short int single_stranged_flag,
		       const char *sequence);

GtkWidget *wrapped_nucleotide_builder_dialog();
void ideal_nucleic_acid_by_widget(GtkWidget *builder_dialog);

/* \} */

/*  ----------------------------------------------------------------------- */
/*                  sequence (assignment)                                   */
/*  ----------------------------------------------------------------------- */
/* section Sequence (Assignment) */
/*! \name Sequence (Assignment) */
/* \{ */

/*! \brief Print the sequence to the console of the given molecule */
void print_sequence_chain(int imol, const char *chain_id);
/*! \brief Assign a FASTA sequence to a given chain in the  molecule */
void assign_fasta_sequence(int imol, const char *chain_id_in, const char *seq);
/*! \brief Assign a PIR sequence to a given chain in the molecule */
void assign_pir_sequence(int imol, const char *chain_id_in, const char *seq);
/* I don't know what this does. */
void assign_sequence(int imol_model, int imol_map, const char *chain_id);

#ifdef __cplusplus/* protection from use in callbacks.c, else compilation probs */
#ifdef USE_GUILE
/*! \brief return the sequence info that has been assigned to molecule
  number imol. return as a list of dotted pairs (list (cons chain-id
  seq)).  To be used in constructing the cootaneer gui. */
SCM sequence_info(int imol);
#endif 
#ifdef USE_PYTHON
/* fill me in, Bernhard. */
#endif 
#endif /* C++ */
/* \} */
 
/*  ----------------------------------------------------------------------- */
/*                  Surfaces                                                */
/*  ----------------------------------------------------------------------- */
/* section Surfaces */
/*! \name Surfaces */
/* \{ */
/*! \brief draw surface of molecule number imol 

if state = 1 draw the surface (normal representation goes away)

if state = 0 don't draw surface */
void do_surface(int imol, int istate);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  FFfearing                                               */
/*  ----------------------------------------------------------------------- */
/*! \name FFFearing */
/* \{ */
/*! \brief fffear search model in molecule number imol_model in map
   number imol_map */
int fffear_search(int imol_model, int imol_map);
/*! \brief set and return the fffear angular resolution in degrees */
void set_fffear_angular_resolution(float f); 
/*! \brief return the fffear angular resolution in degrees */
float fffear_angular_resolution();
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  remote control                                          */
/*  ----------------------------------------------------------------------- */
/* section Remote Control */
/*! \name Remote Control */
/* \{ */
/*! \brief try to make socket listener */
void make_socket_listener_maybe(); 
int coot_socket_listener_idle_func(GtkWidget *w);
void set_coot_listener_socket_state_internal(int sock_state);

void set_socket_string_waiting(const char *s);

/* \} */
/*  ----------------------------------------------------------------------- */
/*                  Display lists                                           */
/*  ----------------------------------------------------------------------- */
/* section Display Lists for Maps */
/*! \brief Should display lists be used for maps? It may speed things
  up if these are turned on (or off) - depends on graphics card and
  drivers.  Pass 1 for on, 0 for off. */
void set_display_lists_for_maps(int i);

/*  ----------------------------------------------------------------------- */
/*                  Preferences Notebook                                    */
/*  ----------------------------------------------------------------------- */
/* section Preferences */
void preferences();
void clear_preferences();
void set_mark_cis_peptides_as_bad(int istate);
int show_mark_cis_peptides_as_bad_state();

/*  ----------------------------------------------------------------------- */
/*                  Browser Help                                            */
/*  ----------------------------------------------------------------------- */
/*! \name Browser Interface */
/* \{ */
/*! \brief try to open given url in Web browser */
void browser_url(const char *url);
/*! \brief set command to open the web browser, 

examples are "open" or "mozilla" */
void set_browser_interface(const char *browser);

/*! \brief the search interface

find words, construct a url and open it. */
void handle_online_coot_search_request(const char *entry_text);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Generic Objects                                         */
/*  ----------------------------------------------------------------------- */
/*! \name Generic Objects */
/* \{ */

/*! \brief create a new generic object with name objname and return the index 
   of the object */
int new_generic_object_number(const char *objname);

/*! \brief add line to generic object object_number */
void to_generic_object_add_line(int object_number, 
				const char *colour,
				int line_width,
				float from_x1, 
				float from_y1, 
				float from_z1, 
				float to_x2, 
				float to_y2, 
				float to_z2);

/*! \brief add point to generic object object_number */
void to_generic_object_add_point(int object_number, 
				 const char *colour,
				 int point_width,
				 float from_x1, 
				 float from_y1, 
				 float from_z1);

/*! \brief add a display list handle generic object */
void to_generic_object_add_display_list_handle(int object_number, int display_list_id); 

/*! \brief set the display status of object number object_number, 

  when they are created, by default objects are not displayed, so we
  generally need this function.  */
void set_display_generic_object(int object_number, short int istate);

/*! \brief is generic display object displayed?

  @return 1 for yes, otherwise 0  */
int generic_object_is_displayed_p(int object_number);

/*! \brief return the index of the object with name name, if not, return -1; */
int generic_object_index(const char *name);

/*! \brief what is the name of generic object number obj_number? 

 @return 0 (NULL) #f  on obj_number not available */
const char *generic_object_name(int obj_number);

/*! \brief return the number of generic display objects */
int number_of_generic_objects();

/*! \brief print to the console the name and display status of the
  generic display objects */
void generic_object_info(); 

/*! \brief does generic display object number obj_no have things to
  display? (predicate name)

@return 0 for no things, 1 for things. */
short int generic_object_has_objects_p(int obj_no); 

/*! \brief close generic object, clear the lines/points etc, not
  available for buttons/displaying etc */
void close_generic_object(int object_number);

/*! \brief has the generic object been closed? 

   @return 1 for yes, 0 othersize
*/
short int is_closed_generic_object_p(int object_number);

/*! \brief a kludgey thing, so that the generic objects gui can be
  called from a callback.  */
void generic_objects_gui_wrapper();



/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Molprobity interface                                    */
/*  ----------------------------------------------------------------------- */
/*! \name Molprobity Interface */
/* \{ */
/*! \brief pass a filename that contains molprobity's probe output in XtalView 
format */
void handle_read_draw_probe_dots(const char *dots_file);

/*! \brief pass a filename that contains molprobity's probe output in unformatted
format */
void handle_read_draw_probe_dots_unformatted(const char *dots_file, int imol, int show_clash_gui_flag);


/*! \brief shall we run molprobity for on edit chi angles intermediate atoms? */
void set_do_probe_dots_on_rotamers_and_chis(short int state);
/*! \brief return the state of if run molprobity for on edit chi
  angles intermediate atoms? */
short int do_probe_dots_on_rotamers_and_chis_state();
/*! \brief shall we run molprobity after a refinement has happened? */
void set_do_probe_dots_post_refine(short int state);
/*! \brief show the state of shall we run molprobity after a
  refinement has happened? */
short int do_probe_dots_post_refine_state();

/*! \brief make an attempt to convert pdb hydrogen name to the name
  used in Coot (and the refmac dictionary, perhaps). */
char *unmangle_hydrogen_name(const char *pdb_hydrogen_name);

/*! \brief set the radius over which we can run interactive probe,
  bigger is better but slower. 

  default is 6.0 */
void set_interactive_probe_dots_molprobity_radius(float r);

/*! \brief return the radius over which we can run interactive probe.
*/
float interactive_probe_dots_molprobity_radius();


/*! \brief Can we run probe (was the executable variable set
  properly?) (predicate).

@return 1 for yes, 2 for no  */
int probe_available_p();

/* \} */

/* DTI stereo mode - undocumented, secret interface for testing, currently. 

state should be 0 or 1. */
/* when it works, call it dti_side_by_side_stereo_mode() */
void set_dti_stereo_mode(short int state);

/*  ----------------------------------------------------------------------- */
/*                  CCP4MG Interface                                        */
/*  ----------------------------------------------------------------------- */
/*! \name CCP4mg Interface */
/* \{ */
/* \breif write a ccp4mg picture description file */
void write_ccp4mg_picture_description(const char *filename);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  CCP4MG Interface                                        */
/*  ----------------------------------------------------------------------- */
/*! \name Aux functions */
/* \{ */
/* \brief Create the "Laplacian" (-ve second derivative) of the given map. */
int laplacian (int imol);
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  Tips                                                    */
/*  ----------------------------------------------------------------------- */
/*! \name Tips Interface */
/* \{ */
/* \} */

/*  ----------------------------------------------------------------------- */
/*                  SMILES                                                  */
/*  ----------------------------------------------------------------------- */
/*! \name SMILES */
/* \{ */
/*! \brief display the SMILES string dialog */
void do_smiles_gui();
/* \} */
/*  ----------------------------------------------------------------------- */
/*                  Fun                                                     */
/*  ----------------------------------------------------------------------- */
/* section Fun */
void do_tw();

/*  ----------------------------------------------------------------------- */
/*                  Text                                                    */
/*  ----------------------------------------------------------------------- */
/*! \name Graphics Text */
/* \{ */
/*! \brief Put text at x,y,z  

@return a text handle

size variable is currently ignored.*/
int place_text(const char*text, float x, float y, float z, int size);

/*! \brief Remove text */
void remove_text(int text_handle);
/* \} */


#endif /* C_INTERFACE_H */
END_C_DECLS

