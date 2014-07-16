/* src/c-interface.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Copyright 2008, 2009 by The University of Oxford
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

// $Id: c-interface.cc 1458 2007-01-26 20:20:18Z emsley $
// $LastChangedDate: 2007-01-26 20:20:18 +0000 (Fri, 26 Jan 2007) $
// $Rev: 1458 $
 
// Load the head if it hasn't been included.
#ifdef USE_PYTHON
#ifndef PYTHONH
#define PYTHONH
#include <Python.h>
#endif
#endif

#include "compat/coot-sysdep.h"

#include <stdlib.h>
#include <string.h> // strncpy, strncmp
#include <iostream>
#include <fstream>
#include <algorithm>

#if !defined(_MSC_VER)
#include <glob.h> // for globbing.  Needed here?
#endif

#ifdef USE_GUILE
#include <libguile.h>
#include "c-interface-scm.hh"
#include "guile-fixups.h"
#endif // USE_GUILE

#ifdef USE_PYTHON
#include "c-interface-python.hh"
#endif // USE_PYTHON

#if defined (WINDOWS_MINGW)
#ifdef DATADIR
#undef DATADIR
#endif // DATADIR
#endif /* MINGW */
#include "compat/sleep-fixups.h"

// Here we used to define GTK_ENABLE_BROKEN if defined(WINDOWS_MINGW)
// Now we don't want to enable broken stuff.  That is not the way.

#define HAVE_CIF  // will become unnessary at some stage.

#include <sys/types.h> // for stating
#include <sys/stat.h>
#if !defined _MSC_VER
#include <unistd.h>
#else
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define snprintf _snprintf
#include <windows.h>
#include <direct.h>
#endif // _MSC_VER

#include <GL/glut.h> // needed for glutGet(GLUT_ELAPSED_TIME);

#include "clipper/ccp4/ccp4_map_io.h"
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include <vector>
#include <string>

#include <mmdb/mmdb_manager.h>
#include "coords/mmdb-extras.h"
#include "coords/mmdb.h"
#include "coords/mmdb-crystal.h"
#include "coords/Cartesian.h"
#include "coords/Bond_lines.h"

#include "utils/coot-utils.hh"
#include "coot-utils/coot-map-utils.hh"
#include "coot-database.hh"
#include "coot-fileselections.h"

// #include "xmap-interface.h"
#include "graphics-info.h"

#include "skeleton/BuildCas.h"

#include "trackball.h" // adding exportable rotate interface


#include "c-interface.h"
#include "cc-interface.hh"
#include "c-interface-ligands.hh"

#include "nsv.hh"

#include "testing.hh"

#include "positioned-widgets.h"

// moving column_label selection to c-interface from mtz bits.
#include "cmtz-interface.hh"
// #include "mtz-bits.h" stuff from here moved to cmtz-interface

// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
//
#ifdef USE_PYTHON
#include "Python.h"
#endif // USE_PYTHON

int svn_revision(); 

char *coot_version() {

   std::string version_string = VERSION;

   version_string += " (revision ";
   version_string += coot::util::int_to_string(svn_revision());
   version_string += ") ";
   
#ifdef USE_GUILE
   version_string += " [with guile ";
   version_string += coot::util::int_to_string(SCM_MAJOR_VERSION);
   version_string += ".";
   version_string += coot::util::int_to_string(SCM_MINOR_VERSION);
   version_string += ".";
   version_string += coot::util::int_to_string(SCM_MICRO_VERSION);
   version_string += " embedded]";
#endif    
#ifdef USE_PYTHON
   version_string += " [with python ";
   version_string += coot::util::int_to_string(PY_MAJOR_VERSION);
   version_string += ".";
   version_string += coot::util::int_to_string(PY_MINOR_VERSION);
   version_string += ".";
   version_string += coot::util::int_to_string(PY_MICRO_VERSION);
   version_string += " embedded]";
#endif    
   int len = version_string.length();

   
   char *r = new char[len+1];
   strncpy(r, version_string.c_str(), len+1);
   return r;
}

// return the coot_revision as a char *.  note that svn_revision()
// returns the svn revision as an it.  More useful?
char *coot_revision() {
  
   std::string revision_string = " (revision ";
   revision_string += coot::util::int_to_string(svn_revision());
   revision_string += ") ";
   int len = revision_string.length();
   
   char *r = new char[len+1];
   strncpy(r, revision_string.c_str(), len+1);
   return r;
}

#ifdef USE_GUILE
SCM coot_sys_build_type_scm() {

   std::string sb = COOT_SYS_BUILD_TYPE;
   std::cout << sb << std::endl;
   SCM r = scm_makfrom0str(sb.c_str());
   return r;
} 
#endif

#ifdef USE_PYTHON
PyObject *coot_sys_build_type_py() {

   std::string sb = COOT_SYS_BUILD_TYPE;
   PyObject *r = PyString_FromString(sb.c_str());
   return r;
} 
#endif // USE_PYTHON

/*!  \brief tell coot that you prefer to run python scripts if/when
  there is an option to do so. */
void set_prefer_python() {

#ifdef USE_PYTHON
   graphics_info_t::prefer_python = 1;
#endif // USE_PYTHON   
}

/*! \brief the python-prefered mode. 

This is available so that the scripting functions know whether on not
to put themselves onto in as menu items.

return 1 for python is prefered, 0 for not. */
int prefer_python() {
   return graphics_info_t::prefer_python;
} 




/*  -------------------------------------------------------------------- */
/*                     Testing Interface:                                */
/*  -------------------------------------------------------------------- */

#ifdef USE_GUILE
SCM test_internal_scm() {

   SCM r = SCM_BOOL_T;

#ifdef BUILT_IN_TESTING   
   int status = greg_internal_tests();
   if (!status)
      r = SCM_BOOL_F;
   else
      status = greg_tests_using_external_data();
   if (!status)
      r = SCM_BOOL_F;
#endif

   return r;
} 
#endif // USE_GUILE

#ifdef USE_GUILE
SCM test_internal_single_scm() {

   SCM r = SCM_BOOL_T;

#ifdef BUILT_IN_TESTING   
   int status = test_internal_single();
   if (!status)
      r = SCM_BOOL_F;
#endif   
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *test_internal_py() {

   PyObject *r = Py_True;

#ifdef BUILT_IN_TESTING   
   int status = test_internal();
   if (!status)
      r = Py_False;
#endif   

   Py_INCREF(r);
   return r;
} 
#endif // USE_PYTHON

#ifdef USE_PYTHON
PyObject *test_internal_single_py() {

   PyObject *r = Py_True;

#ifdef BUILT_IN_TESTING   
   int status = test_internal_single();
   if (!status)
      r = Py_False;
#endif   
   Py_INCREF(r);
   return r;
} 
#endif // USE_PYTHON


// Return 0 if not a valid name ( -> #f in scheme)
// e.g. /a/b/c.pdb "d/e/f.mtz FWT PHWT"
// 
const char *molecule_name(int imol) {

   const char *r = NULL;
   if (is_valid_map_molecule(imol)) {
      r = graphics_info_t::molecules[imol].name_.c_str();
      return r;
   }
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].name_.c_str();
   }
   std::string cmd = "molecule-name";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   
   return r;
}

#ifdef USE_GUILE
SCM molecule_name_stub_scm(int imol, int include_path_flag) {
   std::string r;
   if (is_valid_map_molecule(imol) || is_valid_model_molecule(imol))
      r = graphics_info_t::molecules[imol].name_sans_extension(include_path_flag);
   return scm_makfrom0str(r.c_str());
}
#endif 

#ifdef USE_PYTHON
PyObject *molecule_name_stub_py(int imol, int include_path_flag) {
   std::string r;
   if (is_valid_map_molecule(imol) || is_valid_model_molecule(imol))
      r = graphics_info_t::molecules[imol].name_sans_extension(include_path_flag);
   return PyString_FromString(r.c_str());
}
#endif 

void set_molecule_name(int imol, const char *new_name) {

   if (is_valid_model_molecule(imol) ||
       is_valid_map_molecule(imol)) { 
      graphics_info_t::molecules[imol].set_name(new_name);
   } 
}

void set_show_graphics_ligand_view(int state) {
   graphics_info_t::show_graphics_ligand_view_flag = state;
   graphics_draw();
} 



//  Display characteristics:
//
// 
void set_esoteric_depth_cue(int istate) {

   graphics_info_t::esoteric_depth_cue_flag = istate;
   std::string cmd = "set-esoteric-depth-cue";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
   graphics_draw();
}

int  esoteric_depth_cue_state() {
   add_to_history_simple("esoteric-depth-cue-state");
   return graphics_info_t::esoteric_depth_cue_flag;
}


/*! \brief shall we start up the Gtk and the graphics window? 

   if passed the command line argument --no-graphics, coot will not
   start up gtk itself.

   An interface function for Ralf.
*/
short int use_graphics_interface_state() {

   add_to_history_simple("use-graphics-interface-state");
   return graphics_info_t::use_graphics_interface_flag; 

}

short int python_at_prompt_at_startup_state() {
   return graphics_info_t::python_at_prompt_flag;
}

/*! \brief start Gtk (and graphics) 

   This function is useful if it was not started already (which can be
   achieved by using the command line argument --no-graphics).

   An interface for Ralf */
void start_graphics_interface() {
   add_to_history_simple("start-graphics-interface");
   gtk_main(); 
} 




// Is this a repeat of something?  I don't know (doesn't look like it)
// 
void run_generic_script(const std::vector<std::string> &cmd_strings) {
   
   graphics_info_t g;

#if defined(USE_GUILE) && !defined(WINDOWS_MINGW)
   std::string s = g.state_command(cmd_strings, coot::STATE_SCM);
   safe_scheme_command(s);
#else
#ifdef USE_PYTHON
   std::string s = g.state_command(cmd_strings, coot::STATE_PYTHON);
   safe_python_command(s);
#endif // USE_PYTHON
#endif // USE_GUILE

   std::string cmd = "run-generic-script";
   std::vector<coot::command_arg_t> args;
   for(unsigned int i=0; i<cmd_strings.size(); i++) 
      args.push_back(clipper::String(cmd_strings[i]));
   add_to_history_typed(cmd, args);
}

   



//
// Return the molecule number of the molecule that we just filled.
// Return -1 if there was a failure.
// 
int handle_read_draw_molecule(const char *filename) {

   int r = graphics_info_t::recentre_on_read_pdb;
   return handle_read_draw_molecule_with_recentre(filename, r);
}

void set_convert_to_v2_atom_names(short int state) {
   graphics_info_t::convert_to_v2_atom_names_flag = state;
}




int handle_read_draw_molecule_with_recentre(const char *filename,
					   int recentre_on_read_pdb_flag) {

   int r = -1;
   //
   // cout << "handle_read_draw_molecule: handling " << filename << endl;
   
   graphics_info_t g;
   if (! filename)
      return -1;

   std::string cmd = "handle-read-draw-molecule-with-recentre";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(filename));
   args.push_back(recentre_on_read_pdb_flag);
   add_to_history_typed(cmd, args);

   std::string f(filename);

   // returns e.g. ".ins"

   int  istat = -1;
   std::string extention = coot::util::file_name_extension(filename);
   if (coot::util::extension_is_for_shelx_coords(extention)) {

      //       if (0) { // Don't do this path.  handle_read_draw_molecule() call
	    // get_atom_selection() which does this check and goes
	    // down the shelx path already.
      // But!  That's not the whole story read_shelx_ins_file calls
      // the read_shelx_ins_file() method of molecule_class_info_t,
      // which does different things to handle_read_draw_molecule,
      // specifically, it doesn't call get_atom_selection() and so the
      // Hydrogen names are not fixed.  Currently that is the case.
      // We'd like the fixing of hydrogen names in this
      // read_shelx_ins_file path too.
      
      r = read_shelx_ins_file(filename, recentre_on_read_pdb_flag);

   } else { 
      // recentre and not a backup-restore
      // -1 is for failure strangely.
      int imol = g.create_molecule();
//       std::cout << "DEBUG:: c-interface handle_read_draw_molecule on molecule number: "
// 		<< imol << std::endl;
//       std::cout << " DEBUG:: created placeholder molecule number " << imol << std::endl;
      float bw = graphics_info_t::default_bond_width;
      int bonds_box_type = graphics_info_t::default_bonds_box_type;
      istat = g.molecules[imol].handle_read_draw_molecule(imol, f,
							  coot::util::current_working_dir(),
							  recentre_on_read_pdb_flag, 0,
							  g.convert_to_v2_atom_names_flag,
							  bw, bonds_box_type);

      if (istat == 1) {
	 std::cout << "Molecule " << imol << " read successfully\n";
	 
	 // we do this somewhat awkward in and out thing with the
	 // molecule, because I don't want to (or am not able to) pass a
	 // modifiable dictionary to the handle_read_draw_molecule()
	 // function.  But maybe I *should* add it as an argument... Not
	 // sure.
	 //
	 // So currently we read in the molecule, read the molecule for
	 // types not in the dictionary and then try to read an sbase
	 // description (should be fast) and if found, rerun the bonding
	 // algorithm.

      
	 std::vector<std::string> types_with_no_dictionary =
	    g.molecules[imol].no_dictionary_for_residue_type_as_yet(*g.Geom_p());

	 int first_n_types_with_no_dictionary = types_with_no_dictionary.size();
	 
	 std::cout << "DEBUG:: there were " << types_with_no_dictionary.size() << " types "
		   << "with no dictionary " << std::endl;

	 for (unsigned int i=0; i<types_with_no_dictionary.size(); i++) {
	    if (0)
	       std::cout << "DEBUG:: calling try_dynamic_add: " << types_with_no_dictionary[i]
			 << " with read number " << g.cif_dictionary_read_number << std::endl;
	    int n_bonds = g.Geom_p()->try_dynamic_add(types_with_no_dictionary[i],
						      g.cif_dictionary_read_number);
	    g.cif_dictionary_read_number++;
	 }
	 
	 types_with_no_dictionary = g.molecules[imol].no_dictionary_for_residue_type_as_yet(*g.Geom_p());

	 if (types_with_no_dictionary.size()) {
	    if (g.Geom_p()->try_load_sbase_description(types_with_no_dictionary))
	       g.molecules[imol].make_bonds_type_checked();
	 } else {
	    // perhaps we have read dictionaries for everything (but
	    // first check that there had been dictionaries to read.
	    if (first_n_types_with_no_dictionary > 0) {
	       g.molecules[imol].make_bonds_type_checked();
	    } 
	 } 

	 
	 if (graphics_info_t::nomenclature_errors_mode == coot::PROMPT) { 
	    // Now, did that PDB file contain nomenclature errors?
	    std::vector<std::pair<std::string,coot::residue_spec_t> > nomenclature_errors = 
	       g.molecules[imol].list_nomenclature_errors(g.Geom_p());
	    // gui function checks use_graphics_interface_flag
	    if (nomenclature_errors.size())
	       show_fix_nomenclature_errors_gui(imol, nomenclature_errors);
	 }
	 if (graphics_info_t::nomenclature_errors_mode == coot::AUTO_CORRECT) {
	    fix_nomenclature_errors(imol);
	 }
	 
	 

	 // if the go to atom widget exists, update its optionmenu to
	 // reflect the existance of this new molecule.

	 if (g.go_to_atom_window) {
	    //
	    // 20090620: 
	    // Are we sure that we want to set the go_to_atom_molecule
	    // on reading a pdb file?
	    //
	    // The code handling the residue trees presumes that we
	    // don't.  i.e. the residue trees are not updated if the
	    // new molecule menu item is selected but has the pos as
	    // the go_to_atom_molecule() which we (used to) set here.
	    
	    // g.set_go_to_atom_molecule(imol);  // No.
	    
	    g.update_go_to_atom_window_on_new_mol();
	    // g.update_go_to_atom_window_on_changed_mol(imol);
	 } else {
	    // The Go To Atom window is not displayed.
	    g.set_go_to_atom_molecule(imol);
	 }
      
	 // now force a draw of the molecule
	 //
	 graphics_draw();

	 std::string s("Successfully read coordinates file ");
	 s += filename;
	 s += ".  Molecule number ";
	 s += coot::util::int_to_string(imol);
	 s += " created.";
	 g.add_status_bar_text(s);
	 r =  imol;
      } else {
	 g.erase_last_molecule();
	 std::string s("Failed to read coordinates file ");
	 s += filename;
	 g.add_status_bar_text(s);
	 r =  -1;
      }
   }
   return r; 
}


/*! \brief read coordinates from filename and recentre the new
  molecule at the scren rotation centre. */
int handle_read_draw_molecule_and_move_molecule_here(const char *filename) {
   int imol = handle_read_draw_molecule_with_recentre(filename, 0);
   move_molecule_to_screen_centre_internal(imol);
   return imol;
}

int read_pdb(const char *filename) {
   // history is done in the handle function
   return handle_read_draw_molecule(filename); 
}


/*! \brief replace pdb.  Fail if molecule_number is not a valid model molecule.
  Return -1 on failure.  Else return molecule_number  */
int clear_and_update_model_molecule_from_file(int molecule_number, 
					      const char *file_name) {
   int imol = -1;
   if (is_valid_model_molecule(molecule_number)) {
      atom_selection_container_t asc = get_atom_selection(file_name, 1);
      CMMDBManager *mol = asc.mol;
      graphics_info_t::molecules[molecule_number].replace_molecule(mol);
      imol = molecule_number;
      graphics_draw();
   }
   return imol;

} 


void set_draw_zero_occ_markers(int status) { 
   
   graphics_info_t g;
   g.draw_zero_occ_spots_flag = status;
   std::string cmd = "set-draw-zero-occ-markers";
   std::vector<coot::command_arg_t> args;
   args.push_back(status);
   add_to_history_typed(cmd, args);
   graphics_draw();
}



int first_coords_imol() {

   int imol = -1;
   for (int i=0; i<graphics_n_molecules(); i++) {
      if (graphics_info_t::molecules[i].has_model()) {
	 imol = i;
	 break;
      }
   }
   add_to_history_simple("first-coords-imol");
   return imol;
}

/*! \brief molecule number of first small (<400 atoms) molecule.

return -1 on no such molecule
  */
int first_small_coords_imol() {

   int imol = -1;
   for (int i=0; i<graphics_n_molecules(); i++) {
      if (graphics_info_t::molecules[i].has_model()) {
	 int n_atoms = graphics_info_t::molecules[i].atom_sel.n_selected_atoms;
	 if (n_atoms < N_ATOMS_MEANS_BIG_MOLECULE) {
	    imol = i;
	    break;
	 }
      }
   }
   add_to_history_simple("first-small-coords-imol");
   return imol;
}


int first_unsaved_coords_imol() {

   int imol = -1;
   for (int i=0; i<graphics_n_molecules(); i++) {
      if (graphics_info_t::molecules[i].has_model()) {
	 if (graphics_info_t::molecules[i].Have_unsaved_changes_p()) {
	    imol = i;
	    break;
	 }
      }
   }
   add_to_history_simple("first-unsaved-coords-imol");
   return imol;
} 



