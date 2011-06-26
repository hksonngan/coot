/* src/main.cc
 * 
 * Copyright 2003, 2004, 2005, 2007 The University of York
 * Author: Paul Emsley, Bernhard Lohkamp
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


/*  ----------------------------------------------------------------------- */
/*                  Superpose                                               */
/*  ----------------------------------------------------------------------- */

#if defined _MSC_VER
#include <windows.h>
#endif

// lsq uses this:
#include "graphics-info.h"

#ifdef HAVE_SSMLIB
#include <string>
#include <vector>
#include <gtk/gtk.h> 
#include "c-interface.h"
#include "cc-interface.hh"
#ifdef USE_PYTHON
#include "Python.h"
#endif

#include "ssm_align.h"
#include <iostream>
#include "interface.h"

#else 

#include <gtk/gtk.h>
#include "interface.h"
#include <string>
#include <vector>
#include "c-interface.h"
#include "cc-interface.hh"
#endif // HAVE_SSMLIB

#include "guile-fixups.h"

void superpose(int imol1, int imol2, short int move_copy_of_imol2_flag) { 

#ifdef HAVE_SSMLIB

   std::cout << "superposing molecule " << imol2 << " on to " << imol1  
	     << " (reference)\n";

   if (graphics_info_t::molecules[imol1].has_model()) { 
      if (graphics_info_t::molecules[imol2].has_model()) { 
	 
	 graphics_info_t g;
	 std::string name = graphics_info_t::molecules[imol2].name_for_display_manager();
	 std::string reference_name = graphics_info_t::molecules[imol1].name_for_display_manager();
	 int imol_new = g.superpose_with_atom_selection(graphics_info_t::molecules[imol1].atom_sel,
							graphics_info_t::molecules[imol2].atom_sel,
							imol2, name, reference_name, move_copy_of_imol2_flag);

	 if (is_valid_model_molecule(imol_new)) {
	    // now move the cryst of mol2 to be the same as the cryst of mol1.
	    CMMDBManager *m1 = graphics_info_t::molecules[imol1].atom_sel.mol;
	    CMMDBManager *m2 = graphics_info_t::molecules[imol_new].atom_sel.mol;

	    bool success = coot::util::copy_cell_and_symm_headers(m1, m2);
	    
	 } 

      } else {
	 std::cout << "WARNING:: Molecule " << imol2 << " has no model\n";
      }
   } else {
      std::cout << "WARNING:: Molecule " << imol1 << " has no model\n";
   }

   std::vector<std::string> command_strings;
   command_strings.push_back("superpose");
   command_strings.push_back(graphics_info_t::int_to_string(imol1));
   command_strings.push_back(graphics_info_t::int_to_string(imol2));
   command_strings.push_back(graphics_info_t::int_to_string(move_copy_of_imol2_flag));
   add_to_history(command_strings);
      
#endif // HAVE_SSMLIB
}


void superpose_with_chain_selection(int imol1, int imol2, 
				    const char *chain_imol1,
				    const char *chain_imol2,
				    int chain_used_flag_imol1,
				    int chain_used_flag_imol2,
				    short int move_imol2_flag) {

#ifdef HAVE_SSMLIB

   if (is_valid_model_molecule(imol1)) {
      if (is_valid_model_molecule(imol2)) { 

	 atom_selection_container_t asc_ref = graphics_info_t::molecules[imol1].atom_sel;
	 atom_selection_container_t asc_mov = graphics_info_t::molecules[imol2].atom_sel;
	 std::string name     = graphics_info_t::molecules[imol2].name_for_display_manager();
	 std::string ref_name = graphics_info_t::molecules[imol1].name_for_display_manager();
	 graphics_info_t g;

	 if (chain_used_flag_imol1) {
	    asc_ref.SelectionHandle = asc_ref.mol->NewSelection();
	    asc_ref.mol->SelectAtoms(asc_ref.SelectionHandle, 0,
				     (char *) chain_imol1,
				     ANY_RES, "*",
				     ANY_RES, "*",
				     "*", "*", "*", "*");
	    asc_ref.atom_selection = NULL;
	    asc_ref.mol->GetSelIndex(asc_ref.SelectionHandle,
				     asc_ref.atom_selection, asc_ref.n_selected_atoms);
	    ref_name += " Chain ";
	    ref_name += chain_imol1;
	 }
	 if (chain_used_flag_imol2) {
	    asc_mov.SelectionHandle = asc_mov.mol->NewSelection();
	    asc_mov.mol->SelectAtoms(asc_mov.SelectionHandle, 0,
				     (char *) chain_imol2,
				     ANY_RES, "*",
				     ANY_RES, "*",
				     "*", "*", "*", "*");
	    asc_mov.atom_selection = NULL;
	    asc_mov.mol->GetSelIndex(asc_mov.SelectionHandle,
				     asc_mov.atom_selection, asc_mov.n_selected_atoms);
	    name += " Chain ";
	    name += chain_imol2;
	 }

	 int imol_new = g.superpose_with_atom_selection(asc_ref, asc_mov, imol2, name,
							ref_name, move_imol2_flag);
	 if (chain_used_flag_imol1)
	    asc_ref.mol->DeleteSelection(asc_ref.SelectionHandle);
	 if (chain_used_flag_imol2)
	    asc_mov.mol->DeleteSelection(asc_mov.SelectionHandle);

	 if (is_valid_model_molecule(imol_new)) {
	    // now move the cryst of mol2 to be the same as the cryst of mol1.

	    CMMDBManager *m1 = graphics_info_t::molecules[imol1].atom_sel.mol;
	    CMMDBManager *m2 = graphics_info_t::molecules[imol_new].atom_sel.mol;
	    bool success = coot::util::copy_cell_and_symm_headers(m1, m2);
	 } 
	 
      }
   }
   std::vector<std::string> command_strings;
   std::string chain_imol1_str = "";
   std::string chain_imol2_str = "";
   if (chain_imol1)
      chain_imol1_str = chain_imol1;
   if (chain_imol2)
      chain_imol2_str = chain_imol2;
   command_strings.push_back("superpose-with-chain-selection");
   command_strings.push_back(graphics_info_t::int_to_string(imol1));
   command_strings.push_back(graphics_info_t::int_to_string(imol2));
   command_strings.push_back(single_quote(chain_imol1_str));
   command_strings.push_back(single_quote(chain_imol2_str));
   command_strings.push_back(graphics_info_t::int_to_string(chain_used_flag_imol1));
   command_strings.push_back(graphics_info_t::int_to_string(chain_used_flag_imol2));
   command_strings.push_back(graphics_info_t::int_to_string(move_imol2_flag));
   add_to_history(command_strings);
#endif // HAVE_SSMLIB

}


int superpose_with_atom_selection(int imol1, int imol2,
				   const char *mmdb_atom_sel_str_1, 
				   const char *mmdb_atom_sel_str_2,
				   short int move_copy_of_imol2_flag) {
   int imodel_return = -1; 

#ifdef HAVE_SSMLIB

   if (is_valid_model_molecule(imol1)) {
      if (is_valid_model_molecule(imol2)) {

	 graphics_info_t g;
	 atom_selection_container_t asc_ref = graphics_info_t::molecules[imol1].atom_sel;
	 asc_ref.SelectionHandle = asc_ref.mol->NewSelection();
	 asc_ref.mol->Select(asc_ref.SelectionHandle, STYPE_ATOM,
			     (char *)mmdb_atom_sel_str_1, SKEY_NEW);
	 asc_ref.atom_selection = NULL;
	 asc_ref.mol->GetSelIndex(asc_ref.SelectionHandle,
				  asc_ref.atom_selection, asc_ref.n_selected_atoms);

	 atom_selection_container_t asc_mov = graphics_info_t::molecules[imol2].atom_sel;
	 asc_mov.SelectionHandle = asc_mov.mol->NewSelection();
	 asc_mov.mol->Select(asc_mov.SelectionHandle, STYPE_ATOM,
			     (char *)mmdb_atom_sel_str_2, SKEY_NEW);
	 asc_mov.atom_selection = NULL;
	 asc_mov.mol->GetSelIndex(asc_mov.SelectionHandle,
				  asc_mov.atom_selection, asc_mov.n_selected_atoms);

	 std::cout << "INFO:: reference " << imol1 << " has "
		   << asc_ref.n_selected_atoms << " atoms selected\n";
	 std::cout << "INFO:: moving    " << imol2 << " has " 
		   << asc_mov.n_selected_atoms << " atoms selected\n";

	 std::string name     = graphics_info_t::molecules[imol2].name_for_display_manager();
	 std::string ref_name = graphics_info_t::molecules[imol1].name_for_display_manager();
	 imodel_return = g.superpose_with_atom_selection(asc_ref, asc_mov, imol2, name,
							 ref_name, move_copy_of_imol2_flag);
	 
	 asc_ref.mol->DeleteSelection(asc_ref.SelectionHandle);
	 asc_mov.mol->DeleteSelection(asc_mov.SelectionHandle);

	 // now move the cryst of mol2 to be the same as the cryst of mol1.
	 realtype a[6];
	 realtype vol;
	 int orthcode;
	 CMMDBManager *m1 = graphics_info_t::molecules[imol1].atom_sel.mol;
	 CMMDBManager *m2 = graphics_info_t::molecules[imol2].atom_sel.mol;

	 bool success = coot::util::copy_cell_and_symm_headers(m1, m2);
      }
   }
#endif // HAVE_SSMLIB

   return imodel_return;
}