void hardware_stereo_mode() {

   if (graphics_info_t::use_graphics_interface_flag) { 
      if (graphics_info_t::display_mode != coot::HARDWARE_STEREO_MODE) {
	 int previous_mode = graphics_info_t::display_mode;
	 graphics_info_t::display_mode = coot::HARDWARE_STEREO_MODE;
	 GtkWidget *vbox = lookup_widget(graphics_info_t::glarea, "vbox1");
	 if (!vbox) {
	    std::cout << "ERROR:: failed to get vbox in hardware_stereo_mode!\n";
	 } else {

	    if ( (previous_mode == coot::SIDE_BY_SIDE_STEREO) ||
		 (previous_mode == coot::DTI_SIDE_BY_SIDE_STEREO)  || 
		 (previous_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ) {

	       if (graphics_info_t::glarea_2) {
		  gtk_widget_destroy(graphics_info_t::glarea_2);
		  graphics_info_t::glarea_2 = NULL;
	       }
	    }
	    
	    short int try_hardware_stereo_flag = 1;
	    GtkWidget *glarea = gl_extras(vbox, try_hardware_stereo_flag);
	    if (glarea) { 
	       // std::cout << "INFO:: switch to hardware_stereo_mode succeeded\n";
	       if (graphics_info_t::idle_function_spin_rock_token) { 
		  toggle_idle_spin_function(); // turn it off;
	       }
// BL says:: maybe we should set the set_display_lists_for_maps here for
// windows, actually Mac as well if I remember correctly
// well, it seems actually to be a GTK2 (or gtkglext) thing!!
// or not? So just for windows at the moment
#ifdef WINDOWS_MINGW
//	    std::cout << "BL DEBUG:: set_display_map_disabled!!!!\n";
             set_display_lists_for_maps(0);
#endif // WINDOWS_MINGW
	       gtk_widget_destroy(graphics_info_t::glarea);
	       graphics_info_t::glarea = glarea;
	       gtk_widget_show(glarea);
	       // antialiasing?
	       graphics_info_t g;
	       g.draw_anti_aliasing();
	       graphics_draw();
	    } else {
	       std::cout << "WARNING:: switch to hardware_stereo_mode failed\n";
	       graphics_info_t::display_mode = previous_mode;
	    }
	 }
      } else {
	 std::cout << "Already in hardware stereo mode" << std::endl;
      }
   }
   add_to_history_simple("hardware-stereo-mode");

}

void zalman_stereo_mode() {

  // FIXME this is not really zalman!!!!!
   if (graphics_info_t::use_graphics_interface_flag) { 
      if (graphics_info_t::display_mode != coot::HARDWARE_STEREO_MODE) {
	 int previous_mode = graphics_info_t::display_mode;
	 graphics_info_t::display_mode = coot::ZALMAN_STEREO;
	 GtkWidget *vbox = lookup_widget(graphics_info_t::glarea, "vbox1");
	 if (!vbox) {
	    std::cout << "ERROR:: failed to get vbox in zalman_stereo_mode!\n";
	 } else {

	    if ( (previous_mode == coot::SIDE_BY_SIDE_STEREO) ||
		 (previous_mode == coot::DTI_SIDE_BY_SIDE_STEREO)  || 
		 (previous_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ) {

	       if (graphics_info_t::glarea_2) {
		  gtk_widget_destroy(graphics_info_t::glarea_2);
		  graphics_info_t::glarea_2 = NULL;
	       }
	    }
	    
	    short int try_hardware_stereo_flag = 5;
	    GtkWidget *glarea = gl_extras(vbox, try_hardware_stereo_flag);
	    if (glarea) { 
	       std::cout << "INFO:: switch to zalman_stereo_mode succeeded\n";
	       if (graphics_info_t::idle_function_spin_rock_token) { 
		  toggle_idle_spin_function(); // turn it off;
	       }
	       gtk_widget_destroy(graphics_info_t::glarea);
	       graphics_info_t::glarea = glarea;
	       gtk_widget_show(glarea);
	       // antialiasing?
	       graphics_info_t g;
	       g.draw_anti_aliasing();
	       graphics_draw();
	    } else {
	       std::cout << "WARNING:: switch to zalman_stereo_mode failed\n";
	       graphics_info_t::display_mode = previous_mode;
	    }
	 }
      } else {
	 std::cout << "Already in zalman stereo mode" << std::endl;
      }
   }
   add_to_history_simple("zalman-stereo-mode");

}

void mono_mode() {

   if (graphics_info_t::use_graphics_interface_flag) {
      
      if (graphics_info_t::display_mode != coot::MONO_MODE) { 
	 int previous_mode = graphics_info_t::display_mode;
         GtkWidget *main_win = lookup_widget(graphics_info_t::glarea, "window1");
         int x_size = main_win->allocation.width;
         int y_size = main_win->allocation.height;
	 graphics_info_t::display_mode = coot::MONO_MODE;
	 GtkWidget *vbox = lookup_widget(graphics_info_t::glarea, "vbox1");
	 if (!vbox) {
	    std::cout << "ERROR:: failed to get vbox in mono mode!\n";
	 } else {
	    short int try_hardware_stereo_flag = 0;
	    //BL says:: and we switch the lists_maps back to normal
            //          for windows users; seems to be necessary at the moment
#ifdef WINDOWS_MINGW
	    set_display_lists_for_maps(1);
	    //	    std::cout << "BL DEBUG:: set_display_map_disabled!!!!\n";
#endif // WINDOWS_MINGW
	    GtkWidget *glarea = gl_extras(vbox, try_hardware_stereo_flag);
	    if (glarea) { 
	       std::cout << "INFO:: switch to mono_mode succeeded\n";
	       if (graphics_info_t::idle_function_spin_rock_token) { 
		  toggle_idle_spin_function(); // turn it off;
	       }
	       gtk_widget_destroy(graphics_info_t::glarea);
	       if (graphics_info_t::glarea_2) { 
		  gtk_widget_destroy(graphics_info_t::glarea_2);
		  graphics_info_t::glarea_2 = NULL;
	       }
	       graphics_info_t::glarea = glarea;
	       gtk_widget_show(glarea);
               // now we shall resize to half the window size if we had
               // side-by-side stereo before
               if ((previous_mode == coot::SIDE_BY_SIDE_STEREO) ||
                   (previous_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ||
                   (previous_mode == coot::DTI_SIDE_BY_SIDE_STEREO)) {
                 set_graphics_window_size(x_size/2, y_size);
               }
	       // std::cout << "DEBUG:: mono_mode() update maps and draw\n";
	       graphics_info_t g;
	       g.setRotationCentre(coot::Cartesian(g.X(), g.Y(), g.Z()));
	       g.post_recentre_update_and_redraw();
	       g.draw_anti_aliasing();
	       redraw_background();
	    } else {
	       graphics_info_t::display_mode = previous_mode;
	       std::cout << "WARNING:: switch to mono mode failed\n";
	    }
	 }
      } else {
	 // std::cout << "Already in mono mode" << std::endl; // we know.
      }
   }
   add_to_history_simple("mono-mode");
}

/*! \brief turn on side bye side stereo mode */
void side_by_side_stereo_mode(short int use_wall_eye_flag) {

   if (graphics_info_t::use_graphics_interface_flag) {

      
      // If it wasn't in side by side stereo mode, then we need to
      // generated 2 new glareas by calling gl_extras().
      // 
      if (!((graphics_info_t::display_mode == coot::SIDE_BY_SIDE_STEREO) ||
	    (graphics_info_t::display_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ||
	    (graphics_info_t::display_mode == coot::DTI_SIDE_BY_SIDE_STEREO))) {
	 
	 if (use_wall_eye_flag == 1) {
	    graphics_info_t::in_wall_eyed_side_by_side_stereo_mode = 1;
	    graphics_info_t::display_mode = coot::SIDE_BY_SIDE_STEREO_WALL_EYE;
	 } else {
	    graphics_info_t::in_wall_eyed_side_by_side_stereo_mode = 0;
	    graphics_info_t::display_mode = coot::SIDE_BY_SIDE_STEREO;
	 }
	 // int previous_mode = graphics_info_t::display_mode;
	 short int stereo_mode = coot::SIDE_BY_SIDE_STEREO;
	 if (use_wall_eye_flag)
	    stereo_mode = coot::SIDE_BY_SIDE_STEREO_WALL_EYE;
	 GtkWidget *vbox = lookup_widget(graphics_info_t::glarea, "vbox1");
	 GtkWidget *glarea = gl_extras(vbox, stereo_mode);
	 if (glarea) {
	    if (graphics_info_t::idle_function_spin_rock_token) { 
	       toggle_idle_spin_function(); // turn it off;
	    }
	    gtk_widget_destroy(graphics_info_t::glarea);
	    graphics_info_t::glarea = glarea; // glarea_2 is stored by gl_extras()
	    gtk_widget_show(glarea);
	    gtk_widget_show(graphics_info_t::glarea_2);
	    update_maps();
	    // antialiasing?
	    graphics_info_t g;
	    g.draw_anti_aliasing();
	    redraw_background();
	    graphics_draw();
	 } else {
	    std::cout << "WARNING:: switch to side by side mode failed!\n";
	 } 
      } else {

	 if (use_wall_eye_flag == 1) {
	    graphics_info_t::in_wall_eyed_side_by_side_stereo_mode = 1;
	    graphics_info_t::display_mode = coot::SIDE_BY_SIDE_STEREO_WALL_EYE;
	 } else {
	    graphics_info_t::in_wall_eyed_side_by_side_stereo_mode = 0;
	    graphics_info_t::display_mode = coot::SIDE_BY_SIDE_STEREO;
	 }
	 // were were already in some sort of side by side stereo mode:
	 graphics_draw();
      }
   }
   std::vector<coot::command_arg_t> args;
   args.push_back(use_wall_eye_flag);
   add_to_history_typed("side-by-side-stereo-mode", args);
}

/* DTI stereo mode - undocumented, secret interface for testing, currently */
// when it works, call it dti_side_by_side_stereo_mode()
void set_dti_stereo_mode(short int state) {

   if (graphics_info_t::use_graphics_interface_flag) {
      if (state) { 
	 short int stereo_mode;
	 if (graphics_info_t::display_mode != coot::DTI_SIDE_BY_SIDE_STEREO) {
	    // int previous_mode = graphics_info_t::display_mode;
	    stereo_mode = coot::DTI_SIDE_BY_SIDE_STEREO;
	 } else {
	    stereo_mode = coot::SIDE_BY_SIDE_STEREO;
	 }
	 GtkWidget *vbox = lookup_widget(graphics_info_t::glarea, "vbox1");
	 GtkWidget *glarea = gl_extras(vbox, stereo_mode);
	 if (graphics_info_t::use_graphics_interface_flag) {
	    if (graphics_info_t::idle_function_spin_rock_token) { 
	       toggle_idle_spin_function(); // turn it off;
	    }
	    gtk_widget_destroy(graphics_info_t::glarea);
	    graphics_info_t::glarea = glarea; // glarea_2 is stored by gl_extras()
	    gtk_widget_show(glarea);
	    gtk_widget_show(graphics_info_t::glarea_2);
	    graphics_draw();
	 } else {
	    std::cout << "WARNING:: switch to side by side mode failed!\n";
	 }
      }
   }
   // add_to_history_simple("dti-side-by-side-stereo-mode");
} 


int stereo_mode_state() {
   add_to_history_simple("stereo-mode-state");
   return graphics_info_t::display_mode;
}

void set_hardware_stereo_angle_factor(float f) {
   graphics_info_t::hardware_stereo_angle_factor = f;
   std::string cmd = "set-hardware-stereo-angle-factor";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
   graphics_draw();
}

float hardware_stereo_angle_factor_state() {
   add_to_history_simple("hardware-stereo-angle-factor-state");
   return graphics_info_t::hardware_stereo_angle_factor;
}


void graphics_draw() {
   graphics_info_t::graphics_draw();
}



void 
set_run_state_file_status(short int istat) {
   std::string cmd = "set-run-state-file-status";
   std::vector<coot::command_arg_t> args;
   args.push_back(istat);
   add_to_history_typed(cmd, args);
   
   graphics_info_t::run_state_file_status = istat;
}


void set_sticky_sort_by_date() {

   add_to_history_simple("set-sticky-sort-by-date");
   graphics_info_t g;
   g.sticky_sort_by_date = 1;

}


void unset_sticky_sort_by_date() {

   add_to_history_simple("unset-sticky-sort-by-date");
   graphics_info_t g;
   g.sticky_sort_by_date = 0;

}


void set_filter_fileselection_filenames(int istate) {
   std::string cmd = "set-filter-fileselection-filenames";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);

   graphics_info_t::filter_fileselection_filenames_flag = istate;

}

int filter_fileselection_filenames_state() {
   add_to_history_simple("filter-fileselection-filenames-state");
   return graphics_info_t::filter_fileselection_filenames_flag;
}

/*! \brief is the given file name suitable to be read as coordinates? */
short int file_type_coords(const char *file_name) {

   graphics_info_t g;
   return g.file_type_coords(file_name);
} 


void set_unit_cell_colour(float red, float green, float blue) {

   coot::colour_holder ch(red, green, blue);
   graphics_info_t::cell_colour = ch;
   graphics_draw();

   std::string cmd = "set-unit-cell-colour";
   std::vector<coot::command_arg_t> args;
   args.push_back(red);
   args.push_back(green);
   args.push_back(blue);
   add_to_history_typed(cmd, args);
}

/*! \brief return the new molecule number */

void get_coords_for_accession_code(const char *text) {

   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(text));
   scripting_function("get-ebi-pdb", args);
}





std::vector<std::string> filtered_by_glob(const std::string &pre_directory, 
					  int data_type) { 

   std::vector<std::string> v; // returned object
   std::vector<std::string> globs;

#if !defined(_MSC_VER)

   // std::map<std::string, int, std::less<std::string> >  files;

   if (data_type == COOT_COORDS_FILE_SELECTION)
      globs = *graphics_info_t::coordinates_glob_extensions;
   if (data_type == COOT_DATASET_FILE_SELECTION) 
      globs = *graphics_info_t::data_glob_extensions;
   if (data_type == COOT_MAP_FILE_SELECTION) 
      globs = *graphics_info_t::map_glob_extensions;
   if (data_type == COOT_CIF_DICTIONARY_FILE_SELECTION) 
      globs = *graphics_info_t::dictionary_glob_extensions;
   if (data_type == COOT_SAVE_COORDS_FILE_SELECTION) 
      globs = *graphics_info_t::coordinates_glob_extensions;
   if (data_type == COOT_PHS_COORDS_FILE_SELECTION)
      globs = *graphics_info_t::coordinates_glob_extensions;

   for (unsigned int i=0; i<globs.size(); i++) { 

      std::string file_name_glob = pre_directory;
      file_name_glob += "/";

      file_name_glob += "*";
      file_name_glob += globs[i];
      glob_t myglob;
      int flags = 0;
      glob(file_name_glob.c_str(), flags, 0, &myglob);
      size_t count;

      char **p;
      for (p = myglob.gl_pathv, count = myglob.gl_pathc; count; p++, count--) {
	 std::string f(*p);
	 if (! string_member(f, v))
	    v.push_back(f);
      }
      globfree(&myglob);
   }

   // now we need to sort v;
   std::sort(v.begin(), v.end(), compare_strings);

#endif // MSC

   return v;
}

bool
compare_strings(const std::string &a, const std::string &b) { 
   return a < b ? 1 : 0;
} 


// Return 1 if search appears in list, 0 if not)
// 
short int 
string_member(const std::string &search, const std::vector<std::string> &list) { 
   
   short int v = 0;
   for (unsigned int i=0; i<list.size(); i++) { 
      if (list[i] == search) { 
	 v = 1;
	 break;
      }
   }
   return v;
} 


// --------------------------------------------------------------------
// Ctrl for rotate or pick:
// --------------------------------------------------------------------
// 
// Coot mailing list discussion: users want Ctrl for Rotation or Ctrl
// for picking, so that accidental picking when rotation is meant is
// avoided.
// 
void set_control_key_for_rotate(int state) {
   graphics_info_t::control_key_for_rotate_flag = state;
}

int control_key_for_rotate_state() {
   return graphics_info_t::control_key_for_rotate_flag;
}




/*  ------------------------------------------------------------------------ */
/*                         Model/Fit/Refine Functions:                       */
/*  ------------------------------------------------------------------------ */
void post_model_fit_refine_dialog() {

   GtkWidget *widget = wrapped_create_model_fit_refine_dialog();
   if (graphics_info_t::use_graphics_interface_flag) { 
      gtk_widget_show(widget);
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("post-model-fit-refine-dialog");
   add_to_history(command_strings);
}

void post_other_modelling_tools_dialog() {

   GtkWidget *widget = wrapped_create_model_fit_refine_dialog();
   if (graphics_info_t::use_graphics_interface_flag) { 
      gtk_widget_show(widget);
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("post-other-modelling-tools-dialog");
   add_to_history(command_strings);

} 

void set_auto_read_column_labels(const char *fwt, const char *phwt, 
				 int is_for_diff_map_flag) {

   
   coot::mtz_column_trials_info_t n(fwt, phwt, is_for_diff_map_flag);
   graphics_info_t::user_defined_auto_mtz_pairs.push_back(n);

   std::string cmd = "set-auto-read-column-labels";
   std::vector<coot::command_arg_t> args;
   args.push_back(fwt);
   args.push_back(phwt);
   args.push_back(is_for_diff_map_flag);
   add_to_history_typed(cmd, args);

}


void toggle_idle_spin_function() { 

   graphics_info_t g; 

   if (g.idle_function_spin_rock_token == 0) { 
      g.idle_function_spin_rock_token = gtk_idle_add((GtkFunction)animate_idle_spin, g.glarea);
   } else {
      gtk_idle_remove(g.idle_function_spin_rock_token);
      g.idle_function_spin_rock_token = 0; 
   }
   add_to_history_simple("toggle-idle-function");
}


void toggle_idle_rock_function() { 

   graphics_info_t g; 

   if (g.idle_function_spin_rock_token == 0) { 
      g.idle_function_spin_rock_token =
	 // gtk_idle_add((GtkFunction)animate_idle_rock, g.glarea);
	 gtk_timeout_add(25, // 40 fps
			 (GtkFunction) animate_idle_rock,
			 g.glarea);
      g.time_holder_for_rocking = glutGet(GLUT_ELAPSED_TIME);

      g.idle_function_rock_angle_previous =
	 get_idle_function_rock_target_angle();
   } else {
      gtk_idle_remove(g.idle_function_spin_rock_token);
      g.idle_function_spin_rock_token = 0;
   }
   add_to_history_simple("toggle-idle-rock-function");
}

/*! \brief Settings for the inevitable discontents who dislike the 
   default rocking rates (defaults 1 and 1)  */
void set_rocking_factors(float width, float freq_scale) {

   graphics_info_t g;
   g.idle_function_rock_amplitude_scale_factor = width;
   g.idle_function_rock_freq_scale_factor = freq_scale;

} 

/* Turn on nice animated ligand interaction display */
void toggle_flev_idle_ligand_interactions() {

   graphics_info_t g;
   if (g.idle_function_ligand_interactions_token == 0) {
      set_flev_idle_ligand_interactions(1);
   } else {
      set_flev_idle_ligand_interactions(0);
   }
   add_to_history_simple("toggle-idle-ligand-interactions");
}

void set_flev_idle_ligand_interactions(int state) {

   graphics_info_t g;
   if (state == 0) {
      // turn them off if they were on
      if (g.idle_function_ligand_interactions_token) { 
	 gtk_idle_remove(g.idle_function_ligand_interactions_token);
	 g.idle_function_ligand_interactions_token = 0;
	 for (unsigned int imol=0; imol<g.molecules.size(); imol++) { 
	    if (is_valid_model_molecule(imol)) {
	       g.molecules[imol].draw_animated_ligand_interactions_flag = 0;
	    }
	 }
      }
   } else {
      // turn them on if they were off.
      if (g.idle_function_ligand_interactions_token == 0) {
	 g.idle_function_ligand_interactions_token =
	    gtk_timeout_add(100,
			    (GtkFunction) animate_idle_ligand_interactions,
			    NULL);
	 g.time_holder_for_ligand_interactions = glutGet(GLUT_ELAPSED_TIME);
      }
   }
   g.graphics_draw();
   
} 




// in degrees
void set_idle_function_rotate_angle(float f) {

   graphics_info_t g;
   std::string cmd = "set-idle-function-rotate-angle";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
   g.idle_function_rotate_angle = f; 
}

float idle_function_rotate_angle() {

   std::string cmd = "idle-function-rotate-angle";
   add_to_history_simple(cmd);
   return graphics_info_t::idle_function_rotate_angle;
} 



void do_tw() {

}

// Another name for wrapped_nothing_bad_dialog, but this function also
// displays the widget so nothing is returned.
// 
void info_dialog(const char *txt) {

   graphics_info_t::info_dialog(txt);
   std::string cmd = "info-dialog";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(txt));
   add_to_history_typed(cmd, args);
}

// As info_dialog, but print to console too. Usefull for error and 
// warning messages.
// 
void info_dialog_and_text(const char *txt) {

   graphics_info_t g;
   g.info_dialog_and_text(txt);
   std::string cmd = "info-dialog-and-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(txt));
   add_to_history_typed(cmd, args);
}

void
set_main_window_title(const char *s) {

   graphics_info_t g;
   if (g.use_graphics_interface_flag){
      if (g.glarea) { 
      	 GtkWidget *win = lookup_widget(g.glarea, "window1");
	 if (win) { 
	    GtkWindow *window = GTK_WINDOW(win);
	    gtk_window_set_title(window, s);
	 }
      }
   }
} 



GtkWidget *main_menubar() {

   GtkWidget *w = lookup_widget(graphics_info_t::statusbar, "menubar1");
   return w; 
}

GtkWidget *main_statusbar() {
   return graphics_info_t::statusbar;
} 

GtkWidget *main_toolbar() {

   GtkWidget *w = lookup_widget(graphics_info_t::statusbar, "main_toolbar");
   return w; 
}




/*  ------------------------------------------------------------------------ */
/*                         file selection                                    */
/*  ------------------------------------------------------------------------ */

void
set_directory_for_fileselection(GtkWidget *fileselection1) { 
   graphics_info_t g;
   g.set_directory_for_fileselection(fileselection1);
}

void
save_directory_from_fileselection(const GtkWidget *fileselection) {
   graphics_info_t g;
   g.save_directory_from_fileselection(fileselection);
}

void save_directory_for_saving_from_fileselection(const GtkWidget *fileselection) {
   graphics_info_t g;
   g.save_directory_for_saving_from_fileselection(fileselection);
} 

/* and the gtk2 equivalents, we dont use most of them any more but keep
   them for gtk2 move maybe  */


void
set_directory_for_filechooser(GtkWidget *fileselection1) {
   graphics_info_t g;
   g.set_directory_for_filechooser(fileselection1);
}

void
save_directory_from_filechooser(const GtkWidget *fileselection) {
   graphics_info_t g;
   g.save_directory_from_filechooser(fileselection);
}

void save_directory_for_saving_from_filechooser(const GtkWidget *fileselection) {
   graphics_info_t g;
   g.save_directory_for_saving_from_filechooser(fileselection);
}




bool compare_mtimes(coot::str_mtime a, coot::str_mtime b) {
   return a.mtime > b.mtime;
}




void quanta_buttons() {
   graphics_info_t g;
   g.quanta_buttons();
   add_to_history_simple("quanta-buttons");
}

void quanta_like_zoom() { 
   graphics_info_t::quanta_like_zoom_flag = 1;
   add_to_history_simple("quanta-like-zoom");
} 

void set_scroll_by_wheel_mouse(int istate) {
   graphics_info_t::do_scroll_by_wheel_mouse_flag = istate;
   std::string cmd = "set-scroll-by-mouse-wheel";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
}

int scroll_by_wheel_mouse_state() {
   add_to_history_simple("scroll-by-wheel-mouse-state");
   return graphics_info_t::do_scroll_by_wheel_mouse_flag;
}


//
char* get_text_for_symmetry_size_widget() {

   // convert the density in the graphics_info_t

   graphics_info_t g;
   char *text;

   // we are interfacing with a c function, so we will need to malloc
   // the space for the returned char*.
   //
   // The c function should delete it.
   // 
   // Dontcha just *love" this sort coding! (yeuch)
   // 
   text = (char *) malloc(100);
   snprintf(text,100,"%-5.1f", g.symmetry_search_radius);

   return text;
}

/*! \brief return the density at the given point for the given map */
float density_at_point(int imol, float x, float y, float z) {

   float r = -999.9;
   if (is_valid_map_molecule(imol)) {
      clipper::Coord_orth p(x,y,z);
      r = graphics_info_t::molecules[imol].density_at_point(p);
   }
   return r;
}


#ifdef USE_GUILE
float density_score_residue_scm(int imol, SCM residue_spec_scm, int imol_map) {

   float v = 0.0;
   if (is_valid_map_molecule(imol_map)) { 
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
	 coot::residue_spec_t residue_spec = residue_spec_from_scm(residue_spec_scm);
	 CResidue *r = g.molecules[imol].get_residue(residue_spec);
	 if (r) {
	    PPCAtom residue_atoms = 0;
	    int n_residue_atoms;
	    r->GetAtomTable(residue_atoms, n_residue_atoms);
	    for (unsigned int iat=0; iat<n_residue_atoms; iat++) { 
	       CAtom *at = residue_atoms[iat];
	       float d_at = density_at_point(imol_map, at->x, at->y, at->z);
	       v += d_at * at->occupancy;
	    }
	 } 
      }
   }
   return v;
}
#endif 

#ifdef USE_PYTHON
float density_score_residue_py(int imol, PyObject *residue_spec_py, int imol_map) {

   float v = 0.0;
   if (is_valid_map_molecule(imol_map)) { 
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
	 coot::residue_spec_t residue_spec = residue_spec_from_py(residue_spec_py);
	 CResidue *r = g.molecules[imol].get_residue(residue_spec);
	 if (r) {
	    PPCAtom residue_atoms = 0;
	    int n_residue_atoms;
	    r->GetAtomTable(residue_atoms, n_residue_atoms);
	    for (unsigned int iat=0; iat<n_residue_atoms; iat++) { 
	       CAtom *at = residue_atoms[iat];
	       float d_at = density_at_point(imol_map, at->x, at->y, at->z);
	       v += d_at * at->occupancy;
	    }
	 } 
      }
   }
   return v; 
} 
#endif 

/*! \brief simple density score for given residue (over-ridden by scripting function) */
float density_score_residue(int imol, const char *chain_id, int res_no, const char *ins_code, int imol_map) {
   
   float v = 0.0;
   if (is_valid_map_molecule(imol_map)) { 
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
	 coot::residue_spec_t residue_spec(chain_id, res_no, ins_code);
	 CResidue *r = g.molecules[imol].get_residue(residue_spec);
	 if (r) {
	    PPCAtom residue_atoms = 0;
	    int n_residue_atoms;
	    r->GetAtomTable(residue_atoms, n_residue_atoms);
	    for (unsigned int iat=0; iat<n_residue_atoms; iat++) { 
	       CAtom *at = residue_atoms[iat];
	       float d_at = density_at_point(imol_map, at->x, at->y, at->z);
	       v += d_at * at->occupancy;
	    }
	 } 
      }
   }
   return v;

}


#ifdef USE_GUILE
SCM map_sigma_scm(int imol) {
  
  SCM r = SCM_BOOL_F;
  if (is_valid_map_molecule(imol)) { 
    float s = graphics_info_t::molecules[imol].map_sigma();
    r = scm_double2num(s);
  }
  return r;
}
#endif

#ifdef USE_PYTHON
PyObject *map_sigma_py(int imol) { 

  PyObject *r = Py_False;
  if (is_valid_map_molecule(imol)) { 
    float s = graphics_info_t::molecules[imol].map_sigma();
    r = PyFloat_FromDouble(s);
  }

  if (PyBool_Check(r)) {
    Py_INCREF(r);
  }
  return r;
}
#endif




void set_density_size_from_widget(const char *text) {

   float tmp;
   graphics_info_t g;

   tmp = atof(text);

   if ((tmp > 0.0) && (tmp < 9999.9)) {
      g.box_radius = tmp;
   } else {

      cout << "Cannot interpret " << text << ".  Assuming 10A" << endl;
      g.box_radius = 10.0;
   }
   //
   for (int ii=0; ii<g.n_molecules(); ii++) {
      if (is_valid_map_molecule(ii))
	  g.molecules[ii].update_map();
   }
   graphics_draw();
}

void set_density_size(float f) {

   graphics_info_t g;
   g.box_radius = f;
   for (int ii=0; ii<g.n_molecules(); ii++) {
      g.molecules[ii].update_map();
   }
   graphics_draw();
   std::string cmd = "set-density-size";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
   
}

/*! \brief set the extent of the box/radius of electron density contours */
void set_map_radius(float f) {
   set_density_size(f);
} 

/*! \brief return the extent of the box/radius of electron density contours */
float get_map_radius() {
  float ret = graphics_info_t::box_radius;
  return ret;
} 



void set_display_intro_string(const char *str) {

   if (str) { 
      if (graphics_info_t::use_graphics_interface_flag) { 
	 std::string s(str);
	 graphics_info_t g;
	 g.display_density_level_screen_string = s;
	 g.add_status_bar_text(s);
      }

      std::string cmd = "set-display-intro-string";
      std::vector<coot::command_arg_t> args;
      args.push_back(single_quote(str));
      add_to_history_typed(cmd, args);
   }
} 

void set_swap_difference_map_colours(int i) { 
   graphics_info_t::swap_difference_map_colours = i;
   std::string cmd = "set-swap-difference-map-colours";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
   
}



// return 1 on "yes, it has a cell".
// 
int has_unit_cell_state(int imol) { 

   int istate = 0;
   if (imol >= 0) { 
      if (imol < graphics_info_t::n_molecules()) { 
	 if (graphics_info_t::molecules[imol].has_model() ||
	     graphics_info_t::molecules[imol].has_xmap()) {
	    istate = graphics_info_t::molecules[imol].have_unit_cell;
	 }
      }
   }
   std::string cmd = "has-unit-cell-state";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return istate;
}
 
void set_symmetry_size_from_widget(const char *text) {
   
   float tmp;
   graphics_info_t g;

   tmp = atof(text);

   if ((tmp > 0.0) && (tmp < 9999.9)) {
      g.symmetry_search_radius = tmp;
   } else {

      cout << "Cannot interpret " << text << ".  Assuming 10A" << endl;
      g.symmetry_search_radius = 10.0;
   }
   //
   for (int ii=0; ii<g.n_molecules(); ii++) {
      g.molecules[ii].update_symmetry();
   }
   graphics_draw();
   
}

void set_symmetry_size(float f) {
   graphics_info_t g;
   g.symmetry_search_radius = f;
   for (int ii=0; ii<g.n_molecules(); ii++) {
      g.molecules[ii].update_symmetry();
   }
   graphics_draw();
   std::string cmd = "set-symmetry-size";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
}

/* When the coordinates for one (or some) symmetry operator are missing
   (which happens sometimes, but rarely), try changing setting this to 2
   (default is 1).  It slows symmetry searching, which is why it is not
   set to 2 by default.  */
void set_symmetry_shift_search_size(int shift) {
   
   graphics_info_t::symmetry_shift_search_size = shift;
   std::string cmd = "set-symmetry-shift-search-size";
   std::vector<coot::command_arg_t> args;
   args.push_back(shift);
   add_to_history_typed(cmd, args);
}


void set_symmetry_molecule_rotate_colour_map(int imol, int state) { 
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      g.molecules[imol].symmetry_rotate_colour_map_flag = state;
   }
   std::string cmd = "set-symmetry-molecule-rotate-colour-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(state);
   add_to_history_typed(cmd, args);
   graphics_draw();
} 

int symmetry_molecule_rotate_colour_map_state(int imol) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].symmetry_rotate_colour_map_flag;
   }
   std::string cmd = "symmetry-molecule-rotate-colour-map-state";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
} 

void set_symmetry_colour_by_symop(int imol, int state) {

   if (graphics_info_t::use_graphics_interface_flag) { 
      graphics_info_t g;
      if (is_valid_model_molecule(imol)) { 
	 g.molecules[imol].symmetry_colour_by_symop_flag = state;
	 graphics_draw();
      }
   }
   std::string cmd = "set-symmetry-colour-by-symop";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void set_symmetry_whole_chain(int imol, int state) {
   
   if (graphics_info_t::use_graphics_interface_flag) { 
      graphics_info_t g;
      if (is_valid_model_molecule(imol)) { 
	 g.molecules[imol].symmetry_whole_chain_flag = state;
	 if (g.glarea)
	    g.update_things_on_move_and_redraw();
      }
   }
   std::string cmd = "set-symmetry-whole-chain";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(state);
   add_to_history_typed(cmd, args);
}




void set_fps_flag(int thing) {

   graphics_info_t g;
   g.SetShowFPS(thing);
   std::string cmd = "set-fps-flag";
   std::vector<coot::command_arg_t> args;
   args.push_back(thing);
   add_to_history_typed(cmd, args);
} 

// For people without PCs with fast graphics cards :)  [like me]
// 
void set_active_map_drag_flag(int t) {

   graphics_info_t g;
   g.SetActiveMapDrag(t);
   std::string cmd = "set-active-map-drag-flag";
   std::vector<coot::command_arg_t> args;
   args.push_back(t);
   add_to_history_typed(cmd, args);
}

int get_fps_flag() {

   graphics_info_t g;
   add_to_history_simple("get-fps-flag");
   return g.GetFPSFlag();
} 


//
short int get_active_map_drag_flag() {

   graphics_info_t g;
   add_to_history_simple("get-active-map-drag-flag");
   return g.GetActiveMapDrag();
}

void set_draw_hydrogens(int imol, int istate) {

   graphics_info_t g;
   
   if (is_valid_model_molecule(imol)) {
      g.molecules[imol].set_draw_hydrogens_state(istate);
      graphics_draw();
   } else { 
      std::cout << "WARNING:: No such molecule number " << imol << "\n";
   }
   std::string cmd = "set-draw-hydrogens";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(istate);
   add_to_history_typed(cmd, args);
}

/*! \brief the state of draw hydrogens for molecule number imol.  

return -1 on bad imol.  */
int draw_hydrogens_state(int imol) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].draw_hydrogens();
   }
   return r;
}


void set_show_origin_marker(int istate) {
   graphics_info_t::show_origin_marker_flag = istate;
   graphics_draw();
   std::string cmd = "set-show-origin-marker";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
} 

int  show_origin_marker_state() {
   add_to_history_simple("show-origin-marker-state");
   return graphics_info_t::show_origin_marker_flag;
} 



 


void
handle_symmetry_colour_change(int mol, gdouble* col) {

   //
   graphics_info_t::symmetry_colour[0] = col[0];
   graphics_info_t::symmetry_colour[1] = col[1];
   graphics_info_t::symmetry_colour[2] = col[2];

   graphics_draw();

}

gdouble*
get_map_colour(int imol) {

   //
   gdouble* colour;
   colour = (gdouble *) malloc(4*sizeof(gdouble));

   if (imol < graphics_info_t::n_molecules()) { 
      if (graphics_info_t::molecules[imol].has_xmap()) {
	 colour[0] = graphics_info_t::molecules[imol].map_colour[0][0]; 
	 colour[1] = graphics_info_t::molecules[imol].map_colour[0][1]; 
	 colour[2] = graphics_info_t::molecules[imol].map_colour[0][2];
      }
   }
   std::string cmd = "get-map-colour";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return colour;
}

//! \brief return the colour of the imolth map (e.g.: (list 0.4 0.6
//0.8). If invalid imol return #f.
// 
#ifdef USE_GUILE
SCM map_colour_components(int imol) {

   SCM r = SCM_BOOL(0);
   if (is_valid_map_molecule(imol)) {
      double rc = graphics_info_t::molecules[imol].map_colour[0][0]; 
      double gc = graphics_info_t::molecules[imol].map_colour[0][1]; 
      double bc = graphics_info_t::molecules[imol].map_colour[0][2]; 
      r = SCM_CAR(scm_listofnull);
      // put red at the front of the resulting list
      r = scm_cons(scm_double2num(bc), r);
      r = scm_cons(scm_double2num(gc), r);
      r = scm_cons(scm_double2num(rc), r);
   }
   std::string cmd = "map-colour-components";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r; 
}
#endif
// BL says:: this is for python
#ifdef USE_PYTHON
PyObject *map_colour_components_py(int imol) {
   
   PyObject *r;
   r = Py_False;
   if (is_valid_map_molecule(imol)) {
      double rc = graphics_info_t::molecules[imol].map_colour[0][0];
      double gc = graphics_info_t::molecules[imol].map_colour[0][1];
      double bc = graphics_info_t::molecules[imol].map_colour[0][2];
      r = PyList_New(3);
      // put red at the front of the resulting list
      PyList_SetItem(r, 0, PyFloat_FromDouble(rc));
      PyList_SetItem(r, 1, PyFloat_FromDouble(gc));
      PyList_SetItem(r, 2, PyFloat_FromDouble(bc));
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON


/* Functions for Cancel button on map colour selection  */
void save_previous_map_colour(int imol) {

   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].save_previous_map_colour();
   } 
   std::string cmd = "save-previous-map-colour";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);

} 