void execute_superpose(GtkWidget *w) { 

#ifdef HAVE_SSMLIB

   // We need to extract which molecules are to be superposed:

   int imol1 = graphics_info_t::superpose_imol1;
   int imol2 = graphics_info_t::superpose_imol2;

//    std::cout << "DEBUG:: superpose_imol1: " << graphics_info_t::superpose_imol1 << std::endl;
//    std::cout << "DEBUG:: superpose_imol2: " << graphics_info_t::superpose_imol2 << std::endl;


   short int make_copy;

   GtkWidget *checkbutton = lookup_widget(w, "superpose_dialog_move_copy_checkbutton");
   GtkWidget *chain_mol1_checkbutton =  lookup_widget(w, "superpose_reference_chain_checkbutton");
   GtkWidget *chain_mol2_checkbutton =  lookup_widget(w, "superpose_moving_chain_checkbutton");

   if (GTK_TOGGLE_BUTTON(checkbutton)->active)
      make_copy = 1;
   else 
      make_copy = 0;
   
   
   if (imol1 >= 0 && imol1 < graphics_info_t::n_molecules()) { 
      if (imol2 >= 0 && imol2 < graphics_info_t::n_molecules()) {

	 // now check the chains:
	 //
	 int chain_used_flag_imol1 = 0;
	 int chain_used_flag_imol2 = 0;
	 std::string chain_mol1 = "empty";
	 std::string chain_mol2 = "empty";

	 // These chain_mol1/2 need checking to see if they are set
	 // properly when the use-chain check_button is toggled.
	 // 
	 if (GTK_TOGGLE_BUTTON(chain_mol1_checkbutton)->active) {
	    chain_used_flag_imol1 = 1;
	    chain_mol1 = graphics_info_t::superpose_imol1_chain;
	 }
	 if (GTK_TOGGLE_BUTTON(chain_mol2_checkbutton)->active) {
	    chain_used_flag_imol2 = 1;
	    chain_mol2 = graphics_info_t::superpose_imol2_chain;
	 }

	 // Make sure we are not superposing onto self:
	 short int self_superpose_flag = 0;
	 if (imol1 == imol2) {
	    if (!chain_used_flag_imol1 && !chain_used_flag_imol2) {
	       self_superpose_flag = 1;
	    } else {
	       if (!make_copy) { 
		  self_superpose_flag = 1;
	       } else { 
		  if (chain_used_flag_imol1 && chain_used_flag_imol2) {
		     if (chain_mol1 == chain_mol2) {
			self_superpose_flag = 1;
		     }
		  }
	       }
	    }
	 }

	 if (self_superpose_flag == 1)
	    make_copy = 1;
	    
	 std::string mol1chain_info;
	 std::string mol2chain_info;
	 if (chain_used_flag_imol1) { 
	       mol1chain_info = " Chain " + chain_mol1;
	 }
	 if (chain_used_flag_imol2) { 
	    mol2chain_info = " Chain " + chain_mol2;
	 }
	 
	 std::cout << "INFO:: matching molecule number " << imol2 << mol2chain_info
		   << " onto molecule number "
		   << imol1 << mol1chain_info << std::endl;
	 superpose_with_chain_selection(imol1, imol2, chain_mol1.c_str(), chain_mol2.c_str(),
					chain_used_flag_imol1, chain_used_flag_imol2,
					make_copy);

      } else { 
	 std::cout << "No such molecule as " << imol2 << "\n";
      } 
   } else { 
      std::cout << "No such molecule as " << imol1 << "\n";
   }
#endif // HAVE_SSMLIB
} 


GtkWidget *wrapped_create_superpose_dialog() { 

   GtkWidget *w = 0; // not NULL for compiler reasons.

#ifdef HAVE_SSMLIB

   w = create_superpose_dialog();

   graphics_info_t g;

   GtkWidget *optionmenu1 = lookup_widget(w, "superpose_dialog_reference_mol_optionmenu");
   GtkWidget *optionmenu2 = lookup_widget(w, "superpose_dialog_moving_mol_optionmenu");

   GtkSignalFunc signal_func1 =
      GTK_SIGNAL_FUNC(graphics_info_t::superpose_optionmenu_activate_mol1);
   GtkSignalFunc signal_func2 =
      GTK_SIGNAL_FUNC(graphics_info_t::superpose_optionmenu_activate_mol2);

   graphics_info_t::superpose_imol1 = -1;
   graphics_info_t::superpose_imol2 = -1;
   // and what should the "set" values of graphics_info_t::superpose_imol1 and 2 be?
   // 
   for (int i=0; i<g.n_molecules(); i++) { 
      if (g.molecules[i].has_model()) { 
	 graphics_info_t::superpose_imol1 = i;
	 graphics_info_t::superpose_imol2 = i;
	 break;
      }
   }

   g.fill_option_menu_with_coordinates_options(optionmenu1, signal_func1,
					       graphics_info_t::superpose_imol1);
   g.fill_option_menu_with_coordinates_options(optionmenu2, signal_func2,
					       graphics_info_t::superpose_imol2);

   GtkWidget *chain_ref_om = lookup_widget(w, "superpose_reference_chain_optionmenu");
   GtkWidget *chain_mov_om = lookup_widget(w, "superpose_moving_chain_optionmenu");

   GtkWidget *chain_ref_menu = gtk_menu_new();
   GtkWidget *chain_mov_menu = gtk_menu_new();

   gtk_option_menu_set_menu(GTK_OPTION_MENU(chain_ref_om), chain_ref_menu);
   gtk_option_menu_set_menu(GTK_OPTION_MENU(chain_mov_om), chain_mov_menu);
   
#endif // HAVE_SSMLIB
   return w;
} 


// The callbacks.c callback:
// 
void fill_superpose_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
 						   int is_reference_structure_flag) {
#ifdef HAVE_SSMLIB

    graphics_info_t::fill_superpose_option_menu_with_chain_options(chain_optionmenu,
								   is_reference_structure_flag);
#endif // HAVE_SSMLIB    
}






/*  ----------------------------------------------------------------------- */
/*                  Least squares                                           */
/*  ----------------------------------------------------------------------- */

void clear_lsq_matches() {

   graphics_info_t::lsq_matchers->clear();
}

void add_lsq_match(int reference_resno_start, 
		   int reference_resno_end,
		   const char *chain_id_reference,
		   int moving_resno_start, 
		   int moving_resno_end,
		   const char *chain_id_moving,
		   int match_type) { /* 0: all
                                         1: main
				         2: CA 
				      */

   coot::lsq_range_match_info_t m(reference_resno_start, reference_resno_end,
				  chain_id_reference,
				  moving_resno_start, moving_resno_end,
				  chain_id_moving, match_type);

   graphics_info_t::lsq_matchers->push_back(m);

}

#ifdef USE_GUILE
void add_lsq_atom_pair_scm(SCM atom_spec_ref, SCM atom_spec_moving) { 

   coot::atom_spec_t ref_spec = atom_spec_from_scm_expression(atom_spec_ref);
   coot::atom_spec_t mov_spec = atom_spec_from_scm_expression(atom_spec_moving);

   coot::lsq_range_match_info_t m(ref_spec.chain, ref_spec.resno,
				  ref_spec.insertion_code, ref_spec.atom_name,
				  ref_spec.alt_conf,
				  mov_spec.chain,
				  mov_spec.resno, mov_spec.insertion_code,
				  mov_spec.atom_name, mov_spec.alt_conf);

   graphics_info_t::lsq_matchers->push_back(m);
   
} 
#endif