void restore_previous_map_colour(int imol) {

   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].restore_previous_map_colour();
   }
   graphics_draw();
   std::string cmd = "restore-previous-map-colour";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}


// -------------------------------------------------------------------

gdouble*
get_symmetry_bonds_colour(int idummy) {

   //
   gdouble* colour;
   colour = (gdouble *) malloc(4*sizeof(gdouble));

   colour[0] = graphics_info_t::symmetry_colour[0];
   colour[1] = graphics_info_t::symmetry_colour[1];
   colour[2] = graphics_info_t::symmetry_colour[2];
   return colour;
}





// In future the gui will usefully set a mol number and we
// will use that.
//
void set_show_symmetry_master(short int state) {

   // 
   graphics_info_t g;

   // show symmetry state is no longer part of the molecule(s).
   
      
//       g.molecules[ii].show_symmetry = state;

//       if ( state == 1 ) {
// 	 g.molecules[mol_no].update_symmetry();
//       }
//    }

   g.show_symmetry = state; 
   for (int ii=0; ii<g.n_molecules(); ii++)
      if (is_valid_model_molecule(ii))
	 graphics_info_t::molecules[ii].update_symmetry();
   graphics_draw();

   if (state) { 
      // Now count the number of model molecules that have symmetry
      // available.  If there are none, then pop up a warning.

      int n_has_symm = 0;
      int n_model_molecules = 0;
      for (int ii=0; ii<g.n_molecules(); ii++)
	 if (is_valid_model_molecule(ii)) {
	    n_model_molecules++;
	    mat44 my_matt;
	    int err = graphics_info_t::molecules[ii].atom_sel.mol->GetTMatrix(my_matt, 0, 0, 0, 0);
	    if (err == SYMOP_Ok) {
	       n_has_symm++;
	       break;
	    }
	 }
      if ((n_has_symm == 0) && (n_model_molecules > 0)) {
	 std::string s = "WARNING:: there are no model molecules\n"; 
	 s += " that can display symmetry.  \n\nCRYST1 problem?";
	 if (graphics_info_t::use_graphics_interface_flag) { 
	    GtkWidget *w = g.wrapped_nothing_bad_dialog(s);
	    gtk_widget_show(w);
	 }
      }
   }
   std::string cmd = "set-show-symmetry-master";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void set_show_symmetry_molecule(int imol, short int state) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_show_symmetry(state);
      if (state)
	 graphics_info_t::molecules[imol].update_symmetry();
      graphics_draw();
   }
   std::string cmd = "set-show-symmetry-molecule";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(state);
   add_to_history_typed(cmd, args);
}


void symmetry_as_calphas(int mol_no, short int state) {

   graphics_info_t g;
   if (is_valid_model_molecule(mol_no)) { 
      g.molecules[mol_no].symmetry_as_calphas = state;
      g.molecules[mol_no].update_symmetry();
   }
   graphics_draw();
   std::string cmd = "symmetry-as-calphas";
   std::vector<coot::command_arg_t> args;
   args.push_back(mol_no);
   args.push_back(state);
   add_to_history_typed(cmd, args);

}

short int get_symmetry_as_calphas_state(int imol) {

   graphics_info_t g;
   int r = -1;
   if (is_valid_model_molecule(imol))
      r = g.molecules[imol].symmetry_as_calphas;
       
   std::string cmd = "get-symmety-as-calphas-state";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
} 


//  There is no dependence on the mol_no.  The intereface where we
//  pass (int mol_no) is kept, because molecule symmetry dependence
//  may re-surface in future.
// 
short int get_show_symmetry() {

   add_to_history_simple("get-show-symmetry");
   return graphics_info_t::show_symmetry; // master
}


   
void
set_clipping_front(float v) {

   graphics_info_t::clipping_front = v;
   if (graphics_info_t::clipping_front > 10)
      graphics_info_t::clipping_front = 10;
   graphics_draw();
   std::string cmd = "set-clipping-front";
   std::vector<coot::command_arg_t> args;
   args.push_back(v);
   add_to_history_typed(cmd, args);
   
}


void
set_clipping_back(float v) {

   graphics_info_t::clipping_back = v;
   if (graphics_info_t::clipping_back > 10)
      graphics_info_t::clipping_back = 10;
   graphics_draw();
   std::string cmd = "set-clipping-back";
   std::vector<coot::command_arg_t> args;
   args.push_back(v);
   add_to_history_typed(cmd, args);
}


/*  ----------------------------------------------------------------------- */
/*                         Colour                                           */
/*  ----------------------------------------------------------------------- */

void
set_symmetry_colour_merge(float v) {

   graphics_info_t::symmetry_colour_merge_weight = v;
   graphics_draw();

   std::string cmd = "set-symmetry-colour-merge";
   std::vector<coot::command_arg_t> args;
   args.push_back(v);
   add_to_history_typed(cmd, args);
}

/*! \brief set the symmetry colour base */
void set_symmetry_colour(float r, float g, float b) {

   graphics_info_t::symmetry_colour[0] = r;
   graphics_info_t::symmetry_colour[1] = g;
   graphics_info_t::symmetry_colour[2] = b;

   std::string cmd = "set-symmetry-colour";
   std::vector<coot::command_arg_t> args;
   args.push_back(r);
   args.push_back(g);
   args.push_back(b);
   add_to_history_typed(cmd, args);}


void set_colour_map_rotation_on_read_pdb(float f) {
   graphics_info_t::rotate_colour_map_on_read_pdb = f; 
   std::string cmd = "set-colour-map-rotation-on-read-pdb";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
}

void set_colour_map_rotation_on_read_pdb_flag(short int i) {
   graphics_info_t::rotate_colour_map_on_read_pdb_flag = i; 
   std::string cmd = "set-colour-map-rotation-on-read-pdb-flag";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
}

void set_colour_map_rotation_on_read_pdb_c_only_flag(short int i) {

   graphics_info_t::rotate_colour_map_on_read_pdb_c_only_flag = i;
   for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
      if (is_valid_model_molecule(imol)) {
	 if (graphics_info_t::molecules[imol].Bonds_box_type() == coot::COLOUR_BY_CHAIN_BONDS) {
	    graphics_info_t::molecules[imol].make_bonds_type_checked();
	 }
      }
   }
   graphics_draw();
   std::string cmd = "set-colour-map-rotation-on-read-pdb-c-only-flag";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
}

void set_symmetry_atom_labels_expanded(int state) {
   graphics_info_t::symmetry_atom_labels_expanded_flag = state;
   graphics_draw();
   std::string cmd = "set-symmetry-atom-labels-expanded";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

int get_colour_map_rotation_on_read_pdb_c_only_flag() {
  
  int ret = graphics_info_t::rotate_colour_map_on_read_pdb_c_only_flag;
  return ret;
}

/* widget work */
GtkWidget *wrapped_create_coords_colour_control_dialog() {

   GtkWidget *w = create_coords_colour_control_dialog();

   graphics_info_t g;
   g.fill_bond_colours_dialog_internal(w);
   return w;
}


float get_molecule_bonds_colour_map_rotation(int imol) {

   float r = -1.0;
   if (is_valid_model_molecule(imol))
      r = graphics_info_t::molecules[imol].bonds_colour_map_rotation;
   std::string cmd = "get-molecule-bonds-colour-map-rotation";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
}

void  set_molecule_bonds_colour_map_rotation(int imol, float f) {

   if (is_valid_model_molecule(imol))
      graphics_info_t::molecules[imol].bonds_colour_map_rotation = f;
   std::string cmd = "set-molecule-bonds-colour-map-rotation";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(f);
   add_to_history_typed(cmd, args);
}



// ----------------- Rotation Centre ----------------------

void set_rotation_centre(float x, float y, float z) {
   graphics_info_t g;
   g.setRotationCentre(coot::Cartesian(x,y,z));
   if (g.glarea)
      g.update_things_on_move_and_redraw();
   std::string cmd = "set-rotation-centre";
   std::vector<coot::command_arg_t> args;
   args.push_back(x);
   args.push_back(y);
   args.push_back(z);
   add_to_history_typed(cmd, args);
}

// The redraw happens somewhere else...
void set_rotation_centre_internal(float x, float y, float z) {
   graphics_info_t g;
   g.setRotationCentre(coot::Cartesian(x,y,z));
}

float rotation_centre_position(int axis) {  /* only return one value: x=0, y=1, z=2 */
   graphics_info_t g;
   coot::Cartesian p = g.RotationCentre();
   // std::cout << "DEBUG:: rotation centre : " << p << " axis: " << axis << "\n";
   float r = 0.0;
   if (axis == 0)
      r = p.x();
   if (axis == 1)
      r = p.y();
   if (axis == 2)
      r = p.z();
   std::string cmd = "rotation-centre-position";
   std::vector<coot::command_arg_t> args;
   args.push_back(axis);
   add_to_history_typed(cmd, args);
   return r;
}

void set_colour_by_chain(int imol) { 
   
   if (is_valid_model_molecule(imol)) {
      short int f = graphics_info_t::rotate_colour_map_on_read_pdb_c_only_flag;
      graphics_info_t::molecules[imol].make_colour_by_chain_bonds(f);
      graphics_draw();
   }
   std::string cmd = "set-colour-by-chain";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

void set_colour_by_molecule(int imol) { 

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].make_colour_by_molecule_bonds();
      graphics_draw();
   }
   std::string cmd = "set-colour-by-molecule";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}


/*  Section Map colour*/
/* default for maps is 31 degrees. */
void set_colour_map_rotation_for_map(float f) {

   graphics_info_t::rotate_colour_map_for_map = f;
   std::string cmd = "set-colour-map-rotation-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
}



/*  ----------------------------------------------------------------------- */
/*                         Unit Cell                                        */
/*  ----------------------------------------------------------------------- */
short int
get_show_unit_cell(int imol) {

   std::string cmd = "get-show-unit-cell";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return graphics_info_t::molecules[imol].show_unit_cell_flag;
}

void
set_show_unit_cell(int imol, short int state) {


   //    for (int imol=0; imol<graphics_n_molecules(); imol++) {
   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].show_unit_cell_flag = state;
   }
   //    }
   graphics_draw();
   std::string cmd = "set-show-unit-cell";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void set_show_unit_cells_all(short int istate) {

   for (int imol=0; imol<graphics_n_molecules(); imol++) {
      if (is_valid_model_molecule(imol)) { 
	 graphics_info_t::molecules[imol].show_unit_cell_flag = istate;
      }
      if (is_valid_map_molecule(imol)) { 
	 graphics_info_t::molecules[imol].show_unit_cell_flag = istate;
      }
   }
   graphics_draw();
   std::string cmd = "set-show-unit-cells-all";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
} 



// -----------------------------------------------------------------------
//                       Anisotropic Atoms
// -----------------------------------------------------------------------

void 
set_limit_aniso(short int state) {
   //
   graphics_info_t g; 
   g.show_aniso_atoms_radius_flag = state;
   std::string cmd = "set-limit-aniso";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
} 

void
set_aniso_limit_size_from_widget(const char *text) {
   
   float tmp;
   graphics_info_t g; 

   tmp = atof(text);

   if ((tmp >= 0.0) && (tmp < 99999.9)) {

      g.show_aniso_atoms_radius = tmp;
   } else {
      cout << "Cannot interpret " << text << ".  Assuming 10A" << endl;
      g.show_aniso_atoms_radius = 10.0;
   }
}

float
get_limit_aniso() {

   graphics_info_t g;
   add_to_history_simple("get-limit-aniso");
   return g.show_aniso_atoms_radius;

}

// Do if for all molecule, if we do it for one.
// The Anisotropic Atoms widget has no ability to select molecules.
// 
// This then is not a property of a molecule, but is a property of the
// graphics. 
// 
short int
get_show_aniso() {

   return graphics_info_t::show_aniso_atoms_flag;
}

void
set_show_aniso(int state) {

   graphics_info_t::show_aniso_atoms_flag = state;
   graphics_draw();
}

char *get_text_for_aniso_limit_radius_entry() {
   char *text;
   graphics_info_t g;
   
   text = (char *) malloc(100);
   snprintf(text, 99, "%-5.1f", g.show_aniso_atoms_radius);

   return text;
}

short int
get_show_limit_aniso() {
   
   return graphics_info_t::show_aniso_atoms_radius_flag;
}

void
set_aniso_probability(float f) {

   graphics_info_t::show_aniso_atoms_probability = f;
   graphics_draw();
}

// return e.g. 47.9 (%). 
float
get_aniso_probability() {

   return graphics_info_t::show_aniso_atoms_probability;
}


/*  ---------------------------------------------------------------------- */
/*                         Display Functions                               */
/*  ---------------------------------------------------------------------- */

void set_default_bond_thickness(int t) {

   graphics_info_t g;
   g.default_bond_width = t;

}

/*! \brief set the default represenation type (deafult 1).*/
void set_default_representation_type(int type) {
   graphics_info_t g;
   g.default_bonds_box_type = type;
} 


void set_bond_thickness(int imol, float t) {

   graphics_info_t g;
   g.set_bond_thickness(imol, t);

}

void set_bond_thickness_intermediate_atoms(float t) { 

   graphics_info_t g;
   g.set_bond_thickness_intermediate_atoms(t);

} 

void set_unbonded_atom_star_size(float f) {
   graphics_info_t g;
   g.unbonded_atom_star_size = f;
} 

int get_default_bond_thickness() {
   graphics_info_t g;
   int ret = g.default_bond_width;
   return ret;
}

int make_ball_and_stick(int imol,
			const char *atom_selection_str,
			float bond_thickness,
			float sphere_size,
			int do_spheres_flag) {

   int i = imol;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      gl_context_info_t glci(graphics_info_t::glarea, graphics_info_t::glarea_2);
      int dloi = 
      graphics_info_t::molecules[imol].make_ball_and_stick(std::string(atom_selection_str),
							   bond_thickness,
							   sphere_size, do_spheres_flag,
							   glci, g.Geom_p());
      // std::cout << "dloi: " << dloi << std::endl;
      graphics_draw();
   }
   return i;
}


int
clear_ball_and_stick(int imol) {

  if (graphics_info_t::use_graphics_interface_flag) { 
    if (is_valid_model_molecule(imol)) {
      GLuint dummy_tag = 0;
      graphics_info_t::molecules[imol].clear_display_list_object(dummy_tag);
      graphics_draw();
    }
  }
    return 0;
}

/* clear the given additional representation  */
void
set_show_additional_representation(int imol, int representation_number, int on_off_flag) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_show_additional_representation(representation_number,
									  on_off_flag);
   }
   graphics_draw();
}

/* \brief display/undisplay all the additional representations for the given molecule  */
void
set_show_all_additional_representations(int imol, int on_off_flag) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_show_all_additional_representations(on_off_flag);
   }
   graphics_draw();
}

/* \brief undisplay all the additional representations for the given
   molecule, except the given representation number (if it is off, leave it off)  */
void all_additional_representations_off_except(int imol, int representation_number,
					       short int ball_and_sticks_off_too_flag) {
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].all_additional_representations_off_except(representation_number, ball_and_sticks_off_too_flag);
   }  
   graphics_draw();
}




/* delete the given additional representation  */
void delete_additional_representation(int imol, int representation_number) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].clear_additional_representation(representation_number);
   }
   graphics_draw();
} 

/* return the index of the additional representation.  Return -1 on error */
int additional_representation_by_string(int imol,  const char *atom_selection_str,
					int representation_type,
					int bonds_box_type,
					float bond_width,
					int draw_hydrogens_flag) {
   int r = -1;
   if (is_valid_model_molecule(imol)) {
      coot::atom_selection_info_t info(atom_selection_str);
      graphics_info_t g;
      GtkWidget *dcw = g.display_control_window();
      gl_context_info_t glci(graphics_info_t::glarea, graphics_info_t::glarea_2);
      r = graphics_info_t::molecules[imol].add_additional_representation(representation_type,
									 bonds_box_type,
									 bond_width,
									 draw_hydrogens_flag,
									 info, dcw, glci,
									 g.Geom_p());
   }
   graphics_draw();
   return r;
} 

/* return the index of the additional representation.  Return -1 on error */
int additional_representation_by_attributes(int imol,  const char *chain_id, 
					    int resno_start, int resno_end, 
					    const char *ins_code,
					    int representation_type,
					    int bonds_box_type,
					    float bond_width,
					    int draw_hydrogens_flag) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      GtkWidget *dcw = g.display_control_window();
      coot::atom_selection_info_t info(chain_id, resno_start, resno_end, ins_code);
      gl_context_info_t glci(graphics_info_t::glarea, graphics_info_t::glarea_2);
      r = graphics_info_t::molecules[imol].add_additional_representation(representation_type,
									 bonds_box_type,
									 bond_width,
									 draw_hydrogens_flag,
									 info, dcw, glci, g.Geom_p());
   }
   graphics_draw();
   return r;
}


#ifdef USE_GUILE
SCM additional_representation_info_scm(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      r = SCM_EOL;
      for (int ir=0; ir<graphics_info_t::molecules[imol].add_reps.size(); ir++) {
	 SCM l = SCM_EOL;
	 std::string s = graphics_info_t::molecules[imol].add_reps[ir].info_string();
	 SCM is_show_flag_scm = SCM_BOOL_F;
	 if (graphics_info_t::molecules[imol].add_reps[ir].show_it)
	    is_show_flag_scm = SCM_BOOL_T;
	 float bw = graphics_info_t::molecules[imol].add_reps[ir].bond_width;
	 SCM bond_width_scm = scm_double2num(bw);
	 SCM atom_spec_scm = SCM_BOOL_F;
	 // lines too long, make a rep
	 coot::additional_representations_t rep =
	    graphics_info_t::molecules[imol].add_reps[ir];
	 int type = rep.atom_sel_info.type;
	 if (type == coot::atom_selection_info_t::BY_STRING)
	    atom_spec_scm
	       = scm_makfrom0str(rep.atom_sel_info.atom_selection_str.c_str());
	 else
	    if (type == coot::atom_selection_info_t::BY_ATTRIBUTES) { 
	       atom_spec_scm = SCM_EOL;
	       SCM ins_code_scm = scm_makfrom0str(rep.atom_sel_info.ins_code.c_str());
	       SCM resno_end_scm = SCM_MAKINUM(rep.atom_sel_info.resno_end);
	       SCM resno_start_scm   = SCM_MAKINUM(rep.atom_sel_info.resno_start);
	       SCM chain_id_scm    = scm_makfrom0str(rep.atom_sel_info.chain_id.c_str());
	       atom_spec_scm = scm_cons(ins_code_scm, atom_spec_scm);
	       atom_spec_scm = scm_cons(resno_end_scm, atom_spec_scm);
	       atom_spec_scm = scm_cons(resno_start_scm, atom_spec_scm);
	       atom_spec_scm = scm_cons(chain_id_scm, atom_spec_scm);
	    }

	 l = scm_cons(bond_width_scm, l);
	 l = scm_cons(is_show_flag_scm, l);
	 l = scm_cons(scm_makfrom0str(s.c_str()), l);
	 l = scm_cons(SCM_MAKINUM(ir), l); 
	 r = scm_cons(l, r);
      }
   } 
   return r;
} 

#endif	/* USE_GUILE */

#ifdef USE_PYTHON
PyObject *additional_representation_info_py(int imol) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      r = PyList_New(0);
      for (int ir=0; ir<graphics_info_t::molecules[imol].add_reps.size(); ir++) {
	 PyObject *l = PyList_New(4);
	 std::string s = graphics_info_t::molecules[imol].add_reps[ir].info_string();
	 PyObject *is_show_flag_py = Py_False;
	 if (graphics_info_t::molecules[imol].add_reps[ir].show_it)
	    is_show_flag_py = Py_True;
	 float bw = graphics_info_t::molecules[imol].add_reps[ir].bond_width;
	 PyObject *bond_width_py = PyFloat_FromDouble(bw);
	 PyObject *atom_spec_py = Py_False;
	 // lines too long, make a rep
	 coot::additional_representations_t rep =
	    graphics_info_t::molecules[imol].add_reps[ir];
	 int type = rep.atom_sel_info.type;
	 if (type == coot::atom_selection_info_t::BY_STRING)
	    atom_spec_py
	       = PyString_FromString(rep.atom_sel_info.atom_selection_str.c_str());
	 else
	    if (type == coot::atom_selection_info_t::BY_ATTRIBUTES) { 
	       atom_spec_py = PyList_New(4);
	       PyObject *chain_id_py    = PyString_FromString(rep.atom_sel_info.chain_id.c_str());
	       PyObject *resno_start_py = PyInt_FromLong(rep.atom_sel_info.resno_start);
	       PyObject *resno_end_py   = PyInt_FromLong(rep.atom_sel_info.resno_end);
	       PyObject *ins_code_py    = PyString_FromString(rep.atom_sel_info.ins_code.c_str());
	       PyList_SetItem(atom_spec_py, 0, chain_id_py);
	       PyList_SetItem(atom_spec_py, 1, resno_start_py);
	       PyList_SetItem(atom_spec_py, 2, resno_end_py);
	       PyList_SetItem(atom_spec_py, 3, ins_code_py);
	    }
	 // we dont use the atom_spec_py!? -> decref
	 Py_XDECREF(atom_spec_py);

     Py_XINCREF(is_show_flag_py);
	 PyList_SetItem(l, 0, PyInt_FromLong(ir));
	 PyList_SetItem(l, 1, PyString_FromString(s.c_str()));
	 PyList_SetItem(l, 2, is_show_flag_py);
	 PyList_SetItem(l, 3, bond_width_py);
	 PyList_Append(r, l);
	 Py_XDECREF(l);
      }
   } 
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 

#endif	/* USE_PYTHON */


/*  ----------------------------------------------------------------------- */
/*                  dots display                                            */
/*  ----------------------------------------------------------------------- */
int dots(int imol,
	 const char *atom_selection_str,
	 const char *dots_name,
	 float dot_density, float sphere_size_scale) {

   int idots = -1;
   if (is_valid_model_molecule(imol)) {
      if (atom_selection_str) {
	 // the colour is handled internally to make_dots - there the
	 // state of molecule dots colour (see set_dots_colour()
	 // below) is checked.
	 idots = graphics_info_t::molecules[imol].make_dots(std::string(atom_selection_str),
							    dots_name,
							    dot_density,
							    sphere_size_scale);
      }
   }
   graphics_draw();
   return idots;
}

void set_dots_colour(int imol, float r, float g, float b) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_dots_colour(r,g,b);
      graphics_draw();
   }
}

void unset_dots_colour(int imol) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].unset_dots_colour();
      graphics_draw();
   }
} 


void clear_dots(int imol, int dots_handle) {

   if (is_valid_model_molecule(imol)) {
      bool cleared_p = graphics_info_t::molecules[imol].clear_dots(dots_handle);
      if (cleared_p)
	 graphics_draw();
   }
}

/*! \brief clear the first dots object for imol with given name */
void clear_dots_by_name(int imol, const char *dots_object_name) {

   if (is_valid_model_molecule(imol)) {
      bool cleared = graphics_info_t::molecules[imol].clear_dots(dots_object_name);
      if (cleared)
	 graphics_draw();

   } 
} 


/* return the number of dots sets for molecule number imol */
int n_dots_sets(int imol) {

   int r = -1;

   if ((imol >= 0) && (imol < graphics_info_t::n_molecules())) { 
      r = graphics_info_t::molecules[imol].n_dots_sets();
   } else {
      std::cout << "WARNING:: Bad molecule number: " << imol << std::endl;
   } 
   return r;
}


std::pair<short int, float> float_from_entry(GtkWidget *entry) {

   std::pair<short int, float> p(0,0);
   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(entry));
   if (txt) {
      float f = atof(txt);
      p.second = f;
      p.first = 1;
   }
   return p;
}

std::pair<short int, int> int_from_entry(GtkWidget *entry) {

   std::pair<short int, int> p(0,0);
   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(entry));
   if (txt) {
      int i = atoi(txt);
      p.second = i;
      p.first = 1;
   }
   return p;
}




// -----------------------------------------------------------------------
//                       Smooth Scrolling
// -----------------------------------------------------------------------

void set_smooth_scroll_flag(int v) {

   graphics_info_t::smooth_scroll = v;
}

int  get_smooth_scroll() {
   
   return graphics_info_t::smooth_scroll;
}

// useful interface for gui (entry)
void set_smooth_scroll_steps_str(const char *text) {

   int v;
   v = atoi(text);
   if (v > 0 && v < 10000000) {
      set_smooth_scroll_steps(v);
   } else {
      cout << "Cannot interpret " << text << ".  Assuming 10 steps" << endl;
      set_smooth_scroll_steps(10);
   }
}

// useful interface for scripting
void set_smooth_scroll_steps(int v) {
      graphics_info_t::smooth_scroll_steps = v;
} 

   
char *get_text_for_smooth_scroll_steps() {

   char *text;

   text = (char *) malloc(100);
   snprintf(text, 99, "%-5d", graphics_info_t::smooth_scroll_steps);

   return text;
}

// useful interface for gui (entry)
void  set_smooth_scroll_limit_str(const char *text) {

   float v;

   v = atof(text);

   if (v >0 && v < 1000) { 
      graphics_info_t::smooth_scroll_limit = v;
   } else {
      cout << text << " out of range: using 10A" << endl;
      graphics_info_t::smooth_scroll_limit = 10;
   }
}

// useful for scripting
void set_smooth_scroll_limit(float lim) {
   graphics_info_t::smooth_scroll_limit = lim; 
} 

char *get_text_for_smooth_scroll_limit() {

   char *text;
   
   text = (char *) malloc(100);
   snprintf(text, 99, "%-5.1f", graphics_info_t::smooth_scroll_limit);

   return text;
}

void set_stop_scroll_diff_map(int i) { 
   graphics_info_t::stop_scroll_diff_map_flag = i;
} 

void set_stop_scroll_iso_map(int i) { 
   graphics_info_t::stop_scroll_iso_map_flag = i;
}


void set_stop_scroll_diff_map_level(float f) { 
   graphics_info_t::stop_scroll_diff_map_level = f;
} 

void set_stop_scroll_iso_map_level(float f) { 
   graphics_info_t::stop_scroll_iso_map_level = f;
}



// -----------------------------------------------------------

void set_font_size(int i) { 

   graphics_info_t g;

   g.set_font_size(i);

}

int get_font_size() {
   return graphics_info_t::atom_label_font_size;
}

/*! \brief set the colour of the atom label font - the arguments are
  in the range 0->1 */
void set_font_colour(float red, float green, float blue) {
   graphics_info_t::font_colour = coot::colour_holder(red, green, blue);
   graphics_draw();
}


/*  ---------------------------------------------------------------------- */
/*                         Rotation Centre Cube Size                       */
/*  ---------------------------------------------------------------------- */

void set_rotation_centre_size_from_widget(const gchar *text) { 
   
   float val;
   graphics_info_t g;

   val = atof(text); 
   if ((val > 1000) || (val < 0)) { 
      cout << "Invalid cube size: " << text << ". Assuming 1.0A" << endl; 
      val = 1.0; 
   } 
   g.rotation_centre_cube_size = val; 
   graphics_draw();
}

void set_rotation_centre_size(float f) {
   graphics_info_t g;
   g.rotation_centre_cube_size = f;
   graphics_draw();
} 

gchar *get_text_for_rotation_centre_cube_size() { 
   
   char *text; 
   graphics_info_t g; 

   text = (char *)  malloc (100);
   snprintf(text, 90, "%-6.3f", g.rotation_centre_cube_size); 
   return text; 
}

short int
recentre_on_read_pdb() {
   return graphics_info_t::recentre_on_read_pdb; 
}

void
set_recentre_on_read_pdb(short int i) {
   graphics_info_t::recentre_on_read_pdb = i;
}

/*  ---------------------------------------------------------------------- */
/*                         orthogonal axes                                 */
/*  ---------------------------------------------------------------------- */
void set_draw_axes(int i) {
   graphics_info_t::draw_axes_flag = i;
} 