#ifdef USE_PYTHON
void add_lsq_atom_pair_py(PyObject *atom_spec_ref, PyObject *atom_spec_moving) { 

   coot::atom_spec_t ref_spec = atom_spec_from_python_expression(atom_spec_ref);
   coot::atom_spec_t mov_spec = atom_spec_from_python_expression(atom_spec_moving);

   coot::lsq_range_match_info_t m(ref_spec.chain, ref_spec.resno,
				  ref_spec.insertion_code, ref_spec.atom_name,
				  ref_spec.alt_conf,
				  mov_spec.chain,
				  mov_spec.resno, mov_spec.insertion_code,
				  mov_spec.atom_name, mov_spec.alt_conf);

   graphics_info_t::lsq_matchers->push_back(m);
   
} 
#endif


// return the rtop on a good match 
#ifdef USE_GUILE
SCM
apply_lsq_matches(int imol_reference, int imol_moving) {

   SCM scm_status = SCM_BOOL_F;
   if (is_valid_model_molecule(imol_reference)) {
      if (is_valid_model_molecule(imol_moving)) {
	 graphics_info_t g;
	 std::cout << "INFO:: Matching/moving molecule number " << imol_moving << " to "
		   << imol_reference << std::endl;
	 clipper::Spacegroup new_space_group = g.molecules[imol_reference].space_group().second;
	 clipper::Cell new_cell = g.molecules[imol_reference].cell().second;
	 std::pair<int, clipper::RTop_orth> status_and_rtop =
	    g.apply_lsq(imol_reference, imol_moving, *graphics_info_t::lsq_matchers);
	 if (status_and_rtop.first) {
	    scm_status = rtop_to_scm(status_and_rtop.second);
	 }
	    
      } else {
	 std::cout << "INFO:: Invalid reference molecule number " << imol_reference << std::endl;
      } 
   } else {
      std::cout << "INFO:: Invalid moving molecule number " << imol_moving << std::endl;
   }
   return scm_status;
}
#endif // USE_GUILE

// return the rtop on a good match, don't move the coordinates by applying the matrix.
// 
#ifdef USE_GUILE
SCM
get_lsq_matrix_scm(int imol_reference, int imol_moving) {

   SCM scm_status = SCM_BOOL_F;
   if (is_valid_model_molecule(imol_reference)) {
      if (is_valid_model_molecule(imol_moving)) {
	 graphics_info_t g;
	 std::pair<int, clipper::RTop_orth> status_and_rtop =
	    g.lsq_get_and_apply_matrix_maybe(imol_reference, imol_moving,
					     *(g.lsq_matchers),
					     0); // don't apply the matrix!
	 if (status_and_rtop.first) {
	    scm_status = rtop_to_scm(status_and_rtop.second);
	 }
	    
      } else {
	 std::cout << "INFO:: Invalid reference molecule number " << imol_reference << std::endl;
      } 
   } else {
      std::cout << "INFO:: Invalid moving molecule number " << imol_moving << std::endl;
   }
   return scm_status;
}
#endif // USE_GUILE



#ifdef USE_PYTHON
PyObject *apply_lsq_matches_py(int imol_reference, int imol_moving) {

   PyObject *python_status;
   python_status = Py_False;
   if (is_valid_model_molecule(imol_reference)) {
      if (is_valid_model_molecule(imol_moving)) {
         graphics_info_t g;
         std::cout << "INFO:: Matching/moving molecule number " << imol_moving << " to "
                   << imol_reference << std::endl;
         std::pair<int, clipper::RTop_orth> status_and_rtop =
            g.apply_lsq(imol_reference, imol_moving, *graphics_info_t::lsq_matchers);
         if (status_and_rtop.first) {
            python_status = rtop_to_python(status_and_rtop.second);
         }

      } else {
         std::cout << "INFO:: Invalid reference molecule number " << imol_reference << std::endl;
      }
   } else {
      std::cout << "INFO:: Invalid moving molecule number " << imol_moving << std::endl;
   }
   if (PyBool_Check(python_status)) {
     Py_INCREF(python_status);
   }
   return python_status;
}
#endif // USE_PYTHON

// return the rtop on a good match, don't move the coordinates by applying the matrix.
// 
#ifdef USE_PYTHON
PyObject *get_lsq_matrix_py(int imol_reference, int imol_moving) {

   PyObject *py_status = Py_False;
   if (is_valid_model_molecule(imol_reference)) {
     if (is_valid_model_molecule(imol_moving)) {
       graphics_info_t g;
       std::pair<int, clipper::RTop_orth> status_and_rtop =
         g.lsq_get_and_apply_matrix_maybe(imol_reference, imol_moving,
                                          *(g.lsq_matchers),
                                          0); // don't apply the matrix!
       if (status_and_rtop.first) {
         py_status = rtop_to_python(status_and_rtop.second);
       }
	    
     } else {
       std::cout << "INFO:: Invalid reference molecule number " << imol_reference << std::endl;
     } 
   } else {
     std::cout << "INFO:: Invalid moving molecule number " << imol_moving << std::endl;
   }

   if (PyBool_Check(py_status)) {
     Py_INCREF(py_status);
   }   

   return py_status;
}
#endif // USE_PYTHON

int
apply_lsq_matches_simple(int imol_reference, int imol_moving) {

   int status = 0;
   if (is_valid_model_molecule(imol_reference)) {
      if (is_valid_model_molecule(imol_moving)) {
	 graphics_info_t g;
	 std::cout << "INFO:: Matching/moving molecule number " << imol_moving << " to "
		   << imol_reference << std::endl;
	 std::pair<int, clipper::RTop_orth> statuspair = g.apply_lsq(imol_reference, imol_moving,
								     *graphics_info_t::lsq_matchers);
	 status = statuspair.first;
      } else {
	 std::cout << "INFO:: Invalid reference molecule number " << imol_reference << std::endl;
      } 
   } else {
      std::cout << "INFO:: Invalid moving molecule number " << imol_moving << std::endl;
   }

   return status;
}



GtkWidget *wrapped_create_least_squares_dialog() {

   GtkWidget *lsq_dialog = create_least_squares_dialog();
   int imol_reference = -1;
   int imol_moving = -1;
   int ref_start_resno = -9999;
   int ref_end_resno =   -9999;
   int mov_start_resno = -9999;
   int mov_end_resno =   -9999;
   int match_type = -1; /* 0: CA
		           1: main
   		           2: all  */
   const char *ref_chain_id = 0;
   const char *mov_chain_id = 0;

   GtkWidget *mov_option_menu = lookup_widget(lsq_dialog, "least_squares_moving_molecule_optionmenu");
   GtkWidget *ref_option_menu = lookup_widget(lsq_dialog, "least_squares_reference_molecule_optionmenu");
   GtkWidget *ref_res_range_1 = lookup_widget(lsq_dialog, "least_squares_reference_range_1_entry");
   GtkWidget *ref_res_range_2 = lookup_widget(lsq_dialog, "least_squares_reference_range_2_entry");
   GtkWidget *mov_res_range_1 = lookup_widget(lsq_dialog, "least_squares_moving_range_1_entry");
   GtkWidget *mov_res_range_2 = lookup_widget(lsq_dialog, "least_squares_moving_range_2_entry");

   GtkWidget *match_type_all_check_button =  lookup_widget(lsq_dialog, "least_squares_match_type_all_radiobutton");
   GtkWidget *match_type_main_check_button = lookup_widget(lsq_dialog, "least_squares_match_type_main_radiobutton");
   GtkWidget *match_type_main_calpha_button =lookup_widget(lsq_dialog, "least_squares_match_type_calpha_radiobutton");
   GtkWidget *ref_mol_chain_id_option_menu = lookup_widget(lsq_dialog, "least_squares_reference_chain_id");
   GtkWidget *mov_mol_chain_id_option_menu = lookup_widget(lsq_dialog, "least_squares_moving_chain_id");
   
   const char *txt = 0;

   // fill ref molecule option menu
   // fill mov molecule option menu
   // fill ref molecule chain option menu
   // fill mov molecule chain option menu

   graphics_info_t g;
   GtkSignalFunc callback_func1 = GTK_SIGNAL_FUNC(lsq_ref_mol_option_menu_changed);
   GtkSignalFunc callback_func2 = GTK_SIGNAL_FUNC(lsq_mov_mol_option_menu_changed);
   int imol = first_coords_imol();
   graphics_info_t::lsq_ref_imol = imol;
   graphics_info_t::lsq_mov_imol = imol;
   g.fill_option_menu_with_coordinates_options(ref_option_menu, callback_func1, imol);
   g.fill_option_menu_with_coordinates_options(mov_option_menu, callback_func2, imol);

   // fill with 1 to 999
   gtk_entry_set_text(GTK_ENTRY(ref_res_range_1), "1");
   gtk_entry_set_text(GTK_ENTRY(ref_res_range_2), "999");
   gtk_entry_set_text(GTK_ENTRY(mov_res_range_1), "1");
   gtk_entry_set_text(GTK_ENTRY(mov_res_range_2), "999");

   
   fill_lsq_option_menu_with_chain_options(ref_mol_chain_id_option_menu, 1);
   fill_lsq_option_menu_with_chain_options(mov_mol_chain_id_option_menu, 0);


   return lsq_dialog;

}