GtkWidget *main_window() {
   return graphics_info_t::glarea; 
}; 

int graphics_n_molecules() {
   return graphics_info_t::n_molecules();
}

/* return either 1 (yes, there is at least one hydrogen) or 0 (no
   hydrogens, or no such molecule) */
int molecule_has_hydrogens_raw(int imol) {

   int r = 0;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].molecule_has_hydrogens();
   } 
   return r;
} 



/* a testing/debugging function.  Used in a test to make sure that the
   outside number of a molecule (the vector index) is the same as that
   embedded in the molecule description object.  Return -1 on
   non-valid passed imol. */
int own_molecule_number(int imol) { 

  int r = -1;
  if (is_valid_model_molecule(imol) || is_valid_map_molecule(imol)) { 
     r = graphics_info_t::molecules[imol].MoleculeNumber();
  }
  return r;
}

 
/*  ----------------------------------------------------------------------- */
/*                  utility function                                        */
/*  ----------------------------------------------------------------------- */
// return -1 if atom not found.
int atom_index(int imol, const char *chain_id, int iresno, const char *atom_id) {

   int index;
   index = atom_index_full(imol, chain_id, iresno, "", atom_id, "");

   return index;
}
// return -1 if atom not found.
int atom_index_full(int imol, const char *chain_id, int iresno, const char *inscode, const char *atom_id, const char *altconf) {

   int index = -1;
   graphics_info_t g;

   if (imol >= 0) {
      if (imol < graphics_info_t::n_molecules()) { 
	 // return g.molecules[imol].atom_index(chain_id, iresno, atom_id);
	 index = g.molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
								iresno,
								inscode,
								std::string(atom_id),
								altconf);
      }
   }

   return index;
}

// Refine zone needs to be passed atom indexes (which it then converts
// to residue numbers - sigh).  So we need a function to get an atom
// index from a given residue to use with refine_zone()
// 
int atom_index_first_atom_in_residue(int imol, const char *chain_id, 
				     int iresno, const char *ins_code) {

   int index = -1;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g; 
      index = g.molecules[imol].atom_index_first_atom_in_residue(std::string(chain_id),
								 iresno,
								 std::string(ins_code));
   }
   return index;
}

int atom_index_first_atom_in_residue_with_altconf(int imol,
						  const char *chain_id, 
						  int iresno,
						  const char *ins_code, 
						  const char *alt_conf) {
   
   int index = -1;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      index = g.molecules[imol].atom_index_first_atom_in_residue(std::string(chain_id),
								 iresno,
								 std::string(ins_code),
								 std::string(alt_conf));
   }
   return index; 
} 


float median_temperature_factor(int imol) {

   float low_cut = 2.0;
   float high_cut = 100.0;
   short int low_cut_flag = 0;
   short int high_cut_flag = 0;

   float median = -1.0;
   if (imol < graphics_info_t::n_molecules()) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 median = coot::util::median_temperature_factor(graphics_info_t::molecules[imol].atom_sel.atom_selection,
							graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
							low_cut, high_cut,
							low_cut_flag,
							high_cut_flag);
      } else {
	 std::cout << "WARNING:: molecule " << imol << " has no model\n";
      }
   } else {
      std::cout << "WARNING:: no such molecule as " << imol << "\n";
   }
   return median;
}

float average_temperature_factor(int imol) { 

   float low_cut = 2.0;
   float high_cut = 100.0;
   short int low_cut_flag = 0;
   short int high_cut_flag = 0;

   float av = -1.0;
   if (imol < graphics_info_t::n_molecules()) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 av = coot::util::average_temperature_factor(graphics_info_t::molecules[imol].atom_sel.atom_selection,
						     graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
						     low_cut, high_cut,
						     low_cut_flag,
						     high_cut_flag);
      } else {
	 std::cout << "WARNING:: molecule " << imol << " has no model\n";
      }
   } else {
      std::cout << "WARNING:: no such molecule as " << imol << "\n";
   }
   return av;
}

float standard_deviation_temperature_factor(int imol) { 

   float low_cut = 2.0;
   float high_cut = 100.0;
   short int low_cut_flag = 0;
   short int high_cut_flag = 0;

   float av = -1.0;
   if (is_valid_model_molecule(imol)) { 
      av = coot::util::standard_deviation_temperature_factor(graphics_info_t::molecules[imol].atom_sel.atom_selection,
							     graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
							     low_cut, high_cut,
							     low_cut_flag,
							     high_cut_flag);
   } else {
      std::cout << "WARNING:: molecule " << imol << " is not a valid model\n";
   }
   return av;
}

char *centre_of_mass_string(int imol) {

#ifdef USE_GUILE
   char *s = 0; // guile/SWIG sees this as #f
   if (is_valid_model_molecule(imol)) {
      realtype x, y, z;
      GetMassCenter(graphics_info_t::molecules[imol].atom_sel.atom_selection,
		    graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
		    x, y, z);
      std::string sc = "(";
      sc += graphics_info_t::float_to_string(x);
      sc += " ";
      sc += graphics_info_t::float_to_string(y);
      sc += " ";
      sc += graphics_info_t::float_to_string(z);
      sc += ")";
      s = new char [sc.length() + 1];
      strcpy(s, sc.c_str());
      return s;
   }
   return s;
#else
// BL says:: we need a python version
#ifdef USE_PYTHON
   char *s = 0; // should do for python too
   if (is_valid_model_molecule(imol)) {
      realtype x, y, z;
      GetMassCenter(graphics_info_t::molecules[imol].atom_sel.atom_selection,
		    graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
		    x, y, z);
      std::string sc = "[";
      sc += graphics_info_t::float_to_string(x);
      sc += ",";
      sc += graphics_info_t::float_to_string(y);
      sc += ",";
      sc += graphics_info_t::float_to_string(z);
      sc += "]";
      s = new char [sc.length() + 1];
      strcpy(s, sc.c_str());
      return s;
   }
   return s;
#else
   return 0;
#endif // PYTHON
#endif // GUILE
}

#ifdef USE_PYTHON
char *centre_of_mass_string_py(int imol) {

   char *s = 0; // should do for python too
   if (is_valid_model_molecule(imol)) {
      realtype x, y, z;
      GetMassCenter(graphics_info_t::molecules[imol].atom_sel.atom_selection,
		    graphics_info_t::molecules[imol].atom_sel.n_selected_atoms,
		    x, y, z);
      std::string sc = "[";
      sc += graphics_info_t::float_to_string(x);
      sc += ",";
      sc += graphics_info_t::float_to_string(y);
      sc += ",";
      sc += graphics_info_t::float_to_string(z);
      sc += "]";
      s = new char [sc.length() + 1];
      strcpy(s, sc.c_str());
      return s;
   }
   return s;
}
#endif // PYTHON


void clear_pending_picks() {
   graphics_info_t g;
   g.clear_pending_picks();
}

/* produce debugging output from problematic atom picking  */
void set_debug_atom_picking(int istate) {
   graphics_info_t::debug_atom_picking = istate;
} 


#include "gl-matrix.h"
void print_view_matrix() { 		/* print the view matrix */

   graphics_info_t g;
   GL_matrix m;
   m.from_quaternion(g.quat);
   std::cout << "View Matrix:" << std::endl;
   m.print_matrix();
}

float get_view_matrix_element(int row, int col) {

   graphics_info_t g;
   GL_matrix m;
   m.from_quaternion(g.quat);
   return m.matrix_element(row, col);
}


float get_view_quaternion_internal(int element) {

   if ((element >= 0) &&
       (element < 4)) {
      return graphics_info_t::quat[element];
   } else {
      std::cout << "Bad element for quaternion: " << element
		<< " returning dummy -9999" << std::endl;
      return -9999;
   }
}

void set_view_quaternion(float i, float j, float k, float l) {

   double mag2 = i*i + j*j + k*k + l*l;
   double mag=sqrt(mag2);

   if (fabs(mag) > 0.5) {
      graphics_info_t::quat[0] = i/mag;
      graphics_info_t::quat[1] = j/mag;
      graphics_info_t::quat[2] = k/mag;
      graphics_info_t::quat[3] = l/mag;
      graphics_draw();
   } else {
      std::cout << "Bad view quaternion" << std::endl;
   } 
}



/* Return 1 if we moved to a molecule centre, else go to origin and
   return 0. */
/* centre on last-read (and displayed) molecule with zoom 100. */
// Also, return 0 if there are no molecules to centre on.
// 
// However, if we are already *at* that molecule centre, Reset View
// moves to the centre of the next displayed molecule (with wrapping).
// 
int reset_view() {

   int istat = 0;
   graphics_info_t g;
   coot::Cartesian new_centre(0,0,0);
   int new_centred_molecule;

   std::vector<std::pair<int, coot::Cartesian> > candidate_centres;
   for (int ii=0; ii<g.n_molecules(); ii++) {
      if (is_valid_model_molecule(ii)) {
	 if (mol_is_displayed(ii)) {
	    coot::Cartesian c = g.molecules[ii].centre_of_molecule();
	    std::pair<int, coot::Cartesian> p(ii, c);
	    candidate_centres.push_back(p);
	 }
      }
   }

   if (candidate_centres.size() > 0) { 

      coot::Cartesian current_centre = g.RotationCentre();

      // Were we centred anywhere already?
      // 
      bool was_centred = 0;
      int centred_mol = -1;
      float best_fit_for_centred = 9001.1;
   
      for (unsigned int i=0; i<candidate_centres.size(); i++) {
	 float d = (candidate_centres[i].second - current_centre).length();
	 if (d < best_fit_for_centred) {
	    best_fit_for_centred = d;
	    if (d < 0.1) {
	       was_centred = 1;
	       centred_mol = candidate_centres[i].first;
	    }
	 } 
      }

      if (! was_centred) {
	 // centre on the first molecule then
	 new_centre = candidate_centres[0].second;
	 new_centred_molecule = candidate_centres[0].first;
      } else {
	 // we we centred somewhere and we need to centre on the next
	 // molecule.

	 // What is the next molecule number? (Let's wrap to beginning
	 // if we are on the last molecule).  That only makes to do if
	 // there is more than 1 molecule to centre on.

	 new_centre = candidate_centres[0].second;
	 new_centred_molecule = candidate_centres[0].first;
	 if (candidate_centres.size() > 1) {
            // 1 -> 2 or 3 -> 0 say.
	                              // overwrite centre info if there is a
				      // candidate centre with a
				      // molecule number greater than
				      // the centred_mol;
	    for (unsigned int i=0; i<candidate_centres.size(); i++) {
	       if (candidate_centres[i].first > centred_mol) {
		  new_centred_molecule = candidate_centres[i].first;
		  new_centre = candidate_centres[i].second;
		  break;
	       }
	    }
	 }
      }

      float size = g.molecules[new_centred_molecule].size_of_molecule();
      if (size<1)
	 size = 1;
      float new_zoom = 7.0*size;

      g.setRotationCentreAndZoom(new_centre, new_zoom);
      for(int ii=0; ii<graphics_info_t::n_molecules(); ii++) {
	 graphics_info_t::molecules[ii].update_map();
	 graphics_info_t::molecules[ii].update_symmetry();
      }
      graphics_draw();

   }

   add_to_history_simple("reset-view");
   return istat;
}



// ------------------------------------------------------
//                   Skeleton
// ------------------------------------------------------


void
handle_skeleton_colour_change(int mol, gdouble* map_col) {

   graphics_info_t::skeleton_colour[0] = map_col[0];
   graphics_info_t::skeleton_colour[1] = map_col[1];
   graphics_info_t::skeleton_colour[2] = map_col[2];

   graphics_draw();

}

gdouble*
get_skeleton_colour() {

   //
   gdouble* colour;
   colour = (gdouble *) malloc(4*sizeof(gdouble));

   colour[0] = graphics_info_t::skeleton_colour[0]; 
   colour[1] = graphics_info_t::skeleton_colour[1]; 
   colour[2] = graphics_info_t::skeleton_colour[2]; 

   return colour;
}

void set_skeleton_colour(int imol, float r, float g, float b) {

   graphics_info_t::skeleton_colour[0] = r;
   graphics_info_t::skeleton_colour[1] = g;
   graphics_info_t::skeleton_colour[2] = b;

   graphics_draw();
}

void
skel_greer_on() {

   int i_skel_set = 0;
   graphics_info_t g; 
   
   for (int imol=0; imol<g.n_molecules(); imol++) {

      if (g.molecules[imol].has_xmap() &&
	  ! g.molecules[imol].xmap_is_diff_map) {
	 g.molecules[imol].greer_skeleton_draw_on = 1;
	 i_skel_set = 1;
      }
      if (i_skel_set) break;
   }
   graphics_draw();
}

void
skel_greer_off() {

   for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
      
      if (graphics_info_t::molecules[imol].has_xmap() &&
	  ! graphics_info_t::molecules[imol].xmap_is_diff_map) {
	    graphics_info_t::molecules[imol].greer_skeleton_draw_on = 0;
      }
   }
}


void test_fragment() {

#ifdef HAVE_GSL
   graphics_info_t g;
   g.rotamer_graphs(0);

#endif // HAVE_GSL
}

// we redefine TRUE here somewhere...
// #include <gdk/gdkglconfig.h>
// #include <gtk/gtkgl.h>
// #include <gdk/x11/gdkglx.h>
// #include <gdk/x11/gdkglglxext.h>


int write_connectivity(const char *monomer_name, const char *filename) {

   graphics_info_t g;
   return g.Geom_p()->hydrogens_connect_file(monomer_name, filename);
} 


void screendump_image(const char *filename) {

   graphics_draw();  // does this improve the artifacts problem?
   graphics_draw();
   
   int istatus = graphics_info_t::screendump_image(filename);
   std::cout << "screendump_image status " << istatus << std::endl;
   if (istatus) {
      std::string s = "Screendump image ";
      s += filename;
      s += " written";
      graphics_info_t g;
      g.add_status_bar_text(s);
// BL says: we wanna be nice and convert ppm to bmp for windoze user!?
// but not if we have png!!!
// still use that function with png but only to open file
#ifdef WINDOWS_MINGW
#ifdef USE_PYTHON
      std::string cmd("ppm2bmp(");
      cmd += single_quote(coot::util::intelligent_debackslash(filename));
      cmd += ")";
      safe_python_command(cmd);
#endif // USE_PYTHON
#endif // MINGW
   }
}




void make_image_raster3d(const char *filename) {

   std::string r3d_name = filename;
   r3d_name += ".r3d";
   raster3d(r3d_name.c_str());
#ifdef USE_GUILE

   std::string cmd("(raytrace 'raster3d ");
   cmd += single_quote(r3d_name);
   cmd += " ";
   cmd += single_quote(filename);
   cmd += "'dummy 'dummy)";
   safe_scheme_command(cmd);
   
#else   
#ifdef USE_PYTHON
   std::string cmd("raytrace('raster3d',");
   cmd += single_quote(coot::util::intelligent_debackslash(r3d_name));
   cmd += ",";
   cmd += single_quote(coot::util::intelligent_debackslash(filename));
   cmd += ",1,1)";
   safe_python_command(cmd);
#endif // USE_PYTHON
#endif // USE_GUILE
}

#ifdef USE_PYTHON
void make_image_raster3d_py(const char *filename) {

   std::string r3d_name = filename;
   r3d_name += ".r3d";
   raster3d(r3d_name.c_str());
   std::string cmd("raytrace('raster3d',");
   cmd += single_quote(coot::util::intelligent_debackslash(r3d_name));
   cmd += ",";
   cmd += single_quote(coot::util::intelligent_debackslash(filename));
   cmd += ",1,1)";
   safe_python_command(cmd);
}
#endif // USE_PYTHON

void make_image_povray(const char *filename) {
   std::string pov_name = filename;
   pov_name += ".pov";
   povray(pov_name.c_str());
#ifdef USE_GUILE

   int x_size = graphics_info_t::glarea->allocation.width;
   int y_size = graphics_info_t::glarea->allocation.height;
   std::string cmd("(raytrace 'povray ");
   cmd += single_quote(pov_name);
   cmd += " ";
   cmd += single_quote(filename);
   cmd += " ";
   cmd += graphics_info_t::int_to_string(x_size);
   cmd += " ";
   cmd += graphics_info_t::int_to_string(y_size);
   cmd += ")";
   safe_scheme_command(cmd);

#else   
#ifdef USE_PYTHON
   int x_size = graphics_info_t::glarea->allocation.width;
   int y_size = graphics_info_t::glarea->allocation.height;
   std::string cmd("raytrace('povray',");
   cmd += single_quote(coot::util::intelligent_debackslash(pov_name));
   cmd += ",";
   cmd += single_quote(coot::util::intelligent_debackslash(filename));
   cmd += ",";
   cmd += graphics_info_t::int_to_string(x_size);
   cmd += ",";
   cmd += graphics_info_t::int_to_string(y_size);
   cmd += ")";
   safe_python_command(cmd);
#endif // USE_PYTHON
#endif // USE_GUILE
}

#ifdef USE_PYTHON
void make_image_povray_py(const char *filename) {
   std::string pov_name = filename;
   pov_name += ".pov";
   povray(pov_name.c_str());
   int x_size = graphics_info_t::glarea->allocation.width;
   int y_size = graphics_info_t::glarea->allocation.height;
   std::string cmd("raytrace('povray',");
   cmd += single_quote(coot::util::intelligent_debackslash(pov_name));
   cmd += ",";
   cmd += single_quote(coot::util::intelligent_debackslash(filename));
   cmd += ",";
   cmd += graphics_info_t::int_to_string(x_size);
   cmd += ",";
   cmd += graphics_info_t::int_to_string(y_size);
   cmd += ")";
   safe_python_command(cmd);
}
#endif // USE_PYTHON


void
autobuild_ca_off() { 

   graphics_info_t g; 
   g.autobuild_flag = 0; 

}



/*  ------------------------------------------------------------------------ */
/*                         clipping */
/*  ------------------------------------------------------------------------ */

void do_clipping1_activate(){

   GtkWidget *clipping_window;
   GtkScale *hscale;
   GtkAdjustment *adjustment;
 
   /* connect this to displaying the new clipping window */

   clipping_window = create_clipping_window();

   hscale = GTK_SCALE(lookup_widget(clipping_window, "hscale1")); 
/*    gtk_scale_set_draw_value(hscale, TRUE);  already does */

   adjustment = GTK_ADJUSTMENT 
      (gtk_adjustment_new(0.0, -10.0, 20.0, 0.05, 4.0, 10.1)); 

   gtk_range_set_adjustment(GTK_RANGE(hscale), adjustment);
   gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		       GTK_SIGNAL_FUNC (clipping_adjustment_changed), NULL);
   
   gtk_widget_show(clipping_window); 
}

void clipping_adjustment_changed (GtkAdjustment *adj, GtkWidget *window) { 

   /*    printf("Clipping adjustment: %f\n", adj->value); */

   set_clipping_front(adj->value);
   set_clipping_back (adj->value);
}
 


/*  ----------------------------------------------------------------------- */
/*                        virtual trackball                                 */
/*  ----------------------------------------------------------------------- */

void
vt_surface(int v){ 

   graphics_info_t g;
   g.set_vt_surface(v);
   std::vector<std::string> command_strings;
//    command_strings.push_back("vt-surface");
//    command_strings.push_back(graphics_info_t::int_to_string(v));
//    add_to_history(command_strings);
}

int vt_surface_status() { 

   graphics_info_t g;
   return g.vt_surface_status();
}

/*  ----------------------------------------------------------------------- */
/*                        save coordintes                                   */
/*  ----------------------------------------------------------------------- */


// return status 1 is good, 0 is fail.
int save_coordinates(int imol, const char *filename) { 

   int ierr = 0;
   if (is_valid_model_molecule(imol)) { 
      ierr = graphics_info_t::molecules[imol].save_coordinates(filename);
   }
   
   std::vector<std::string> command_strings;
   command_strings.push_back("save-coordinates");
   command_strings.push_back(coot::util::int_to_string(imol));
   command_strings.push_back(single_quote(filename));
   add_to_history(command_strings);
   return ierr;
}


void set_save_coordinates_in_original_directory(int i) {

   // not used now, I think.
   graphics_info_t::save_coordinates_in_original_dir_flag = i;

}


/* access to graphics_info_t::save_imol for use in callback.c */
int save_molecule_number_from_option_menu() {
   return graphics_info_t::save_imol;
}

/* access from callback.c, not to be used in scripting, I suggest. */
void set_save_molecule_number(int imol) {
   graphics_info_t::save_imol = imol;
}



/*  ----------------------------------------------------------------------- */
/*                        .phs file reading                                 */
/*  ----------------------------------------------------------------------- */

void
read_phs_and_coords_and_make_map(const gchar *pdb_filename){

   // This function is the .phs equivalent of c.f. make_and_draw_map,
   // map_fill_from_mtz.  We have previously stored the phs_filename
   // in the static graphics_info_t.
   // 
   graphics_info_t g; 

   int imol = graphics_info_t::create_molecule();

   // don't forget that this is a map.
   //
   int istat = g.molecules[imol].make_map_from_phs(std::string(pdb_filename),
						   g.get_phs_filename());

   if (istat != -1) { 
      graphics_draw();
   } else {
      // give us a warning message then
      g.erase_last_molecule();
      std::string w = "Sadly, the cell or space group is not comprehensible in\n";
      w += "the pdb file: ";
      w += pdb_filename;
      w += "\n";
      w += "Can't make map from phs file.";
      GtkWidget *widget = wrapped_nothing_bad_dialog(w);
      gtk_widget_show(widget);
   } 
}

/*! \brief read a phs file, the cell and symm information is from
  previously read (most recently read) coordinates file

 For use with phs data filename provided on the command line */
int 
read_phs_and_make_map_using_cell_symm_from_previous_mol(const char *phs_filename) {

   clipper::Spacegroup spacegroup; 
   clipper::Cell cell;
   int r = -1;

   int imol_ref = -1;

   for (int i=graphics_info_t::n_molecules()-1; i>=0; i--) {
      if (is_valid_model_molecule(i)) {
	 imol_ref = i;
	 break;
      }
   }

   if (imol_ref > -1) 
      r = read_phs_and_make_map_using_cell_symm_from_mol(phs_filename, imol_ref);

   return r;
}


/*! \brief read a phs file and use the cell and symm in molecule
  number imol and use the resolution limits reso_lim_low and
  reso_lim_high  */
int
read_phs_and_make_map_with_reso_limits(int imol_ref, const char* phs_filename,
				       float reso_lim_low, float reso_lim_high) {
   // This function is the .phs equivalent of c.f. make_and_draw_map,
   // map_fill_from_mtz.  We have previously stored the phs_filename
   // in the static graphics_info_t.
   // 
   graphics_info_t g;
   int imol = g.create_molecule();

   clipper::Spacegroup spacegroup; 
   clipper::Cell cell;
   short int got_cell_symm_flag = 0;
   int istat = -1; // returned value

   if (is_valid_model_molecule(imol_ref) || is_valid_map_molecule(imol_ref)) {
      if (g.molecules[imol_ref].have_unit_cell) { 
	 try {
	    std::pair<clipper::Cell,clipper::Spacegroup> xtal =
	       coot::util::get_cell_symm( g.molecules[imol_ref].atom_sel.mol );
	    cell = xtal.first;
	    spacegroup = xtal.second;
	    got_cell_symm_flag = 1;
	 } catch ( std::runtime_error except ) {
	    std::cout << "WARNING:: Cant get spacegroup from coordinates!\n";
	    // get the cell/symm from a map:
	    if (g.molecules[imol_ref].has_xmap()) {
	       cell = g.molecules[imol_ref].xmap.cell();
	       spacegroup = g.molecules[imol_ref].xmap.spacegroup(); 
	       got_cell_symm_flag = 1;
	    }
	 }
      }
   }

   if (got_cell_symm_flag) { 

      // don't forget that this is a map.
      //
      std::string phs_file(phs_filename);
      istat = g.molecules[imol].make_map_from_phs_using_reso(phs_file,
							     spacegroup,
							     cell, 
							     reso_lim_low, reso_lim_high,
							     graphics_info_t::map_sampling_rate);

      if (istat != -1) {
	 imol = istat;
	 graphics_draw();
      } else {
	 g.erase_last_molecule();
	 std::string w = "Sadly, something bad happened reading phs file using\n";
	 w += "the molecule number ";
	 w += coot::util::int_to_string(imol_ref); 
	 w += "\n";
	 w += "Can't make map from phs file.";
	 GtkWidget *widget = wrapped_nothing_bad_dialog(w);
	 gtk_widget_show(widget);
      }
   } else {
      g.erase_last_molecule();
      // give us a warning message then
      std::string w = "Sadly, the cell or space group is not comprehensible in\n";
      w += "the molecule number ";
      w += coot::util::int_to_string(imol_ref); 
      w += "\n";
      w += "Can't make map from phs file.";
      GtkWidget *widget = wrapped_nothing_bad_dialog(w);
      gtk_widget_show(widget);
   }

   return istat;
}



int
read_phs_and_make_map_using_cell_symm_from_mol(const char *phs_filename_str, int imol_ref) { 

   clipper::Spacegroup spacegroup; 
   clipper::Cell cell;
   short int got_cell_symm_flag = 0;
   int imol = -1;// set bad molecule initally
   
   graphics_info_t g; 
//       std::cout << "DEBUG:: read_phs_and_make_map_using_cell_symm_from_mol "
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().a << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().b << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().c << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().alpha << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().beta << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().gamma << "  " 
// 		<< std::endl;

   if (is_valid_model_molecule(imol_ref) || is_valid_map_molecule(imol_ref)) {
      if (g.molecules[imol_ref].have_unit_cell) { 
	 try {
	    std::pair<clipper::Cell,clipper::Spacegroup> xtal =
	       coot::util::get_cell_symm( g.molecules[imol_ref].atom_sel.mol );
	    cell = xtal.first;
	    spacegroup = xtal.second;
	    got_cell_symm_flag = 1;
	 } catch ( std::runtime_error except ) {
	    std::cout << "WARNING:: Cant get spacegroup from coordinates!\n";
	    // get the cell/symm from a map:
	    if (g.molecules[imol_ref].has_xmap()) {
	       cell = g.molecules[imol_ref].xmap.cell();
	       spacegroup = g.molecules[imol_ref].xmap.spacegroup(); 
	       got_cell_symm_flag = 1;
	    }
	 }
      }

      if (got_cell_symm_flag) {
	 std::string phs_filename(phs_filename_str); 

	 imol = g.create_molecule();
	 g.molecules[imol].make_map_from_phs(spacegroup, cell, phs_filename);
	 graphics_draw();
      }
   }

   return imol;
}


int
read_phs_and_make_map_using_cell_symm_from_mol_using_implicit_phs_filename(int imol_ref) { 

   clipper::Spacegroup spacegroup; 
   clipper::Cell cell;
   short int got_cell_symm_flag = 0;
   int imol = -1; // bad molecule
   
   graphics_info_t g; 
//       std::cout << "DEBUG:: read_phs_and_make_map_using_cell_symm_from_mol "
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().a << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().b << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().c << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().alpha << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().beta << "  " 
// 		<< g.molecules[imol_ref].atom_sel.mol->get_cell().gamma << "  " 
// 		<< std::endl;

   if (is_valid_model_molecule(imol_ref) || is_valid_map_molecule(imol_ref)) { 
      if (g.molecules[imol_ref].have_unit_cell) {
	 try {
	    std::pair<clipper::Cell,clipper::Spacegroup> xtal =
	       coot::util::get_cell_symm( g.molecules[imol_ref].atom_sel.mol );
	    cell = xtal.first;
	    spacegroup = xtal.second;
	    got_cell_symm_flag = 1;
	 } catch ( std::runtime_error except ) {
	    std::cout << "WARNING:: Cant get spacegroup from coordinates!\n";
	    // get the cell/symm from a map:
	    if (g.molecules[imol_ref].has_xmap()) {
	       cell = g.molecules[imol_ref].xmap.cell();
	       spacegroup = g.molecules[imol_ref].xmap.spacegroup(); 
	       got_cell_symm_flag = 1;
	    }
	 }
      }

      if (got_cell_symm_flag) {
	 std::string phs_filename(graphics_get_phs_filename()); 

	 imol = g.create_molecule();
	 g.molecules[imol].make_map_from_phs(spacegroup, cell, phs_filename);
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: Failed to get cell/symm - skipping.\n";
      } 
   }
   return imol;
}

int
read_phs_and_make_map_using_cell_symm(const char *phs_file_name,
				      const char *hm_spacegroup, float a, float b, float c,
				      float alpha, float beta, float gamma) { /*! in degrees */

   clipper::Spacegroup spacegroup; 
   clipper::Cell cell;
   graphics_info_t g;

   spacegroup.init(clipper::Spgr_descr(std::string(hm_spacegroup)));
   cell.init (clipper::Cell_descr(a, b, c, 
				  clipper::Util::d2rad(alpha), 
				  clipper::Util::d2rad(beta), 
				  clipper::Util::d2rad(gamma)));
   
   std::string phs_filename(phs_file_name); 

   int imol = g.create_molecule();
   g.molecules[imol].make_map_from_phs(spacegroup, cell, phs_filename);
   graphics_draw();
   return imol;
}


void
graphics_store_phs_filename(const gchar *phs_filename) {

   graphics_info_t g;
   g.set_phs_filename(std::string(phs_filename));
}


const char *
graphics_get_phs_filename() {

   graphics_info_t g;
   return g.get_phs_filename().c_str(); 
}

short int possible_cell_symm_for_phs_file() {

   if (graphics_info_t::n_molecules() == 0) { 
      return 0; 
   } else {
      return 1; 
   } 
}

// return a string to each of the cell parameters in molecule imol.
// 
gchar *get_text_for_phs_cell_chooser(int imol, char *field) { 

   // we first look in atomseletion

   graphics_info_t g;
   gchar *retval = NULL;
   retval = (gchar *) malloc(12); 
   int ihave_cell = 0; 
   realtype cell[6];
   const char *spgrp = NULL; 

   if (imol >= 0) { 
      if (imol < graphics_info_t::n_molecules()) { 
	 if (graphics_info_t::molecules[imol].has_model()) { 
	    if (g.molecules[imol].have_unit_cell) { 

	       ihave_cell = 1; 

	       realtype vol;
	       int orthcode;
	       g.molecules[imol].atom_sel.mol->GetCell(cell[0], cell[1], cell[2],
						       cell[3], cell[4], cell[5],
						       vol, orthcode);
	       spgrp   = g.molecules[imol].atom_sel.mol->GetSpaceGroup(); 

	    } else { 

	       ihave_cell = 1; 

	       clipper::Spacegroup spacegroup = g.molecules[imol].xmap.spacegroup(); 
	       clipper::Cell       ccell      = g.molecules[imol].xmap.cell();

	       cell[0] = g.molecules[imol].xmap.cell().a(); 
	       cell[1] = g.molecules[imol].xmap.cell().b(); 
	       cell[2] = g.molecules[imol].xmap.cell().c(); 
	       cell[3] = g.molecules[imol].xmap.cell().alpha() * RADTODEG; 
	       cell[4] = g.molecules[imol].xmap.cell().beta()  * RADTODEG; 
	       cell[5] = g.molecules[imol].xmap.cell().gamma() * RADTODEG; 

	       spgrp = spacegroup.descr().symbol_hm().c_str(); 
	    } 
   

	    if (spgrp) { 
	       if ( ! (strcmp(field, "symm") ) ) { 
		  snprintf(retval, 11, "%-s", spgrp);
	       }
	       if ( ! (strcmp(field, "a") ) ) { 
		  snprintf(retval, 11, "%7.3f", cell[0]);   
	       } 
	       if ( ! (strcmp(field, "b") ) ) { 
		  snprintf(retval, 11, "%7.2f",  cell[1]);  
	       } 
	       if ( ! (strcmp(field, "c") ) ) { 
		  snprintf(retval, 11, "%7.2f",  cell[2]);  
	       } 
	       if ( ! (strcmp(field, "alpha") ) ) { 
		  snprintf(retval, 11, "%6.2f",   cell[3]); 
	       } 
	       if ( ! (strcmp(field, "beta") ) ) { 
		  snprintf(retval, 11, "%6.2f",  cell[4]);  
	       } 
	       if ( ! (strcmp(field, "gamma") ) ) { 
		  snprintf(retval, 11, "%6.2f",   cell[5]); 
	       }


	       if (! ihave_cell) { 
		  strcpy(retval, "  -  "); 
	       }
	    }
	 }
      }
   }
   return retval; 
}


/*  ----------------------------------------------------------------------- */
/*                        undo last move                                    */
/*  ----------------------------------------------------------------------- */
void undo_last_move() {

   graphics_info_t g;
   g.undo_last_move(); // does a redraw
} 

int go_to_atom_molecule_number() {
   graphics_info_t g;
   return g.go_to_atom_molecule();
}

char *go_to_atom_chain_id() {
   graphics_info_t g; 
   gchar *txt = (gchar *)malloc(100);
   strcpy(txt, g.go_to_atom_chain()); 
   return txt; 
}

char *go_to_atom_atom_name() {
   graphics_info_t g;
   gchar *txt = (gchar *)malloc(10);
   snprintf(txt, 9, "%s", g.go_to_atom_atom_name()); 
   return txt; 
}

int go_to_atom_residue_number() {
   graphics_info_t g;
   return g.go_to_atom_residue();
}

char *go_to_atom_ins_code() {
   graphics_info_t g;
   gchar *txt = (gchar *)malloc(10);
   snprintf(txt, 9, "%s", g.go_to_atom_ins_code()); 
   return txt; 
}

char *go_to_atom_alt_conf() {
   graphics_info_t g;
   gchar *txt = (gchar *)malloc(10);
   snprintf(txt, 9, "%s", g.go_to_atom_alt_conf()); 
   return txt; 
}


// Note that t3 is an atom name with (possibly) an altLoc tag (after the comma).
// 
int set_go_to_atom_chain_residue_atom_name(const char *t1, int iresno, const char *t3) {

   graphics_info_t g; 
   int success = set_go_to_atom_chain_residue_atom_name_no_redraw(t1, iresno, t3, 1);
   if (success) { 
      CAtom *at = 0; // passed but not used, it seems.
      GtkWidget *window = graphics_info_t::go_to_atom_window;
      if (window)
	 g.update_widget_go_to_atom_values(window, at);
   }
   graphics_draw();
   return success;
}

int set_go_to_atom_chain_residue_atom_name_full(const char *chain_id, 
						int resno,
						const char *ins_code,
						const char *atom_name,
						const char *alt_conf) {
   
   graphics_info_t g; 
   g.set_go_to_atom_chain_residue_atom_name(chain_id, resno, ins_code, atom_name, alt_conf);
   int success = g.try_centre_from_new_go_to_atom();
   if (success) { 
      CAtom *at = 0; // passed but not used, it seems.
      GtkWidget *window = graphics_info_t::go_to_atom_window;
      if (window)
	 g.update_widget_go_to_atom_values(window, at);
   }
   graphics_draw();
   return success;
}




// Note that t3 is an atom name with (possibly) an altLoc tag (after the comma).
// 
int set_go_to_atom_chain_residue_atom_name_no_redraw(const char *t1, int iresno, const char *t3,
						     short int make_the_move_flag) {


   graphics_info_t g; 
   // so we need to split t3 if it has a comma
   // 
   std::string t3s(t3);
   std::string::size_type icomma = t3s.find_last_of(",");
   if (icomma == string::npos) {

      // there was no comma, conventional usage:
      g.set_go_to_atom_chain_residue_atom_name(t1, iresno, t3); 

   } else { 

      std::string atname = t3s.substr(0,icomma);
      std::string altloc = t3s.substr(icomma+1, t3s.length());
      g.set_go_to_atom_chain_residue_atom_name(t1, iresno,
					       atname.c_str(),
					       altloc.c_str());

   }
   CAtom *at = 0; // passed but not used, it seems.
   GtkWidget *window = graphics_info_t::go_to_atom_window;
   if (window)
      g.update_widget_go_to_atom_values(window, at);
   
   int success = 0;
   if (make_the_move_flag) { 
      success = g.try_centre_from_new_go_to_atom(); 
   } else {
      success = 1;
   } 
   g.update_things_on_move();
   return success; 
}




int set_go_to_atom_chain_residue_atom_name_strings(const char *t1, const char *t2, const char *t3)
{
   int it2 = atoi(t2); 
   return set_go_to_atom_chain_residue_atom_name(t1, it2, t3); 
}

// FIXME to use altconf.
// 
// int set_go_to_atom_from_spec(const coot::atom_spec_t &atom_spec) { 
   
//    return set_go_to_atom_chain_residue_atom_name(atom_spec.chain, 
// 						 atom_spec.resno,
// 						 atom_spec.atom_name);

// }


int 
goto_next_atom_maybe_new(GtkWidget *goto_atom_window) { 

//    int it2 = atoi(t2); 
//    return goto_near_atom_maybe(t1, it2, t3, res_entry, +1);

   graphics_info_t g;
   return g.intelligent_next_atom_centring(goto_atom_window);
   
} 


int 
goto_previous_atom_maybe_new(GtkWidget *goto_atom_window) { 

//    int it2 = atoi(t2); 
//    return goto_near_atom_maybe(t1, it2, t3, res_entry, +1);

   graphics_info_t g;
   return g.intelligent_previous_atom_centring(goto_atom_window);
   
} 


// DELETE ME.
// int 
// goto_prev_atom_maybe(const gchar *t1, const gchar *t2, const gchar *t3,
// 		     GtkEntry *res_entry) { 

//    int it2 = atoi(t2); 
//    return goto_near_atom_maybe(t1, it2, t3, res_entry, -1); 
   
// } 

// 
// int 
// goto_near_atom_maybe(const char *t1, int ires, const char *t3,
// 		     GtkEntry *res_entry, int idiff) { 

//    graphics_info_t g; 

//    int ires_l = ires + idiff ; // for next residue, or previous.
   
//    g.set_go_to_atom_chain_residue_atom_name(t1, ires_l, t3); 
   
//    int success = g.try_centre_from_new_go_to_atom(); 

//    if (success) {
//       char *txt = (char *)malloc(6);
//       snprintf(txt, 5, "%d", ires_l); 
//       gtk_entry_set_text(GTK_ENTRY(res_entry), txt);
//       update_things_on_move_and_redraw(); 
//    }
//    return success; 
// }


/* For dynarama callback sake. The widget/class knows which coot
   molecule that it was generated from, so in order to go to the
   molecule from dynarama, we first need to the the molecule - because
   set_go_to_atom_chain_residue_atom_name() does not mention the
   molecule (see "Next/Previous Residue" for reasons for that).  This
   function simply calls the graphics_info_t function of the same
   name. */
void set_go_to_atom_molecule(int imol) {

   graphics_info_t g;
   int current_go_to_atom_molecule = g.go_to_atom_molecule();
   g.set_go_to_atom_molecule(imol);
   if (current_go_to_atom_molecule != imol)
      update_go_to_atom_window_on_other_molecule_chosen(imol);
   std::vector<std::string> command_strings;
   command_strings.push_back("set-go-to-atom-molecule");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);

}



/*  ----------------------------------------------------------------------- */
/*                  bond representation                                     */
/*  ----------------------------------------------------------------------- */

void graphics_to_ca_representation(int imol) {

   graphics_info_t g;
   if (is_valid_model_molecule(imol))
      g.molecules[imol].ca_representation();
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_ca_representation" << std::endl;
   graphics_draw();
   
   std::vector<std::string> command_strings;
   command_strings.push_back("graphics-to-ca-representation");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
} 

void graphics_to_ca_plus_ligands_representation   (int imol) { 
   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      g.molecules[imol].ca_plus_ligands_representation();
      graphics_draw();
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("graphics-to-ca-plus-ligands-representation");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
}


void graphics_to_bonds_representation(int imol) {
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) { 
      g.molecules[imol].bond_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-bonds-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_bonds_representation" << std::endl;
   graphics_draw();

}

void graphics_to_bonds_no_waters_representation(int imol) { 
   graphics_info_t g;
   if (is_valid_model_molecule(imol)){ 
      g.molecules[imol].bonds_no_waters_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-bonds-no-waters-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_bonds_no_waters_representation"
		<< std::endl;
   graphics_draw();

} 

void graphics_to_sec_struct_bonds_representation(int imol) { 
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) { 
      g.molecules[imol].bonds_sec_struct_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-sec-struct-bonds-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_sec_struct_bonds_representation"
		<< std::endl;
   graphics_draw();
} 

void graphics_to_ca_plus_ligands_sec_struct_representation(int imol) { 
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) { 
      g.molecules[imol].ca_plus_ligands_sec_struct_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-ca-plus-ligands-sec-struct-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_ca_plus_ligands_sec_struct_representation"
		<< std::endl;
   graphics_draw();
}

void graphics_to_rainbow_representation(int imol) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].ca_plus_ligands_rainbow_representation();
      std::vector<std::string> command_strings;
      // BL says:: or maybe we want to keep the following name and change above
      //command_strings.push_back("graphics-to-ca-plus-ligands-rainbow-representation");
      command_strings.push_back("graphics-to-rainbow-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		//BL says:: as above
		//<< " in graphics_to_ca_plus_ligands_rainbow_representation"
		<< " in graphics_to_rainbow_representation"
		<< std::endl;
   graphics_draw();
}

void graphics_to_b_factor_representation(int imol) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].b_factor_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-b-factor-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_b_factor_representation"
		<< std::endl;
   graphics_draw();
}

void graphics_to_b_factor_cas_representation(int imol) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].b_factor_representation_as_cas();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-b-factor-cas-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_b_factor_representation"
		<< std::endl;
   graphics_draw();
}

void graphics_to_occupancy_representation(int imol) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t::molecules[imol].occupancy_representation();
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-to-occupancy-representation");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
   }
   else
      std::cout << "WARNING:: no such valid molecule " << imol
		<< " in graphics_to_occupancy_representation"
		<< std::endl;
   graphics_draw();
}



int
graphics_molecule_bond_type(int imol) { 

   graphics_info_t g;
   // std::cout << "graphics_molecule_bond_type for mol: " << imol << std::endl;
   if (is_valid_model_molecule(imol)) { 
      std::vector<std::string> command_strings;
      command_strings.push_back("graphics-molecule-bond-type");
      command_strings.push_back(graphics_info_t::int_to_string(imol));
      add_to_history(command_strings);
      return g.molecules[imol].Bonds_box_type();
   }
   return -1;
}

void change_model_molecule_representation_mode(int up_or_down) {
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();
   if (pp.first) {
      int imol = pp.second.first;
      int bond_type = graphics_molecule_bond_type(imol);
      
      if (bond_type == 1) { /* atom type bonds */
	 if (up_or_down == 1) 
	    graphics_to_ca_representation(imol);
	 else
	    graphics_to_occupancy_representation(imol);
      }
      if (bond_type == 2) { /* CA bonds */
	 if (up_or_down == 1)
	    set_colour_by_chain(imol);
	 else
	    graphics_to_bonds_representation(imol);
      }
      if (bond_type == 3) { /* segid-coloured bonds */
	 if (up_or_down == 1) 
	    graphics_to_ca_plus_ligands_representation(imol);
	 else
	    graphics_to_ca_representation(imol);
      }
      if (bond_type == 4) { /* CA_BONDS_PLUS_LIGANDS */
	 if (up_or_down == 1)
	    graphics_to_bonds_no_waters_representation(imol);
	 else
	    set_colour_by_chain(imol);
      }
      if (bond_type == 5) { /* BONDS_NO_WATERS */
	 if (up_or_down == 1) 
	    graphics_to_sec_struct_bonds_representation(imol);
	 else
	    graphics_to_ca_plus_ligands_representation(imol);
      }
      if (bond_type == 6) { /* BONDS_SEC_STRUCT_COLOUR */
	 if (up_or_down == 1)
	    graphics_to_ca_plus_ligands_sec_struct_representation(imol);
	 else
	    graphics_to_bonds_no_waters_representation(imol);
      }
      if (bond_type == 7) { /* CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR */
	 if (up_or_down == 1)
	    set_colour_by_molecule(imol);
	 else
	    graphics_to_sec_struct_bonds_representation(imol);
      }
      if (bond_type == 8) { /* COLOUR_BY_MOLECULE_BONDS */
	 if (up_or_down == 1) 
	    graphics_to_rainbow_representation(imol);
	 else
	    graphics_to_ca_plus_ligands_sec_struct_representation(imol);
      }
      if (bond_type == 9) { /* COLOUR_BY_RAINBOW_BONDS */
	 if (up_or_down == 1)
	    graphics_to_b_factor_representation(imol);
	 else
	    set_colour_by_molecule(imol);
      }
      if (bond_type == 10) { /* COLOUR_BY_B_FACTOR_BONDS */
	 if (up_or_down == 1)
	    graphics_to_occupancy_representation(imol);
	 else
	    graphics_to_rainbow_representation(imol);
      }
      if (bond_type == 11) { /* COLOUR_BY_OCCUPANCY_BONDS */
	 if (up_or_down == 1)
	    graphics_to_bonds_representation(imol);
	 else
	    graphics_to_b_factor_representation(imol);
      }
   } 
} 


int
set_b_factor_bonds_scale_factor(int imol, float f) {

   int r = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_b_factor_bonds_scale_factor(f);
      r = 1;
   }
   graphics_draw();
   return r;
}



// -------------------------------------------------------------------------
//                        skeletonization level
// -------------------------------------------------------------------------
//

gchar *get_text_for_skeletonization_level_entry() { 

   graphics_info_t g;
   gchar *txt = (gchar *)malloc(10); 
   snprintf(txt, 9, "%f", g.skeleton_level); 

   return txt; 
} 

void set_skeletonization_level_from_widget(const char *txt) { 

   float tmp; 
   graphics_info_t g;

   tmp = atof(txt); 

   if (tmp > 0.0 &&  tmp < 9999.9) { 
      g.skeleton_level = tmp; 
   } else { 
      
      cout << "Cannot interpret " << txt << " using 0.2 instead" << endl; 
      g.skeleton_level = 0.2; 
   } 

         
   for (int imol=0; imol<g.n_molecules(); imol++) {
      if (g.molecules[imol].has_xmap() &&
	  g.molecules[imol].xmap_is_diff_map != 1) {
	 // 
	 g.molecules[imol].update_clipper_skeleton();
      } 
   }
   graphics_draw();
}


gchar *get_text_for_skeleton_box_size_entry() { 

   graphics_info_t g;
   gchar *txt = (gchar *)malloc(10); 

   snprintf(txt, 9, "%f", g.skeleton_box_radius); 
   return txt;
} 

void set_skeleton_box_size_from_widget(const char *txt) { 
   float tmp; 
   graphics_info_t g;

   tmp = atof(txt); 

   if (tmp > 0.0 &&  tmp < 9999.9) { 
      g.skeleton_box_radius = tmp; 
   } else { 
      
      cout << "Cannot interpret " << txt << " using 0.2 instead" << endl; 
      g.skeleton_box_radius = 0.2; 
   }

   set_skeleton_box_size(g.skeleton_box_radius);
}

void set_skeleton_box_size(float f) {

   graphics_info_t g;
   g.skeleton_box_radius = f;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-skeleton-box-size");
   command_strings.push_back(graphics_info_t::float_to_string(f));
   add_to_history(command_strings);
      
   for (int imol=0; imol<g.n_molecules(); imol++) {
      if (g.molecules[imol].has_xmap() &&
	  g.molecules[imol].xmap_is_diff_map != 1) {
	 // 
	 g.molecules[imol].update_clipper_skeleton();
      }
   }
   graphics_draw();
} 

/*  ----------------------------------------------------------------------- */
/*                  Utility Functions                                       */
/*  ----------------------------------------------------------------------- */
// These functions are for storing the molecule number and (some other
// number) as an int and used with GPOINTER_TO_INT and GINT_TO_POINTER.
int encode_ints(int i1, int i2) {
   int i = 1000 * i1 + i2;
   return i;
}

std::pair<int, int> decode_ints(int i) {
   int j = i/1000;
   int k = i - j * 1000;
   return std::pair<int, int>(j,k);
}

void store_keyed_user_name(std::string key, std::string user_name, std::string passwd) {

   std::pair<std::string, std::string> p(user_name, passwd);
   graphics_info_t::user_name_passwd_map[key] = p;

}

											 


/*  ----------------------------------------------------------------------- */
/*                  map and molecule control                                */
/*  ----------------------------------------------------------------------- */



void save_display_control_widget_in_graphics(GtkWidget *widget) { 

   graphics_info_t g; 
   g.save_display_control_widget_in_graphics(widget);
}

void 
post_display_control_window() { 

   GtkWidget *widget = wrapped_create_display_control_window();
   gtk_widget_show(widget);
   std::vector<std::string> command_strings;
   command_strings.push_back("post-display-control-window");
   add_to_history(command_strings);
}


 
void add_map_display_control_widgets() { 

   graphics_info_t g; 

   for (int ii=0; ii<g.n_molecules(); ii++)
      if (g.molecules[ii].has_xmap() || g.molecules[ii].has_nxmap())
	 g.molecules[ii].update_map_in_display_control_widget(); 

}


void add_mol_display_control_widgets() { 

   graphics_info_t g; 
   
   for (int ii=0; ii<g.n_molecules(); ii++) {
      if (g.molecules[ii].has_model()) { 
	 g.molecules[ii].new_coords_mol_in_display_control_widget(); 
      } 
   } 
} 


void add_map_and_mol_display_control_widgets() { 

   add_mol_display_control_widgets(); 
   add_map_display_control_widgets(); 
}


// resets to NULL the scroll group too.
void reset_graphics_display_control_window() { 

   graphics_info_t g; 
   g.save_display_control_widget_in_graphics(NULL);
}

void close_graphics_display_control_window() {
   graphics_info_t g; 
   GtkWidget *w = g.display_control_window();
   if (w) {
      gtk_widget_destroy(w);
      reset_graphics_display_control_window();
   }
} 

/*! \brief make the map displayed/undisplayed, 0 for off, 1 for on */
void set_map_displayed(int imol, int state) {

   graphics_info_t g;
   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].set_map_is_displayed(state);
      if (g.display_control_window())
	 set_display_control_button_state(imol, "Displayed", state);
      graphics_draw();
   }
}

void set_draw_map_standard_lines(int imol, short int state) {

   graphics_info_t g;
   if (is_valid_map_molecule(imol)) {
      g.molecules[imol].set_map_is_displayed_as_standard_lines(state);
      graphics_draw();
   }
}


#ifdef USE_GUILE
void display_maps_scm(SCM maps_list_scm) {

   graphics_info_t g;
   int n_mol = graphics_n_molecules();
   std::vector<bool> map_on(n_mol, false);
   SCM s_test = scm_list_p(maps_list_scm);
   if (scm_is_true(s_test)) {
      SCM n_scm = scm_length(maps_list_scm);
      int n = scm_to_int(n_scm);
      for (unsigned int i=0; i<n; i++) {
	 SCM item_scm = scm_list_ref(maps_list_scm, SCM_MAKINUM(i));
	 if (scm_is_true(scm_integer_p(item_scm))) {
	    int imol = scm_to_int(item_scm);
	    if (is_valid_map_molecule(imol)) {
	       map_on[imol] = true;
	    }
	 }
      }
   }

   for (unsigned int imol=0; imol<n_mol; imol++) { 
      if (is_valid_map_molecule(imol)) {
	 if (map_on[imol])
	    g.molecules[imol].set_map_is_displayed(1);
	 else
	    g.molecules[imol].set_map_is_displayed(0);
      } 
   }
   graphics_draw();
} 
#endif 


#ifdef USE_PYTHON
void display_maps_py(PyObject *pyo) {

   graphics_info_t g;
   int n_mol = graphics_n_molecules();
   std::vector<bool> map_on(n_mol, false);
   if (PyList_Check(pyo)) {
      int n = PyObject_Length(pyo);
      for (unsigned int i=0; i<n; i++) {
	 PyObject *item_py = PyList_GetItem(pyo, i);
	 if (PyInt_Check(item_py)) {
	    int imol = PyInt_AsLong(item_py);
	    if (is_valid_map_molecule(imol)) {
	       map_on[imol] = true;
	    }
	 } 
      }
   }
   for (unsigned int imol=0; imol<n_mol; imol++) { 
      if (is_valid_map_molecule(imol)) {
	 if (map_on[imol])
	    g.molecules[imol].set_map_is_displayed(1);
	 else
	    g.molecules[imol].set_map_is_displayed(0);
      } 
   }
   graphics_draw();
} 
#endif 



// button_type is "Displayed" or "Active"
void
set_display_control_button_state(int imol, const std::string &button_type, int state) {

   graphics_info_t g;
   if (g.display_control_window()) {
      
      std::string button_name = "";
      
      if (button_type == "Displayed") {
	 button_name = "displayed_button_";
	 button_name += coot::util::int_to_string(imol);
      }
      
      if (button_type == "Active") {
	 button_name = "active_button_";
	 button_name += coot::util::int_to_string(imol);
      }
      GtkWidget *button = lookup_widget(g.display_control_window(), button_name.c_str());
      if (button) {
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), state);
      } else {
	 std::cout << "Opps failed to find " << button_type << " button for mol "
		   << imol << std::endl;
      }
   }
}

/*! \brief make the coordinates molecule displayed/undisplayed, 0 for off, 1 for on */
void set_mol_displayed(int imol, int state) {
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_mol_is_displayed(state);
      if (g.display_control_window())
	 set_display_control_button_state(imol, "Displayed", state);
      graphics_draw();
   } else {
      std::cout << "not valid molecule" << std::endl;
   } 
} 
/*! \brief make the coordinates molecule active/inactve (clickable), 0
  for off, 1 for on */
void set_mol_active(int imol, int state) {
   
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_mol_is_active(state);
      if (g.display_control_window())
	 set_display_control_button_state(imol, "Active", state);
      graphics_draw();
   } else {
      std::cout << "not valid molecule" << std::endl;
   } 
} 



int mol_is_displayed(int imol) { 

   graphics_info_t g; 
   return g.molecules[imol].draw_it; 
} 

int mol_is_active(int imol) { 

   graphics_info_t g; 

   if (is_valid_model_molecule(imol)) { 
      return g.molecules[imol].atom_selection_is_pickable();
   }
   return 0;
} 

int map_is_displayed(int imol) { 

   if (is_valid_map_molecule(imol)) { 
      graphics_info_t g;
      return g.molecules[imol].is_displayed_p();
   }
   return 0; // -1 maybe? No, I think not.
}

/*! \brief if on_or_off is 0 turn off all maps displayed, for other
  values of on_or_off turn on all maps */
void set_all_maps_displayed(int on_or_off) {

   graphics_info_t g;
   int nm = graphics_info_t::n_molecules();
   for (int imol=0; imol<nm; imol++) {
      if (is_valid_map_molecule(imol)) {
	 graphics_info_t::molecules[imol].set_mol_is_active(on_or_off);
	 if (g.display_control_window())
	    set_display_control_button_state(imol, "Displayed", on_or_off);
      }
   }
   graphics_draw();
} 

/*! \brief if on_or_off is 0 turn off all models displayed and active,
  for other values of on_or_off turn on all models. */
void set_all_models_displayed_and_active(int on_or_off) {

   graphics_info_t g;
   int nm = graphics_info_t::n_molecules();
   for (int imol=0; imol<nm; imol++) {
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t::molecules[imol].set_mol_is_active(on_or_off);
	 if (g.display_control_window())
	    set_display_control_button_state(imol, "Active", on_or_off);
	 if (g.display_control_window())
	    set_display_control_button_state(imol, "Displayed", on_or_off);
      }
   }
   graphics_draw();
}