void
lsq_ref_mol_option_menu_changed(GtkWidget *item, GtkPositionType pos) {

   std::cout << "ref option menu changed to pos: " << pos << std::endl;
   graphics_info_t::lsq_ref_imol = pos;

   // now change the chain optionmenu for the reference structure:
   GtkWidget *ref_mol_chain_id_option_menu = lookup_widget(item, "least_squares_reference_chain_id");
   fill_lsq_option_menu_with_chain_options(ref_mol_chain_id_option_menu, 1);
   
   
}

void
lsq_mov_mol_option_menu_changed(GtkWidget *item, GtkPositionType pos) {

   std::cout << "mov option menu changed to pos: " << pos << std::endl;
   graphics_info_t::lsq_mov_imol = pos;

   // now change the chain optionmenu for the moving structure:
   GtkWidget *mov_mol_chain_id_option_menu = lookup_widget(item, "least_squares_moving_chain_id");
   fill_lsq_option_menu_with_chain_options(mov_mol_chain_id_option_menu, 0);
}



int apply_lsq_matches_by_widget(GtkWidget *lsq_dialog) {

   int imol_reference = graphics_info_t::lsq_ref_imol;
   int imol_moving    = graphics_info_t::lsq_mov_imol;
   int ref_start_resno = -9999;
   int ref_end_resno =   -9999;
   int mov_start_resno = -9999;
   int mov_end_resno =   -9999;
   int match_type = -1; /* 0: CA
		           1: main
   		           2: all  */

   GtkWidget *mov_option_menu = lookup_widget(lsq_dialog, "least_squares_moving_molecule_optionmenu");
   GtkWidget *ref_option_menu = lookup_widget(lsq_dialog, "least_squares_reference_molecule_optionmenu");
   GtkWidget *ref_res_range_1 = lookup_widget(lsq_dialog, "least_squares_reference_range_1_entry");
   GtkWidget *ref_res_range_2 = lookup_widget(lsq_dialog, "least_squares_reference_range_2_entry");
   GtkWidget *mov_res_range_1 = lookup_widget(lsq_dialog, "least_squares_moving_range_1_entry");
   GtkWidget *mov_res_range_2 = lookup_widget(lsq_dialog, "least_squares_moving_range_2_entry");

   GtkWidget *match_type_all_check_button =  lookup_widget(lsq_dialog, "least_squares_match_type_all_radiobutton");
   GtkWidget *match_type_main_check_button = lookup_widget(lsq_dialog, "least_squares_match_type_main_radiobutton");
   GtkWidget *match_type_calpha_check_button =lookup_widget(lsq_dialog, "least_squares_match_type_calpha_radiobutton");
   GtkWidget *ref_mol_chain_id_option_menu = lookup_widget(lsq_dialog, "least_squares_reference_chain_id");
   GtkWidget *mov_mol_chain_id_option_menu = lookup_widget(lsq_dialog, "least_squares_moving_chain_id");

   GtkWidget *copy_checkbutton = lookup_widget(lsq_dialog, "least_squares_move_copy_checkbutton");
   if (copy_checkbutton) { 
      if (GTK_TOGGLE_BUTTON(copy_checkbutton)->active) {
	 int new_imol_moving = copy_molecule(imol_moving);
	 imol_moving = new_imol_moving;
	 graphics_info_t::lsq_mov_imol = imol_moving;
      }
   }
   
   const char *txt = 0;

   txt = gtk_entry_get_text(GTK_ENTRY(ref_res_range_1));
   ref_start_resno = atoi(txt);
   txt = gtk_entry_get_text(GTK_ENTRY(ref_res_range_2));
   ref_end_resno = atoi(txt);
   txt = gtk_entry_get_text(GTK_ENTRY(mov_res_range_1));
   mov_start_resno = atoi(txt);
   txt = gtk_entry_get_text(GTK_ENTRY(mov_res_range_2));
   mov_end_resno = atoi(txt);


   std::string ref_chain_id_str = graphics_info_t::lsq_match_chain_id_ref;
   std::string mov_chain_id_str = graphics_info_t::lsq_match_chain_id_mov;

   if (GTK_TOGGLE_BUTTON(match_type_all_check_button)->active)
      match_type = 0;
   if (GTK_TOGGLE_BUTTON(match_type_main_check_button)->active)
      match_type = 1;
   if (GTK_TOGGLE_BUTTON(match_type_calpha_check_button)->active)
      match_type = 2;
   
   std::cout << "INFO:: reference from " << ref_start_resno << " to " <<  ref_end_resno << " chain "
	     << ref_chain_id_str << " moving from " << mov_start_resno << " to "
	     << mov_end_resno << " chain " <<  mov_chain_id_str << " match type: " << match_type
	     << std::endl;

   clear_lsq_matches();

   add_lsq_match(ref_start_resno, ref_end_resno, ref_chain_id_str.c_str(),
		 mov_start_resno, mov_end_resno, mov_chain_id_str.c_str(),
		 match_type);
   return apply_lsq_matches_simple(imol_reference, imol_moving);
} 