// Bleugh.
char *
show_spacegroup(int imol) { 

   if (is_valid_model_molecule(imol) || is_valid_map_molecule(imol)) {
      std::string spg = graphics_info_t::molecules[imol].show_spacegroup();
      std::cout << "INFO:: spacegroup: " << spg << std::endl;
      unsigned int l = spg.length();
      char *s = new char[l+1];
      strncpy(s, spg.c_str(), l+1);
      return s;
   } else { 

      // If it was a bad molecule, return pointer to null.
      std::cout << "Unknown molecule " << imol << std::endl;
      char *s = new char[1];
      s[0] = 0;
      return s;
   }
}

#ifdef USE_GUILE
/*! \brief return the spacegroup as a string, return scheme false if unable to do so. */
SCM space_group_scm(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_map_molecule(imol) || is_valid_model_molecule(imol)) {
      std::string s =  graphics_info_t::molecules[imol].show_spacegroup();
      r = scm_from_locale_string(s.c_str());
   }
   return r;

}
#endif

#ifdef USE_PYTHON
PyObject *space_group_py(int imol) {

   PyObject *r = Py_False;
   if (is_valid_map_molecule(imol) || is_valid_model_molecule(imol)) {
      std::string s =  graphics_info_t::molecules[imol].show_spacegroup();
      r = PyString_FromString(s.c_str());
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif





#ifdef USE_GUILE
/*! \brief return a list of symmetry operators as strings - or scheme false if
  that is not possible. */
SCM symmetry_operators_scm(int imol) {

   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol) || is_valid_map_molecule(imol)) { 
      std::pair<bool, clipper::Spacegroup> sg =
	 graphics_info_t::molecules[imol].space_group();
      if (! sg.second.is_null()) {
	 r = SCM_EOL;
	 std::vector<std::string> sv =
	    graphics_info_t::molecules[imol].get_symop_strings();
	 for (unsigned int i=0; i<sv.size(); i++) {
	    SCM s = scm_from_locale_string(sv[i].c_str());
	    r = scm_cons(s, r);
	 }
	 r = scm_reverse(r);
      } else {
	 std::cout << "WARNING:: in symmetry_operators_scm() null space group " << std::endl;
      } 
   }
   return r;
} 
#endif


#ifdef USE_PYTHON
/*! \brief return a list of symmetry operators as strings - or python false if
  that is not possible. */
PyObject *symmetry_operators_py(int imol) {

   PyObject *o = Py_False;
   if (is_valid_model_molecule(imol) || is_valid_map_molecule(imol)) { 
      std::pair<bool, clipper::Spacegroup> sg =
	 graphics_info_t::molecules[imol].space_group();
      if (! sg.second.is_null()) {
	 std::vector<std::string> sv =
	    graphics_info_t::molecules[imol].get_symop_strings();
	 o = PyList_New(sv.size());
	 for (unsigned int i=0; i<sv.size(); i++) {
	    PyList_SetItem(o, i, PyString_FromString(sv[i].c_str()));
	 }
      } else {
	 std::cout << "WARNING:: in symmetry_operators_py() null space group " << std::endl;
      }
   }
   if PyBool_Check(o) {
     Py_INCREF(o);
   }
   return o;
} 
#endif 


#ifdef USE_GUILE
SCM
symmetry_operators_to_xHM_scm(SCM symmetry_operators) {
   SCM r = SCM_BOOL_F;
   clipper::Spacegroup sg = scm_symop_strings_to_space_group(symmetry_operators);
   if (! sg.is_null())
      r = scm_from_locale_string(sg.symbol_hm().c_str()); 
   return r;
}
#endif 

#ifdef USE_PYTHON
PyObject *
symmetry_operators_to_xHM_py(PyObject *symmetry_operators) {
   PyObject *o = Py_False;
   clipper::Spacegroup sg = py_symop_strings_to_space_group(symmetry_operators);
   if (! sg.is_null())
      o = PyString_FromString(sg.symbol_hm().c_str()); 
   if PyBool_Check(o) {
     Py_INCREF(o);
   }
   return o;
}
#endif 



/*  ----------------------------------------------------------------------- */
/*                  zoom                                                    */
/*  ----------------------------------------------------------------------- */

void 
scale_zoom_internal(float f) {

   graphics_info_t g;
   g.zoom *= fabs(f);
}

void scale_zoom(float f) {

   scale_zoom_internal(f);
   graphics_draw();

}

float zoom_factor() {
   graphics_info_t g;
   return g.zoom;
}

void set_smooth_scroll_do_zoom(int i) {
   graphics_info_t g;
   g.smooth_scroll_do_zoom = i; 
}


int  smooth_scroll_do_zoom() {

   return graphics_info_t::smooth_scroll_do_zoom;
} 


float smooth_scroll_zoom_limit() {

   return graphics_info_t::smooth_scroll_zoom_limit; 
}


void set_smooth_scroll_zoom_limit(float f) {

   graphics_info_t::smooth_scroll_zoom_limit = f; 
}

void set_zoom_adjustment(GtkWidget *w) { 
   graphics_info_t::set_zoom_adjustment(w);
} 

// BL says:: I like a function to change the zoom externally
// havent found one anywhere
void set_zoom(float f) {
   graphics_info_t::zoom = f;
}





void clear_moving_atoms_object() {  /* redraw done here. */

   graphics_info_t g;
   g.clear_moving_atoms_object();
   
}

void clear_up_moving_atoms() {

   graphics_info_t g;
   // std::cout << "c-interface clear_up_moving_atoms..." << std::endl;
   g.clear_up_moving_atoms();
   g.clear_moving_atoms_object();

} 




// either alpha helix, beta strand or ramachandran goodness
// (see ideal/simple_restraint.hh link torsions)
void set_refine_ramachandran_angles(int state) {
   graphics_info_t::do_rama_restraints = state;
} 

int refine_ramachandran_angles_state() {
   return graphics_info_t::do_rama_restraints;
}

void chiral_volume_molecule_option_menu_item_select(GtkWidget *item, GtkPositionType pos) { 

   graphics_info_t::chiral_volume_molecule_option_menu_item_select_molecule = pos;

}


void set_dragged_refinement_steps_per_frame(int v) {

   graphics_info_t g;
   g.dragged_refinement_steps_per_frame = v;
}

int dragged_refinement_steps_per_frame() {
   return graphics_info_t::dragged_refinement_steps_per_frame; 
} 




#ifdef USE_GUILE
SCM safe_scheme_command(const std::string &scheme_command) {
   std::vector<std::string> cs;
   cs.push_back(DIRECT_SCM_STRING);
   cs.push_back(scheme_command);
   add_to_history(cs);
   return graphics_info_t::safe_scheme_command(scheme_command);
}
#endif // USE_GUILE


#ifdef USE_GUILE
SCM safe_scheme_command_test(const char *cmd) {

   std::string s = cmd;
   return safe_scheme_command(s);
}
#else  // not guile
// dummy function
void safe_scheme_command(const std::string &scheme_command) { /* do nothing */
   // here only for compilation purposes.
}
#endif // USE_GUILE


void safe_python_command(const std::string &python_cmd) {

#ifdef USE_PYTHON
   PyRun_SimpleString((char *)python_cmd.c_str());
#endif   
}


#ifdef USE_PYTHON
// We need a function to clean up the returned types from safe_python_command_with_return
// especially lists and floats. Who knows why? Maybe a Python bug!
// 'local function' currently
PyObject *py_clean_internal(PyObject *o) {

   PyObject *ret = NULL;
   if (PyList_Check(o)) {
     int l = PyObject_Length(o);
      ret = PyList_New(0);
      for (int item=0; item<l; item++) {
	 PyObject *py_item = PyList_GetItem(o, item);
	 py_item = py_clean_internal(py_item);
	 if (py_item == NULL) {
	   PyErr_Print();
	 }
	 PyList_Append(ret, py_item);
	 // Py_XDECREF(py_item); no!
      }
   } else {
      if (PyBool_Check(o)) {
	 // apparently doesnt seem to need resetting
	 int i = PyInt_AsLong(o);
	 ret = o;
      } else {
	 if (PyInt_Check(o)) {
	    // apparently doesnt seem to need resetting
	    int i=PyInt_AsLong(o);
	    ret = o;
	 } else {
	    if (PyFloat_Check(o)) {
	       // float is a copy 
	       double f = PyFloat_AsDouble(o);
	       ret = PyFloat_FromDouble(f);
	    } else {
	       if (PyString_Check(o)) {
		  ret = o;
	       } else {
		  if (PyFunction_Check(o)) {
		     ret = PyObject_Str(o);
		  } else { 
		     if (o == Py_None) {
			//std::cout << "BL DEBUG:: have PyNone, not sure what to do with it!?" <<std::endl;
			ret = o;
		     } else {
			std::cout <<"WARNING:: py_clean_internal: incomprehensible argument passed  "
				  << PyString_AsString(PyObject_Str(o)) <<std::endl;
		     }
		  }
	       }
	    }
	 }
      }
   }
   return ret;
}
#endif  // USE_PYTHON

int pyrun_simple_string(const char *python_command) { 
  
#ifdef USE_PYTHON
  return PyRun_SimpleString(python_command);
#endif 
  return -1;
} 

#ifdef USE_PYTHON
// BL says:: let's have a python command with can receive return values
// we need to pass the script file containing the funcn and the funcn itself
// returns a PyObject which can then be used further
// returns NULL for failed run
PyObject *safe_python_command_with_return(const std::string &python_cmd) {

   PyObject *ret = NULL;

   if (python_cmd != "") {

      PyObject *pName, *pModule;
      PyObject *globals;
      PyObject *pValue = NULL;
      // include $COOT_PYTHON_DIR in module search path
      // but is alread (except for windows - shall fix BL)
      //PyRun_SimpleString("import sys, os");
      //PyRun_SimpleString("sys.path.append(os.getenv('COOT_PYTHON_DIR'))");

      // Build the name object
      const char *modulename = "__main__";
      pName = PyString_FromString(modulename);
      pModule = PyImport_Import(pName);
      pModule = PyImport_AddModule("__main__");
      globals = PyModule_GetDict(pModule);
      Py_XINCREF(globals);

      if (globals == NULL) {
	std::cout<<"ERROR:: globals are NULL when running "<< python_cmd <<std::endl;
      } else {
	if (! PyDict_Check(globals)) {
	  std::cout<<"ERROR:: could not get globals when running " << python_cmd <<std::endl;
	} else {
	   char *py_command_str = new char[python_cmd.length() + 2];
	   strncpy(py_command_str, python_cmd.c_str(), python_cmd.length()+1);
	   // std::cout << "running PyRun_String() on :" << py_command_str << ":" << std::endl;
	   pValue = PyRun_String(py_command_str, Py_eval_input, globals, globals);

	   if (0) // debugging
	      std::cout << "DEBUG:: in safe_python_command_with_return() pValue is "
			<< pValue << std::endl;
	   
	   if (pValue != NULL) {
	      if (pValue != Py_None) {
		 
		 // ret = py_clean_internal(pValue); // old style 'sometimes-copy'
		 ret = pValue;
		 if (! ret)
		    ret = Py_None;
	      } else {
		 ret = Py_None;
	      }
	   } else {

	      // deal with pValue = NULL
	      
	      // there is an Error. Could be a syntax error whilst trying to evaluate a statement
	      // so let's try to run it as a statement
	      if (PyErr_ExceptionMatches(PyExc_SyntaxError)) {
		 std::cout << "error (syntax error)" << std::endl;
		 
		 PyErr_Clear();
		 pValue = PyRun_String((char *)python_cmd.c_str(), Py_single_input, globals, globals);
		 if (pValue != NULL) {
		    ret = pValue;
		 }
	      } else {
		 std::cout << "error (not syntax error)" << std::endl;
		 PyErr_Print();
	      }
	   }
	   delete[] py_command_str;
	   // Py_XDECREF(pValue); // No. We want objects from here to have a refcount of 1.
	}
      }
      // Clean up
      Py_XDECREF(pModule);
      Py_XDECREF(pName);
      Py_XDECREF(globals);
   }

   // Running PyBool_Check(NULL) crashes.
   if (ret == NULL) {
      Py_INCREF(Py_None);
      return Py_None; // don't try to convert this to a SCM thing.
   }
   
   if (PyBool_Check(ret)) { 
      Py_INCREF(ret);
   }
   if (ret == Py_None) {
      Py_INCREF(ret);
   }
   return ret;
}
#endif //PYTHON

#ifdef USE_PYTHON
PyObject *safe_python_command_test(const char *cmd) {

   std::string s = cmd;
   return safe_python_command_with_return(s);
}
#endif //PYTHON

#ifdef USE_PYTHON
/* BL says:: try not to use this!!!! */
/* doesnt work anyway!? */
void safe_python_command_with_unsafe_thread(const char *cmd) {

  Py_BEGIN_ALLOW_THREADS;
  PyRun_SimpleString((char *)cmd);
  Py_END_ALLOW_THREADS;
}

#endif //PYTHON

void safe_python_command_by_char_star(const char *python_cmd) {

#ifdef USE_PYTHON
   PyRun_SimpleString((char *)python_cmd);
#endif   
}

// functions to run python commands in guile and vice versa
// ignoring return values for now
#ifdef USE_PYTHON
PyObject *run_scheme_command(const char *cmd) {

  PyObject *ret_py = Py_None;

#ifdef USE_GUILE
  SCM ret_scm = safe_scheme_command(cmd);
  ret_py = scm_to_py(ret_scm);
#endif // USE_GUILE

  if (PyBool_Check(ret_py) || ret_py == Py_None) {
    Py_INCREF(ret_py);
  }
  return ret_py;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM run_python_command(const char *python_cmd) {

   // SCM_UNSPECIFIED corresponds to Py_None
   SCM ret_scm = SCM_UNSPECIFIED; // not SCM_UNDEFINED; :)

#ifdef USE_PYTHON
   PyObject *ret = safe_python_command_with_return(python_cmd);
   if (ret != Py_None) {
     ret_scm = py_to_scm(ret);
   } else {
       Py_XDECREF(ret);
   }
   // 15092012 correct this (I think)
   // need to decref if Py_None I (strongly) believe. (increfed in 
   // safe_python_command_with_return.
   // This does not crash. No idea why previous was...
   // Note: not all decrefs have increfs! Some increfs happens with python 
   // API. There may be some more fixing required with respect to ref counting.
   // FIXME!
   
   // 20120904 comment this out
   // Py_XDECREF(ret);
   // as it causes a crash
   // e.g. open Extensions->Settings->Keybindings invokes this function
   // then running a python function (e.g. a key-binding) causes a crash.
   // I believe that Py_XDECREF(ret) is wrong here because we have never
   // called Py_INCREF(ref).  So perhaps we should be using both Py_INCREF()
   // and Py_XDECREF(). 
   
#endif // USE_PYTHON
  
  return ret_scm;
}
#endif // USE_GUILE

#ifdef USE_GUILE
// Return a list describing a residue like that returned by
// residues-matching-criteria (list return-val chain-id resno ins-code)
// This is a library function really.  There should be somewhere else to put it.
// It doesn't need expression at the scripting level.
// return a null list on problem
SCM scm_residue(const coot::residue_spec_t &res) {
   SCM r = SCM_EOL;

//    std::cout <<  "scm_residue on: " << res.chain << " " << res.resno << " "
// 	     << res.insertion_code  << std::endl;
   r = scm_cons(scm_makfrom0str(res.insertion_code.c_str()), r);
   r = scm_cons(SCM_MAKINUM(res.resno), r);
   r = scm_cons(scm_makfrom0str(res.chain.c_str()), r);
   r = scm_cons(SCM_BOOL_T, r);
   return r;
}
#endif

#ifdef USE_PYTHON
// Return a list describing a residue like that returned by
// residues-matching-criteria [return-val, chain-id, resno, ins-code]
// This is a library function really.  There should be somewhere else to put it.
// It doesn't need expression at the scripting level.
// return a null list on problem
PyObject *py_residue(const coot::residue_spec_t &res) {
   PyObject *r;
   r = PyList_New(4);

//    std::cout <<  "py_residue on: " << res.chain << " " << res.resno << " "
// 	     << res.insertion_code  << std::endl;
   Py_XINCREF(Py_True);
   PyList_SetItem(r, 0, Py_True);
   PyList_SetItem(r, 1, PyString_FromString(res.chain.c_str()));
   PyList_SetItem(r, 2, PyInt_FromLong(res.resno));
   PyList_SetItem(r, 3, PyString_FromString(res.insertion_code.c_str()));

   return r;
}
#endif // USE_PYTHON

#ifdef USE_GUILE 
// Return a SCM list object of (residue1 residue2 omega) 
SCM cis_peptides(int imol) {
   SCM r = SCM_EOL;

   // more info on the real cis peptides derived from atom positions:

   if (is_valid_model_molecule(imol)) {

      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      std::vector<coot::util::cis_peptide_info_t> v =
	 coot::util::cis_peptides_info_from_coords(mol);

      for (unsigned int i=0; i<v.size(); i++) {
	 coot::residue_spec_t r1(v[i].chain_id_1,
				 v[i].resno_1,
				 v[i].ins_code_1);
	 coot::residue_spec_t r2(v[i].chain_id_2,
				 v[i].resno_2,
				 v[i].ins_code_2);
	 SCM scm_r1 = scm_residue(r1);
	 SCM scm_r2 = scm_residue(r2);
	 SCM scm_residue_info = SCM_EOL;
// 	 std::cout << "DEBUG:: cis pep with omega: "
// 		   << v[i].omega_torsion_angle
// 		   << std::endl;
// 	 SCM scm_omega = 
// 	    scm_double2num(clipper::Util::rad2d(v[1].omega_torsion_angle));
	 SCM scm_omega = scm_double2num(v[i].omega_torsion_angle);
	 scm_residue_info = scm_cons(scm_omega, scm_residue_info);
	 scm_residue_info = scm_cons(scm_r2, scm_residue_info);
	 scm_residue_info = scm_cons(scm_r1, scm_residue_info);

	 // add scm_residue_info to r
	 r = scm_cons(scm_residue_info, r);
      }
      r = scm_reverse(r);
   }
   return r;
}
#endif //  USE_GUILE 

#ifdef USE_PYTHON 
// Return a python list object of [residue1, residue2, omega] 
PyObject *cis_peptides_py(int imol) {
   PyObject *r;
   r = PyList_New(0);

   // more info on the real cis peptides derived from atom positions:

   if (is_valid_model_molecule(imol)) {

      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      std::vector<coot::util::cis_peptide_info_t> v =
	 coot::util::cis_peptides_info_from_coords(mol);

      for (unsigned int i=0; i<v.size(); i++) {
	 coot::residue_spec_t r1(v[i].chain_id_1,
				 v[i].resno_1,
				 v[i].ins_code_1);
	 coot::residue_spec_t r2(v[i].chain_id_2,
				 v[i].resno_2,
				 v[i].ins_code_2);
	 PyObject *py_r1, *py_r2, *py_residue_info;
	 py_r1 = py_residue(r1);
	 py_r2 = py_residue(r2);
	 py_residue_info = PyList_New(3);
// 	 std::cout << "DEBUG:: cis pep with omega: "
// 		   << v[i].omega_torsion_angle
// 		   << std::endl;
// 	 SCM scm_omega = 
// 	    scm_double2num(clipper::Util::rad2d(v[1].omega_torsion_angle));
	 PyObject *py_omega;
	 py_omega = PyFloat_FromDouble(v[i].omega_torsion_angle);
	 PyList_SetItem(py_residue_info, 2, py_omega);
	 PyList_SetItem(py_residue_info, 1, py_r2);
	 PyList_SetItem(py_residue_info, 0, py_r1);

	 // add py_residue_info to r
	 PyList_Append(r, py_residue_info);
	 Py_XDECREF(py_residue_info);

      }
   }

   return r;
}
#endif //  USE_PYTHON


void post_scripting_window() {

#ifdef USE_GUILE
   post_scheme_scripting_window();
#endif    

}


/*! \brief pop-up a scripting window for scheming */
void post_scheme_scripting_window() {

#ifdef USE_GUILE

  if (graphics_info_t::guile_gui_loaded_flag == TRUE) { 

     scm_c_eval_string("(coot-gui)");

  } else { 
     // we don't get a proper status from guile_gui_loaded_flag so
     // lets check again here whether MAPVIEW_GUI_DIR was defined.
     char *t; 
     t = getenv(COOT_SCHEME_DIR); // was #defined
     if (t) { 
	std::cout << COOT_SCHEME_DIR << " was defined to be " << t << std::endl
		  << "   but loading of scripting window scheme code failed." 
		  << "   Nevertheless you will get a simple scripting window." 
		  << std::endl;
        // load the fallback window if we have COOT_SCHEME_DIR (only Windows?!)
        // only for gtk2!
#if (GTK_MAJOR_VERSION > 1)
        GtkWidget *window; 
        GtkWidget *scheme_entry; 
        window = create_scheme_window();

        scheme_entry = lookup_widget(window, "scheme_window_entry");
        setup_guile_window_entry(scheme_entry); // USE_PYTHON and USE_GUILE used here
        gtk_widget_show(window);        
#endif /* GTK_MAJOR_VERSION */
     } else { 
	std::cout << COOT_SCHEME_DIR << " was not defined - cannot open ";
	std::cout << "scripting window" << std::endl; 
     } 
  } 
#endif

}

/*! \brief pop-up a scripting window for pythoning */
void post_python_scripting_window() {

#ifdef USE_PYTHON

  if (graphics_info_t::python_gui_loaded_flag == TRUE) {

     PyRun_SimpleString("coot_gui()");

  } else {
     // we don't get a proper status from python_gui_loaded_flag so
     // lets check again here whether MAPVIEW_GUI_DIR was defined.
     char *t;
     t = getenv("COOT_PYTHON_DIR"); // was #defined
     if (t) {
        std::cout << "COOT_PYTHON_DIR was defined to be " << t << std::endl
                  << "  but no PyGtk and hence no coot_gui."
                  << std::endl;
     } else {
        std::cout << "COOT_PYTHON_DIR  was not defined - cannot open ";
        std::cout << "scripting window" << std::endl;
     }
// so let's load the usual window!!
  GtkWidget *window; 
  GtkWidget *python_entry; 
  window = create_python_window();

  python_entry = lookup_widget(window, "python_window_entry");
  setup_python_window_entry(python_entry); // USE_PYTHON and USE_GUILE used here
  gtk_widget_show(window);
  }

  // clear the entry here
#else
  std::cout << "No python" << std::endl;
#endif

}


/* called from c-inner-main */
// If you edit this again, move it to graphics_info_t.
// 
void run_command_line_scripts() {

//     std::cout << "There are " << graphics_info_t::command_line_scripts->size() 
//  	     << " command line scripts to run\n";

    for (unsigned int i=0; i<graphics_info_t::command_line_scripts->size(); i++)
       run_script((*graphics_info_t::command_line_scripts)[i].c_str());

    for (unsigned int i=0; i<graphics_info_t::command_line_commands.commands.size(); i++)
       if (graphics_info_t::command_line_commands.is_python)
	  safe_python_command(graphics_info_t::command_line_commands.commands[i].c_str());
       else 
	  safe_scheme_command(graphics_info_t::command_line_commands.commands[i].c_str());

    graphics_info_t g;
    for (unsigned int i=0; i<graphics_info_t::command_line_accession_codes.size(); i++) {
       std::cout << "get accession code " << graphics_info_t::command_line_accession_codes[i]
		 << std::endl;
       std::vector<std::string> c;
       c.push_back("get-eds-pdb-and-mtz");
       c.push_back(single_quote(graphics_info_t::command_line_accession_codes[i]));
       
#ifdef USE_GUILE
       std::string sc = g.state_command(c, graphics_info_t::USE_SCM_STATE_COMMANDS);
       safe_scheme_command(sc.c_str());
#else	  
#ifdef USE_PYTHON
       std::string pc = g.state_command(c, graphics_info_t::USE_PYTHON_STATE_COMMANDS);
       safe_python_command(pc.c_str());
#endif
#endif
    }
}

void run_update_self_maybe() { // called when --update-self given at command line

#ifdef USE_LIBCURL
   if (graphics_info_t::update_self) {
#ifdef USE_GUILE
      safe_scheme_command("(update-self)");
#endif // USE_GUILE
#ifdef USE_PYTHON
      safe_python_command("update_self()");
#endif // USE_PYTHON
   }

#endif // USE_LIBCURL
}


void
set_guile_gui_loaded_flag() { 
   
   graphics_info_t g; 
   g.guile_gui_loaded_flag = TRUE; 
}

void
set_python_gui_loaded_flag() { 
   
   graphics_info_t g; 
   g.python_gui_loaded_flag = TRUE; 
}

void set_found_coot_gui() { 
   
   graphics_info_t g; 
#ifdef USE_GUILE
   cout << "Coot Scheme Scripting GUI code found and loaded." << endl; 
   g.guile_gui_loaded_flag = TRUE; 
#endif // USE_GUILE
}

void set_found_coot_python_gui() {
  
#ifdef USE_PYTHON
   graphics_info_t g;
   cout << "Coot Python Scripting GUI code found and loaded." << endl; 
   g.python_gui_loaded_flag = TRUE;
#endif // USE_PYTHON

}

// return an atom index
int atom_spec_to_atom_index(int imol, char *chain, int resno, char *atom_name) { 
   graphics_info_t g; 
   if (imol < graphics_n_molecules()) 
      return g.molecules[imol].atom_spec_to_atom_index(chain, resno, atom_name);
   else
      return -1;
}

int full_atom_spec_to_atom_index(int imol, const char *chain, int resno,
				 const char *inscode, const char *atom_name,
				 const char *altloc) {

   if (imol < graphics_n_molecules()) 
      return graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain), resno, std::string(inscode), std::string(atom_name), std::string(altloc));
   else
      return -1;
}



// ??? FIXME for the future, this doesn't work when we have both guile
// and python.  We need to choose which script interpretter to use
// based on filename extension.
void
run_script(const char *filename) { 

   struct stat buf;
   int status = stat(filename, &buf);
   std::string fn(filename);
   if (status == 0) {

      short int is_python = 0;

      std::string::size_type ipy = fn.rfind(".py");
      if (ipy != std::string::npos) {
	 if (fn.substr(ipy) == ".py")
	    is_python = 1;
      }
	 
      if (is_python) { 
	 run_python_script(filename);
      } else { 
	 run_guile_script(filename);
      }
   } else { 
      std::cout  << "WARNING:: Can't run script: " << filename 
		 << " no such file." << std::endl;
   }
}

// If we have both GUILE and PYTHON, use the state file as if it were GUILE
// 
void
run_state_file() { 
   std::string filename;
#ifdef USE_GUILE
   filename = "0-coot.state.scm";
   struct stat buf;
   int status = stat(filename.c_str(), &buf);
   if (status == 0) { 
      run_guile_script(filename.c_str());
      graphics_info_t::state_file_was_run_flag = true;
   }
#else 
#ifdef USE_PYTHON
   filename = "0-coot.state.py";
   struct stat buf;
   int status = stat(filename.c_str(), &buf);
   if (status == 0) { 
      run_python_script(filename.c_str());
      graphics_info_t::state_file_was_run_flag = true;
   }
#endif
#endif
}

#ifdef USE_PYTHON
void
run_state_file_py() { 
   std::string filename;
   filename = "0-coot.state.py";
   struct stat buf;
   int status = stat(filename.c_str(), &buf);
   if (status == 0) { 
      run_python_script(filename.c_str());
      graphics_info_t::state_file_was_run_flag = true;
   }
}
#endif // USE_PYTHON


void
run_state_file_maybe() { 

   std::string filename("0-coot.state.scm");
#ifdef USE_PYTHON
#ifndef USE_GUILE
   filename = "0-coot.state.py";
#endif
#endif
   graphics_info_t g;

   /*  0: never run it */
   /*  1: ask to run it */
   /*  2: always run it */
   if (g.run_state_file_status == 1 || g.run_state_file_status == 2) { 
      
      // can we stat a status file?
      // 
      struct stat buf;
      int status = stat(filename.c_str(), &buf);
      if (status == 0) { 
	 if (g.run_state_file_status == 2) {
	    run_script(filename.c_str());
	    graphics_info_t::state_file_was_run_flag = true;
	 } else {
	    if (graphics_info_t::use_graphics_interface_flag) { 
	       GtkWidget *dialog = wrapped_create_run_state_file_dialog();
	       gtk_widget_show(dialog);
	    } 
	 }
      }
   }
}