void
fill_lsq_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
					int is_reference_structure_flag) {

   // c.f void
   // graphics_info_t::fill_superpose_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
   // int is_reference_structure_flag) {

   GtkSignalFunc callback_func;
   int imol = -1;
   if (is_reference_structure_flag) {  
      imol = graphics_info_t::lsq_ref_imol;
      callback_func =
	 GTK_SIGNAL_FUNC(lsq_reference_chain_option_menu_item_activate);
   } else {
      imol = graphics_info_t::lsq_mov_imol;
       callback_func =
	 GTK_SIGNAL_FUNC(lsq_moving_chain_option_menu_item_activate);
   }

   std::cout << "in fill_lsq_option_menu_with_chain_options " << imol << " "
	     << is_reference_structure_flag << std::endl;
   
   if (imol >=0 && imol < graphics_info_t::n_molecules()) { 
      std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_optionmenu,
								      imol, callback_func);
      if (is_reference_structure_flag) {
	 graphics_info_t::lsq_match_chain_id_ref = set_chain;
      } else {
	 graphics_info_t::lsq_match_chain_id_mov = set_chain;
      }
      
   } else {
      std::cout << "ERROR in imol in fill_lsq_option_menu_with_chain_options "
		<< std::endl;
   }
}


void lsq_reference_chain_option_menu_item_activate(GtkWidget *item,
						   GtkPositionType pos) {
   graphics_info_t::lsq_match_chain_id_ref = menu_item_label(item);
}

void lsq_moving_chain_option_menu_item_activate(GtkWidget *item,
						GtkPositionType pos) {
   graphics_info_t::lsq_match_chain_id_mov = menu_item_label(item);
} 

/*  ----------------------------------------------------------------------- */
/*               LSQ-improve               */
/*  ----------------------------------------------------------------------- */
/*! \name LSQ-improve */
/* \{ */
void lsq_improve(int imol_ref, const char *ref_selection,
		 int imol_moving, const char *moving_selection,
		 int n_res, float dist_crit) {

   if (is_valid_model_molecule(imol_ref)) { 
      if (is_valid_model_molecule(imol_moving)) {
	 CMMDBManager *mol_ref = graphics_info_t::molecules[imol_ref].atom_sel.mol;
	 graphics_info_t::molecules[imol_moving].lsq_improve(mol_ref, ref_selection,
							     moving_selection,
							     n_res, dist_crit);
      }
   }

} 
/* \} */


// --------------------------------------------------------------------------
//  for supperposition of maps
//  by arbitary rotation round screen axis (as sister function of transform-map
// --------------------------------------------------------------------------

int
rotate_map_round_screen_axis_x(float r_degrees) {

   // Find screen axis x
   // 
   return -1; 
} 

int 
rotate_map_round_screen_axis_y(float r_degrees) {

   return -1; 
}

int
rotate_map_round_screen_axis_z(float r_degrees) {

   return -1; 
} 

   