GtkWidget *wrapped_create_run_state_file_dialog() {

#ifdef USE_GUILE
   std::string filename("0-coot.state.scm");
#else
// BL says:: we might want to have it in python too
#ifdef USE_PYTHON
   std::string filename("0-coot.state.py");
#else
   std::string filename("0-coot.state.scm"); // unusual
#endif // python
#endif // USE_GUILE
   short int il = 1;
   GtkWidget *w = create_run_state_file_dialog();

   GtkWidget *vbox_mols = lookup_widget(w, "mols_vbox");

   graphics_info_t g;
   std::vector<std::string> v = g.save_state_data_and_models(filename, il);
   for (unsigned int i=0; i<v.size(); i++) { 
      //       std::cout << "Got molecule: " << v[i] << std::endl;
      std::string s = "    ";
      s += v[i];
      GtkWidget *label = gtk_label_new(s.c_str());
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start(GTK_BOX(vbox_mols), label, FALSE, FALSE, 2);
      gtk_widget_show(label);
   } 
   return w;
}

#ifdef USE_PYTHON
GtkWidget *wrapped_create_run_state_file_dialog_py() {

   std::string filename("0-coot.state.py");
   short int il = 1;
   GtkWidget *w = create_run_state_file_dialog();

   GtkWidget *vbox_mols = lookup_widget(w, "mols_vbox");

   graphics_info_t g;
   std::vector<std::string> v = g.save_state_data_and_models(filename, il);
   for (unsigned int i=0; i<v.size(); i++) { 
      //       std::cout << "Got molecule: " << v[i] << std::endl;
      std::string s = "    ";
      s += v[i];
      GtkWidget *label = gtk_label_new(s.c_str());
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start(GTK_BOX(vbox_mols), label, FALSE, FALSE, 2);
      gtk_widget_show(label);
   } 
   return w;
}
#endif // USE_PYTHON


void
run_guile_script(const char *filename) { 

#ifdef USE_GUILE
   std::string thunk("(lambda() "); 
   thunk += "(load ";
   thunk += "\"";
   thunk += filename;
   thunk += "\"))";

   SCM handler = scm_c_eval_string ("(lambda (key . args) "
     "(display (list \"Error in proc:\" key \" args: \" args)) (newline))"); 

   SCM scm_thunk = scm_c_eval_string(thunk.c_str());
   scm_catch(SCM_BOOL_T, scm_thunk, handler);
#endif // USE_GUILE   

} 

void
run_python_script(const char *filename_in) { 

#ifdef USE_PYTHON

   std::string s = coot::util::intelligent_debackslash(filename_in);
   std::string simple = "execfile(";
   simple += single_quote(s);
   simple += ")";
   std::cout << "Running python script " << s  << std::endl;
   // not a const argument?  Dear oh dear....
   PyRun_SimpleString((char *)simple.c_str());

#endif // USE_PYTHON
}

int
import_python_module(const char *module_name, int use_namespace) { 

  int err = 1;

#ifdef USE_PYTHON
  
  std::string simple;
  if (use_namespace) {
    simple = "import ";
    simple += module_name;
  } else {
    simple = "from ";
    simple += module_name;
    simple += " import *";
  }
  std::cout << "Importing python module " << module_name 
	    << " using command " << simple << std::endl;
  // not a const argument?  Dear oh dear....
  err = PyRun_SimpleString((char *)simple.c_str());
#endif // USE_PYTHON
  return err;
}



void add_on_rama_choices(){  // the the menu

   // std::cout << "adding rama molecule options:" << std::endl;

   // first delete all the current menu items.
   //
   graphics_info_t g;
   GtkWidget* menu = lookup_widget(GTK_WIDGET(g.glarea), "rama_plot_menu");

   if (menu) {
      gtk_container_foreach(GTK_CONTAINER(menu),
			    my_delete_ramachandran_mol_option,
			    (gpointer) menu);
   
      std::string name;
      for (int i=0; i<g.n_molecules(); i++) {
	 if (g.molecules[i].has_model() > 0) {
	    name = graphics_info_t::molecules[i].dotted_chopped_name();
	    update_ramachandran_plot_menu_manual(i, name.c_str());
	 }
      }
   }
}


void destroy_edit_backbone_rama_plot() { 

   graphics_info_t g;
   g.destroy_edit_backbone_rama_plot();

} 


/*  ----------------------------------------------------------------------- */
/*           sequence_view                                                  */
/*  ----------------------------------------------------------------------- */

// A pure sequence function, not sequence view, so that people can cut
// n paste the sequence of a pdb file from the console.  There will be
// a scripting level function called print-sequence that gets called
// for every chain in the mol
// 
void print_sequence_chain(int imol, const char *chain_id) {

   std::string seq;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      CModel *model_p = mol->GetModel(imod);
      CChain *chain_p;
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 if (std::string(chain_p->GetChainID()) == chain_id) { 
	    int nres = chain_p->GetNumberOfResidues();
	    PCResidue residue_p;
	    int residue_count_block = 0;
	    int residue_count_line = 0;
	    if (nres > 0 ) {
	       residue_count_block = chain_p->GetResidue(0)->GetSeqNum();
	       residue_count_line  = residue_count_block;
	       if (residue_count_block > 0)
		  while (residue_count_block > 10)
		     residue_count_block -= 10;
	       if (residue_count_line > 0)
		  while (residue_count_line > 50)
		     residue_count_line -= 50;
	    }
	    for (int ires=0; ires<nres; ires++) {
	       residue_p = chain_p->GetResidue(ires);
	       seq += coot::util::three_letter_to_one_letter(residue_p->GetResName());
	       if (residue_count_block == 10) {
		  seq += " ";
		  residue_count_block = 0;
	       }
	       if (residue_count_line == 50) {
		  seq += "\n";
		  residue_count_line = 0;
	       }
	       residue_count_block++;
	       residue_count_line++;
	    }
	 }
      }
      std::cout << "> " << graphics_info_t::molecules[imol].name_sans_extension(0)
		<< " chain " << chain_id << std::endl;
      std::cout << seq << std::endl;
   }
}

void do_sequence_view(int imol) {

   if (is_valid_model_molecule(imol)) {
      bool do_old_style = 0; 
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      std::vector<CResidue *> r = coot::util::residues_with_insertion_codes(mol);
      if (r.size() > 0) {
	 do_old_style = 1;
      } else {
	 // was it a big shelx molecule?
	 if (graphics_info_t::molecules[imol].is_from_shelx_ins()) {
	    std::pair<bool, int> max_resno =
	       coot::util::max_resno_in_molecule(graphics_info_t::molecules[imol].atom_sel.mol);
	    if (max_resno.first)
	       if (max_resno.second > 3200)
		  do_old_style = 1;
	 } 
      } 


      if (do_old_style) { 
	 sequence_view_old_style(imol);
      } else {
	 nsv(imol);
      } 
   } 

} 


/*  ----------------------------------------------------------------------- */
/*           rotate moving atoms peptide                                    */
/*  ----------------------------------------------------------------------- */

void change_peptide_carbonyl_by(double angle) { /* in degrees. */
   graphics_info_t g;
   g.change_peptide_carbonyl_by(angle);
} 

void change_peptide_peptide_by(double angle) {   /* in degress */
   graphics_info_t g;
   g.change_peptide_peptide_by(angle);
}

void execute_setup_backbone_torsion_edit(int imol, int atom_index) {
   graphics_info_t g;
   g.execute_setup_backbone_torsion_edit(imol, atom_index);
}

void setup_backbone_torsion_edit(short int state) { 

   graphics_info_t g;
   graphics_info_t::in_backbone_torsion_define = state;
   if (state) { 
      std::cout << "click on an atom in the peptide to change" << std::endl; 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
   } else { 
      g.normal_cursor();
   }
}

void setup_dynamic_distances(short int state) {

   graphics_info_t::in_dynamic_distance_define = state;
   if (state) {
      pick_cursor_maybe();
   }
}


void set_refine_with_torsion_restraints(int istate) {

   graphics_info_t::do_torsion_restraints = istate;

} 


int refine_with_torsion_restraints_state() {

   return graphics_info_t::do_torsion_restraints;

} 


void set_backbone_torsion_peptide_button_start_pos(int ix, int iy) { 
   
   graphics_info_t g;
   g.set_backbone_torsion_peptide_button_start_pos(ix, iy);
} 

void change_peptide_peptide_by_current_button_pos(int ix, int iy) { 

   graphics_info_t g;
   g.change_peptide_peptide_by_current_button_pos(ix, iy);
}

void set_backbone_torsion_carbonyl_button_start_pos(int ix, int iy) { 

   graphics_info_t g;
   g.set_backbone_torsion_carbonyl_button_start_pos(ix, iy);

} 

void change_peptide_carbonyl_by_current_button_pos(int ix, int iy) { 

   graphics_info_t g;
   g.change_peptide_carbonyl_by_current_button_pos(ix, iy);

} 

/*  ----------------------------------------------------------------------- */
/*                  cif stuff                                               */
/*  ----------------------------------------------------------------------- */

// and make (and display) a sigma_a map.
// 
// Pass the file name of the cif file and the molecule number for which
// we will calculate sfs.
// 
int read_cif_data(const char *filename, int imol_coordinates) {

      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.

   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      cout << "Reading cif file: " << filename << endl; 
      graphics_info_t g; 
      int imol = g.create_molecule();
      int istat = g.molecules[imol].make_map_from_cif(imol,
						      std::string(filename), imol_coordinates);

      std::cout << "DEBUG in read_cif_data() istat is " << istat << std::endl;

      // std::cout << "DEBUG:: in read_cif_data, istat is " << istat << std::endl;
      if (istat != -1) { 
	 graphics_draw();
      } else {
	 g.erase_last_molecule();
	 imol = -1;
      } 
      return imol;
   }
   return -1; // which is status in an error
}

// and make (and display) a 2fofc map.
// 
// Pass the file name of the cif file and the molecule number for which
// we will calculate sfs.
// 
int read_cif_data_2fofc_map(const char *filename, int imol_coordinates) {

   int imol = -1; 
      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.

   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      
      cout << "Reading cif file: " << filename << endl; 

      graphics_info_t g; 

      imol = g.create_molecule();
      int istat = g.molecules[imol].make_map_from_cif_2fofc(imol,
							    std::string(filename),
							    imol_coordinates);

      if (istat != -1) { 
	 graphics_draw();
      } else {
	 g.erase_last_molecule();
	 imol = -1; 
      } 
   }
   return imol;
}


// and make (and display) a fofc map.
// 
// Pass the file name of the cif file and the molecule number for which
// we will calculate sfs.
// 
int read_cif_data_fofc_map(const char *filename, int imol_coordinates) {

      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.

   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      
      cout << "Reading cif file: " << filename << endl; 

      graphics_info_t g; 

      int imol = g.create_molecule();
      int istat = g.molecules[imol].make_map_from_cif_fofc(imol,
							   std::string(filename),
							   imol_coordinates);

      if (istat != -1) { 
	 graphics_draw();
	 return imol;
      }
      return -1; // an error
   }
}



// This cif file, we presume, has phases.
// So we don't need a molecule to calculate them from.
// 
int auto_read_cif_data_with_phases(const char *filename) {

   int returned_mol_index = read_cif_data_with_phases_sigmaa(filename);
   read_cif_data_with_phases_diff_sigmaa(filename);
   return returned_mol_index;
}

int read_cif_data_with_phases_sigmaa(const char *filename) {

   graphics_info_t g; 
   int imol = -1;
   
   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      
      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.
      std::string fn = filename;
      imol = g.create_molecule();
      int istat = g.molecules[imol].make_map_from_cif(imol, fn);
      if (istat != -1) {
	 g.scroll_wheel_map = imol;
	 g.activate_scroll_radio_button_in_display_manager(imol);
	 graphics_draw();
      } else {
	 g.erase_last_molecule();
	 imol = -1;
      }
   }
   return imol;
}

int read_cif_data_with_phases_diff_sigmaa(const char *filename) {

   graphics_info_t g; 
   int imol = -1;
   
   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      
      std::cout << "Reading cif file (with phases - diff) : " << filename << std::endl; 

      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.
      std::string fn = filename;
      imol = g.create_molecule();
      int istat = g.molecules[imol].make_map_from_cif_diff_sigmaa(imol, fn);
      if (istat != -1) {
	 g.scroll_wheel_map = imol;
	 g.activate_scroll_radio_button_in_display_manager(imol);
	 graphics_draw();
      } else {
	 g.erase_last_molecule();
	 imol = -1;
      }
   }
   return imol;
} 


int read_cif_data_with_phases_fo_fc(const char *filename) {

   return read_cif_data_with_phases_nfo_fc(filename, molecule_map_type::TYPE_FO_FC);
} 

int read_cif_data_with_phases_2fo_fc(const char *filename) {

   return read_cif_data_with_phases_nfo_fc(filename, molecule_map_type::TYPE_2FO_FC);
}

int read_cif_data_with_phases_fo_alpha_calc(const char *filename) {
   return read_cif_data_with_phases_nfo_fc(filename, molecule_map_type::TYPE_FO_ALPHA_CALC);
}

int read_cif_data_with_phases_nfo_fc(const char *filename,
				     int map_type) {
   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << endl;
      }
      return -1; // which is status in an error
   } else {
      
      // This function is the .cif equivalent of
      // c.f. read_phs_and_coords_and_make_map or make_and_draw_map,
      // map_fill_from_mtz.

      graphics_info_t g; 

      int imol = g.create_molecule();
      std::string f(filename);
      short int swap_col = graphics_info_t::swap_difference_map_colours;

      int istat = g.molecules[imol].make_map_from_cif_nfofc(imol, f, map_type, swap_col);

      if (istat != -1) {

	 g.scroll_wheel_map = imol; // change the current scrollable map.
	 graphics_draw();
	 return imol;
      } else {
	 g.erase_last_molecule();
      } 
      return -1; // error
   }
}

int handle_shelx_fcf_file_internal(const char *filename) {

   graphics_info_t g;
   std::vector<std::string> cmd;
   cmd.push_back("handle-shelx-fcf-file");
   cmd.push_back(single_quote(coot::util::intelligent_debackslash(filename)));

// #ifdef USE_GUILE   
//    std::string s = g.state_command(cmd, coot::STATE_SCM);
//    safe_scheme_command(s);
// #endif

// #ifdef USE_PYTHON
// #ifndef USE_GUILE
//    std::string s = g.state_command(cmd, coot::STATE_PYTHON);
//    safe_python_command(s);
// #endif    
// #endif

   return read_small_molecule_data_cif(filename);
}


/* Use the environment variable COOT_REFMAC_LIB_DIR to find cif files
   in subdirectories and import them all. */
void import_all_refmac_cifs() {

   graphics_info_t g;
   g.import_all_refmac_cifs();

} 



/*  ----------------------------------------------------------------------- */
/*                  atom labelling                                          */
/*  ----------------------------------------------------------------------- */
/* The guts happens in molecule_class_info_t, here is just the
   exported interface */
int add_atom_label(int imol, char *chain_id, int iresno, char *atom_id) {

   int i = 0;
   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      i = g.molecules[imol].add_atom_label(chain_id, iresno, atom_id);
   }
   return i;
} 

int remove_atom_label(int imol, char *chain_id, int iresno, char *atom_id) {
   graphics_info_t g;
   return g.molecules[imol].remove_atom_label(chain_id, iresno, atom_id);
} 

void remove_all_atom_labels() { 
   graphics_info_t g;
   g.remove_all_atom_labels();
} 

void set_label_on_recentre_flag(int i) { 
   graphics_info_t::label_atom_on_recentre_flag = i;
} 

int centre_atom_label_status() { 
   return graphics_info_t::label_atom_on_recentre_flag;
} 

void set_brief_atom_labels(int istat) {
   graphics_info_t::brief_atom_labels_flag = istat;
   graphics_draw();
}

int brief_atom_labels_state() {
   return graphics_info_t::brief_atom_labels_flag;
}

/*  ----------------------------------------------------------------------- */
/*                  scene rotation (by script)                              */
/*  ----------------------------------------------------------------------- */
/* stepsize in degrees */
void rotate_y_scene(int nsteps, float stepsize) {

   // 20101108 [Gatwick airport] Note: there is code in orient_view()
   // that presumes (with good reason) that this actually rotates the
   // view by nstep*stepsize*2
   // 

   float spin_quat[4];
   graphics_info_t g;

   // spin it 1 degree
   float tbs =  g.get_trackball_size(); 
   for(int i=0; i<nsteps; i++) { 
     trackball(spin_quat, 0, 0, 0.0174533*stepsize, 0.000, tbs);
     add_quats(spin_quat, g.quat, g.quat);
     graphics_draw();
   }
} 

/* stepsize in degrees */
void rotate_x_scene(int nsteps, float stepsize) { 

  float spin_quat[4];
   graphics_info_t g;

   // spin it 1 degree
   float tbs =  g.get_trackball_size(); 
   for(int i=0; i<nsteps; i++) { 
     trackball(spin_quat, 0, 0, 0.0, 0.0174533*stepsize, tbs);
     add_quats(spin_quat, g.quat, g.quat);
     graphics_draw();
   }
} 

void rotate_z_scene(int nsteps, float stepsize) { 

   // c.f globjects.cc:do_screen_z_rotate()
   // 

   float spin_quat[4];
   graphics_info_t g;
   for(int i=0; i<nsteps; i++) { 
      trackball(spin_quat, 
		1.0, 1.0,
		1.0, 1.0 + 0.0174533*stepsize,
		0.4);
      add_quats(spin_quat, g.quat, g.quat);
      graphics_draw();
   }
}

/*! \brief Bells and whistles rotation 

    spin, zoom and translate.

    where axis is either x,y or z,
    stepsize is in degrees, 
    zoom_by and x_rel etc are how much zoom, x,y,z should 
            have changed by after nstep steps.
*/
void spin_zoom_trans(int axis, int nsteps, float stepsize, float zoom_by, 
		     float x_rel, float y_rel, float z_rel) {

   float spin_quat[4];
   graphics_info_t g;
   float tbs =  g.get_trackball_size();
   float x_frag = 0.0;
   float y_frag = 0.0;
   float z_frag = 0.0;
   if (nsteps != 0) { 
      x_frag = x_rel/float(nsteps);
      y_frag = y_rel/float(nsteps);
      z_frag = z_rel/float(nsteps);
   }
   float zoom_init = g.zoom;
   float zoom_final = g.zoom * zoom_by;
   float zoom_frag = 1.0;
   if (nsteps !=0) {
      zoom_frag = (zoom_final - zoom_init)/float(nsteps);
   }
   int sss = graphics_info_t::smooth_scroll;

   // setRotationCentre looks at this and does a smooth scroll if its
   // on.
   graphics_info_t::smooth_scroll = 0;
   
   // std::cout << "zoom_frag is " << zoom_frag << std::endl;
   for(int i=0; i<nsteps; i++) {
      if (axis == 1) { 
	 trackball(spin_quat, 0, 0, 0.0, 0.0174*stepsize, tbs);
	 add_quats(spin_quat, g.quat, g.quat);
      }
      if (axis == 2) { 
	 trackball(spin_quat, 0, 0, 0.0174*stepsize, 0.000, tbs);
	 add_quats(spin_quat, g.quat, g.quat);
      }
      if (axis == 3) { 
	 trackball(spin_quat, 1.0, 1.0, 1.0, 1.0 + 0.0174*stepsize, 0.4);
	 add_quats(spin_quat, g.quat, g.quat);
      }
      g.zoom = zoom_init + float(i+1)*zoom_frag;
      coot::Cartesian c(g.X() + x_frag, g.Y() + y_frag, g.Z() + z_frag);
      g.setRotationCentre(c);
      graphics_draw();
   }
   graphics_info_t::smooth_scroll = sss;
}

void translate_scene_x(int nsteps) {
   std::cout << "placeholder" << std::endl;
} 

void translate_scene_y(int nsteps) {
   std::cout << "placeholder" << std::endl;
}
void translate_scene_z(int nsteps) {
   std::cout << "placeholder" << std::endl;
}




/*  ----------------------------------------------------------------------- */
/*                  graphics background colour                              */
/*  ----------------------------------------------------------------------- */
/* stepsize in degrees */
void set_background_colour(double red, double green, double blue) {

   graphics_info_t g;

   if (g.use_graphics_interface_flag) {
     if(g.glarea_2) {
       g.make_current_gl_context(g.glarea_2);
       glClearColor(red,green,blue,1.0);
       g.background_colour[0] = red; 
       g.background_colour[1] = green; 
       g.background_colour[2] = blue; 
       glFogfv(GL_FOG_COLOR, g.background_colour);
     }
     g.make_current_gl_context(g.glarea);
     glClearColor(red,green,blue,1.0);
     g.background_colour[0] = red; 
     g.background_colour[1] = green; 
     g.background_colour[2] = blue; 
     glFogfv(GL_FOG_COLOR, g.background_colour);
     if (g.do_anti_aliasing_flag) {
       // update the antialias?!
       g.draw_anti_aliasing();
     }
     graphics_draw();
   }
}

void
redraw_background() {

   graphics_info_t g;

   double red   = g.background_colour[0];
   double green = g.background_colour[1]; 
   double blue  = g.background_colour[2]; 
   if (g.use_graphics_interface_flag && !background_is_black_p()) {
     if(g.glarea_2) {
       g.make_current_gl_context(g.glarea_2);
       glClearColor(red,green,blue,1.0);
       glFogfv(GL_FOG_COLOR, g.background_colour);
     }
     g.make_current_gl_context(g.glarea);
     glClearColor(red,green,blue,1.0);
     glFogfv(GL_FOG_COLOR, g.background_colour);
     graphics_draw();
   }

}

int background_is_black_p() {

   graphics_info_t g;
   return g.background_is_black_p();
}



// ------------------------------------------------------------------
//                                Utility
// ------------------------------------------------------------------
// 
// File system Utility function: maybe there is a better place for it...
// Return like mkdir: mkdir returns zero on success, or -1 if an  error  occurred
//
// if it already exists as a dir, return 0 of course.
// 
int
make_directory_maybe(const char *dir) {
   return coot::util::create_directory(std::string(dir));
} 


void add_coordinates_glob_extension(const char *ext) { 
   
   graphics_info_t g;
   g.add_coordinates_glob_extension(std::string(ext));
} 

void add_data_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.add_data_glob_extension(std::string(ext));
} 

void add_dictionary_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.add_dictionary_glob_extension(std::string(ext));
} 

void add_map_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.add_map_glob_extension(std::string(ext));
} 

void remove_coordinates_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.remove_coordinates_glob_extension(std::string(ext));
} 

void remove_data_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.remove_data_glob_extension(std::string(ext));
} 

void remove_dictionary_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.remove_dictionary_glob_extension(std::string(ext));
} 

void remove_map_glob_extension(const char *ext) { 
   graphics_info_t g;
   g.remove_map_glob_extension(std::string(ext));
} 



int do_anti_aliasing_state() {
   return graphics_info_t::do_anti_aliasing_flag;
} 


void set_do_anti_aliasing(int state) {

   graphics_info_t g;
   g.set_do_anti_aliasing(state);
} 


void set_do_GL_lighting(int state) {
   graphics_info_t::do_lighting_flag = state;
   setup_lighting(state);
   graphics_draw();
}


int do_GL_lighting_state() {
   return graphics_info_t::do_lighting_flag;
}


/*  ----------------------------------------------------------------------- */
/*                  crosshairs                                              */
/*  ----------------------------------------------------------------------- */
void set_draw_crosshairs(short int i) { 

   graphics_info_t g;
   g.draw_crosshairs_flag = i;
   if (i > 0 ) { 
      g.crosshairs_text(); 
      graphics_draw();
   }
} 

short int draw_crosshairs_state() {
   return graphics_info_t::draw_crosshairs_flag; 
}


/*  ----------------------------------------------------------------------- */
/*                  citation notice                                         */
/*  ----------------------------------------------------------------------- */
void citation_notice_off() { 

   graphics_info_t::show_citation_notice = 0;

} 

/*  ----------------------------------------------------------------------- */
/*                  cursor function                                         */
/*  ----------------------------------------------------------------------- */
void normal_cursor() {

   graphics_info_t g;
   g.normal_cursor();
   graphics_draw();
}

void fleur_cursor() {
   graphics_info_t g;
   g.fleur_cursor();
   graphics_draw();

}

void pick_cursor_maybe() {

   graphics_info_t g;
   g.pick_cursor_maybe();
   graphics_draw();
}

void rotate_cursor() {
   normal_cursor();
}

void set_pick_cursor_index(int i) {
   graphics_info_t::pick_cursor_index = GdkCursorType(i);
}


/*  ------------------------------------------------------------------------ */
/*                       povray/raster3d interface                           */
/*  ------------------------------------------------------------------------ */
void raster3d(const char *filename) {

   graphics_info_t g;
   g.raster3d(std::string(filename));
}

void renderman(const char *filename) {
   graphics_info_t g;
   g.renderman(filename);
}

void povray(const char *filename) {

   graphics_info_t g;
   g.povray(std::string(filename));
}

void set_raster3d_bond_thickness(float f) { 
   graphics_info_t::raster3d_bond_thickness = f;
} 

void set_raster3d_bone_thickness(float f) { 
   graphics_info_t::raster3d_bone_thickness = f;
} 

void set_raster3d_atom_radius(float f) { 

   graphics_info_t::raster3d_atom_radius = f;

} 

void set_raster3d_density_thickness(float f) {

   graphics_info_t::raster3d_density_thickness = f;

} 

void set_raster3d_water_sphere(int state) {

   graphics_info_t::raster3d_water_sphere_flag = state;

} 

void
raster_screen_shot() {  // run raster3d or povray and guile
                		         // script to render and display image

   // do some checking for povray/render here:

// BL says: lets make it for python too:
#ifdef USE_GUILE
   std::string cmd("(render-image)");  // this is a render function 
   
   // cmd = "(povray-image)";

   safe_scheme_command(cmd);

#else
#ifdef USE_PYTHON
   std::string cmd("render_image()");  // this is a render function 
   
   // cmd = "(povray-image)";

   safe_python_command(cmd);
#endif // USE_PYTHON
#endif //USE_GUILE
}

#ifdef USE_PYTHON
void
raster_screen_shot_py() {  // run raster3d or povray and guile
                		         // script to render and display image

   // do some checking for povray/render here:

   std::string cmd("render_image()");  // this is a render function 
   
   // cmd = "(povray-image)";

   safe_python_command(cmd);
}
#endif // USE_PYTHON


void set_renderer_show_atoms(int istate) {

   graphics_info_t::renderer_show_atoms_flag = istate;
}

/*! \brief turn off shadows for raster3d output - give argument 0 to turn off  */
void set_raster3d_shadows_enabled(int state) {
   graphics_info_t::raster3d_enable_shadows = state;
} 



/*  ----------------------------------------------------------------------- */
/*                  browser url                                          */
/*  ----------------------------------------------------------------------- */
void browser_url(const char *url) {

   if (url) { 
      std::string u(url);
      std::vector<std::string> commands;
      commands.push_back("system");
      std::string s = graphics_info_t::browser_open_command;
      if (s == "firefox" || s == "mozilla" || s == "netscape") { 
	 s += " -remote 'openURL(";
	 s += u;
	 s += ",new-window)'";
	 commands.push_back(single_quote(s));
      } else {
	 if (s == "open") {
	    s += " ";
	    s += url;
	 } else {
	    s += " ";
	    s += url;
	 }
	 commands.push_back(single_quote(s));
      }

      std::string c = languagize_command(commands);
#ifdef USE_GUILE
      safe_scheme_command(c);
#else
#ifdef USE_PYTHON
      c = "open_url(";
      c += single_quote(u);
      c += ")";
      safe_python_command(c);
#endif
#endif
   }
}


void set_browser_interface(const char *browser) {

   if (browser) {
      graphics_info_t::browser_open_command = browser;
   } 
} 

void handle_online_coot_search_request(const char *entry_text) {

   if (entry_text) {
      clipper::String text(entry_text);
      std::vector<clipper::String> bits = text.split(" ");
      if (bits.size() > 0) { 
	 std::string s = "http://www.google.co.uk/search?q=";
	 s += bits[0];
	 for (unsigned int i=1; i<bits.size(); i++) {
	    s += "+";
	    s += bits[i];
	 }
	 // s += "+coot+site%3Awww.ysbl.york.ac.uk";
	 s += "+coot+site%3Awww.biop.ox.ac.uk";
	 browser_url(s.c_str());
      }
   } 
} 


/*  ----------------------------------------------------------------------- */
/*                  remote control                                          */
/*  ----------------------------------------------------------------------- */
/* section Remote Control */

// called by c_inner_main() if we have guile
void make_socket_listener_maybe() {


   std::vector<std::string> cmd;

   if (graphics_info_t::try_port_listener) { 
      cmd.push_back("open-coot-listener-socket");
      cmd.push_back(graphics_info_t::int_to_string(graphics_info_t::remote_control_port_number));
      cmd.push_back(single_quote(graphics_info_t::remote_control_hostname));

      // This is not a static function, perhaps it should be:
      graphics_info_t g;
#ifdef USE_GUILE
      std::string scm_command = g.state_command(cmd, coot::STATE_SCM);

      safe_scheme_command(scm_command);
#else
#ifdef USE_PYTHON
      std::string python_command = g.state_command(cmd, coot::STATE_PYTHON);

      safe_python_command(python_command);
#endif // USE_PYTHON
#endif // USE_GUILE
      if (graphics_info_t::coot_socket_listener_idle_function_token == -1)
	 if (graphics_info_t::listener_socket_have_good_socket_state) 
	    graphics_info_t::coot_socket_listener_idle_function_token =
	       gtk_idle_add((GtkFunction) coot_socket_listener_idle_func,
			    graphics_info_t::glarea);
   }
}

void set_coot_listener_socket_state_internal(int sock_state) {
   graphics_info_t::listener_socket_have_good_socket_state = sock_state;
}

void set_remote_control_port(int port_number) {
  graphics_info_t::remote_control_port_number = port_number;
}

int get_remote_control_port_number() {
  return graphics_info_t::remote_control_port_number;
}


int coot_socket_listener_idle_func(GtkWidget *w) { 

#ifdef USE_GUILE
   std::cout << "DEBUG:: running socket idle function" << std::endl;
   if (graphics_info_t::listener_socket_have_good_socket_state) { 
      std::cout << "DEBUG:: running guile function" << std::endl;
      safe_scheme_command("(coot-listener-idle-function-proc)");
   }
#else
#ifdef USE_PYTHON
   //   std::cout << "DEBUG:: running socket idle function" << std::endl;
   if (graphics_info_t::listener_socket_have_good_socket_state) { 
      //std::cout << "DEBUG:: running python function" << std::endl;
      safe_python_command("coot_listener_idle_function_proc()");
   }
#endif // USE_PYTHON
#endif // USE_GUILE
   return 1;
}

/* tooltips */
void
set_tip_of_the_day_flag (int state) {
   graphics_info_t g;
   if (state == 0) {
	g.do_tip_of_the_day_flag = 0;
   } else {
	g.do_tip_of_the_day_flag = 1;
   }
}

/*  ----------------------------------------------------------------------- */
/*                  Surfaces                                                */
/*  ----------------------------------------------------------------------- */
void do_surface(int imol, int state) {

   float colour_scale = graphics_info_t::electrostatic_surface_charge_range; // 0.5, by default
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      graphics_info_t::molecules[imol].make_surface(state, *g.Geom_p(), colour_scale);
      graphics_draw();
   }
}

int molecule_is_drawn_as_surface_int(int imol) {

   int r = 0;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].molecule_is_drawn_as_surface();
   }
   return r;
} 


#ifdef USE_GUILE
void do_clipped_surface_scm(int imol, SCM residues_specs) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::residue_spec_t> res_specs_vec = scm_to_residue_specs(residues_specs);
      float col_scale = g.electrostatic_surface_charge_range;
      graphics_info_t::molecules[imol].make_surface(res_specs_vec, *g.Geom_p(), col_scale);
      graphics_draw();
   } 
}
#endif //USE_GUILE

#ifdef USE_PYTHON
void do_clipped_surface_py(int imol, PyObject *residues_specs) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::residue_spec_t> res_specs_vec = py_to_residue_specs(residues_specs);
      float col_scale = g.electrostatic_surface_charge_range;
      graphics_info_t::molecules[imol].make_surface(res_specs_vec, *g.Geom_p(), col_scale);
      graphics_draw();
   } 
}
#endif //USE_PYTHON

void set_electrostatic_surface_charge_range(float v) {
   graphics_info_t::electrostatic_surface_charge_range = v;
} 

float get_electrostatic_surface_charge_range() {
   return graphics_info_t::electrostatic_surface_charge_range;
} 

/*  ----------------------------------------------------------------------- */
/*           Sharpen                                                        */
/*  ----------------------------------------------------------------------- */
void sharpen(int imol, float b_factor) {

   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].sharpen(b_factor, false, 0);
      graphics_draw();
   }
}

void sharpen_with_gompertz_scaling(int imol, float b_factor,
				   short int do_gompertz_scaling_flag,
				   float gompertz_factor) {

   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].sharpen(b_factor, do_gompertz_scaling_flag,
					       gompertz_factor);
      graphics_draw();
   }
}


/*  ----------------------------------------------------------------------- */
/*                  Views                                                   */
/*  ----------------------------------------------------------------------- */
int add_view_here(const char *view_name) {

   std::string name(view_name);
   float quat[4];
   for (int i=0; i<4; i++)
      quat[i] = graphics_info_t::quat[i];
   graphics_info_t g;
   coot::Cartesian rc = g.RotationCentre();
   float zoom = graphics_info_t::zoom;
   coot::view_info_t view(quat, rc, zoom, name);
   graphics_info_t::views->push_back(view);
   return (graphics_info_t::views->size() -1);
}

int add_view_raw(float rcx, float rcy, float rcz, float quat0, float quat1, 
		  float quat2, float quat3, float zoom, const char *view_name) {

   float quat[4];
   quat[0] = quat0;
   quat[1] = quat1;
   quat[2] = quat2;
   quat[3] = quat3;
   coot::Cartesian rc(rcx, rcy, rcz);
   coot::view_info_t v(quat, rc, zoom, view_name);
   graphics_info_t::views->push_back(v);
   return (graphics_info_t::views->size() -1);
}


int remove_named_view(const char *view_name) {

   int r=0;
   bool found = 0;
   std::string vn(view_name);
   std::vector<coot::view_info_t> new_views;

   // don't push back the view if it has the same name and if we
   // haven't found a view with that name before.
   for (unsigned int iv=0; iv<graphics_info_t::views->size(); iv++) {
      if ((*graphics_info_t::views)[iv].view_name == vn) {
	 if (found == 1) {
	    new_views.push_back((*graphics_info_t::views)[iv]);
	 }
	 found = 1;
      } else {
	 new_views.push_back((*graphics_info_t::views)[iv]);
      }
   }
   if (found) {
      r = 1;
      *graphics_info_t::views = new_views;
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("remove_named_view");
   command_strings.push_back(single_quote(coot::util::intelligent_debackslash(view_name)));
   add_to_history(command_strings);
   return r;
} 

/*! \brief the given view number */
void remove_view(int view_number) {

   std::vector<coot::view_info_t> new_views;
   for (unsigned int iv=0; iv<graphics_info_t::views->size(); iv++) {
      if (int(iv) != view_number)
	 new_views.push_back((*graphics_info_t::views)[iv]);
   }
   *graphics_info_t::views = new_views;

   std::string cmd = "remove-view";
   std::vector<coot::command_arg_t> args;
   args.push_back(view_number);
   add_to_history_typed(cmd, args);
}


void play_views() {

   int nsteps = 2000;
   if (graphics_info_t::views_play_speed > 0.000000001)
      nsteps = int(2000.0/graphics_info_t::views_play_speed);
   float play_speed = 1.0; 
   if (graphics_info_t::views_play_speed > 0.0)
      play_speed = graphics_info_t::views_play_speed;
   
//    std::cout << "DEBUG:: # Views "<< graphics_info_t::views->size() << std::endl;
   for (unsigned int iv=0; iv<graphics_info_t::views->size(); iv++) {
      coot::view_info_t view1 = (*graphics_info_t::views)[iv];
      // std::cout << "DEBUG:: View "<< iv << " " << view1.view_name << std::endl;
      if (! (view1.is_simple_spin_view_flag ||
	     view1.is_action_view_flag)) {
	 if ((iv+1) < graphics_info_t::views->size()) { 
// 	    std::cout << "DEBUG:: interpolating to  "<< iv+1 << " "
// 		      << view1.view_name << std::endl;
	    coot::view_info_t view2 = (*graphics_info_t::views)[iv+1];
	    if (! (view2.is_simple_spin_view_flag ||
		   view2.is_action_view_flag)) { 
	       coot::view_info_t::interpolate(view1, view2, nsteps);
	       update_things_on_move_and_redraw();
	    }
	 }
      } else {
	 // a simple spin  or an action view here:
// 	    std::cout << "DEBUG:: simple spin "
// 		      << view1.view_name << std::endl;
	 int n_spin_steps = int (float (view1.n_spin_steps) / play_speed);
	 float dps = view1.degrees_per_step*0.5 * play_speed;
	 rotate_y_scene(n_spin_steps, dps);
	 if ((iv+1) < graphics_info_t::views->size()) { 
 	    std::cout << "DEBUG:: interpolating to  "<< iv+1 << " "
 		      << view1.view_name << std::endl;
	    coot::view_info_t view2 = (*graphics_info_t::views)[iv+1];
	    if (!view2.is_simple_spin_view_flag && !view2.is_action_view_flag) {
	       // the quat was not set because this is a simple
	       // rotate, so we must generate it from the current
	       // position
	       coot::Cartesian rc(graphics_info_t::RotationCentre_x(),
				  graphics_info_t::RotationCentre_y(),
				  graphics_info_t::RotationCentre_z());
	       coot::view_info_t current_view(graphics_info_t::quat,
					      rc, graphics_info_t::zoom, "dummy");
	       coot::view_info_t::interpolate(current_view, view2, nsteps);
	       update_things_on_move_and_redraw();
	    }
	 }
      }
   }
   add_to_history_simple("play-views");
}

void remove_this_view() {

   graphics_info_t g;
   coot::Cartesian rc = g.RotationCentre();
   float quat[4];
   for (int i=0; i<4; i++)
      quat[i] = graphics_info_t::quat[i];
   float zoom =  g.zoom;

   int r=0;
   bool found = 0;
   coot::view_info_t v(quat, rc, zoom, "");

   // don't push back the view if it has the same name and if we
   // haven't found a view with that name before.
   std::vector<coot::view_info_t> new_views;
   for (unsigned int iv=0; iv<graphics_info_t::views->size(); iv++) {
      if ((*graphics_info_t::views)[iv].matches_view(v)) {
	 if (found == 1) {
	    new_views.push_back((*graphics_info_t::views)[iv]);
	 }
      } else {
	 new_views.push_back((*graphics_info_t::views)[iv]);
      }
   }
   if (found) {
      r = 1;
      *graphics_info_t::views = new_views;
   }
   add_to_history_simple("remove-this-view");
}


int go_to_first_view(int snap_to_view_flag) {

   std::string cmd = "go-to-first-view";
   std::vector<coot::command_arg_t> args;
   args.push_back(snap_to_view_flag);
   add_to_history_typed(cmd, args);
   return go_to_view_number(0, snap_to_view_flag);
}

void clear_all_views() {
   graphics_info_t::views->clear();
}

int go_to_view_number(int view_number, int snap_to_view_flag) {

   int r = 0;
   graphics_info_t g;
   if ((int(graphics_info_t::views->size()) > view_number) && (view_number >= 0)) {
      coot::view_info_t view = (*graphics_info_t::views)[view_number];
      if (view.is_simple_spin_view_flag) {
	 int nsteps = 2000;
	 if (graphics_info_t::views_play_speed > 0.000000001)
	    nsteps = int(2000.0/graphics_info_t::views_play_speed);
	 float play_speed = 1.0; 
	 if (graphics_info_t::views_play_speed > 0.0)
	    play_speed = graphics_info_t::views_play_speed;
	 int n_spin_steps = int (float (view.n_spin_steps) / play_speed);
	 float dps = view.degrees_per_step*0.5 * play_speed;
	 rotate_y_scene(n_spin_steps, dps);
      } else {
	 if (view.is_action_view_flag) {
	    // do nothing
	 } else {
	    if (snap_to_view_flag) {
	       g.setRotationCentre(view.rotation_centre);
	       g.zoom = view.zoom;
	       for (int iq=0; iq<4; iq++)
		  g.quat[iq] = view.quat[iq];
	    } else {
	       coot::view_info_t this_view(g.quat, g.RotationCentre(), g.zoom, "");
	       int nsteps = 2000;
	       if (graphics_info_t::views_play_speed > 0.000000001)
		  nsteps = int(2000.0/graphics_info_t::views_play_speed);
	       coot::view_info_t::interpolate(this_view,
					      (*graphics_info_t::views)[view_number], nsteps);
	    }
	 }
	 update_things_on_move_and_redraw();
      }
   }
   std::string cmd = "go-to-view-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(view_number);
   args.push_back(snap_to_view_flag);
   add_to_history_typed(cmd, args);
   return r;
}


/*! \brief return the number of views */
int n_views() {
   add_to_history_simple("n-views");
   return graphics_info_t::views->size();
}

/*! \brief return the name of the given view, if view_number does not
  specify a view return #f */
#ifdef USE_GUILE
SCM view_name(int view_number) {

   SCM r = SCM_BOOL_F;
   int n_view = graphics_info_t::views->size();
   if (view_number < n_view)
      if (view_number >= 0) {
	 std::string name = (*graphics_info_t::views)[view_number].view_name;
	 r = scm_makfrom0str(name.c_str());
      }
   return r;
}
#endif	/* USE_GUILE */
#ifdef USE_PYTHON
PyObject *view_name_py(int view_number) {

   PyObject *r;
   r = Py_False;
   int n_view = graphics_info_t::views->size();
   if (view_number < n_view)
      if (view_number >= 0) {
         std::string name = (*graphics_info_t::views)[view_number].view_name;
         r = PyString_FromString(name.c_str());
      }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}

#endif // PYTHON

#ifdef USE_GUILE
SCM view_description(int view_number) { 

   SCM r = SCM_BOOL_F;
   if (view_number >= 0)
      if (view_number < int(graphics_info_t::views->size())) {
	 std::string d = (*graphics_info_t::views)[view_number].description;
	 if (d != "") {
	    r = scm_makfrom0str(d.c_str());
	 }
      }
   return r;
}
#endif	/* USE_GUILE */
#ifdef USE_PYTHON
PyObject *view_description_py(int view_number) {

   PyObject *r;
   r = Py_False;
   if (view_number >= 0)
      if (view_number < int(graphics_info_t::views->size())) {
         std::string d = (*graphics_info_t::views)[view_number].description;
         if (d != "") {
            r = PyString_FromString(d.c_str());
         }
      }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // PYTHON


/*! \brief save views to view_file_name */
void save_views(const char *view_file_name) {

   unsigned int n_views = graphics_info_t::views->size();
   if (n_views > 0) {
      std::ofstream f(view_file_name);
      if (! f) {
	 std::cout << "Cannot open view output file"
		   << view_file_name << std::endl;
      } else {
#ifdef USE_GUILE
	 f << "; Views\n";
#else
#ifdef USE_PYTHON
	 f << "# Views\n";
#endif // PYTHON
#endif // GUILE
	 for (unsigned int i=0; i<n_views; i++) {
	    f << (*graphics_info_t::views)[i];
	 }
	 std::string s = "Views written to file ";
	 s += view_file_name;
	 add_status_bar_text(s.c_str());
	 // "ching!" sound here.
      }
   } else {					
      std::cout << "no views to save" << std::endl;
   }
}

// return the view number
int add_action_view(const char *view_name, const char *action_function) {
   std::string name(view_name);
   std::string func(action_function);
   coot::view_info_t view(name, func);  // an action view
   graphics_info_t::views->push_back(view);
   return (graphics_info_t::views->size() -1);
}

// return the view number of the new view
int insert_action_view_after_view(int view_number, const char *view_name, const char *action_function) {
   int r = -1; 
   std::string name(view_name);
   std::string func(action_function);
   coot::view_info_t view(name, func);  // an action view
   int n_views = graphics_info_t::views->size(); 
   if (view_number >= n_views) {
      graphics_info_t::views->push_back(view);
      r = (graphics_info_t::views->size() -1);
   } else {
      // insert a view
      std::vector <coot::view_info_t> new_views;
      for (int iview=0; iview<n_views; iview++) {
	 new_views.push_back((*graphics_info_t::views)[iview]);
	 if (iview == view_number)
	    new_views.push_back(view);
      }
      *graphics_info_t::views = new_views; // bleuch.
      r = view_number + 1;
   }
   return r;
}

void add_view_description(int view_number, const char *descr) {

   if (view_number < int(graphics_info_t::views->size()))
      if (view_number >= 0)
	 (*graphics_info_t::views)[view_number].add_description(descr);
}

#ifdef USE_GUILE
void go_to_view(SCM view) {
   
   SCM len_view_scm = scm_length(view);
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)      
   int len_view = scm_to_int(len_view_scm);
#else    
   int len_view = gh_scm2int(len_view_scm);
#endif
   
   if (len_view == 4) { 

      graphics_info_t g;
      int nsteps = 2000;
      if (graphics_info_t::views_play_speed > 0.000000001)
	 nsteps = int(2000.0/graphics_info_t::views_play_speed);
   
      // What is the current view:
      // 
      std::string name("Current Position");
      float quat[4];
      for (int i=0; i<4; i++)
	 quat[i] = graphics_info_t::quat[i];
      coot::Cartesian rc = g.RotationCentre();
      float zoom = graphics_info_t::zoom;
      coot::view_info_t view_c(quat, rc, zoom, name);


      // view_target is where we want to go
      float quat_target[4];
      SCM quat_scm = scm_list_ref(view, SCM_MAKINUM(0));
      SCM len_quat_scm = scm_length(quat_scm);
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
      int len_quat = scm_to_int(len_quat_scm);
#else
      int len_quat = gh_scm2int(len_quat_scm);
#endif       
      if (len_quat == 4) { 
	 SCM q0_scm = scm_list_ref(quat_scm, SCM_MAKINUM(0));
	 SCM q1_scm = scm_list_ref(quat_scm, SCM_MAKINUM(1));
	 SCM q2_scm = scm_list_ref(quat_scm, SCM_MAKINUM(2));
	 SCM q3_scm = scm_list_ref(quat_scm, SCM_MAKINUM(3));
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
	 quat_target[0] = scm_to_double(q0_scm);
	 quat_target[1] = scm_to_double(q1_scm);
	 quat_target[2] = scm_to_double(q2_scm);
	 quat_target[3] = scm_to_double(q3_scm);
#else
	 quat_target[0] = gh_scm2double(q0_scm);
	 quat_target[1] = gh_scm2double(q1_scm);
	 quat_target[2] = gh_scm2double(q2_scm);
	 quat_target[3] = gh_scm2double(q3_scm);
#endif 	 

	 SCM rc_target_scm = scm_list_ref(view, SCM_MAKINUM(1));
	 SCM len_rc_target_scm = scm_length(rc_target_scm);
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
	 int len_rc_target = scm_to_int(len_rc_target_scm);
#else	 
	 int len_rc_target = gh_scm2int(len_rc_target_scm);
#endif 	 
	 if (len_rc_target == 3) {

	    SCM centre_x = scm_list_ref(rc_target_scm, SCM_MAKINUM(0));
	    SCM centre_y = scm_list_ref(rc_target_scm, SCM_MAKINUM(1));
	    SCM centre_z = scm_list_ref(rc_target_scm, SCM_MAKINUM(2));

#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
	    double x = scm_to_double(centre_x);
	    double y = scm_to_double(centre_y);
	    double z = scm_to_double(centre_z);
#else
	    double x = gh_scm2double(centre_x);
	    double y = gh_scm2double(centre_y);
	    double z = gh_scm2double(centre_z);
#endif 	    
	    coot::Cartesian rc_target(x,y,z);

	    SCM target_zoom_scm = scm_list_ref(view, SCM_MAKINUM(2));
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
	    double zoom_target = scm_to_double(target_zoom_scm);
#else 	    
	    double zoom_target = gh_scm2double(target_zoom_scm);
#endif 	    

	    SCM name_target_scm = scm_list_ref(view, SCM_MAKINUM(3));
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
	    std::string name_target = scm_to_locale_string(name_target_scm);
#else
	    std::string name_target = SCM_STRING_CHARS(name_target_scm);
#endif
	       
	    coot::view_info_t view_target(quat_target, rc_target, zoom_target, name_target);
	    
	    // do the animation
	    coot::view_info_t::interpolate(view_c, view_target, nsteps);
	 } else {
	    std::cout << "WARNING:: bad centre in view" << std::endl;
	 }
      } else {
	 std::cout << "WARNING:: bad quat in view" << std::endl;
      } 
   }
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
void go_to_view_py(PyObject *view) {
   
   int len_view;

   len_view = PyObject_Length(view);
   
   if (len_view == 4) { 

      PyObject *quat_python;
      graphics_info_t g;
      int nsteps = 2000;
      if (graphics_info_t::views_play_speed > 0.000000001)
         nsteps = int(2000.0/graphics_info_t::views_play_speed);
   
      // What is the current view:
      // 
      std::string name("Current Position");
      float quat[4];
      for (int i=0; i<4; i++)
         quat[i] = graphics_info_t::quat[i];
      coot::Cartesian rc = g.RotationCentre();
      float zoom = graphics_info_t::zoom;
      coot::view_info_t view_c(quat, rc, zoom, name);

      // view_target is where we want to go
      float quat_target[4];
      quat_python = PyList_GetItem(view, 0);
      int len_quat = PyObject_Length(quat_python);
      if (len_quat == 4) { 
         PyObject *q0_python = PyList_GetItem(quat_python, 0);
         PyObject *q1_python = PyList_GetItem(quat_python, 1);
         PyObject *q2_python = PyList_GetItem(quat_python, 2);
         PyObject *q3_python = PyList_GetItem(quat_python, 3);
         quat_target[0] = PyFloat_AsDouble(q0_python);
         quat_target[1] = PyFloat_AsDouble(q1_python);
         quat_target[2] = PyFloat_AsDouble(q2_python);
         quat_target[3] = PyFloat_AsDouble(q3_python);

         PyObject *rc_target_python = PyList_GetItem(view, 1);
         int len_rc_target = PyObject_Length(rc_target_python);
         if (len_rc_target == 3) {

            PyObject *centre_x = PyList_GetItem(rc_target_python, 0);
            PyObject *centre_y = PyList_GetItem(rc_target_python, 1);
            PyObject *centre_z = PyList_GetItem(rc_target_python, 2);

            double x = PyFloat_AsDouble(centre_x);
            double y = PyFloat_AsDouble(centre_y);
            double z = PyFloat_AsDouble(centre_z);
            coot::Cartesian rc_target(x,y,z);

            PyObject *target_zoom_python = PyList_GetItem(view, 2);
            double zoom_target = PyFloat_AsDouble(target_zoom_python);

            PyObject *name_target_python = PyList_GetItem(view, 3);
            std::string name_target = PyString_AsString(name_target_python);
               
            coot::view_info_t view_target(quat_target, rc_target, zoom_target, name_target);
            
            // do the animation
            coot::view_info_t::interpolate(view_c, view_target, nsteps);
         } else {
            std::cout << "WARNING:: bad centre in view" << std::endl;
         }
      } else {
         std::cout << "WARNING:: bad quat in view" << std::endl;
      }
   }
} 
#endif // PYTHON


int add_spin_view(const char *view_name, int n_steps, float degrees_total) {

   coot::view_info_t v(view_name, n_steps, degrees_total);
   graphics_info_t::views->push_back(v);
   std::string cmd = "add-spin-view";
   std::vector<coot::command_arg_t> args;
   args.push_back(view_name);
   args.push_back(n_steps);
   args.push_back(degrees_total);
   add_to_history_typed(cmd, args);
   return (graphics_info_t::views->size() -1);
}

void set_views_play_speed(float f) {
   graphics_info_t::views_play_speed = f;
   std::string cmd = "set-views-play-speed";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);

} 

float views_play_speed() {
   return graphics_info_t::views_play_speed;
   add_to_history_simple("views-play-speed");
}


/*  ----------------------------------------------------------------------- */
/*                  remote control                                          */
/*  ----------------------------------------------------------------------- */

void set_socket_string_waiting(const char *s) {

   // wait for lock:
   while (graphics_info_t::socket_string_waiting_mutex_lock != 0) {
      std::cout << "Waiting for lock! "
		<< graphics_info_t::socket_string_waiting_mutex_lock
		<< std::endl;
      usleep(1000000);
   }
   
   std::cout << " =============== setting mutex lock =========" << std::endl;
   // 
   // (This mutex lock *and* waiting flag may be overly complex now
   // that we simply use g_idle_add())
   graphics_info_t::socket_string_waiting_mutex_lock = 1;
   graphics_info_t::socket_string_waiting = s;
   graphics_info_t::have_socket_string_waiting_flag = 1;

   std::cout << "DEBUG:: set_socket_string_waiting() socket_string_waiting set to " 
	     << graphics_info_t::socket_string_waiting << std::endl;

   GSourceFunc f = graphics_info_t::process_socket_string_waiting_bool;
   g_idle_add(f, NULL); // if f returns FALSE then f is not called again.

   

   // old way, generates a Xlib async error sometimes?   
//       gtk_widget_queue_draw_area(graphics_info_t::glarea, 0, 0,
// 				 graphics_info_t::glarea->allocation.width,
// 				 graphics_info_t::glarea->allocation.height);
      
//    std::cout << "INFO:: ---- set_socket_string_waiting set to :"
// 	     << graphics_info_t::socket_string_waiting
// 	     << ":" << std::endl;

// another old way:
    //   gint return_val;
   //   GdkEventExpose event;
   //    gtk_signal_emit_by_name(GTK_OBJECT(graphics_info_t::glarea),
   //                           "configure_event",
   // 			   &event, &return_val);
   
}


// should be in c-interface-maps?

void set_map_sharpening_scale_limit(float f) {
   graphics_info_t::map_sharpening_scale_limit = f;
} 


// should be in c-interface-maps?
// 
/*! \brief sets the density map of the given molecule to be drawn as a
  (transparent) solid surface. */
void set_draw_solid_density_surface(int imol, short int state) {

   if (is_valid_map_molecule(imol)) { 
      graphics_info_t::molecules[imol].set_draw_solid_density_surface(state);
      graphics_draw();
   }
}

void
set_solid_density_surface_opacity(int imol, float opacity) {

   if (is_valid_map_molecule(imol)) {
      graphics_info_t::molecules[imol].density_surface_opacity = opacity;
      graphics_draw();
   }
}

void
set_flat_shading_for_solid_density_surface(short int state) {
   graphics_info_t::do_flat_shading_for_solid_density_surface = state;
   graphics_draw();
}

/*! \brief simple on/off screendoor transparency at the moment, an
  opacity > 0.0 will turn on screendoor transparency (stippling). */
void set_transparent_electrostatic_surface(int imol, float opacity) {

   if (is_valid_model_molecule(imol)) {
      bool flag = 0;
      if (opacity > 0.0)
	 if (opacity < 0.9999)
	    flag = 1;
      graphics_info_t::molecules[imol].transparent_molecular_surface_flag = flag;
      graphics_draw();
   } 

} 

/*! \brief return 1.0 for non transparent and 0.5 if screendoor
  transparency has been turned on. */
float get_electrostatic_surface_opacity(int imol) {

   float r = -1;
   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].transparent_molecular_surface_flag == 1)
	 r = 0.5;
      else
	 r = 1.0;
   }
   return r;
} 

